# Skill: agh-run

**全流程自动推进模式** PM 主导，一次性执行clarify → propose → apply → archive 全部阶段，仅在人工审批节点暂停止。各 Agent 编排业务 Skill，Skill 通过数据操作 Skill（FeatureService/SpecService/KnowledgeService/CodeGraphService）访问数据层；apply 阶段 DE 遵循 TDD 循环（R1 → 红 → 绿 + R2 → 白盒 → 重构 + R3）。
**日志规则** 同各阶段 skill 定义，追加日志到 `log/process.log`

## 核心规则：自动推荐

→ skill 与逐阶段执行的唯一区别

1. 当某阶段到达 DONE 标记时，**禁止停下来等待用户输入下一个命令*
2. 替代行为：打印推进心跳后，立即读取下一阶段。?skill 文件并继续执行3. 所有阶段内的人工审批节点（SR1/SR2/SR3/SR4、逐任务人工确认、Proposal确认、模式选择）照常暂停等待用户决策4. → `.state.md` 中写。?`auto_advance: true`，用于断点恢复时识别模式


## 推进触发条件

| 当前阶段完成标记                                                | 推进动作                                                                                      |
| ------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| current_step=INIT-DONE                                  | `[PM] → 自动推进 → propose 阶段` 然后读取并执行skills/agh-propose.md                                   |
| current_step=PROPOSE-DONE（fast/standard）或 SR1 通过（full）? | `[PM] → 自动推进 → apply 阶段` 然后读取并执行skills/agh-apply.md                                       |
| current_step=SR3-DONE                                   | `[PM] → 自动推进 → archive 阶段`，重复repair_round=0, repair_task=""，然后读取并执行 skills/agh-archive.md |
| phase=done                                              | `[PM] → 全流程完成` 打印最终摘要                                                                     |

---

## Fast 模式特殊行为：连续流

→ mode=fast 时，/agh-run → propose→apply→archive 合并为一个连续流，最大限度减少人工交互：

**整个 fast 连续流中，用户仅需在以下时刻做出决策：**

- Init 阶段 Proposal 确认 + 模式选择
- Apply 阶段的人工确认（唯一审批点，合并发?SR2/SR3/SR4 的职能）

**被跳过的暂停点：**

- SR1（fast 模式无此节点）?- SR4（fast 模式直接结项目- 阶段间等待（propose→apply→archive 全部自动衔接口
  **推进触发条件覆盖（fast 模式）：**


| 当前阶段完成标记                  | 推进动作                                                         |
| ------------------------- | ------------------------------------------------------------ |
| current_step=INIT-DONE    | 自动推进 → propose（无暂停止                                          |
| current_step=PROPOSE-DONE | 自动推进 → apply（无暂停，跳）?SR1→                                     |
| completed_steps 包含 SPEC-GENERATED | 自动推进 → archive（无暂停，跳过 SR4），重置 repair_round=0, repair_task="" |
| phase=done                | 打印最终摘要                                                       |


---

## 被覆盖的行为

执行各阶段?skill 时，以下结束语被替换为自动推进：

- `可执行/agh-propose` → 自动推进
- `可执行/agh-apply` → 自动推进
- `可执行/agh-archive` → 自动推进
- 任何形式。?下一步请用户输入命令"提示 → 自动推进

---

## 断点恢复

PM 恢复时检查`.state.md` → `auto_advance: true`→

1. 根据 phase + current_step 确定当前位置
2. 读取对应阶段。?skill 文件，从中断点继续执行3. 该阶段完成后继续自动推进到下一阶段

---

## 执行序列

### Phase 1: Init

1. `[PM] → /agh-run 启动，进行clarify 阶段`
2. → .state.md 中写。`auto_advance: true`
3. 读取 skills/agh-clarify.md，按其定义执行全部步骤4. 到达 INIT-DONE 后：`[PM] → clarify 完成，自动推荐 propose`

### Phase 2: Propose

1. 读取 skills/agh-propose.md，按其定义执行全部步骤（）SR1 审批如适用户
2. 到达 PROPOSE-DONE（fast/standard）或 SR1 通过（full）后：`[PM] → propose 完成，自动推荐 apply`

### Phase 3: Apply

1. 读取 skills/agh-apply.md，按其定义执行全部步骤
2. 所有人工审批节点（逐任务确认、SR2、SR3）照常暂停等待用户
3. 阅读 skills/agh-apply.md 中 Step 4（Spec 文档生成），执行 Spec 生成+校验
4. 到达 completed_steps 包含 SPEC-GENERATED 后：`[PM] → apply 完成（含 Spec 文档就绪），自动推荐 archive`

### Phase 4: Archive

1. 读取 skills/agh-archive.md，按其定义执行全部步骤（）SR4 审批如适用户
2. 到达 phase=done 后：打印最终摘要

---

## 最终摘要格式：

```
══════════════════════════════════════
[/agh-run 全流程完成]
需求编码 {feature-name}
模式: {fast|standard|full}
总耗时: {→ init 启动。?archive 完成的时间}
归档产物:
  - output/baselines/requirement-spec.md（full模式）
  - output/baselines/design.md（standard/full模式）
  - output/*
项目状态 DONE
══════════════════════════════════════
```

---

## 异常处理

- 任何阶段的异常处理逻辑不变，遵循各类 skill 文件定义
- 如果某阶段因异常暂停（如超过5轮修复上升人工），用户解决后流程继续自动推进
- 人工审批驳回后的回退逻辑不变，回退完成后继续自动推荐- 会话中断后重新执行`/agh-run`，通过 auto_advance 标记自动恢复推进模式

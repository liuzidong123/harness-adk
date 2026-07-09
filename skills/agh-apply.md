# Skill: agh-apply

开发实现 + 审计验证 + 人工审批。DE/TE Agent 编排业务 Skill，Skill 通过数据操作 Skill（FeatureService/SpecService/KnowledgeService/CodeGraphService）访问数据层。DE 遵循 TDD 循环（R1 → 红 → 绿 + R2 → 白盒 → 重构 + R3）。按 mode 裁剪步骤。
**日志规则** → `templates/logging-standard.md`

---

## 前置检查
1. 读取 `.state.md` 获取当前 req_id
2. 验证 `.state.md` → current_step=PROPOSE-DONE
3. 读取 mode 字段确定流程裁剪方式
4. 验证 `output/{feature-name}/plan-action.md` 存在且非空

## 断点续作
1. 读取 `completed_steps`、`repair_round`、`repair_task`
2. 跳过已完成 Task，从未完成继续

---

## fast 模式

DE 一次性开发所有任务 → TE 轻量审计 → 人工确认（唯一审批点）

**Step 1: DE 批量开发**
1. handoff-DE 注明数据操作 Skill 白名单：**FeatureService**（Feature 配置）、**SpecService**（规格读取）、**KnowledgeService**（约束查询 → 关系图谱层）、**CodeGraphService**（代码结构分析 → L1 静态索引）
2. DE Agent 编排：配置生成 Skill → 功能代码生成 Skill → R2 Implementation Review Skill → 冲突检测 Skill → 合规校验 Skill
3. DE 执行 dev-test → post-verify

**Step 2: TE 轻量审计**
1. handoff-TE 注明数据操作 Skill：**FeatureService**（Feature 验证）、**KnowledgeService**（约束检查）、**SpecService**（需求一致性）
2. TE Agent 编排：黑盒测试执行 Skill → 测试脚本执行 Skill

**Step 3: 人工确认（唯一审批点）**

**Step 4: Spec 文档生成（PM 强制执行）**
同 fast 模式 Step 4 规范。

---

## standard 模式

DE 遵循 TDD 循环（R1 → 红 → 绿 + R2 → 白盒 → 重构 + R3），逐批次开发。

### TDD 循环（每批次的完整流程）

```
FOR 每个 Task-{N} IN plan-action.md:
  
  1. R1 Pre-Review（DE + TE 联合审查）
     → DE Agent 编排 R1 Pre-Review Skill
     → 调用 KnowledgeService（关系图谱层 + LLM Wiki 层）→ 约束/冲突检测
     → 调用 CodeGraphService（L1 静态 + L2 语义）→ 代码分析
     → 调用 FeatureService → Feature 关系检查
     → 输出审核报告 + 修正需求 + 测试需求规格
  
  2. 红阶段（TE 先行生成黑盒测试）
     → TE Agent 编排 黑盒测试用例生成 Skill
     → 调用 FeatureService（边界条件）+ KnowledgeService（异常场景）
     → 生成黑盒测试用例（必然失败）
  
  3. 绿阶段 + R2 Implementation Review（DE 实现）
     → DE Agent 编排 配置生成 Skill → 功能代码生成 Skill
     → 调用 SpecService（接口定义）+ KnowledgeService（平台规则）+ CodeGraphService（代码结构）
     → DE Agent 编排 R2 Implementation Review Skill
     → 调用 CodeGraphService（L1 Diff + L2 语义）→ Code Diff vs Spec/Knowledge/Feature 审查
     → 输出 R2 审查报告（Pass → 继续 / Fail → 修正）
     → 黑盒测试通过（目标）
  
  4. 白盒阶段（TE 生成白盒测试）
     → TE Agent 编排 白盒测试用例生成 Skill
     → 调用 CodeGraphService（L1 CFG/分支 + L2 语义）
     → 调用 FeatureService（MC/DC 覆盖）
     → 输出覆盖率报告（分支 ≥ 80%）
  
  5. 重构阶段 + R3 Refactoring Review（DE 优化）
     → DE Agent 编排 R3 Refactoring Review Skill
     → 调用 CodeGraphService（L1 结构 + L2 语义）
     → 调用 KnowledgeService（最佳实践）+ FeatureService（配置校验）
     → 保持黑盒 100% + 白盒 ≥ 80% 覆盖

NEXT Task
```

**Step 1: 并行批次开发 + TEST-1 逐批次审计**

按 plan-action.md 的依赖分批，每批内串行 TDD 循环，批次间可并行。

```
读取 plan-action.md Task 列表和依赖
计算并行批次: Batch-1: deps=none → ...

FOR 每个 Batch（跳过已完成 Task）:
  并行派发 Batch 内所有 Task → DE（每 Task 执行完整 TDD 循环）
  等待所有 DE 完成
  并行派发 Batch 内所有 Task → TE TEST-1
  等待所有 TE 完成
  失败 Task 进入修复循环
  [人工批量确认本批次]
END FOR
```

**Step 2: SR2 功能评审**
检查所有 DE code-reports 含 R2 通过结论、TEST-1 全部 PASS、repair_round 未超限。
用户确认通过/驳回。

**Step 3: TE 最终审计（TEST-2）**
TE Agent 编排全部测试执行 Skill + 缺陷分析 Skill。执行全量黑盒回归 + 白盒覆盖率验证。

**Step 4: SR3 最终评审**
检查 final-test-report PASS、无 TODO 残留、追溯矩阵完整。用户确认通过/驳回。

**Step 5: Spec 文档生成（PM 强制执行）**
同 fast 模式 Step 4 规范。

---

## full 模式

apply 阶段与 standard 完全一致，区别仅在 propose 阶段的 SR1 评审。

---

## 修复循环（所有模式通用）

```
DE 自修循环 (内部):
  for attempt in 1..3:
    修复 → 重新执行 TDD 循环（含 R2/R3 审查）
    if PASS: break
    if FAIL: attempt++
  if 3次都FAIL: 回报PM status=failed

PM 派修循环 (外部):
  for round in 1..5:
    PM 收到 DE failed → 分析原因
    写入 handoff: R{round+1}（追加失败原因 + 上轮产出 + 重新执行 R2 审查）
    DE 再修 → TE 再审计
    if PASS: 清除标记
    if FAIL: round++
  if 5轮都FAIL: 上报人工介入
```

DE 修复必须重新执行 TDD 循环中的相应阶段（至少重新执行 R2 审查），不可跳过。

---

## 异常处理

- SubAgent 回报 status=failed: 检查原因，决定重试或上升
- 浏览器环境不可用: 提示用户安装 Playwright 依赖
- 断点恢复时发现不一致: 以 `.state.md` 为准，重新校验文件状态
- **CodeGraphService** 不可用：白盒测试阶段降级为无覆盖率度量的黑盒回归
- **KnowledgeService** 不可用：R1/R3 审查降级为人工判断

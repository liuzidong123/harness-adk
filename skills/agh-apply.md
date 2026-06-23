# Skill: agh-apply

开发实现。审计验证 → 人工审批。按 mode 裁剪步骤。。
**日志规则* → `templates/logging-standard.md`

---

## 前置检查

1. 读取 `.state.md` 获取当前 req_id
2. 验证 `.state.md` → current_step=PROPOSE-DONE
3. 读取 mode 字段确定流程裁剪方式
4. 验证 `output/{feature-name}/specs/plan-action.md` 存在且非空
5. 不满足则阻塞，提示用户先完成 /agh-propose

## 断点续作

1. 读取 `.state.md` → completed_steps
2. 读取 `repair_round` → `repair_task` 字段，恢复修复循环上下文
3. 跳过已完成的 Task，从未完成的 Task 继续
4. → repair_round > 0，从修复循环的当前轮次继续（而非从第 1 轮重新开始）
5. `[PM] 断点恢复，从 {step_id} 继续（repair_round={N}）`

---

> DE、TE 在实现和审计中必须参考`docs/` 下行业技术标准规范文档（`docs/` 始终可读，不受白名单限制）→

## fast 模式

DE 一次性开发所有任务 → TE 轻量审计 → 人工确认（唯一审批点）


**Step 1: DE 批量开发**

1. `[PM] fast 模式，DE 批量开发所有任务`
2. 写入 handoff: `handoffs/{feature-name}-DEV1-R1.md`
- to: DE
- openspec_strategy: direct（fast 模式）
- 白名单: `output/{feature-name}/plan-action.md`, `output/{feature-name}/proposal.md`, 已有代码
- 期望输出: `output/{feature-name}/`, `output/{feature-name}/code-report-v{N}.md`（含 Task↔文件↔测试追溯矩阵）
- 约束: 完成后执行 dev-test → post-verify
3. 更新 `.state.md`: current_step=DEV-1, current_role=DE
4. 派发任务:
- [Claude Code] spawn SubAgent，注入 handoff + agents/de.md + 白名单文件
- [Cline] 切换角色为 DE，指示读取 handoff
5. 接收回报，校验输出文件存在
6. `[PM] 开发完成`

**Step 2: TE 轻量审计**

1. `[PM] fast 模式，TE 轻量审计`
2. 写入 handoff: `handoffs/{feature-name}-TEST1-R1.md`
- to: TE
- openspec_strategy: direct（fast 模式）
- 白名单: `output/{feature-name}`, `output/{feature-name}/proposal.md`, `.state.md`
- 期望输出: `output/{feature-name}/temp-test-report-v{N}.md`（含基础追溯验证）
- 约束: 根据 .state.md 中 test_strategy 执行对应验证；如 test_strategy=manual，生成人工检查清单
3. 派发任务给 TE
4. 接收回报:
- PASS → 继续 Step 3
- FAIL → 修复循环（最多 5 轮）

**Step 3: 人工确认（唯一审批点）**

1. `[PM] 进入人工确认`
2. 向用户呈现：
  ```
  [人工确认]
  模式: fast
  产出文件: output/{feature-name}/
  代码报告: output/{feature-name}/code-report-v{N}.md
  审计报告: output/{feature-name}/temp-test-report-v{N}.md
  请确认 通过 / 驳回（请说明原因）
  ```
3. 用户通过:
- **生成/更新 Spec 文档到 `openspec/specs/`**:
    - `openspec/specs/{feature-name}-spec.md`（含 SHALL + GWT + 追溯矩阵 + 技术图表）
    - `openspec/specs/test-cases-spec.md`（增量更新测试用例规格）
    - **图表要求（必须）**: Spec 文档必须包含以下 Mermaid 图表，基于实际代码生成：
        - 框架图（flowchart/graph）: 展示模块/组件间关系
        - 逻辑流程图（flowchart）: 展示核心业务逻辑分支
        - 类图（classDiagram）: 展示新增/修改的类、成员变量、方法
        - 时序图（sequenceDiagram）: 展示关键交互流程
- 更新 `.state.md`: sr_status.SR2=skipped, sr_status.SR3=approved, phase=apply, current_step=SR3-DONE
- `[PM] 确认通过（fast模式），可执行/agh-archive`
4. 用户驳回:
- 记录原因，回退 DE 修复

---

## standard 模式

> DE、TE 在实现和审计中必须参考`docs/` 下行业技术标准规范文档（`docs/` 始终可读，不受白名单限制）→
> 并行批次开发（按依赖分批，TE TEST-1 逐批次）→ SR2 → TE TEST-2 最终审计 R3

> **Step 1: 并行批次开发 + TEST-1 逐批次审计**

> → 并行优化：无依赖Task 同批并行开发和审计。仅 Claude Code 模式支持并行；Cline 模式退化为逐任务串行业。

```
读取 plan-action.md 中的 Task 列表和依赖关系（[deps: ...]）?计算并行批次:
  Batch-1: 所有deps=none → Task
  Batch-2: 依赖仅在 Batch-1 中的 Task
  Batch-N: 依赖仅在前序 Batch 中的 Task
  （无依赖标注时，所有Task 视为 deps=none，归入同一批次）。
FOR 每个 Batch（跳过已完成。Task）。
    并行派发 Batch 内所有Task → DE (DEV-B{N})
    等待所有DE 完成
    并行派发 Batch 内所有Task → TE (TEST-1-{N})
    等待所有TE 完成
    对失败的 Task 进入修复循环（可并行修复杂。    人工批量确认本批次）?记入 completed_steps
END FOR
```

对每个Batch-{B}→

1. `[PM] 启动 Batch-{B}，包含Task: {列表}，并行派发给 DE`
2. → Batch 内每个Task-{N} 写入 handoff:
- `handoffs/{feature-name}-DEV1-T{N}-R1.md`
- to: DE
- task_phase: audit（DE 仅出现在 apply 阶段）  - repair_round: 0
- openspec_strategy: review（standard 模式）  - 白名单:  `output/{feature-name}/design.md`（Task-{N} 部分析 `output/{feature-name}/proposal.md`, 已有代码, 前序 Batch 产出代码
- 期望输出: `output/{feature-name}/`, `output/{feature-name}/code-report-v{N}.md`
- 约束: TDD 模式，完成后执行 dev-test → post-verify
3. 并行派发:
- [Claude Code] 同时 spawn 多个 DE SubAgent
- [Cline] 逐个串行执行
4. 等待所有DE 完成，校验各自输出文件存在线
5. 更新 `.state.md`: current_step=DEV-B{B}
6. `[PM] Batch-{B} 开发完成，并行派发 TE TEST-1-{B} 审计`
7. → Batch 内每个Task-{N} 写入 TE handoff:
- `handoffs/{feature-name}-TEST1-T{N}-R1.md`
- to: TE
- task_phase: audit（apply 阶段审计执行   - openspec_strategy: review（standard 模式）
- 白名单:  `output/{feature-name}/`, `output/{feature-name}/de-reports/drafts/code-report-v{N}.md`
- 期望输出: `output/{feature-name}/temp-test-report-{batch}.md`
- 约束: 根据 .state.md → test_strategy 执行验证
8. 并行派发:
- [Claude Code] 同时 spawn 多个 TE SubAgent
- [Cline] 逐个串行执行
9. 等待所有TE 完成，汇总审计结果
- 全部 PASS → 人工批量确认本批次   - 部分 FAIL → 失败。?Task 进入修复循环，通过期?Task 等待
10. 更新 `.state.md`: completed_steps 追加 DEV-B{B} + TEST-1-{B}
11. 人工批量确认:
  ```
    [人工确认 Batch-{B}]
    通过Task: {列表}
    审计报告: test/temp-test-report-{batch}.md
    请确认 通过 / 驳回（指定Task 和原因）
  ```
12. 确认通过 → 下一个Batch

**Step 2: SR2 功能评审**

1. `[PM] 所有Task 完成，启动SR2 功能评审`
2. 检查SR2 通过标准（全部满足方可提交）:
- → DE code-reports 全部 status=done
- → TEST-1 全部 temp-test-report 存在且结果PASS
- 。repair_round >= 5 的未解决问题
3. 向用户呈现：
  ```
   [人工审批节点 SR2]
   模式: standard
   已完整Task: {N} →    Batch 审计: {全部 PASS / N → PASS, M → FAIL}
   修复轮次: repair_round={R}
   请确认 通过 / 驳回（请说明原因）?   ```
  ```
4. 通过:
- 写入 `output/{feature-name}/approvals/SR2-record.md`
- 更新 `.state.md`: sr_status.SR2=approved, completed_steps 追加 SR2-DONE
- `[PM] SR2 通过，启动TE 最终审计`
5. 驳回: 记录原因，回退指定 Task

**Step 3: TE 最终审计（TEST-2）?*

1. `[PM] 启动 TEST-2 最终审计`
2. 写入 handoff: `handoffs/{feature-name}-TEST2-R1.md`
- to: TE
- task_phase: audit（apply 阶段最终审计）
- openspec_strategy: review（standard 模式）?   - 白名单：`output/{feature-name}/`, `test/test-cases.md`（前置设计的测试用例子）, `.state.md`
- 期望输出: `output/{feature-name}/final-test-report.md`
- 约束: 执行全部 A/B/C 三类测试用例，验证追溯矩阵完整
3. 结论:
- PASS → 继续 SR3
- FAIL → 修复循环

**Step 4: SR3 最终评估*

1. `[PM] 启动 SR3 最终评审`
2. 检查SR3 通过标准:
- → final-test-report 结论 PASS
- → 无占位符/TODO 残留
- → 测试用例追溯矩阵完整
3. 向用户呈现:

  ```

   [人工审批节点 SR3]
   模式: standard
   最终审计报告 test/final-test-report.md
   产出 output/{feature-name}/
   请确认 通过 / 驳回（请说明原因）
 
  ```

4. 通过:
- **生成/更新 Spec 文档到 `openspec/specs/`**:
    - `openspec/specs/{feature-name}-spec.md`（含 SHALL + GWT + 追溯矩阵 + 技术图表）
    - `openspec/specs/test-cases-spec.md`（增量更新测试用例规格）
    - **图表要求（必须）**: Spec 文档必须包含以下 Mermaid 图表，基于实际代码生成：
        - 框架图（flowchart/graph）: 展示模块/组件间关系
        - 逻辑流程图（flowchart）: 展示核心业务逻辑分支
        - 类图（classDiagram）: 展示新增/修改的类、成员变量、方法
        - 时序图（sequenceDiagram）: 展示关键交互流程
- 写入 `output/{feature-name}/approvals/SR3-record.md`
- 更新 `.state.md`: sr_status.SR3=approved, phase=apply, current_step=SR3-DONE, completed_steps 追加 TEST-2 + SR3-DONE
- `[PM] SR3 通过，可执行 /agh-archive`
5. 驳回: 记录原因，回退修复

---

## full 模式

## → standard 模式相同流程，无裁剪：

（full 模式） apply 阶段与 standard 完全一致，区别仅在 propose 阶段 的 SR1 评审批

## 修复循环（所有模式通用户

```
DE 自修循环 (内部):
  for attempt in 1..3:
     修复问题 → ran dev-test
     if PASS: break
     if FAIL: attempt++
  if 3次都FAIL: 回报PM status=failed

PM 派修循环 (外部):
  for round in 1..5:
     PM 收到 DE failed → 检查原。?     写入。?handoff: R{round+1}
     DE 再修 → TE 再审计     if PASS: 清除标记
     if FAIL: round++
  if 5轮都FAIL: 上报人工介入
```

1. `[PM] Task-{N} 审计失败（repair_round={R}/5），派发修复杂。DE`
2. 更新 `.state.md`: repair_round={R+1}, repair_task=Task-{N}
3. 写入 handoff: `handoffs/{feature-name}-DEV1-T{N}-R{R+1}.md`
- repair_round: {R+1}
- repair_origin: "de-self-repair" | "pm-redispatch"
- 附加: 上轮失败原因、失败报告路线?   - 白名单追溯 上轮产出。?+ 上轮代码
4. DE 修复（允许3 次内部自修循环）→ TE 重新审计
5. 审计通过:
- 更新 `.state.md`: repair_round=0, repair_task=""
6. 审计再次失败（R+1 < 5）? 进入下一切?PM 派修
7. repair_round → 5: `[PM] Task-{N} 超过最大重试次数，上升人工审核`
- 更新 `.state.md`: repair_round=5

## 异常处理

- SubAgent 回报 status=failed: 检查原因，决定重试或上限?- 浏览器环境不可用: 提示用户安装 Playwright 依赖
- 断点恢复时发现不一切? → `.state.md` 为准，重新校验文件状态
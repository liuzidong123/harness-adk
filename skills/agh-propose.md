# Skill: agh-propose

需求分析 → 架构设计 → 测试用例 → 计划编排 → 人工评审。按 mode 裁剪步骤。
**日志规则:** 见 `templates/logging-standard.md`

---

## 前置检查
1. 读取 `.state.md` 获取当前 req_id
2. 验证 `.state.md` 中 phase=init 且 current_step=INIT-DONE
3. 读取 mode 字段确定流程裁剪方式
4. 验证 `output/{feature-name}/proposal.md` 存在且非空
5. 不满足则阻塞，提示用户先执行 /agh-clarify

---

## fast 模式

跳过 BA/SA/TE，PM 直接基于 proposal 生成执行计划。
**Step 1: PM 直接编排计划**

1. `[PM] fast 模式，跳过需求分析/架构设计/测试用例，直接编排计划`
2. 读取 `output/{feature-name}/proposal.md`
3. 生成简版 plan-action.md（任务列表 + 执行顺序，无需求对照表）
4. 写入 `output/{feature-name}/plan-action.md`（过程产物，非 spec）
5. 更新 `.state.md`: phase=propose, current_step=PROPOSE-DONE, sr_status.SR1=skipped
6. `[PM] 计划编排完成（fast模式，跳过SR1），可执行 /agh-apply`

> SR1 审批跳过
---

## standard 模式

跳过 BA，SA + TE + UX 按需并行执行，无 SR1。
**Step 1: 并行调度 SA + TE + UX**

> **并行优化**: SA、TE 和 UX 同时派发，互不依赖（TE 基于 proposal 设计用例，UX 基于 proposal 设计视觉）。仅 Claude Code 模式支持并行；Cline 模式退化为串行。
> UX 仅在 output_type 需要视觉设计时激活（见 docs/harness-flow.md §3 产出类型表）。
> SA、TE 在设计中必须参考 `docs/` 下行业技术标准规范文档（`docs/` 始终可读，不受白名单限制）。
1. `[PM] standard 模式，跳过BA需求分析，并行调度 SA + TE${UX需激活 ' + UX' : ''}`
2. 写入 handoff-SA: `handoffs/{feature-name}-SA-R1.md`
    - to: SA
    - task_phase: design
    - openspec_strategy: review（standard 模式）
    - 白名单: `output/{feature-name}/proposal.md`
    - 期望输出: `output/{feature-name}/drafts/design-v{N}.md`（含需求↔Task↔验证追溯矩阵）
    - 约束: 简版设计（架构 + Tasks清单 + 需求映射简表，无需时序图）。因 standard 模式跳过 BA，SA 需在 design.md 中补充 Proposal 要点→Task→验证方式的追溯矩阵
3. 写入 handoff-TE: `handoffs/{feature-name}-TE-R1.md`
    - to: TE
    - task_phase: design（propose 阶段前置测试用例设计）
    - openspec_strategy: review（standard 模式）
    - 白名单: `output/{feature-name}/proposal.md`
    - 期望输出: `test/drafts/test-cases-v{N}.md`（A/B/C 三类 + 人工审阅清单）
    - 约束: 基于 Proposal 设计测试用例，含文件存在性(A)、内容完整性(B)、流程一致性(C) 三类
4. 如需 UX: 写入 handoff-UX: `handoffs/{feature-name}-UX-R1.md`
    - to: UX
    - task_phase: design
    - openspec_strategy: review（standard 模式）
    - 白名单: `output/{feature-name}/proposal.md`, 相关模板文件
    - 期望输出: `output/{feature-name}/ux/drafts/ux-design-v{N}.md` + `specs/ux/wireframes/`（按 output_type）
    - 约束: 产出 UX 设计规格 + wireframe，等待用户审批
5. 更新 `.state.md`: current_step=SA+TE${UX需激活 '+UX' : ''}
6. 并行派发任务:
    - [Claude Code] 同时 spawn SubAgent（SA + TE + UX），各自注入对应 handoff + agent 定义 + 白名单文件
    - [Cline] 串行执行：先 SA，再 TE，最后 UX
7. 等待所有回报，校验:
    - `output/{feature-name}/design.md` 存在且非空
    - `test/test-cases.md` 存在且非空
    - 含 UX: `output/{feature-name}/ux/ux-design.md` 存在且非空
8. 更新 `.state.md`: current_handoff=""
9. 含 UX 且需审批: 向用户呈现 UX 设计，等待用户确认后继续
10. `[PM] SA + TE${UX需激活 ' + UX' : ''} 完成，技术方案和测试用例已生成`

**Step 2: PM 计划编排**

1. `[PM] 启动计划编排`
2. 读取 design.md 中的 Tasks 清单 + testcases.md
3. 编排执行计划，写入 `output/{feature-name}/plan-action.md`（过程产物，非 spec）
    - 必须标注 Task 间依赖关系（见 plan-action.md 格式要求）
4. 更新 `.state.md`: phase=propose, current_step=PROPOSE-DONE, sr_status.SR1=skipped, completed_steps 追加 SA, TE, UX, PLAN
5. `[PM] 计划编排完成（standard模式，跳过SR1），可执行 /agh-apply`

> SR1 审批跳过
---

## full 模式

完整流程：BA → (SA + TE) → PM编排 → SR1。
**Step 1: 调度 BA 需求分析**

1. `[PM] 启动 BA 需求分析，派发任务给 BA`
2. 写入 handoff: `handoffs/{feature-name}-BA-R1.md`
    - to: BA
    - openspec_strategy: review（full 模式）
    - 白名单: reference/*, `specs/proposal.md`
    - 期望输出: `openspec/specs/{feature-name}-requirements.md`（OpenSpec 格式，含 SHALL + GWT + 追溯矩阵）
3. 更新 `.state.md`: current_step=BA, current_handoff={feature-name}-BA-R1.md
4. 派发任务:
    - [Claude Code] spawn SubAgent，注入 handoff + agents/ba.md + 白名单文件
    - [Cline] 切换角色为 BA，指示读取 handoff
5. 接收回报，校验 `openspec/specs/{feature-name}-requirements.md` 存在且非空
6. 更新 `.state.md`: current_handoff=""
7. `[PM] BA 完成，需求规格已生成`

**Step 2: 并行调度 SA 架构设计 + TE 测试用例设计**

> **并行优化**: SA 和 TE 同时派发，均基于 requirement-spec.md 工作。仅 Claude Code 模式支持并行；Cline 模式退化为串行（先 SA 后 TE）。
> SA、TE 在设计中必须参考 `docs/` 下行业技术标准规范文档（`docs/` 始终可读，不受白名单限制）。
1. `[PM] 并行调度 SA 架构设计 + TE 测试用例设计`
2. 写入 handoff-SA: `handoffs/{feature-name}-SA-R1.md`
    - to: SA
    - openspec_strategy: review（full 模式）
    - 白名单: `openspec/specs/{feature-name}-requirements.md`, `openspec/specs/{feature-name}-requirements.md`（OpenSpec 归档版）
    - 期望输出: `output/{feature-name}/drafts/design-v{N}.md`（OpenSpec 格式，含需求↔Task↔验证追溯矩阵）
3. 写入 handoff-TE: `handoffs/{feature-name}-TE-R1.md`
    - to: TE
    - openspec_strategy: review（full 模式）
    - 白名单: `openspec/specs/{feature-name}-requirements.md`, `openspec/specs/{feature-name}-requirements.md`
    - 期望输出: `test/drafts/test-cases-v{N}.md`（OpenSpec 格式，含 TC↔需求追溯矩阵）
4. 更新 `.state.md`: current_step=SA+TE
5. 并行派发任务:
    - [Claude Code] 同时 spawn 两个 SubAgent（SA + TE），各自注入对应 handoff + agent 定义 + 白名单文件
    - [Cline] 串行执行：先 SA 后 TE
6. 等待所有回报，校验:
    - `output/{feature-name}/design.md` 存在且非空
    - `test/test-cases.md` 存在且非空
7. 更新 `.state.md`: current_handoff=""
8. `[PM] SA + TE 完成，技术方案和测试用例已生成`

**Step 3: PM 计划编排**

1. `[PM] 启动 PLAN 计划编排`
2. 读取 design.md Tasks 清单 + testcases.md 用例列表
3. 编排执行计划，写入 `output/{feature-name}/plan-action.md`（过程产物，非 spec）
    - 必须标注 Task 间依赖关系（见 plan-action.md 格式要求）
4. 更新 `.state.md`: current_step=PLAN
5. `[PM] PLAN 完成，执行计划已编排`

**Step 4: 需求评审（SR1）**

1. `[PM] 启动 SR1 需求评审`
2. 向用户呈现摘要：
    - 需求规格要点
    - 技术方案要点
    - 测试覆盖情况
    - 执行计划
3. 等待用户决策:
    - **通过**:
        - 创建 baselines: `output/{feature-name}/baselines/requirement-spec.v1.md` 等
          > 注：此处 baselines 是 propose 阶段的过程快照，用于 SR1 驳回时回退。与 `output/{feature-name}/baselines/`（archive 阶段的归档版本历史）不同。
        - 写入 `output/{feature-name}/approvals/SR1-record.md`
        - 更新 `.state.md`: phase=propose, current_step=PROPOSE-DONE, sr_status.SR1=approved
        - `[PM] SR1 通过，可执行 /agh-apply`
    - **驳回**:
        - 记录驳回原因到 SR1-record.md
        - 回退到对应步骤重新执行
---

## plan-action.md 格式要求

PM 编排计划时，必须标注 Task 间依赖关系，用于 apply 阶段的并行批次调度：

```markdown
# 执行计划: {feature-name}

## Tasks

- Task-1: {描述} [deps: none]
- Task-2: {描述} [deps: none]
- Task-3: {描述} [deps: Task-1]
- Task-4: {描述} [deps: Task-1, Task-2]
```

**依赖判断标准:**
- Task-B 的实现需要 Task-A 的产出代码（如 Task-B 调用 Task-A 创建的函数）则 `[deps: Task-A]`
- Task-B 仅在逻辑上与 Task-A 相关但代码不依赖则 `[deps: none]`
- 无法确定时，标记为 `[deps: none]`（宁可并行多一些，让 TE 审计发现问题）
  **批次计算规则:**
- Batch-1: 所有 `[deps: none]` 的 Task
- Batch-2: 依赖仅在 Batch-1 中已完成的 Task
- Batch-N: 依赖仅在前序 Batch 中已完成的 Task
- 无依赖标注时，所有 Task 归入 Batch-1（全部并行）

---

## 异常处理

- 任何步骤中 SubAgent 回报 status=failed: PM 检查失败原因，决定重试或上升人工
- 文件校验失败（不存在或为空）: 重新派发任务，轮次+1
- 轮次达到 5 次: 上升人工审核

---

## 产物分类规则

> **关键规则:** 区分 Spec 文档和过程产物的存放位置

| 产物类型 | 存放位置 | 说明 |
|---------|---------|------|
| **Spec 文档** | `openspec/specs/` | 含 SHALL 语句 + GWT 验收条件 + 追溯矩阵的正式规格文档 |
| **过程产物** | `output/{feature-name}/` | proposal.md、plan-action.md、code-report、test-report 等开发过程文件 |

### Spec 文档命名规则

- 需求规格: `openspec/specs/{feature-name}-spec.md`
- 测试用例规格: `openspec/specs/test-cases-spec.md`（增量更新，每次需求变更后更新）
- 架构设计规格: `openspec/specs/{feature-name}-architecture-spec.md`（如有）

### test-cases-spec.md 更新时机

- **fast 模式**: apply 阶段 SR3 通过后，PM 增量更新 `openspec/specs/test-cases-spec.md`
- **standard/full 模式**: apply 阶段 SR3 通过后，PM 基于 TE 的测试用例增量更新 `openspec/specs/test-cases-spec.md`
- 更新内容: 追加新 TC-ID、更新追溯矩阵、更新版本号和变更历史

### 过程产物命名规则

- 需求草稿: `output/{feature-name}/proposal.md`
- 执行计划: `output/{feature-name}/plan-action.md`
- 代码报告: `output/{feature-name}/code-report-v{N}.md`
- 测试报告: `output/{feature-name}/temp-test-report-v{N}.md`

### 不允许的操作

- **禁止**将 proposal.md、plan-action.md 等过程产物写入 `openspec/specs/`
- **禁止**将 Spec 文档写入 `output/`（archive 阶段的 baseline 快照除外）

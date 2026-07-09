# Skill: agh-propose

需求分析 → 架构设计 → 测试用例 → 计划编排 → 人工评审。Agent 编排业务 Skill，Skill 通过数据操作 Skill（FeatureService/SpecService/KnowledgeService/CodeGraphService）访问数据层。按 mode 裁剪步骤。
**日志规则:** 见 `templates/logging-standard.md`

---

## 前置检查
1. 读取 `.state.md` 获取当前 req_id
2. 验证 `.state.md` 中 phase=init 且 current_step=INIT-DONE
3. 读取 mode 字段确定流程裁剪方式
4. 验证 `output/{feature-name}/proposal.md` 存在且非空
5. 不满足则阻塞

---

## fast 模式

跳过 BA/SA/TE，PM 直接基于 proposal 生成执行计划。

**Step 1: PM 直接编排计划**
1. 读取 `output/{feature-name}/proposal.md`
2. 调用 **SpecService** 查询已有规格约束（如有），调用 **KnowledgeService** 快速查询相关规则
3. 生成简版 plan-action.md（任务列表 + 执行顺序，无需求对照表）
4. 写入 `output/{feature-name}/plan-action.md`
5. 更新 `.state.md`: phase=propose, current_step=PROPOSE-DONE, sr_status.SR1=skipped
6. `[PM] 计划编排完成（fast模式，跳过SR1），可执行 /agh-apply`

---

## standard 模式

跳过 BA，SA + TE + UX 按需并行执行，无 SR1。

**Step 1: 并行调度 SA + TE + UX**

1. `[PM] standard 模式，并行调度 SA + TE${需UX激活 ' + UX' : ''}`
2. 写入 handoff-SA: `handoffs/{feature-name}-SA-R1.md`
    - to: SA
    - task_phase: design
    - openspec_strategy: review
    - 白名单: `output/{feature-name}/proposal.md`
    - 数据操作 Skill: **SpecService**（写入设计）、**FeatureService**（查询/生成 Feature 草案）、**KnowledgeService**（查询技术栈/平台约束 → 关系图谱层）、**CodeGraphService**（分析现有代码结构 → L1 静态索引）
    - 期望输出: `output/{feature-name}/drafts/design-v{N}.md`
    - 约束: 简版设计（架构 + Tasks清单 + 需求映射简表）
3. 写入 handoff-TE: `handoffs/{feature-name}-TE-R1.md`
    - to: TE
    - task_phase: design
    - openspec_strategy: review
    - 数据操作 Skill: **FeatureService**（Feature 边界条件）、**KnowledgeService**（约束/异常场景）、**SpecService**（验收标准）
    - 期望输出: `test/drafts/test-cases-v{N}.md`
    - 约束: 基于 Proposal 设计测试用例，A/B/C 三类
4. 如需 UX: 写入 handoff-UX
5. 并行派发任务
6. 等待所有回报，校验存在性
7. `[PM] SA + TE 完成`

**Step 2: PM 计划编排**
1. 读取 design.md Tasks 清单 + testcases.md
2. 生成 plan-action.md
3. 更新 `.state.md`: phase=propose, current_step=PROPOSE-DONE, sr_status.SR1=skipped
4. `[PM] 计划编排完成（standard模式，跳过SR1），可执行 /agh-apply`

---

## full 模式

完整流程：BA → (SA + TE) → PM编排 → SR1。

**Step 1: 调度 BA 需求分析**
1. 写入 handoff-BA
    - 数据操作 Skill: **FeatureService**（生成 Feature 草案映射）、**SpecService**（写入需求规格）、**KnowledgeService**（行业标准查询 → LLM Wiki 层 + 关系图谱层）
    - 期望输出: `openspec/specs/{feature-name}-requirements.md`
2. BA Agent 编排：需求澄清 Skill → 需求分析 Skill
3. 校验输出后继续

**Step 2: 并行调度 SA + TE**
1. 写入 handoff-SA / handoff-TE
    - SA 数据操作 Skill: **SpecService** + **FeatureService** + **KnowledgeService** + **CodeGraphService**
    - TE 数据操作 Skill: **FeatureService** + **KnowledgeService** + **SpecService**
2. 并行派发
3. 校验后继续

**Step 3: PM 计划编排**
1. 读取 design.md + testcases.md
2. 生成 plan-action.md（含依赖关系）
3. `[PM] PLAN 完成`

**Step 4: 需求评审（SR1）**
1. 向用户呈现摘要
2. 用户决策：通过/驳回

---

## plan-action.md 格式要求

```markdown
# 执行计划: {feature-name}

## Tasks

- Task-1: {描述} [deps: none] [feature: FEATURE_X_ENABLE]
- Task-2: {描述} [deps: none] [feature: FEATURE_Y_CTRL]
- Task-3: {描述} [deps: Task-1] [feature: FEATURE_X_ENABLE]
```

**批次计算规则**: Batch-1: deps=none → 全部并行；Batch-N: 依赖仅在前序 Batch 中

---

## Agent → Skill → DataService 调度总结

| 阶段 | Agent | 编排的 Skill | 数据操作 Skill |
|------|-------|-------------|---------------|
| BA | BA 业务分析智能体 | 需求澄清 → 需求分析 | FeatureService（草案生成）→ SpecService（需求写入）→ KnowledgeService（标准查询 → LLM Wiki + 关系图谱） |
| SA | SA 系统分析智能体 | 需求矩阵设计 → 概要设计生成 | FeatureService（配置设计）→ SpecService（设计写入）→ KnowledgeService（技术栈/平台约束）→ CodeGraphService（L1 静态 + L2 语义） |
| TE (design) | TE 测试智能体 | 黑盒测试用例生成 | FeatureService（边界条件）→ KnowledgeService（约束/标准）→ SpecService（验收标准） |

---

## 异常处理

- 任何步骤回报 status=failed: PM 检查原因，决定重试或上升人工
- 文件校验失败: 重新派发，轮次+1
- 轮次达 5 次: 上升人工审核

---

## 产物分类规则

| 产物类型 | 存放位置 | 说明 |
|---------|---------|------|
| **Spec 文档** | `openspec/specs/` | 含 SHALL + GWT + 追溯矩阵的正式规格 |
| **过程产物** | `output/{feature-name}/` | proposal.md、plan-action.md、code-report 等 |

- **禁止**将过程产物写入 `openspec/specs/`
- Spec 文档在 apply 阶段 SR3 通过后生成/更新

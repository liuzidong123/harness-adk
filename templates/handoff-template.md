---
handoff_id: "{feature-name}-{STEP-ID}-R{N}"
from: PM
to: "{BA|SA|DE|TE|UX}"
status: pending

# 任务阶段: design（propose阶段前置设计）| audit（apply阶段审计执行task_phase: design | audit
task_type: "{需求分析|架构设计|编码实现|审计验证|测试用例设计|视觉设计}"
output_type: "{android-app|backend-api|cli-tool|data-pipeline|infrastructure|documentation|game|library|custom}"
tech_stack: "{language}/{package_manager}"
openspec_strategy: "{direct|review|change}"
created_at: "{YYYY-MM-DDTHH:MM:SSZ}"
completed_at: ""

# 修复轮次追踪
repair_round: {0-5}
repair_origin: ""  # "de-self-repair" | "pm-redispatch"
---

## 任务描述

{一段话描述本次任务的目标和范围}

## 输入文件（白名单纯。
仅以下文件可被读取，禁止读取白名单外的任何文件：

- {file_path_1}
- {file_path_2}

## 数据操作 Skill 白名单

执行角色仅可通过以下数据操作 Skill 访问数据层（禁止直接操作数据层）：

- [ ] FeatureService — Feature 配置查询/生成/更新
- [ ] SpecService — Spec 需求规格读写
- [ ] KnowledgeService — 领域知识查询（LLM Wiki 层 + 关系图谱层）
- [ ] CodeGraphService — 代码结构分析（L1 静态 / L2 语义 / L3 增量）

## 期望输出

| 阶段 | 输出路径 | 说明 |
|------|---------|------|
| design | `specs/drafts/{artifact}-v{N}.md` | 前期设计产出 |
| audit  | `test/drafts/report-{type}-v{N}.md` | 后期审计报告 |

- 格式: {模板引用或格式描述}
- OpenSpec 参考 `skills/agh-openspec.md`

## 约束

- {constraint_1}
- {constraint_2}

## 参考Skill

- `skills/{skill-file}.md` 中的 {Step N}

## 完成回报（由执行角色填写）。
- status: {done | failed}
  - `done`: 任务成功完成，所有期望输出已生成
  - `failed`: 任务失败，无法在当前轮次内完整- output_files: ["{file_path_1}", "{file_path_2}"]
- summary: "{一句话描述完成情况或失败原因}"
- issues: "{具体错误信息，无问题时填 N/A}"
- dev-test结果: "{PASS/FAIL/SKIP}"  # DE 专用
- code-review结果: "{R1:PASS/R2:PASS/R3:PASS}"  # DE 专用（三阶段 Code-Review）
- test结果: "{黑盒通过率/白盒覆盖率}"  # TE 专用

## 轮次信息

- 当前轮次: {N}/5
- 上轮失败原因: {摘要N/A}
- 失败报告路径: {path → N/A}

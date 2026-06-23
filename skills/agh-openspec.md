# Skill: agh-openspec

通用制品版本管理（OpenSpec 协议）。各角色通过本协议管理其产出物的版本、追溯和变更历史。可通过 `/agh-openspec` 快捷触发 OpenSpec 工作流程
*底层实现。 `@fission-ai/openspec` npm 包（`openspec` CLI），通过 `skills/openspec-init.md` 使用日志规则* → `templates/logging-standard.md`

---

## 什么是 OpenSpec→

OpenSpec 是一套轻量制品版本协议，强制每个角色产出物包含：

- **版本**(v1, v2, ...)
- **状态标志** (draft / review / approved / deprecated)
- **追溯矩阵** (traceability matrix → 需求↔实现↔验证的双向追溯)
- **变更历史** (changelog)

### 核心目录结构

```
{artifact-dir}/
├── specs/          # 已定稿的制品规范（永久保留）
├── drafts/         # 未定稿的工作草稿（可丢弃）└── changes/        # 变更记录（增量日志）
    └── archive/    # 已归档的历史变更
```

### 工作流策略

OpenSpec 支持三种角色协作策略，由 handoff → `openspec_strategy` 字段指定义


| 策略       | 流程                                  | 适用场景               |
| -------- | ----------------------------------- | ------------------ |
| `direct` | 角色直接产出 → PM 校验 → 存入 drafts/         | 简单?快速任务（fast 模式）?  |
| `review` | 角色产出 → 人工审批 → 写入 specs/             | 标准任务（standard 模式）? |
| `change` | 检查specs/ → 修改 → 生成 change → 审批 → 归档 | 变更既有制品（CHANGE 场景）? |


---

## 快捷触发行为

当用户输出`/agh-openspec` 时：

1. **检测项目状态*
- 根据 `.state.md` 确定当前阶段和产出类型
- 检查各角色产出物目录中→ specs/ → changes/ 状态
2. **场景判断**
- **NEW**: 各角色specs/ 为空 → 新建制品
- **RESUME**: → drafts 未定稿或 changes 未归档 → 继续编辑
- **REVIEW**: 全部已归档 → 审查现有制品
3. **自动设置 feature-name**（如无可从上下文推断）

---

## OpenSpec 制品格式

### 基础 Front-matter（所有角色通用户

```yaml
---
artifact_type: requirement-spec | design | code | test-case | ux-design
role: BA | SA | DE | TE | UX
version: v{整数} (YYYY-MM-DD)
status: draft | review | approved | deprecated
spec_name: {feature-name}
---
```

### 追溯矩阵（通用片段）。

```markdown
## 追溯矩阵

| 上游ID | 描述 | 本层实现 | 下游验证 |
|--------|------|---------|---------|
| F001 | ... | 实现位置 | TC-001 |
```

### 变更历史（通用片段）。

```markdown
## 变更历史

| 版本 | 日期 | 变更内容 | 作为|
|------|------|---------|------|
| v1 | YYYY-MM-DD | 初始版本 | {role} |
```

---

## 角色特有格式

### BA → 需求规范

```
字段要求:
- 每个需求用 SHALL 语句 + GWT 验收条件
- 追溯矩阵: 需求ID → Proposal 要点
- 输出: drafts/requirement-spec-v{N}.md → 审批次→ specs/requirement-spec.md
```

### SA → 技术设计

```
字段要求:
- 架构概述 + 技术选型 + 需求技术对照表
- 技术图表（必须，Mermaid 语法，结合实际代码）:
  - 框架图（flowchart/graph）: 模块/组件间关系
  - 逻辑流程图（flowchart）: 核心业务逻辑分支
  - 类图（classDiagram）: 新增/修改的类、成员变量、方法
  - 时序图（sequenceDiagram）: 关键交互流程
- Tasks 拆解清单（含依赖和复杂度）
- 追溯矩阵: 需求ID → Task-ID → 验证方式
- 输出: drafts/design-v{N}.md → 审批 → specs/design.md
```

### DE → 代码报告

```
字段要求:
- 实现摘要 + 文件清单
- 代码结构图（必须，Mermaid 语法，基于实际代码）:
  - 框架图: 本次变更涉及的模块/文件间关系
  - 逻辑流程图: 新增/修改的核心逻辑分支
  - 类图: 新增/修改的类定义、成员变量、方法
  - 时序图: 关键交互流程
- TDD 测试结果（测试数/通过/失败）
- 追溯矩阵: Task-ID → 实现文件 → 测试用例
- 输出: drafts/code-report-v{N}.md → 审批 → specs/code-report.md
```

### TE → 测试用例

```
字段要求:
- 测试用例列表（每个用户 TC-ID, 关联需求 类型, 步骤, 期望）
- 追溯矩阵: 测试用例ID → 需求ID → Task-ID
- 输出: drafts/test-cases-v{N}.md → 审批次→ specs/test-cases.md
```

### UX → 视觉设计

```
字段要求:
- 设计规格说明（布局/配色/交互）
- 追溯矩阵: 设计元素 → 页面/模块ID
- 输出: drafts/ux-design-v{N}.md → 审批次→ specs/ux-design.md
```

## 角色 Handoff 集成

每个角色 handoff 文件 `期望输出` 中指导 OpenSpec 策略和输出路径：

```yaml
openspec_strategy: direct | review | change
openspec_output: |
  - drafts/{artifact}-v{N}.md    # 草稿
  - specs/{artifact}.md          # 定稿（review 策略经审批后端
```

PM 调度时根据 选择 strategy→ - fast → direct
---
- standard → review
- full → review
- CHANGE 场景 → change
---

## 修复循环

同主流程修复循环规则：最多 3 轮，超过上升人工处理

## 异常处理

---
- drafts/ 目录不存储 自动创建
- 版本号冲突 自动递增幅 max+1
- 追溯矩阵不完整 标记录draft，要求角色补充- 变更冲突: 显示冲突内容，人工裁决
```

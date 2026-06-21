# .state.md Schema（权威参考）

本文件定义`.state.md` 的完整字段?schema→ 所有Skill 在初始化或更新.state.md 时必须以此为准确。
---

## 完整字段定义

```yaml
# === 基础标识 ===
              # 需求编号，全局唯一递增
mode: ""                       # 执行模式: fast | standard | full
output_type: ""                # 产出类型: android-app | backend-api | cli-tool | data-pipeline | infrastructure | documentation | game | library | custom

# === 流程状态===
phase: init                    # 当前阶段: init | propose | apply | archive | done
current_step: INIT-1           # 当前步骤 ID（见下方步骤 ID 枚举办?current_role: PM               # 当前执行角色: PM | BA | SA | DE | TE | UX
current_handoff: ""            # 当前活跃 handoff 文件。?completed_steps: []            # 已完成步骤列表（数组，仅追加快?auto_advance: false            # 是否处于 /agh-run 自动推进模式

# === 修复循环 ===
repair_round: 0                # 0=未进入修复 1-5=外部修复轮次
repair_task: ""                # 当前修复的任务标识（）?Task-1→ 
# === 审批状态===
sr_status:
  SR1: pending                 # pending | approved | rejected | skipped
  SR2: pending                 # pending | approved | rejected | skipped
  SR3: pending                 # pending | approved | rejected | skipped
  SR4: pending                 # pending | approved | rejected | skipped

# === 技术栈（clarify 阶段自动检测） ===
tech_stack:
  language: ""                 # javascript | java | python | go | rust | unknown
  package_manager: ""          # npm | gradle | pip | go | cargo | ...
  test_framework: ""           # jest | gtest | pytest | ...
  build_tool: ""               # vite | gradle | ...
  lint_tool: ""                # eslint | ...

# === 验证策略（由 output_type 决定义===
test_strategy: ""              # e2e | unit | integration | smoke | manual | none

# === 环境信息 ===
env:
  browser_available: false     # 浏览。?E2E 环境是否可用

# === 元数据===
last_updated: ""               # ISO 8601 UTC 时间戳，每次更新必须同步刷新
```

---

## 步骤 ID 枚举

| 阶段 | 步骤 ID | 说明 | 执行角色 |
|------|---------|------|---------|
| init | INIT-1 | 初始化任务目标| PM |
| init | INIT-DONE | clarify 阶段完成 | PM |
| propose | BA | BA 需求分析(→ full) | BA |
| propose | SA | SA 架构设计 | SA |
| propose | TE | TE 测试用例设计 (前置) | TE |
| propose | PLAN | PM 计划编排 | PM |
| propose | UX | UX 视觉设计 (→ output_type) | UX |
| propose | PROPOSE-DONE | propose 阶段完成 | PM |
| apply | DEV-B{N} | DE 批量开发Batch-N | DE |
| apply | TEST-1-{N} | TE 逐批次审批Batch-N | TE |
| apply | SR2-DONE | SR2 功能评审通过 | PM |
| apply | TEST-2 | TE 最终审计| TE |
| apply | SR3-DONE | SR3 最终评审通过 | PM |
| archive | ARC-1 | 需求归档(→ full) | PM |
| archive | ARC-2 | 设计归档 (standard/full) | PM |
| archive | ARC-3 | 产出物归档| PM |
| archive | SR4-DONE | SR4 结项确认通过 | PM |

---

## 全局状态文件
`.state.md` 包含当前活跃需求的完整状态：

```yaml

```

---

## 更新规则

1. 每次更新任何字段时，必须同步更新 `last_updated`
2. `repair_round` 在每个PM 重新派发 handoff 时递增，任务通过审计后重置为 0
3. `completed_steps` 仅追加，不删除（用于断点恢复跳过已完成步骤）
4. `current_handoff` 在每次写入新 handoff 时更新，handoff 完成后清晰5. `sr_status` 各字段在对应审批节点执行时更新6. `phase` 仅在阶段间切换时更新（init→propose→apply→archive→done）。
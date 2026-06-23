# Skill: agh-archive

产物归档 + 结项确认。PM 执行，支持首次归档和变更归档两种模式。。
**日志规则* → `templates/logging-standard.md`

---

## 前置检查

1. 读取 `.state.md` 获取当前 req_id → sr_status.SR3
2. 验证 sr_status.SR3=approved（standard/full）或 sr_status.SR3=approved（fast，在apply中已设置）
3. 验证 `output/{feature-name}` 存在且非空. 不满足则阻塞，提示用户先完成 /agh-apply

## 归档模式检查

- **首次归档**: `output/{feature-name}/baselines/` 为空 → 直接复制
- **变更归档**: `output/{feature-name}/baselines/` 已有文件 → merge 模式

## Step ARC-1: 规格基线归档

**执行角色:** PM

1. `[PM] 启动 ARC-1 规格基线归档`
2. **Spec 文档归档到 `openspec/specs/`**:
- 如本次需求产生了新的 Spec 文档（含 SHALL + GWT + 追溯矩阵），更新/创建到 `openspec/specs/{feature-name}-spec.md`
- 如本次需求更新了测试用例规格，增量更新 `openspec/specs/test-cases-spec.md`
- Spec 文档不放入 `output/` 目录
3. **过程产物基线快照到 `output/{feature-name}/baselines/`**:
- `output/{feature-name}/proposal.md` → `output/{feature-name}/baselines/proposal.v{N}.md`
- `output/{feature-name}/plan-action.md` → `output/{feature-name}/baselines/plan-action.v{N}.md`（如有）
- `output/{feature-name}/design.md` → `output/{feature-name}/baselines/design.v{N}.md`（如有）
4. 版本号自动递增（检测已有 baseline 文件确定 N）
5. 校验基线文件存在且非空
6. `[PM] ARC-1 完成`

> **关键规则:**
> - `openspec/specs/` 存放正式 Spec 文档（SHALL + GWT + 追溯矩阵），每次需求变更时更新
> - `output/{feature-name}/baselines/` 存放过程产物的版本快照（proposal/plan-action/code-report/test-report）
> - **禁止**将 proposal.md、plan-action.md 等过程产物写入 `openspec/specs/`

## Step ARC-2: 测试报告归档

**执行角色:** PM

1. `[PM] 启动 ARC-2 测试报告归档`
2. 将最终测试报告归档到 `output/{feature-name}/baselines/`:
    - `test/final-test-report.md` → `output/{feature-name}/baselines/final-test-report.v{N}.md`（如有）
    - `output/{feature-name}/temp-test-report-v{N}.md` → `output/{feature-name}/baselines/temp-test-report.v{N}.md`（如有）
3. `[PM] ARC-2 完成`

## Step ARC-3: 产出物最终确认

**执行角色:** PM

1. `[PM] 启动 ARC-3 产出物最终确认`
2. 根据 output_type 执行额外归档策略。。


| output_type | 额外归档                             | 说明                |
| ----------- | -------------------------------- | ----------------- |
| android-app | app/build/outputs/ → output/lib/ | 复制 Native so 构建产物 |
| 其他          | →                                | 产出物已→ output/     |


3. 校验 `output/{feature-name}/` 非空
4. `[PM] ARC-3 完成`

## Step SR4: 项目结项确认（人工审批）

**执行角色:** PM（人机交互）

**fast 模式** 跳过 SR4，直接结项目- 更新 `.state.md`: phase=done, sr_status.SR4=skipped

- `[PM] 项目结项完成（fast模式）。需求{feature-name} 已归档。`

**standard 模式** 简单 SR4（一句确认）→ - `[PM] 归档完成，请确认结项（Y/N）`

- 用户确认:
    - 更新 `.state.md`: phase=done, sr_status.SR4=approved
    - `[PM] 项目结项完成。需求{feature-name} 已归档。`

**full 模式** 完整 SR4→ 1. `[PM] 启动 SR4 项目结项确认`

2. 向用户呈现归档摘要：

- 归档模式（首）?变更新
- 产出类型: {output_type}
- 技术设计 output/{feature-name}/design.md
- 最终产出 output/{feature-name}/ 文件清单
- 本次需求编码 {feature-name}

1. 等待用户决策略
- **确认结项**:
- 写入 `specs/approvals/SR4-record.md`
    - 更新 `.state.md`:
      ```yaml
      phase: done
      current_step: SR4-DONE
      sr_status.SR4: approved
      ```
    - `[PM] 项目结项完成。需求{feature-name} 已归档。`
- **驳回**:
    - 记录原因，根据问题回退到对应阶段。

## CHANGE 模式特殊处理

> 注：`output/{feature-name}/baselines/` 存放的是过程产物的归档版本历史（每次变更归档前的快照）。用于归档回溯和版本追溯。
> `openspec/specs/` 存放的是正式 Spec 文档，每次需求变更时直接更新（不保留历史版本）。

1. 归档前自动备份过程产物到 `output/{feature-name}/baselines/`:
- `output/{feature-name}/baselines/proposal.v{N}.md`
- `output/{feature-name}/baselines/plan-action.v{N}.md`
- `output/{feature-name}/baselines/design.v{N}.md`
2. Spec 文档直接更新到 `openspec/specs/`（不保留历史版本，baseline 快照在 `output/` 中）
3. 版本号自动递增（检测已有 baseline 文件确定 N）
4. merge 时保持已有内容结构，仅追加或更新变更部分

## 变更归档 Merge 策略

归档时按以下规则处理 output/{feature-name}/ 文件的合并：

### 新增需求（本次 feature-name 引入的全新内容）

- 追加到spec 文件末尾
- → `<!-- {feature} START -->` / `<!-- {feature} END -->` 注释标注来源
- 保持已有内容不变

### 修改需求（本次 feature-name 修改了已有内容）

- 定位到对照feature-name 标注的段。- 替换该段落内部 - 更新注释标注为对应 feature-name

### 删除需求（本次 feature-name 废弃了已有内容）

- 不物理删除原文 - 在对应段落开头添加 `[DEPRECATED by {feature}] → {废弃原因}`
- 保留原文供追溯

### 无标注的历史内容

- 首次归档 feature-name 标注的内容视为初始版本，不做修改
- 如需修改，先补充标注再执行替。。

## 异常处理

- 目标目录不存储 自动创建
- 文件复制失败: 重试一次，仍失败则报错上升人工
- merge 冲突（变更归档）: 呈现冲突内容，请求人工决策
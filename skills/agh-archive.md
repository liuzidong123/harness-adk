# Skill: agh-archive

产物归档 + 结项确认。PM 执行，支持首次归档和变更归档两种模式。。
**日志规则* → `templates/logging-standard.md`

---

## 前置检查
1. 读取 `.state.md` 获取当前 req_id → sr_status.SR3
2. 验证 sr_status.SR3=approved（standard/full）或 sr_status.SR3=approved（fast，在apply中已设置）?3. 验证 `output/` 存在且非。?4. 不满足则阻塞，提示用户先完成 /agh-apply

## 归档模式检查
- **首次归档**: `specs/baselines/` 为空 → 直接复制
- **变更归档**: `specs/baselines/` 已有文件 → merge 模式

## Step ARC-1: 规格基线归档

**执行角色:** PM

1. `[PM] 启动 ARC-1 规格基线归档`
2. 创建当前规格快照顾`specs/baselines/`→    - `specs/requirement-spec.md` → `specs/baselines/requirement-spec.v{N}.md`（如有）
   - `specs/design.md` → `specs/baselines/design.v{N}.md`（如有）
   - `specs/proposal.md` → `specs/baselines/proposal.v{N}.md`
3. 版本号自动递增（检测已）?baseline 文件确定 N→ 4. 校验基线文件存在且非。?5. `[PM] ARC-1 完成`

## Step ARC-2: 测试报告归档

**执行角色:** PM

1. `[PM] 启动 ARC-2 测试报告归档`
2. 将最终测试报告复制到 `specs/baselines/`→    - `test/final-test-report.md` → `specs/baselines/final-test-report.v{N}.md`（如有）
3. `[PM] ARC-2 完成`

## Step ARC-3: 产出物最终确认
**执行角色:** PM

1. `[PM] 启动 ARC-3 产出物最终确认`
2. 根据 output_type 执行额外归档策略。。
| output_type | 额外归档 | 说明 |
|-------------|---------|------|
| android-app | app/build/outputs/ → output/lib/ | 复制 Native so 构建产物 |
| ppt | → | 产出物已→ output/ |
| 其他 | → | 产出物已→ output/ |

3. 校验 `output/` 非空
4. `[PM] ARC-3 完成`

## Step SR4: 项目结项确认（人工审批）

**执行角色:** PM（人机交互）

**fast 模式。?* 跳过 SR4，直接结项目- 更新 `.state.md`: phase=done, sr_status.SR4=skipped
- `[PM] 项目结项完成（fast模式）。需求{feature-name} 已归档。`

**standard 模式。?* 简单?SR4（一句确认）→ - `[PM] 归档完成，请确认结项（Y/N）`
- 用户确认:
  - 更新 `.state.md`: phase=done, sr_status.SR4=approved
  - `[PM] 项目结项完成。需求{feature-name} 已归档。`

**full 模式。?* 完整 SR4→ 1. `[PM] 启动 SR4 项目结项确认`
2. 向用户呈现归档摘要：
   - 归档模式（首）?变更新   - 产出类型: {output_type}
   - 技术设计 specs/design.md
   - 最终产出 output/ 文件清单
   - 本次需求编码 {feature-name}
3. 等待用户决策略   - **确认结项**:
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

> 注：`specs/baselines/` 存放的是归档版本历史（每次变更归档前的快照）。用于归档回溯和版本追溯
1. 归档前自动备份当→ specs/ → specs/baselines/
   - specs/baselines/requirement-spec.v{N}.md
   - specs/baselines/design.v{N}.md
   - specs/baselines/proposal.v{N}.md
2. 版本号自动递增（检测已）?baseline 文件确定 N→ 3. merge 时保持已有内容结构，仅追加或更新变更部分

## 变更归档 Merge 策略

归档时按以下规则处理 specs/ 文件的合并：

### 新增需求（本次 feature-name 引入的全新内容）
- 追加快?spec 文件末尾
- → `<!-- {feature} START -->` / `<!-- {feature} END -->` 注释标注来源
- 保持已有内容不变

### 修改需求（本次 feature-name 修改了已有内容）
- 定位到对照feature-name 标注的段。?- 替换该段落内部?- 更新注释标注为最。?feature-name

### 删除需求（本次 feature-name 废弃了已有内容）
- 不物理删除原。?- 在对应段落开头添。? `[DEPRECATED by {feature}] → {废弃原因}`
- 保留原文供追溯
### 无标注的历史内容
- 首次遇到期?feature-name 标注的内容视为初始版本，不做修改
- 如需修改，先补充标注再执行替。。
## 异常处理

- 目标目录不存储 自动创建
- 文件复制失败: 重试一次，仍失败则报错上升人工
- merge 冲突（变更归档）: 呈现冲突内容，请求人工决策
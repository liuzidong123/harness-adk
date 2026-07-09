# Skill: agh-archive

产物归档 + 结项确认 + 一致性快照检查。PM 执行，支持首次归档和变更归档两种模式。
**日志规则** → `templates/logging-standard.md`

---

## 前置检查

1. 读取 `.state.md` 获取当前 req_id → sr_status.SR3
2. 验证 sr_status.SR3=approved
3. 验证 `output/{feature-name}` 存在且非空
4. **Spec 文件强制检查（阻断条件）**:
   - 检查 `openspec/specs/{feature-name}-spec.md` 存在且非空
   - 检查 `openspec/specs/test-cases-spec.md` 存在且非空
   - 检查 test-cases-spec.md 包含新 feature 的 TC-ID 条目
   - 如任一项不满足 → `[PM] ⛔ 阻断`
5. **一致性快照检查（阻断条件）**:
   - 检查 code-report.md 中 R2 Review 结论为 Pass
   - 检查黑盒测试通过率 = 100%（或手动验证通过）
   - 检查白盒覆盖率 ≥ 80%（如适用）
   - 检查 Feature ↔ Spec 双向绑定完整
   - 如任一项不满足 → `[PM] ⛔ 阻断: 一致性快照检查未通过，需修复后重新归档`

## 归档模式检查

- **首次归档**: `output/{feature-name}/baselines/` 为空 → 直接复制
- **变更归档**: `output/{feature-name}/baselines/` 已有文件 → merge 模式

## Step ARC-1: 规格基线归档

**执行角色:** PM

1. Spec 文档归档到 `openspec/specs/`
2. 过程产物基线快照到 `output/{feature-name}/baselines/`
3. 版本号自动递增
4. 校验基线文件存在且非空

## Step ARC-2: 测试报告归档

**执行角色:** PM

1. 测试报告归档到 `output/{feature-name}/baselines/`

## Step ARC-3: 产出物最终确认

**执行角色:** PM

按 output_type 执行额外归档策略。
校验 `output/{feature-name}/` 非空。

## Step SR4: 项目结项确认（人工审批）

- fast 模式跳过 SR4
- standard 模式简单确认
- full 模式完整 SR4

## 一致性快照最终检查（归档前）

```
[PM] 归档前一致性快照检查
1. openspec/specs/{feature-name}-spec.md → 存在且非空 ✅
2. openspec/specs/test-cases-spec.md → 含新 TC-ID ✅
3. code-report R2 Review → Pass ✅
4. 黑盒测试通过率 → 100% ✅
5. 白盒覆盖率 → ≥80% ✅ (如适用)
6. Feature ↔ Spec 追溯 → 完整 ✅
7. 三阶段 Code-Review 记录 → 完整 ✅
→ 一致性快照通过，可归档
```

## CHANGE 模式特殊处理

1. 归档前自动备份过程产物到 baseline
2. Spec 文档直接更新到 `openspec/specs/`
3. 版本号自动递增
4. Merge 策略：
   - 新增需求 → 追加到 `<!-- {feature} START --> / <!-- {feature} END -->` 标注
   - 修改需求 → 替换 `{feature}` 标注段落
   - 删除需求 → `[DEPRECATED by {feature}]` 标记（不物理删除）

## 异常处理

- 目标目录不存在 → 自动创建
- 文件复制失败 → 重试一次
- merge 冲突 → 呈现冲突内容，请求人工决策
- 一致性快照检查失败 → 回退到对应阶段修复

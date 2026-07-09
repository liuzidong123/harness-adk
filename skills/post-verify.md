# Skill: post-verify

DE 交付前校验标准操作规程。dev-test 通过后、填写回报前执行。校验含三阶段 Code-Review 完整性。

**日志规则：** 见 `templates/logging-standard.md`

---

## 触发时机

dev-test skill 全部通过后执行。这是 DE 交付前的最后一道自检。

## Step 1: 运行 verify.sh

1. 执行 `./scripts/verify.sh A`（文件存在性检查）
2. 确认退出码为 0
3. 如失败：检查缺失文件，补充后重试

## Step 1a: 运行 verify.sh B（阶段产出物完整性）

1. 执行 `./scripts/verify.sh B ${feature_name}`（阶段产出物完整性，含单元测试文件检查）
2. 确认退出码为 0
3. 如失败：补充缺失的交付物后重试

## Step 2: 产出物完整性

1. 对照 handoff 中"期望输出"列表
2. 逐一确认每个文件：存在、非空、格式正确
3. 如有缺失：补充生成

## Step 3: 三阶段 Code-Review 完整性检查

1. 确认 code-report.md 中包含三阶段 Code-Review 结果：
   - **R1 Pre-Review**: 审核结论已记录（PASS/CONDITIONAL_PASS）
   - **R2 Implementation Review**: 结论为 Pass
   - **R3 Refactoring Review**: 质量报告已记录
2. 如缺失任一阶段记录：回退补充执行对应 Review Skill

## Step 4: 数据操作 Skill 合规检查

1. 确认代码未直接操作数据层（Feature/Spec/Knowledge/Code 存储）
2. 确认所有数据读写通过 FeatureService/SpecService/KnowledgeService/CodeGraphService
3. 确认 CodeGraphService 图谱已增量更新（L3）

## Step 5: 无越权修改检查

1. 列出本次所有文件变更（新增 + 修改）
2. 对照 handoff 白名单 + 期望输出路径
3. 确认没有修改白名单外的文件
4. 如有越权修改：撤销该修改

## Step 6: 回归检查

1. 如果是修复轮次（R>1）：确认之前通过的测试仍然通过
2. 如果有已有代码：确认未破坏已有功能（黑盒 100% + 白盒 ≥ 80%）

## 输出

在 code-report.md 中记录：

```
## 自检结果
- R1 Pre-Review: PASS
- R2 Implementation Review: PASS
- R3 Refactoring Review: PASS
- dev-test: PASS
- post-verify: PASS
```

## 失败处理

- verify.sh 退出码非 0：根据错误信息修复，重新执行
- 三阶段 Code-Review 记录缺失：回退补充执行对应 Review Skill
- 发现越权修改：撤销后重新检查
- 发现直接操作数据层：改用数据操作 Skill 后重新检查
- 回归失败：修复回归问题，重新执行 dev-test + post-verify

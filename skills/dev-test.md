# Skill: dev-test

DE 开发自测标准操作规程。编码完成后、提交回报前必须执行。此步在 TDD 循环的绿阶段（R2 通过后）和重构阶段（R3）之间执行，验证黑盒/白盒测试与构建。
**日志规则** → `templates/logging-standard.md`

---

## 触发时机

DE 完成编码实现（绿阶段 + R2 Implementation Review 通过后），在填写 code-report.md 之前执行。

## 前置: 读取技术栈信息

1. 读取 `.state.md` → tech_stack、test_strategy、output_type 字段
2. 根据 tech_stack.language 确定命令路由

## Step 1: 测试执行（黑盒 + 白盒）

根据 tech_stack.language 路由测试命令。

| language | 检测方式 | 默认命令 |
|----------|---------|---------|
| javascript | package.json scripts.test | npm test / yarn test / pnpm test |
| python | pytest.ini / pyproject.toml | pytest / python -m pytest |
| go | go.mod | go test ./... |
| rust | Cargo.toml | cargo test |
| java | pom.xml / build.gradle | mvn test / gradle test |
| unknown | .state.md tech_stack.test_framework | 用户指定命令 |

测试分类执行：
- **黑盒测试**（红阶段生成）：验证输入→输出符合 Spec，通过率必须 100%
- **白盒测试**（白盒阶段生成）：验证分支/路径/条件覆盖，覆盖率 ≥ 80%

跳过条件：test_strategy=none / manual → 跳过此步，记录原因。

执行后记录：黑盒通过数 / 白盒覆盖率 / 失败数。如有失败：修复代码，重新运行。

## Step 2: Lint 检查

根据 tech_stack.language 路由 lint 命令。自动修复可修复项。

## Step 3: 构建验证

根据 tech_stack.language 路由构建命令。

跳过条件：test_strategy=manual / none → 跳过。

## Step 4: 自检清单

逐项确认:
- [ ] 所有新增代码有对应黑盒测试（test_strategy=none/manual 时改为确认无需自动化测试）
- [ ] 黑盒测试全部通过（通过率 100%）
- [ ] 白盒覆盖率 ≥ 80%（如适用）
- [ ] R2 Implementation Review 已通过
- [ ] Lint 无错误（或已跳过且记录原因）
- [ ] 构建成功（或已跳过且记录原因）
- [ ] 未修改白名单外的文件
- [ ] 未引入新的安全漏洞（无硬编码密钥、无 SQL 拼接等）
- [ ] 数据操作 Skill 使用正确（未直接操作数据层）

## 输出

将结果记录到 `output/{feature-name}/drafts/code-report.md`：

```
## 测试结果
- 黑盒测试数: {N}
- 黑盒通过: {N}（100%）
- 白盒覆盖率: {N}%（≥80%）
- 失败: 0

## 自检结果
- R2 Implementation Review: PASS
- dev-test: PASS
```

## 失败处理

- 任何一步失败：修复后从该步重新执行
- DE 内部自修最多 3 次（子循环）：超出后在 handoff 回报中标注 status=failed，附带错误日志
- 3 次限制是 DE 角色内部自修上限；PM 层面的修复循环最多 5 轮
- 每轮 PM 派发修复时，DE 内部最多尝试 3 次自修；仍失败则回报 PM

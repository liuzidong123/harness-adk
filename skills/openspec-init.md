# Skill: openspec-init

使用 `@fission-ai/openspec` npm 包（`openspec` CLI）初始化和管理OpenSpec 制品。不自行创建 OpenSpec 目录结构——全部委托给 `openspec` CLI 命令。。
**前置条件:** Node.js >= 20.19.0，已全局安装 `@fission-ai/openspec`（`npm install -g @fission-ai/openspec@latest`）?**日志规则* → `templates/logging-standard.md`

---

## 快捷触发行为

当用户输出`/openspec-init` 时：

1. **检查openspec CLI**
- 运行 `openspec --version` 确认 CLI 可用
- 如不可用，提示执行`npm install -g @fission-ai/openspec@latest`
2. **检测当前项目*
- 读取 `.state.md` 获取当前 REQ-ID
- 运行 `openspec status --json` 检测项目是否已初始。?OpenSpec
3. **两阶段流程*
- **Phase 1**: `openspec init` → 初始化项目级 OpenSpec（仅首次）?   - **Phase 2**: 基于 openspec CLI → change/spec 管理，集成到 Android Game Harness 角色流程

---

## Step 1: 项目初始化（首次）。

如项目尚未初始化 OpenSpec，执行：

```bash
cd {project_root}
openspec init --tools none --no-interactive
```

选项说明确| 选项 | 用户|
|------|------|
| `--tools none` | 不配置AI 工具集成（mini-harness 自行管理）?|
| `--no-interactive` | 非交互式，适合 agent 自动|
| `--force` | 如已有旧版文件，自动清理 |

初始化后验证：`openspec status --json`

---

## Step 2: 角色制品管理

各角色通过 `openspec` CLI 管理其产出物。所有CLI 命令支持 `--json` 输出，便。?agent 解析。。

### BA → 需求规范

```bash
# 创建变更
openspec change start requirement-spec -m "{feature-name} 需求规格

# 查看变更状态openspec status --json

# 验证规格完整openspec validate requirement-spec --json

# 归档完成的需求规范openspec archive requirement-spec
```

### SA → 技术设计

```bash
# 创建设计变更
openspec change start design-{feature} -m "{feature-name} 技术设计

# 列出所有活跃变更openspec list --json

# 设计完成后归档openspec archive design-{feature}
```

### DE → 代码报告

```bash
# → Task 创建代码变更
openspec change start code-task-{N} -m "Task-{N} 实现"

# 查看规格差异
openspec show code-task-{N} --json
```

### TE → 测试用例

```bash
# 创建测试用例变更
openspec change start test-{feature} -m "{feature-name} 测试用例"

# 验证测试规格
openspec validate test-{feature} --all --json
```

### UX → 视觉设计

```bash
# 创建设计变更
openspec change start ux-{feature} -m "REQ-{N} UX 设计"
```

---

## Step 3: 状态查询与验证

```bash
# 查看完整项目状态（JSON 格式）?agent 解析。?openspec status --json

# 列出所有specs
openspec list --specs --json

# 列出所有changes
openspec list --json

# 查看某个 change/spec 详情
openspec show {item-name} --json

# 批量验证
openspec validate --all --json
```

---

## → Android Game Harness 的映射

Android Game Harness 角色制品牌?OpenSpec 命名约定义


| 角色  | OpenSpec change 命名 | 对应 Android Game Harness 产物         | 数据操作 Skill |
| --- | ------------------ | ---------------------------------- | ------------- |
| BA  | `requirement-spec` | ba/drafts/requirement-spec-v{N}.md | SpecService + FeatureService + KnowledgeService |
| SA  | `design-{feature}` | sa/drafts/design-v{N}.md           | SpecService + FeatureService + KnowledgeService + CodeGraphService |
| DE  | `code-task-{N}`    | de/drafts/code-report-v{N}.md      | FeatureService + SpecService + KnowledgeService + CodeGraphService |
| TE  | `test-{feature}`   | te/drafts/test-cases-v{N}.md       | FeatureService + KnowledgeService + CodeGraphService |
| UX  | `ux-{feature}`     | ux/drafts/ux-design-v{N}.md        | KnowledgeService + SpecService |

> **说明**：OpenSpec CLI 作为版本化持久层，数据操作 Skill（FeatureService/SpecService/KnowledgeService/CodeGraphService）的读写最终通过 `openspec change/spec/archive` 命令落地和追溯。


---

## 完整工作流示例（BA 需求规格）

```bash
# 1. 初始化（仅首次）
openspec init --tools none --no-interactive

# 2. 创建需求规格变更 openspec change start requirement-spec -m "F001 用户登录需求

# 3. 检查状态 openspec status --json

# 4. 编写完成后验证 openspec validate requirement-spec --json

# 5. 归档（SR1 审批通过后）
openspec archive requirement-spec
```

---

## 异常处理


| 问题          | 处理                                                 |
| ----------- | -------------------------------------------------- |
| CLI 未安排     | `npm install -g @fission-ai/openspec@latest`       |
| 版本 < 1.4.0  | `npm update -g @fission-ai/openspec`               |
| 项目标init     | 自动执行 `openspec init --tools none --no-interactive` |
| validate 失败 | → CLI 输出的错误信息逐一修复                                  |
| archive 冲突  | `openspec show {item} --json` 查看差异，人工裁决            |

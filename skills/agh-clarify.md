# Skill: agh-clarify

需求初始化与澄清。PM 主导，人机协作。环境预检时检测数据操作 Skill 依赖（FeatureService/SpecService/KnowledgeService/CodeGraphService）的可⽤性。
**日志规则** → `templates/logging-standard.md`

---

## 前置检查

1. 检查 `.state.md` 是否存在
2. 如存在，读取其中 feature-name，检查 phase
3. 检测场景模式（按优先级从高到低）：
- **RESUME**: output/{feature-name} → phase 非空 phase≠done → 有未完成的流程
- **CHANGE**: output/{feature-name}/baselines/ 目录下有归档文件（有已归档历史需求）→ 变更模式
- **NEW**: 以上均不满足 → 全新项目

⚠️ phase=done + output/{feature-name}/baselines/ 有文件时，必须进行 CHANGE 模式

## 环境预检（含数据操作 Skill 检测）

1. 自动检测项目技术栈（按优先级依次检测）
2. 检测包管理器、测试框架、构建工具、lint 工具
3. **数据操作 Skill 基础设施检测**：
   - **FeatureService** 可用性：检查 `.state.md` 中已定义的 Feature 配置（如有）
   - **SpecService** 可用性：检查 `openspec/specs/` 目录是否存在
   - **KnowledgeService** 可用性：检查 Knowledge 存储配置（LLM Wiki + 关系图谱两层）
   - **CodeGraphService** 可用性：检查代码仓库是否为 Git 仓库、Clang 工具链是否可用
4. 浏览器可用性检测（仅当 output_type 涉及 UI 时执行）
5. 将所有检测结果写入 `.state.md` → tech_stack → env 字段
6. 如检测结果不完整，向用户展示并请求补充

```
[环境检测结果]
语言: {language}
包管理器: {package_manager}
测试框架: {test_framework}
构建工具: {build_tool}
Lint: {lint_tool}
数据操作 Skill:
  FeatureService: {可用/不可用}
  SpecService: {可用/不可用}
  KnowledgeService: {可用/不可用}
  CodeGraphService: {可用/不可用}
以上信息是否正确？如有遗漏请补充
```

## Step 1: 初始化任务目标

**执行角色:** PM

1. 生成需求名称 feature-name
2. 创建隔离目录结构
3. 写入 `.state.md`（完整 schema → `templates/state-template.md`）
4. `[PM] 初始化完成，进入需求澄清`

## Step 2: 需求澄清（人机协作）

**执行角色:** PM

1. 读取 `docs/` 目录下的参考资料
2. 调用 **KnowledgeService**（LLM Wiki 层 + 关系图谱层）查询行业背景知识和历史案例
3. 基于参考资料和 Knowledge 查询结果，逐轮向用户提问
4. CHANGE 模式下读取已有 baseline，仅围绕变更点提问
5. 根据用户回答，生成 Proposal 草稿

## Step 3: 产出类型选择

**执行角色:** PM

推荐 output_type 和 test_strategy 默认值，用户确认后写入 `.state.md`。

## Step 4: 模式选择

**执行角色:** PM

根据需求规模推荐模式（fast/standard/full），用户选择后写入 `.state.md`。

## Step 5: Proposal 定稿

**执行角色:** PM

1. Proposal 草稿写入 `output/{feature-name}/proposal.md`（过程产物）
2. 向用户呈现 Proposal 全文，请求确认
3. 确认通过 → 更新 `.state.md`: `phase: init, current_step: INIT-DONE`
4. 用户要求修改 → 循环修改

## Proposal 格式

```markdown
# Proposal: {项目/需求标题}

## 背景与目标

## 范围
- 包含: {列举}
- 不包含 {列举}

## 关键约束
- {约束1}
- {约束2}

## 参考资料
- {来源列表}

## 已知 Knowledge 约束
- {Knowledge 查询到的相关规则/约束}
```

## 异常处理

- reference/ 为空：提示用户补充参考资料或直接口述需求
- RESUME 模式用户选择放弃：清理 `.state.md`，重新进行 NEW 模式
- **KnowledgeService** 不可用时降级为纯人机交互（无背景知识辅助）

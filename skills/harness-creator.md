# Skill: harness-creator

根据用户自定义的角色和职责定义，参考Android Game Harness-main 的设计理念，生成适配 Gerrit 仓库风格的新 harness 框架项目标

---

## 前置检查

1. 确认当前工作目录为空或用户指定的目标目录
2. 确认用户已准备好角色定义信息

---

## 执行流程

### Phase 1: 收集用户输入

#### Step 1: 收集项目元数据

向用户提问（每轮  个问题）→

1. **项目名称**：用于生成目录名和文件引用（）?`tv-input-service`→
2. **项目主语言**：Java / Kotlin / Python / Go / 其他
3. **构建系统**：Android.bp / pom.xml / Makefile / 其他

#### Step 2: 收集角色定义

逐轮收集角色信息，每个角色需定义

```
角色名称: {→ PM、BA、SA、DE、TE、reviewer、自定义}
身份: {一句话描述角色定位}
职责: {列举核心职责}
输入: {该角色读取哪些文件信息}
输出: {该角色产出哪些文件}
阻塞条件: {什么情况下该角色无法执行}
禁止事项: {该角色绝对不能做的事}
```

**约束。?*
- PM 角色必须保留（调度中枢）
- 角色数量 → 2（至）?PM + 1 个执行角色）
- reviewer 角色为可选的只读评审代理

**默认角色模板（用户可修改/增删）：**

| 角色 | 默认身份 | 默认职责 |
|------|---------|---------|
| PM | 流程调度中枢 | 调度、检查、人机交互、动态同步管理 |
| BA | 业务分析智能体 | 需求澄清+分析，编排业务 Skill（SHALL + GWT）|
| SA | 系统分析智能体 | 需求矩阵+概要设计，编排系统分析 Skill |
| DE | 开发智能体 | TDD 编码实现 + 三阶段 Code-Review（R1/R2/R3）|
| TE | 测试智能体 | 黑盒+白盒测试用例生成、审计验证 |
| reviewer | 只读评审代理 | 代码/架构/配置评审 |

**数据操作 Skill（所有执行角色共享的数据层访问）：**

| 数据操作 Skill | 职责 | 存储架构 |
|---------------|------|---------|
| FeatureService | Feature 配置管理（开关/参数/依赖/作用域/生命周期） | Feature 存储 |
| SpecService | Spec 需求规格读写 | Spec 存储 |
| KnowledgeService | 领域知识查询/沉淀 | Hybrid: LLM Wiki 层（语义检索）+ 关系图谱层（确定性查询），三级缓存 |
| CodeGraphService | 代码结构分析 | L1 静态索引（Clang AST）+ L2 LLM 语义 + L3 增量更新 |

> **架构原则**：Agent 编排业务 Skill，业务 Skill 通过数据操作 Skill 访问数据层；执行角色禁止直接操作数据层。

#### Step 3: 收集规范需求

1. **测试规范**：是否需要测试规范文档？（默认：是）
2. **架构规范**：是否需要分层架构检查？（默认：是）
3. **配置风险检查*：是否需求Feature/Config 配置检查？（默认：是）
4. **隐性约定文件*：是否需要隐性业务约定模板？（默认：是）

#### Step 4: 确认收集结果

向用户呈现完整收集结果，请求确认。用户可修改后重新确认。。

---

### Phase 2: 生成框架文件

按以下顺序生成所有文件。每个文件通过模板变量替换生成熟。

#### 模板变量体系

| 变量 | 来源 | 示例子?|
|------|------|--------|
| `{PROJECT_NAME}` | Step 1 | tv-input-service |
| `{ROLES}` | Step 2 | PM, BA, SA, DE, TE, reviewer |
| `{ROLE_LIST}` | Step 2 | PM/BA/SA/DE/TE/reviewer（路径用户|
| `{PM_RULES}` | Step 2 (PM) | PM 的完整规范|
| `{ROLE_DEFINITIONS}` | Step 2 | 所有角色的契约定义 |
| `{ROLE_ISOLATION_RULES}` | Step 2 | 角色隔离规则（自动生成） |
| `{HAS_REVIEWER}` | Step 2 | true/false |
| `{HAS_BA}` | Step 2 | true/false |
| `{HAS_SA}` | Step 2 | true/false |
| `{LANGUAGE}` | Step 1 | Java |
| `{BUILD_SYSTEM}` | Step 1 | Android.bp |
| `{PHASES}` | 固定 | explore, propose, apply, archive |

#### 生成顺序

```
1. 顶层文件
   ├── AGENTS.md
   ├── CLAUDE.md
   └── REVIEW.md（仅）?HAS_REVIEWER=true→ 

2. .claude/ 目录
   ├── settings.local.json.example
   ├── commands/（空目录）。
   ├── rules/（空目录）。
   ├── agents/
   →   ├── PM.md
   →   ├── {角色}.md（每个用户定义的角色）。
   →   └── reviewer.md（仅）?HAS_REVIEWER=true→ 
   ├── skills/
   →   ├── tspec/
   →   →   ├── explore.md
   →   →   ├── propose.md
   →   →   ├── apply.md
   →   →   └── archive.md
   →   ├── data-services/
   →   →   ├── feature-service.md
   →   →   ├── spec-service.md
   →   →   ├── knowledge-service.md
   →   →   └── code-graph-service.md
   →   ├── code-review/
   →   →   ├── r1-pre-review.md
   →   →   ├── r2-implementation-review.md
   →   →   └── r3-refactoring-review.md
   →   ├── prepare-review/
   →   →   └── SKILL.md
   →   ├── architecture-review/
   →   →   └── SKILL.md
   →   └── config-risk-review/
   →       └── SKILL.md
   └── hooks/
       ├── guard_write.py
       ├── ensure_change_context.py
       └── run_checks.sh

3. docs/ 目录
   ├── architecture/
   →   ├── index.md
   →   └── implicit-contracts.md
   ├── product/
   →   └── index.md
   └── standards/
       ├── testing.md
       ├── tvinput.md（可选）
       └── database.md（可选）

4. openspec/ 目录
   ├── changes/
   →   └── archive/
   └── specs/

5. 校验脚本
   └── scripts/check-harness.sh
```

#### 角色隔离规则自动生成逻辑

根据用户定义的角色列表，自动生成以下隔离规则

```
FOR 每个角色:
  - {角色名}：只做{职责}，禁止{禁止事项}
END FOR

共同禁止步
- 禁止读取白名单外的文件
- 禁止引用对话历史中其他角色的推理
- 禁止修改上游制品
- 禁止调度其他角色（PM 除外部。
```

#### tspec skills 角色适配逻辑

根据用户定义的角色，自动适配 tspec skills 中的角色调度。。

**explore.md（对照/pdt-init）：**
- 固定：PM 主导需求澄清
- 如有 BA：增BA 需求分析步骤
- 如无 BA：PM 直接处理需求

**propose.md（对照/pdt-propose）：**
- 如有 BA：BA 需求分析。?SA 架构设计
- 如无 BA：SA 直接口proposal 设计
- 如有 SA：SA 架构设计
- TE 测试用例设计
- PM 计划编排

**apply.md（对照/pdt-apply）：**
- DE 开发（必须）?DE 角色。。
- TE 审计（必须有 TE 角色）。
- 修复循环（DE→TE，最）?5 轮）
- SR2/SR3 人工审批

**archive.md（对照/pdt-archive）：**
- PM 执行归档
- SR4 结项确认

---

### Phase 3: 校验

#### Step 5: 文件完整性检查

1. 检查所有必需文件是否存在且非。。
2. 检查目录结构是否完整
3. 检查模板变量是否全部替换（无残暴?`{变量名}`）。

#### Step 6: 结构一致性验证。

1. .claude/agents/ 下文件数 = 用户定义角色。。
2. tspec skills 引用的角色与用户定义一切。
3. AGENTS.md 包含所有角色的隔离规则
4. CLAUDE.md 包含四阶段流程纪律

#### Step 7: 呈现生成结果

向用户呈现：
```
[harness-creator 生成完成]
项目名称: {PROJECT_NAME}
角色列表: {ROLES}
生成文件。? {N}
目录结构:
  ├── AGENTS.md
  ├── CLAUDE.md
  ├── REVIEW.md
  ├── .claude/...
  ├── docs/...
  ├── tspec/...
  └── scripts/check-harness.sh

校验结果: {PASS/FAIL}
```

---

## 异常处理

- 目标目录非空：提示用户确认是否覆盖
- 角色定义不完整：提示用户补充缺失字段
- 模板变量替换失败：记录错误，跳过该文件，最终报告
- 校验失败：列出缺失文件，提示用户手动补充

## 使用方式

→ Claude Code 对话框中输入。。
```
请执行harness-creator skill，我要创建一个新。?harness 框架
```

或在 skill 被注册为命令后：
```
/harness-creator
```

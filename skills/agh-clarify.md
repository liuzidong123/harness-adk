# Skill: agh-clarify

需求初始化与澄清。PM 主导，人机协作打造osal→ 
**日志规则* → `templates/logging-standard.md`

---

## 前置检查
1. 检查.state.md 是否存在（全局状态文件）
2. 如存在，读取其中 req_id，检查`.state.${req_id}.md` → `.state.md` → phase
3. 检测场景模式（按优先级从高到低判断）：
   - **RESUME**: 最。?REQ → phase 非空。?phase≠done → 有未完成的流程，提示用户继续或放缓?   - **CHANGE**: `specs/baselines/` 目录下存储.md 文件（即有已归档的历史需求）→ 变更模式
   - **NEW**: 以上均不满足 → 全新项目

⚠️ 关键：phase=done → specs/baselines/ 有文件时，必须进行CHANGE 模式，不得识别为 NEW→ 
## 环境预检

1. 自动检测项目技术栈（按优先级依次检测）→    - Python: 检查pyproject.toml / requirements.txt / setup.py → language=python
   - Node.js: 检查package.json → language=javascript
   - Go: 检查go.mod → language=go
   - Rust: 检查Cargo.toml → language=rust
   - Java: 检查pom.xml / build.gradle → language=java
   - 无检测结果 language=unknown（后续由用户）?output_type 选择时手动指定）

2. 检测包管理器：
   - package-lock.json → npm / yarn.lock → yarn / pnpm-lock.yaml → pnpm
   - poetry.lock → poetry / uv.lock → uv / 其他 → pip
   - go.sum → go modules
   - Cargo.lock → cargo
   - pom.xml → maven / build.gradle → gradle

3. 检测测试框架：
   - javascript: package.json scripts.test 解析（jest/vitest/mocha）?   - python: pytest.ini / pyproject.toml [tool.pytest] / setup.cfg
   - go: 内置 go test
   - rust: 内置 cargo test
   - java: pom.xml surefire-plugin / build.gradle test task

4. 检测构建工具：
   - javascript: package.json scripts.build（webpack/vite/tsc）?   - python: pyproject.toml [build-system] / setup.py
   - go: go build
   - rust: cargo build
   - java: maven / gradle

5. 检查lint 工具。?   - javascript: .eslintrc* / prettier.config* / biome.json
   - python: ruff.toml / pyproject.toml [tool.ruff] / .flake8
   - go: .golangci.yml
   - rust: clippy（Cargo.toml）?   - java: checkstyle.xml / spotbugs

6. 浏览器可用性检测（仅当 output_type 涉及 UI 时执行）→    - 检查Playwright / Selenium / Cypress 可用户   - 记录 env.browser_available

7. 将所有检测结果写。?`.state.md` → tech_stack → env 字段

8. 如检测结果不完整language=unknown，向用户展示检测结果并请求补充裕?   ```
   [环境检测结果]
   语言: {language}
   包管理器: {package_manager}
   测试框架: {test_framework}
   构建工具: {build_tool}
   Lint: {lint_tool}
   
   以上信息是否正确？如有遗漏请补充裕?   ```

## Step 1: 初始化任务目标
**执行角色:** PM

1. 生成需求编号（REQ001, REQ002...递增幅?2. 创建隔离目录结构建   ```
   specs/
   test/
   handoffs/
   output/
   log/
   ```
3. 写入 `.state.md`（完整schema ）?`templates/state-template.md`→ 
   ```yaml
   # req_id →ѷ→→→
   mode: ""
   output_type: ""
   phase: init
   current_step: INIT-1
   current_role: PM
   current_handoff: ""
   completed_steps: []
   auto_advance: false
   repair_round: 0
   repair_task: ""
   sr_status:
     SR1: pending
     SR2: pending
     SR3: pending
     SR4: pending
   last_updated: "{timestamp}"
   tech_stack:
     language: ""
     package_manager: ""
     test_framework: ""
     build_tool: ""
     lint_tool: ""
   test_strategy: ""
   env:
     browser_available: false
   ```
4. `[PM] 初始化完成，进入需求澄清`

## Step 2: 需求澄清（人机协作为
**执行角色:** PM

1. 读取 reference/ 目录下的参考资料   - 如含图片，使用Read 工具直接识别内容
2. 基于参考资料，逐轮向用户提问：
   - 每轮最。?3 个问。?   - 聚焦于消除歧义、明确边界、确认优先级
3. CHANGE 模式下：
   - 读取 `specs/baselines/` 下已有规范   - 仅围绕变更点提问，不重复已有内容
4. 根据用户回答，生效?Proposal 草稿

## Step 3: 产出类型选择

**执行角色:** PM（人机交互）

1. 基于需求澄清结果和环境检测，PM 推荐 output_type→    ```
   [产出类型选择]
   根据需求分析，建议产出类型。? {推荐类型}

     android-app    → Android 原生应用（Kotlin/C++，需 NDK + 模拟器测试）
     backend-api    → 后端服务/API（REST/gRPC，需接口测试）?     cli-tool       → 命令行工具（需功能测试）?     data-pipeline  → 数据管道/ETL（需数据验证     infrastructure ）?基础设施代码（Terraform/K8s，需 plan/dry-run 验证     documentation  ）?文档/规格（需人工审阅，无自动化测试）
     game           → 游戏（Android 原生/OpenGL ES，需 NDK + 模拟器测试）
     library        。SDK（需单元测试 + API 兼容性）
     custom         → 自定义（请描述验证方式）

   请选择或确认
   ```

2. 推荐逻辑。?   - 检测到 build.gradle.kts + CMakeLists.txt + C++ 源码 → android-app
   - 检测到 Express/FastAPI/Gin/Spring 。backend-api
   - 检测到 CLI 框架（commander/click/cobra）→ cli-tool
   - reference/ 中全是文件+ language=unknown → documentation
   - 用户明确认游戏"/"game"/"GLES" → game
   - 检测到 Terraform/Pulumi/CDK → infrastructure
   - 检测到 dbt/Airflow/Spark → data-pipeline
   - 无明确信息。?请用户选择

3. 用户确认后写。?`.state.md`: output_type={选择}

4. 根据 output_type 推导 test_strategy 默认值：
   - android-app → unit（GTest + JUnit 双通道）?   - backend-api → integration
   - cli-tool → integration
   - data-pipeline → smoke
   - infrastructure → smoke
   - documentation → manual
   - game → unit
   - library → unit
   - custom → 由用户指导。
5. 写入 test_strategy → .state.md

6. → output_type=android-app，检查NDK/CMake 可用性并更新 env.ndk_available

## Step 4: 模式选择

**执行角色:** PM（人机交互）

Proposal 草稿完成后，PM 根据需求规模向用户推荐模式。。
```
[模式选择]
根据需求规模分析，建议使用 {推荐模式} 模式。。
  fast     → 小调整（bug修复、≤5个文件、无需重新设计师             流程：PM出plan ）?DE开发芽?TE轻量审计 → 人工确认 → 归档
             预估。?-10分钟

  standard → 新功能（需设计，不跨模块）
             流程：SA设计 → TE用例 → DE开发芽?TE审计 → SR2+SR3 → 归档
             预估。?5-20分钟

  full     → 大型需求（跨模块、需完整评审链）
             流程：BA需求。?SA设计 → TE用例 → SR1 → DE开发芽?SR2+SR3 → SR4
             预估。?0+分钟

请选择模式:
```

推荐逻辑。?- 涉及文件 →  且无新架构建推荐 fast
- 单模块新功能或中等改变更推荐 standard
- 跨模块、多角色协作、需完整追溯 → 推荐 full

用户选择后，写入 `.state.md`: `mode: {fast|standard|full}`

## Step 5: Proposal 定稿

**执行角色:** PM

1. → Proposal 草稿写入 `specs/proposal.md`
2. 向用户呈现?Proposal 全文，请求确认3. 用户确认通过期?   - 更新 `.state.md`: `phase: init, current_step: INIT-DONE`
   - `[PM] Proposal 定稿完成（模式 {mode}），可执行/agh-propose`
4. 用户要求修改变   - 根据反馈修改 Proposal
   - 重新呈现，循环直到确认
## Proposal 格式

```markdown
# Proposal: {项目/需求标题}

## 背景与目录{为什么要做这件事}

## 范围
- 包含: {列举}
- 不包含 {列举}

## 关键约束
- {约束1}
- {约束2}

## 参考资料- {来源列表}
```

## 异常处理

- reference/ 为空：提示用户补充参考资料或直接口述需求- RESUME 模式用户选择放弃：清理未完成。.state.md，重新进行NEW 模式

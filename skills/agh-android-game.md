# Skill: agh-android-game

Android 原生游戏开发。可通过 `/agh-android-game` 快捷触发（自动设计output_type=library, tech_stack=java/gradle, test_strategy=unit 并执行/agh-run），也可在主流程中手动指定后自动加载本补充规则
**日志规则* → `templates/logging-standard.md`

---

## 快捷触发行为

当用户输出`/agh-android-game` 时：

1. 如无活跃 feature-name: 执行 agh-clarify.md，自动设计output_type=library、tech_stack=java/gradle、test_strategy=unit
2. 如有活跃 feature-name → tech_stack.language=java: 从当前断点继续?3. 设置后自动进行/agh-run 流程

## 主流程集成点

→ tech_stack.language=java 且产出为 Android Native 游戏时，主流程在以下位置加载本补充规则：


| 阶段      | 集成行为                                                                                                 |
| ------- | ---------------------------------------------------------------------------------------------------- |
| clarify | 自动设置 output_type=library, test_strategy=unit, tech_stack.language=java, tech_stack.build_tool=gradle |
| propose | SA 方案中包含native C++ → + Kotlin 胶水层设计（调用 CodeGraphService 分析现有代码结构）；TE 测试用例包含 GTest 单元测试 + JUnit                                     |
| apply   | DE 实现包含 C++ 核心逻辑 + Kotlin 胶水层（TDD 循环含 R1/R2/R3 三阶段 Code-Review），TE 使用 `gradle test` + `ctest` 双通道验证                                    |
| archive | 额外归档 so 构建产物→ output/lib/                                                                            |


---

## 模式裁剪


| 模式       | SA   | UX      | 审批        | DE   |
| -------- | ---- | ------- | --------- | ---- |
| fast     | 跳过   | 无UX     | 1次人工确认    | 批量实现 |
| standard | 架构方案 | 仅需UX时介入 | 完成确认      | 分批实现 |
| full     | 完整方案 | 按需      | SR1 + SR3 | 逐模式  |


---

## Android 游戏特有步骤（propose 阶段追加）。

### SA 架构设计中包含

1. `[PM] 提示 SA 架构设计需包含 Android 游戏特有内容`
2. SA 阅读 `docs/` 下行业技术标准规范文档（根据设计范围选择，不限于以下）：
- `docs/android-game-architecture.md`
- `docs/rendering-pipeline.md`
- `docs/game-loop-design.md`
- `docs/input-system.md`
- `docs/memory-asset-mgmt.md`
- `docs/audio-system.md`
- `docs/performance-baseline.md`
- `docs/testing-qa.md`
- `docs/build-ci-cd.md`
3. SA 设计文件 `sa/design.md` 中必须包含 Android 特有信息:
- **层次架构**: Kotlin 层（GameActivity）↔ JNI 胶水 ↔ C++ 核心层（Renderer/ECS）
- **渲染管线**: OpenGL ES 3.0 / Vulkan 版本选择
- **ABI**: armeabi-v7a / arm64-v8a / x86 / x86_64
- **CMake 集成**: CMakeLists.txt → native 库链接（game-activity / EGL / GLESv3 / jnigraphics）
- **测试策略**: GTest（C++ 单元测试）? JUnit（Kotlin 集成测试）
4. SA 输出 `docs/` 参考文档引用列表，标明每篇文档在设计中的使用依赖
5. 输出: `output/{feature-name}/specs/design.md`（含上述 Android 特有内容 + 参考文档引用）

---

## Android 游戏特有步骤（apply 阶段）。

### DE 实现流程

1. `[PM] 派发 DE 实现任务`
2. 写入 handoff:
- 白名单：`docs/` 下相关行业标准文件, `output/{feature-name}/specs/design.md`, `output/{feature-name}/specs/proposal.md`, `app/src/main/cpp/`, `app/src/main/java/`
- 数据操作 Skill: **FeatureService**（Feature 配置）、**SpecService**（规格读取）、**KnowledgeService**（Android 游戏技术栈约束 → LLM Wiki + 关系图谱）、**CodeGraphService**（C++/Kotlin 代码结构分析 → L1 静态索引 Clang AST + L2 语义）
- 期望输出: `output/{feature-name}/`, 修改 `app/src/main/cpp/` → `app/src/main/java/` 下源码
- 约束: TDD 模式（R1 Pre-Review → 红 → 绿 + R2 → 白盒 → 重构 + R3），基于 `docs/` 标准进行编码；C++ 部分用 GTest，Kotlin 部分用 JUnit；dev-test 使用 `gradle test`
3. 派发任务: DE（DE 需先阅读 `docs/` 中相关技术标准文档再进行编码，DE Agent 编排：配置生成 → R1 Pre-Review → 功能代码生成 → R2 Implementation Review → 冲突检测 → 合规校验 → R3 Refactoring Review）
4. 接收回报，校验
- 输出文件存在
- `gradle test` 通过
- GTest 编译且运行通过
- 三阶段 Code-Review 记录完整（R1/R2/R3）

### TE 校验规则

TE 阅读 `docs/` 下测试和质量标准文档（`docs/testing-qa.md`, `docs/performance-baseline.md`），对照标准设计测试用例和执行审计。
TE Agent 编排：黑盒测试用例生成 Skill（红阶段，调用 FeatureService 边界条件）→ 白盒测试用例生成 Skill（白盒阶段，调用 CodeGraphService L1 CFG/分支 + FeatureService MC/DC）→ 测试执行 Skill → 缺陷分析 Skill。
TE 使用双通道验证：
- **C++**: `cd app && cmake -DBUILD_TESTS=ON .. && ctest` → 验证 GTest 测试
- **Kotlin**: `cd app && gradle test` → 验证 JUnit 测试
- **构建验证**: `cd app && gradle assembleDebug` → 确认 APK 可构建
- **覆盖率**: 黑盒通过率 100% + 白盒分支覆盖率 ≥ 80%

### 性能基准校验（如适用）


| 指标     | 警告       | 失败       |
| ------ | -------- | -------- |
| 帧率     | < 55 FPS | < 30 FPS |
| 帧延迟    | > 50ms   | > 100ms  |
| 内存 PSS | 超基于?20%  | 超基于?50%  |


---

## 修复循环

同主流程修复循环规则：DE 自修复；PM 指派；人工介入。

## 产物分类规则

> **关键规则:** 区分 Spec 文档和过程产物的存放位置

| 产物类型 | 存放位置 | 说明 |
|---------|---------|------|
| **Spec 文档** | `openspec/specs/` | 含 SHALL + GWT + 追溯矩阵的正式规格（如 `{feature-name}-spec.md`、`test-cases-spec.md`） |
| **过程产物** | `output/{feature-name}/` | proposal.md、plan-action.md、code-report、test-report 等 |

- **禁止**将 proposal.md、plan-action.md 等过程产物写入 `openspec/specs/`
- Spec 文档在 apply 阶段人工确认通过后生成/更新到 `openspec/specs/`
- 过程产物在 archive 阶段快照到 `output/{feature-name}/baselines/`

### test-cases-spec.md 更新规则

每次需求变更产生新的测试用例后，必须增量更新 `openspec/specs/test-cases-spec.md`：
- 在对应分类章节追加新测试用例（TC-ID、测试名、描述、预期）
- 更新追溯矩阵（新增 SHALL → 测试用例映射）
- 更新版本号和变更历史
- 更新时机: apply 阶段 SR3 通过后执行（fast 模式由 PM 执行，standard/full 模式由 PM 基于 TE 测试用例执行）

### Spec 图表规则

生成到 `openspec/specs/` 的 Spec 文档必须包含以下 Mermaid 图表，基于实际代码生成：

| 图表类型 | Mermaid 语法 | 内容要求 |
|---------|-------------|---------|
| 框架图 | `flowchart` / `graph` | 模块/组件/层次间的依赖和调用关系 |
| 逻辑流程图 | `flowchart` | 核心业务逻辑的分支、循环、状态转换 |
| 类图 | `classDiagram` | 新增/修改的类、成员变量、方法签名、枚举、继承关系 |
| 时序图 | `sequenceDiagram` | 关键交互流程（输入→处理→状态变更→渲染） |

- 图表内容必须与实际代码一致，禁止臆造不存在的类或方法
- 图表在 Spec 文档的"技术设计"章节中呈现

## 异常处理

- gradle 构建失败: 检查NDK 版本、AGP 版本、CMake 版本
- GTest 失败: 白盒测试用例不通过，按具体断言失败信息修复
- 无模拟器/设备: TE 标注 "无模拟器可用，仅工程验证"
- OpenGL ES 版本不兼容 降级别?GLES 3.0 兼容模式


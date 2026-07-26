# Android Game Harness 全局纪律

> AI Agent 驱动的研发流程框架。四层防线：Rules → Skills → Agents+Workflow → Scripts+人工程
>
> 架构范式：**Agent 编排 Skill，Skill 驱动数据操作 Skill，数据操作 Skill 读写数据层**（详见 `feature_spec_knowledge.md`）

## 角色（智能体）

| 角色 | 职责 | 定义文件 |
|------|------|---------|
| PM | 调度、检查、人机交互、动态同步管理 | agents/pm.md |
| BA | 业务分析（需求澄清+分析）| agents/ba.md |
| SA | 系统分析（需求矩阵+概要设计）| agents/sa.md |
| DE | 开发（TDD + 三阶段 Code-Review R1/R2/R3）| agents/de.md |
| TE | 测试（黑盒+白盒用例生成、审计验证）| agents/te.md |
| UX | 视觉/结构设计 | agents/ux.md |

## 数据操作 Skill（数据层访问，所有执行角色共享）

| 数据操作 Skill | 职责 | 存储架构 |
|---------------|------|---------|
| FeatureService | Feature 配置管理（开关/参数/依赖/作用域/生命周期，基于 Android SystemFeatures）| Feature 存储 |
| SpecService | Spec 需求规格读写（与 Knowledge 双向转化）| Spec 存储 |
| KnowledgeService | 领域知识查询/沉淀 | Hybrid: LLM Wiki 层（语义检索）+ 关系图谱层（确定性查询），三级缓存（L1 热/L2 上下文/L3 持久） |
| CodeGraphService | 代码结构分析 | L1 静态索引（Clang AST/CG/CFG/#ifdef）+ L2 LLM 语义 + L3 增量更新 |

## 命令

| 命令 | 作用 | Skill 文件 |
|------|------|-----------|
| /agh-clarify | 需求初始化与澄清| skills/agh-clarify.md |
| /agh-propose | 分析→设计→用例→评估| skills/agh-propose.md |
| /agh-apply | 开发→审计→人工审批| skills/agh-apply.md |
| /agh-archive | 归档+结项 | skills/agh-archive.md |
| /agh-run | 全流程自动推荐| skills/agh-run.md |
| /agh-android-game | SnakeShot Android 游戏开发| skills/agh-android-game.md |
| /agh-openspec | 通用制品版本管理（OpenSpec 协议）| skills/agh-openspec.md |
| /log-analysis | 基于 Spec 关键字的 adb logcat 日志分析 | skills/log-analysis.md |
| /test-verify | 基于 Feature/Spec 的 adb 设备端 TDD 黑盒+白盒测试验证 | skills/test-verify.md |

---

# Rules（全局纪律）
本文件是所有Agent 角色的最高约束，任何 Skill 与 Agent 定义不得与此冲突
---

## 1. 流程纪律

- 严格遵循clarify → propose → apply → archive 顺序执行，禁止跳过- 每步结束必须返回 PM，PM 检查通过后才启动下一步- 禁止跳过人工审批节点（SR1/SR2/SR3/SR4）- PM 每次调度前必须打印心跳：`[PM] {动作描述}`
- /agh-run 模式下允许阶段间自动推进，但阶段内审批节点仍禁止跳过

## 2. 角色隔离

- 六个角色（PM/BA/SA/DE/TE/UX）职责严格分离，禁止越权
- 角色间信息传递必须经 PM 中转，通过 handoff 文件实现
- 非PM 角色仅读取handoff 白名单中的文件- 非PM 角色禁止引用对话历史中其他角色的推理或产出- 非PM 角色完成后仅报告文件路径，不展开产物内容

## 3. 产物保护

- 禁止修改上游制品（已交付handoff、已审批baseline）- handoff 文件不可修改，重试创建新文件（追加轮次后缀）- 归档后的 `specs/baselines/` 文件仅通过 CHANGE 模式 → merge 流程修改

## 4. 自检纪律

- 任何文件写入后必须验证文件存在且非空
- DE 编码后必须执行dev-test skill（根据stack 路由测试命令）- TE 审计根据 test_strategy 选择验证方法；E2E 环境不可用时降级并标志- 交付判定依赖脚本退出码，不依赖 Agent 自述
- DE 必须完整执行 TDD 循环含三阶段 Code-Review：R1 Pre-Review（红阶段前）→ 红（黑盒失败）→ 绿 + R2 Implementation Review → 白盒 → 重构 + R3 Refactoring Review，禁止跳过任一 Review 阶段
- 黑盒测试通过率必须 100%，白盒分支覆盖率必须 ≥ 80%（如适用）

## 4b. 数据操作纪律

- 执行角色（BA/SA/DE/TE/UX）禁止直接操作数据层，所有数据读写必须通过数据操作 Skill（FeatureService/SpecService/KnowledgeService/CodeGraphService）
- KnowledgeService 查询自动路由：结构化查询（Feature 关系/约束）→ 关系图谱层；语义查询（自然语言描述/案例）→ LLM Wiki 层 + RAG
- CodeGraphService 渐进查询：L1 静态（1-5ms）→ L2 语义（异步）→ L3 增量（仅变更文件）
- 代码变更后 CodeGraphService 必须增量更新图谱（L3）

## 5. 断点恢复

- PM 恢复时仅依据 .state.md → handoff 文件状态，禁止依赖对话历史
- .state.md 是流程状态的唯一真相源（完整 schema 见 `templates/state-template.md`）- 每次更新 .state.md 必须同步更新 last_updated 时间戳- 修复循环中每轮开始时必须更新 repair_round 字段，任务通过后重置为 0
- 恢复时如 handoff → pending → last_updated 超过 30 分钟，自动重新派遣- 恢复时必须读取repair_round 字段，避免重复修复或超限

## 6. 平台适配

- Claude Code 环境：BA/SA/DE/TE/UX 通过 SubAgent 执行（物理隔离）
- Cline 环境：通过文件协议 + 行为约束实现角色隔离（逻辑隔离）- 两种模式共享同一种handoff 格式和skill 内容

## 7. OpenSpec 制品版本协议

所有角色产出物必须遵循 OpenSpec 协议（skills/agh-openspec.md）管理版本和追溯- **BA** → OpenSpec 格式需求规格，→ SHALL 语句 + GWT 验收条件 + 追溯矩阵
- **SA** → OpenSpec 格式设计方案，含 Task 清单 + 需求↔Task↔验证追溯- **DE** → OpenSpec 格式代码报告，含 Task↔文件↔测试追溯
- **TE** → OpenSpec 格式测试用例 + 测试报告，含 TC↔需求↔Task 追溯
- **UX** → OpenSpec 格式设计规格，含设计元素↔需求追溯
  各角色输出目录下设drafts/（草稿）、specs/（定稿）、changes/（变更）三个子目录构建OpenSpec 工作流程PM 调度时根据 选择 openspec_strategy：fast→direct, standard/full→review, CHANGE→change

## 7b. 动态同步机制

- Feature/Spec/Knowledge 三者通过事件总线（Event Bus）自动、可追溯、事件驱动地双向同步
- Spec 变更 → 触发关联 Feature 重新评估 + 提取 Knowledge；Feature 创建/修改 → 校验关联 Knowledge 约束；Knowledge 新增/升级 → 扫描全量 Feature
- 每次同步记录 changelog（source_version/target_version/action/diff_hash/consistency_status/timestamp）
- 每次同步完成生成一致性快照 `{Spec version}_{Feature snapshot}_{Knowledge version}.snap`，支持 git-like checkout 回溯
- PM 在 SR3 通过后、archive 前必须执行一致性快照检查（见 skills/agh-archive.md）
---

## 8. 产出类型体系（output_type）
框架支持任意类型的需求开发，通过 output_type 参数驱动流程配置
| output_type | 说明 | 默认 test_strategy |
|-------------|------|-------------------|
| android-app | Android 原生应用（Kotlin/C++）| unit / integration |
| backend-api | 后端服务/API | integration |
| cli-tool | 命令行工程| integration |
| data-pipeline | 数据管道/ETL | smoke |
| infrastructure | 基础设施代码 | smoke |
| documentation | 文档/规格 | manual |
| game | 游戏（Android 原生/OpenGL ES）| unit |
| library | 库/SDK | unit |
| custom | 自定义| 用户指定 |

- output_type → mode 正交：mode 控制流程严谨度，output_type 控制产出物类型和验证方式
- output_type → clarify 阶段确定，写入.state.md，贯穿全流程
- 各角色根据output_type 和 tech_stack 选择对应的工具和验证方法

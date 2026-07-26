# Skill: log-analysis

基于 Spec 关键字的 adb logcat 日志分析。根据问题描述，定位关联 Spec，提取代码关键字，从 logcat 中过滤并分析问题根因。
**日志规则** → `templates/logging-standard.md`

---

## 概述

本 skill 接收用户的问题描述，通过 SpecService 搜索关联的 Spec 文档，从 Spec 中提取代码关键字（类名、方法名、TAG、错误码等），然后通过 `adb logcat` 获取设备日志，按关键字过滤并分析问题。

## 触发时机

用户输入 `/log-analysis` 或在对话中描述了需要分析的运行时问题（crash、ANR、功能异常、性能问题等）。

## 前置检查

1. 检查 `adb devices` 确认设备已连接
2. 检查 `openspec/specs/` 目录下是否存在 Spec 文件
3. 如无设备连接：提示用户连接设备，或提供已有日志文件路径进行离线分析
4. 如无 Spec 文件：降级为纯关键字分析（从问题描述中提取关键字）

## 核心流程

```
用户问题描述
    │
    ▼
┌─────────────────────────┐
│ Step 1: 问题解析         │ ← 提取问题领域、关键词、症状类型
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│ Step 2: Spec 关联定位    │ ← SpecService 搜索关联 Spec
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│ Step 3: 代码关键字提取   │ ← 从 Spec 中提取 TAG/类名/方法名/错误码
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│ Step 4: Logcat 获取过滤  │ ← adb logcat + 关键字过滤
└─────────┬───────────────┘
          │
          ▼
┌─────────────────────────┐
│ Step 5: 日志分析诊断     │ ← 模式匹配 + 时序分析 + 根因定位
└─────────┬───────────────┘
          │
          ▼
      分析报告
```

---

## Step 1: 问题解析

**执行角色:** PM 或发起请求的 Agent

1. 接收用户问题描述，提取以下信息：
   - **问题领域**: 渲染/音频/输入/计分/游戏逻辑/UI/性能/崩溃/ANR
   - **症状类型**: crash / ANR / 功能异常 / 性能问题 / 兼容性问题
   - **现象关键词**: 用户描述中的关键名词和动词
   - **复现条件**: 操作步骤、设备型号、Android 版本、平台（MTK/RTK/AML）

2. 输出问题解析摘要：
```
[问题解析]
领域: {渲染|音频|输入|计分|游戏逻辑|...}
症状: {crash|ANR|功能异常|...}
关键词: {提取的关键词列表}
复现条件: {条件描述}
```

## Step 2: Spec 关联定位

**数据操作 Skill:** SpecService（查询）、KnowledgeService（约束查询）

1. 基于问题领域和关键词，调用 **SpecService** 搜索关联 Spec：
   - 按关键词匹配 Spec 标题和内容
   - 按领域过滤 Spec 类型（如"渲染" → rendering-spec.md）
   - 按 Feature 绑定查询关联的 Spec

2. 调用 **KnowledgeService** 查询已知问题模式：
   - 关系图谱层：查询该领域已知的约束规则和历史问题
   - LLM Wiki 层：查询类似问题的历史案例和解决方案

3. 输出关联 Spec 列表：
```
[关联 Spec]
1. openspec/specs/rendering-spec.md — 渲染管线规格（匹配关键词: Renderer, OpenGL）
2. openspec/specs/game-logic-spec.md — 游戏逻辑规格（匹配关键词: Snake, GameState）
3. ...
```

## Step 3: 代码关键字提取

**数据操作 Skill:** SpecService（读取 Spec 内容）、CodeGraphService（L1 静态索引验证）

1. 读取每个关联 Spec 的内容，提取代码级关键字：

| 提取目标 | 提取方式 | 示例 |
|---------|---------|------|
| **Android TAG** | 匹配 `Log.d/i/w/e(TAG, ...)` 中的 TAG 常量 | `TAG = "Renderer"`, `TAG = "SnakeGame"` |
| **类名** | 匹配 Spec 中引用的类定义 | `Renderer`, `BitmapFont`, `TextureAsset` |
| **方法名** | 匹配 Spec 中引用的方法/函数 | `renderSnake()`, `handleInput()`, `updateGame()` |
| **错误码** | 匹配 Spec 中定义的错误码/异常 | `GL_ERROR`, `EGL_BAD_CONTEXT`, `GAME_OVER` |
| **Feature 宏** | 匹配 Spec 中绑定的 Feature | `FEATURE_HDR_ENABLE`, `FEATURE_SNAKE_FULL_SCREEN` |
| **日志级别** | 根据问题类型选择过滤级别 | crash → `*:E` / ANR → `*:W` / 功能 → `*:D` |

2. 调用 **CodeGraphService**（L1 静态索引）验证关键字的有效性：
   - 确认 TAG 常量在代码中实际存在
   - 确认类名/方法名在代码中实际定义
   - 补充 Spec 中未提及但代码中存在的关联 TAG

3. 输出关键字清单：
```
[代码关键字清单]
TAG: ["Renderer", "SnakeGame", "BitmapFont", "TextureAsset"]
类名: ["Renderer", "BitmapFont", "TextureAsset", "AudioEngine"]
方法名: ["renderSnake", "handleInput", "updateGame", "onDrawFrame"]
错误码: ["GL_ERROR", "GAME_OVER"]
Feature: ["FEATURE_SNAKE_FULL_SCREEN"]
Log 级别: "*:E" (crash) 或 "*:W" (ANR) 或 "*:D" (功能)
```

## Step 4: Logcat 获取与过滤

**执行方式:** adb 命令行

### 4.1 获取日志

```bash
# 实时获取（默认最近 5000 条）
adb logcat -d -t 5000

# 指定时间范围获取
adb logcat -d -t "01-01 12:00:00.000"

# 获取崩溃日志
adb logcat -d -b crash

# 获取 ANR 日志
adb logcat -d -b anr

# 获取所有缓冲区
adb logcat -d -b all
```

### 4.2 关键字过滤

```bash
# 按 TAG 过滤（多个 TAG 用 | 分隔）
adb logcat -d -t 5000 | grep -E "Renderer|SnakeGame|BitmapFont"

# 按 TAG + 日志级别过滤
adb logcat -d -t 5000 *:E | grep -E "Renderer|SnakeGame"

# 按类名/方法名过滤
adb logcat -d -t 5000 | grep -E "renderSnake|handleInput|updateGame"

# 按错误码过滤
adb logcat -d -t 5000 | grep -E "GL_ERROR|GAME_OVER|EGL_BAD"

# 组合过滤（TAG + 关键字）
adb logcat -d -t 5000 | grep -E "Renderer|SnakeGame" | grep -iE "error|exception|crash|fatal"

# 反向过滤（排除噪音日志）
adb logcat -d -t 5000 | grep -E "Renderer|SnakeGame" | grep -v "Choreographer\|SurfaceFlinger"
```

### 4.3 过滤策略

根据问题类型选择过滤策略：

| 问题类型 | 日志级别 | 缓冲区 | 过滤策略 |
|---------|---------|--------|---------|
| **Crash** | `*:E` + `*:F` | crash + main | TAG + Exception/Backtrace |
| **ANR** | `*:W` + `*:I` | anr + main | TAG + "ANR in" + CPU usage |
| **功能异常** | `*:D` + `*:I` | main | TAG + 功能相关关键字 |
| **性能问题** | `*:I` + `*:W` | main | TAG + "fps"/"frame"/"slow"/"jank" |
| **渲染问题** | `*:E` + `*:W` | main | TAG + "GL"/"EGL"/"render"/"draw" |
| **音频问题** | `*:D` + `*:I` | main | TAG + "audio"/"sound"/"sfx" |

## Step 5: 日志分析诊断

**数据操作 Skill:** KnowledgeService（历史案例匹配）、CodeGraphService（L2 语义分析）

### 5.1 模式匹配分析

从过滤后的日志中识别以下模式：

| 模式 | 匹配规则 | 严重级别 |
|------|---------|---------|
| **Fatal Exception** | `FATAL EXCEPTION` + 堆栈 | 🔴 阻塞 |
| **Java/Kotlin Crash** | `java.lang.*Exception` + 堆栈 | 🔴 阻塞 |
| **Native Crash** | `signal 11 (SIGSEGV)` / `signal 6 (SIGABRT)` + backtrace | 🔴 阻塞 |
| **ANR** | `ANR in` + 包名 + 主线程堆栈 | 🟡 警告 |
| **OpenGL Error** | `GL_ERROR` / `EGL_BAD_*` + 上下文 | 🟡 警告 |
| **Null Pointer** | `NullPointerException` + 位置 | 🔴 阻塞 |
| **Illegal State** | `IllegalStateException` + 位置 | 🟡 警告 |
| **Timeout** | `timeout` / `timed out` + 操作名 | 🟡 警告 |
| **OOM** | `OutOfMemoryError` + 内存信息 | 🔴 阻塞 |

### 5.2 时序分析

分析日志时间线，识别问题发生序列：

```
[时间线分析]
12:00:01.100  Renderer → Surface created
12:00:01.200  Renderer → OpenGL ES 3.0 initialized
12:00:02.000  SnakeGame → Game started
12:00:05.500  Renderer → renderSnake() called          ← 正常
12:00:05.510  Renderer → GL_ERROR: 0x501               ← 异常点
12:00:05.510  Renderer → renderSnake() failed           ← 结果
12:00:05.520  SnakeGame → Game state: ERROR              ← 传播
```

### 5.3 根因定位

基于模式匹配和时序分析，定位根因：

1. **崩溃根因**: 从堆栈中提取崩溃位置（文件:行号 → 对应 Spec 需求）
2. **ANR 根因**: 从主线程堆栈识别阻塞操作（I/O / 锁等待 / 长计算）
3. **功能根因**: 从日志序列中识别逻辑断点（预期行为 vs 实际行为）
4. **性能根因**: 从帧时间日志识别瓶颈（渲染耗时 / 内存抖动 / GC 频繁）

### 5.4 关联 Spec

将分析结果关联回 Spec：

| 日志证据 | 关联 Spec | 关联 SHALL | 建议修复 |
|---------|----------|-----------|---------|
| `GL_ERROR 0x501` at `Renderer.cpp:120` | rendering-spec.md | SHALL-3.1: OpenGL 状态管理 | 检查 GL 上下文生命周期 |
| `NullPointerException` at `BitmapFont.java:45` | architecture-spec.md | SHALL-2.1: 字体加载 | 增加空值检查 |

---

## 输出格式

```markdown
# Log 分析报告

## 问题描述
{用户输入的问题描述}

## 关联 Spec
| Spec | 匹配原因 | 关键需求 |
|------|---------|---------|
| rendering-spec.md | 关键词匹配: Renderer | SHALL-3.1 |
| game-logic-spec.md | 关键词匹配: Snake | SHALL-1.2 |

## 提取的关键字
- TAG: ["Renderer", "SnakeGame"]
- 类名: ["Renderer", "BitmapFont"]
- 方法名: ["renderSnake", "updateGame"]
- 错误码: ["GL_ERROR"]

## 过滤命令
```bash
adb logcat -d -t 5000 | grep -E "Renderer|SnakeGame" | grep -iE "error|exception"
```

## 日志分析

### 关键日志片段
```
{过滤后的关键日志}
```

### 时间线
{问题发生的时间序列}

### 模式匹配
| 模式 | 出现次数 | 首次出现 | 严重级别 |
|------|---------|---------|---------|
| GL_ERROR | 3 | 12:00:05.510 | 🟡 警告 |

### 根因分析
**根因:** {一句话描述根因}
**证据:** {支持根因的日志证据}
**关联 Spec:** {SHALL 条目}
**影响范围:** {受影响的模块/功能}

## 建议修复
1. {修复建议 1}
2. {修复建议 2}

## 复现验证
- 设备: {adb devices 输出}
- Android 版本: {版本}
- 复现步骤: {步骤}
```

---

## Agent 编排

本 skill 可由以下 Agent 编排：

| Agent | 使用场景 | 调用的数据操作 Skill |
|-------|---------|---------------------|
| **PM** | 接收用户问题描述，派发 log 分析任务 | SpecService（定位 Spec）→ KnowledgeService（历史案例） |
| **DE** | 代码修复前的日志诊断 | SpecService（关联需求）→ CodeGraphService（L1 代码验证 + L2 语义分析） |
| **TE** | 审计阶段的日志验证 | SpecService（需求一致性）→ CodeGraphService（代码定位） |

## 数据操作 Skill 依赖

| 数据操作 Skill | 用途 | 调用时机 |
|---------------|------|---------|
| **SpecService** | 搜索关联 Spec、读取 Spec 内容提取关键字 | Step 2-3 |
| **KnowledgeService** | 查询已知问题模式、历史案例匹配 | Step 2, Step 5 |
| **CodeGraphService** | L1 验证关键字有效性、L2 语义分析代码上下文 | Step 3, Step 5 |

## 异常处理

| 问题 | 处理 |
|------|------|
| 设备未连接 | 提示 `adb devices` 检查，或提供日志文件路径离线分析 |
| Spec 为空 | 降级为纯关键字分析：从问题描述 + `docs/` 参考资料提取关键字 |
| 过滤结果为空 | 扩大过滤范围：去掉日志级别限制，或增加关键字 |
| 日志量过大 | 先按时间范围缩小（`-t 1000`），再逐步扩大 |
| 无法定位根因 | 输出已有分析 + 未解决项，建议增加日志埋点或启用 DEBUG 级别 |

## 禁止事项

- 禁止修改设备上的日志（仅读取）
- 禁止在生产设备上执行 `adb logcat -c`（清除日志）
- 禁止直接操作数据层（必须通过数据操作 Skill）
- 禁止跳过 Spec 关联步骤直接分析日志（确保分析有需求依据）

## 使用方式

```
/log-analysis 问题描述: 游戏启动后渲染画面闪烁，偶发 crash
/log-analysis 设备: 设备A 问题: 吃食物后分数没有增加
```

或在对话中自然描述问题：
```
我发现在 MTK 平台上游戏会 crash，日志里有 SIGSEGV，帮我分析一下
```

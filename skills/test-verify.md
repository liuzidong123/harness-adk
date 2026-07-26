# Skill: test-verify

基于 Feature/Spec 的 adb 设备端 TDD 测试验证。执行黑盒测试（需求验证）和白盒测试（代码覆盖），生成测试报告。
**日志规则** → `templates/logging-standard.md`

---

## 概述

本 skill 接收 Feature/Spec 标识，通过 SpecService 读取需求规格，通过 FeatureService 读取配置，通过 CodeGraphService 分析代码结构，然后通过 `adb` 在真实设备上执行黑盒测试（输入→输出验证）和白盒测试（代码覆盖率），最终生成结构化测试报告。

与现有 TE agent 的区别：TE 侧重**测试用例设计**（propose 阶段）和**审计执行**（apply 阶段的 TEST-1/TEST-2），本 skill 侧重**设备端运行时验证**——通过 adb 实际操作设备并收集结果。

## 触发时机

用户输入 `/test-verify` 或在对话中要求对某个 Feature/Spec 进行设备端测试验证。

## 前置检查

1. 检查 `adb devices` 确认设备已连接且状态为 `device`
2. 检查目标 APK 是否已安装：`adb shell pm list packages | grep {package_name}`
3. 检查 `openspec/specs/` 下目标 Spec 文件存在
4. 检查 `openspec/specs/test-cases-spec.md` 中是否有对应 TC-ID
5. 如设备未连接或 APK 未安装：提示用户先执行构建和安装

---

## 核心流程

```
Feature/Spec 标识输入
    │
    ▼
┌─────────────────────────────┐
│ Step 1: Spec/Feature 读取    │ ← SpecService + FeatureService
└─────────┬───────────────────┘
          │
          ▼
┌─────────────────────────────┐
│ Step 2: 测试用例派生         │ ← 从 Spec SHALL + Feature 边界生成
└─────────┬───────────────────┘
          │
          ▼
┌─────────────────────────────┐
│ Step 3: 黑盒测试执行 (adb)   │ ← 输入模拟 + 输出验证
└─────────┬───────────────────┘
          │
          ▼
┌─────────────────────────────┐
│ Step 4: 白盒测试执行 (adb)   │ ← 代码覆盖率 + 分支覆盖
└─────────┬───────────────────┘
          │
          ▼
┌─────────────────────────────┐
│ Step 5: 测试报告生成         │ ← 关联 Spec + 覆盖率 + 结论
└─────────────────────────────┘
```

---

## Step 1: Spec/Feature 读取

**数据操作 Skill:** SpecService（读取 Spec）、FeatureService（读取 Feature 配置）

1. 接收输入：Feature 标识（如 `FEATURE_SNAKE_FULL_SCREEN`）或 Spec 标识（如 `rendering-spec`）
2. 调用 **SpecService** 读取关联 Spec：
   - 提取所有 SHALL 语句作为黑盒测试基准
   - 提取验收条件（GWT）作为测试场景
   - 提取技术设计中的接口定义作为白盒测试基准
3. 调用 **FeatureService** 读取 Feature 配置：
   - Feature 类型（bool/enum/int/string）
   - 取值范围和默认值
   - 依赖关系（必选/可选/互斥）
   - 作用域（平台/版本/机型）
4. 输出测试上下文：

```
[测试上下文]
Feature: FEATURE_SNAKE_FULL_SCREEN (bool, default: true)
Spec: rendering-spec.md
SHALL 条目:
  - SHALL-3.1: 蛇应全屏渲染，占据整个显示区域
  - SHALL-3.2: 菜单按钮应覆盖在游戏画面之上
验收条件:
  - GWT-3.1: Given 游戏状态=PLAYING, When 渲染帧, Then 蛇占据屏幕 90%+ 面积
  - GWT-3.2: Given 游戏状态=MENU, When 渲染帧, Then 菜单按钮可见且可点击
Feature 边界: bool → true/false 两个测试值
```

## Step 2: 测试用例派生

**数据操作 Skill:** SpecService（读取 SHALL/GWT）、FeatureService（边界条件）、CodeGraphService（L1 代码结构）

### 黑盒测试用例派生

从 Spec 的 SHALL + GWT 直接派生：

| 派生规则 | 方法 | 示例 |
|---------|------|------|
| **正常场景** | GWT 的 When→Then 直接映射 | 游戏启动 → 蛇出现在屏幕中央 |
| **边界值** | Feature 类型的极值 | bool → true/false；enum → 每个枚举值 |
| **状态转换** | 游戏状态机的所有合法转换 | MENU→PLAYING, PLAYING→PAUSED, PLAYING→GAME_OVER |
| **异常输入** | 非法/越界输入 | 快速连续点击、屏幕旋转、后台切回 |
| **Feature 组合** | 关联 Feature 的开关组合 | FULL_SCREEN=true + SPEED=1/2/3/4 |

### 白盒测试用例派生

从 CodeGraphService L1 静态索引派生：

| 派生规则 | 方法 | 数据来源 |
|---------|------|---------|
| **分支覆盖** | `if/switch/case` 的每个分支 | CodeGraphService L1 → CFG |
| **路径覆盖** | 函数的所有执行路径 | CodeGraphService L1 → CG |
| **条件覆盖** | 布尔表达式的 T/F 组合 | CodeGraphService L1 → AST |
| **Feature 宏覆盖** | `#ifdef` / `#ifndef` 的所有组合 | CodeGraphService L1 → #ifdef 映射 |

输出测试用例清单：

```
[黑盒测试用例] (BB-001 ~ BB-0xx)
BB-001: 游戏启动 → 蛇出现在屏幕中央 [SHALL-3.1]
BB-002: 菜单状态 → 按钮可见且可点击 [SHALL-3.2]
BB-003: SPEED=1 → 蛇移动速度符合预期 [Feature 边界]
BB-004: SPEED=4 → 蛇移动速度最快 [Feature 边界]
BB-005: 连续快速点击 → 不崩溃 [异常输入]
...

[白盒测试用例] (WB-001 ~ WB-0xx)
WB-001: Renderer::renderSnake() 分支 true 路径 [CFG]
WB-002: Renderer::renderSnake() 分支 false 路径 [CFG]
WB-003: handleButtonDown() → MENU 分支 [switch-case]
WB-004: handleButtonDown() → PLAYING 分支 [switch-case]
...
```

## Step 3: 黑盒测试执行（adb）

通过 adb 在设备上实际执行测试，验证输入→输出。

### 3.1 设备准备

```bash
# 确认设备连接
adb devices

# 安装 APK（如未安装）
adb install -r app/build/outputs/apk/debug/app-debug.apk

# 清除旧日志
adb logcat -c

# 启动应用
adb shell am start -n {package_name}/{activity_name}
```

### 3.2 输入模拟

```bash
# 点击（坐标 x,y）
adb shell input tap {x} {y}

# 长按
adb shell input swipe {x1} {y1} {x2} {y2} {duration_ms}

# 按键事件
adb shell input keyevent { keycode }
# 常用 keycode: 4=BACK, 3=HOME, 24=VOLUME_UP, 25=VOLUME_DOWN

# 文本输入
adb shell input text "{text}"

# 滑动
adb shell input swipe {x1} {y1} {x2} {y2} {duration_ms}
```

### 3.3 输出验证

```bash
# 截图验证（视觉对比）
adb shell screencap -p /sdcard/test_screenshot.png
adb pull /sdcard/test_screenshot.png ./test_output/

# 日志验证（检查预期输出）
adb logcat -d -t 200 | grep -E "{expected_tag}"

# 状态验证（Activity/Fragment 状态）
adb shell dumpsys activity activities | grep -E "{package_name}"

# 内存验证
adb shell dumpsys meminfo {package_name} | grep "TOTAL"

# 帧率验证
adb shell dumpsys gfxinfo {package_name} | grep "Total frames"

# 进程验证
adb shell pidof {package_name}
```

### 3.4 黑盒测试执行模板

```bash
# === BB-001: 游戏启动 → 蛇出现在屏幕中央 ===
adb shell am start -n {package}/{activity}
sleep 2  # 等待启动完成
adb shell screencap -p /sdcard/bb001.png
adb logcat -d -t 100 | grep -E "Renderer|SnakeGame" > ./test_output/bb001.log
# 验证: 截图中蛇可见 + 日志中无 ERROR

# === BB-002: 菜单按钮可点击 ===
adb shell input tap {menu_button_x} {menu_button_y}
sleep 1
adb logcat -d -t 50 | grep -E "GameState|PLAYING" > ./test_output/bb002.log
# 验证: 日志显示状态从 MENU→PLAYING

# === BB-003: SPEED=1 蛇移动速度 ===
adb shell input tap {speed_button_x} {speed_button_y}  # 切换到 SPEED=1
adb shell input tap {start_button_x} {start_button_y}
sleep 3
adb logcat -d -t 500 | grep -E "updateGame|frame" > ./test_output/bb003.log
# 验证: 帧间隔符合 SPEED=1 预期 (0.3s)
```

## Step 4: 白盒测试执行（adb）

在设备上验证代码内部逻辑的覆盖率。

### 4.1 代码覆盖率收集

```bash
# Android Instrumented Tests（带覆盖率）
adb shell am instrument -w \
  -e coverage true \
  -e coverageFile /sdcard/coverage.ec \
  {package_name}.test/androidx.test.runner.AndroidJUnitRunner

# 拉取覆盖率文件
adb pull /sdcard/coverage.ec ./test_output/
```

### 4.2 分支覆盖验证

```bash
# 通过日志验证分支执行
# 在代码关键分支处添加日志埋点:
#   LOGD(TAG, "BRANCH: if (condition) → true path");
#   LOGD(TAG, "BRANCH: if (condition) → false path");

adb logcat -d -t 1000 | grep "BRANCH:" > ./test_output/branches.log
# 统计每个分支是否被执行到
```

### 4.3 Feature 宏覆盖验证

```bash
# 检查编译时 Feature 配置
adb shell getprop | grep "feature\." > ./test_output/feature_props.log

# 检查运行时 Feature 查询
adb shell dumpsys package {package_name} | grep "feature" > ./test_output/feature_dump.log

# 验证: 每个 FEATURE_* 的 getprop/dumpsys 结果与 Spec 一致
```

### 4.4 性能覆盖验证

```bash
# 帧率测试（60秒采样）
adb shell dumpsys gfxinfo {package_name} reset
sleep 60
adb shell dumpsys gfxinfo {package_name} > ./test_output/gfxinfo.log

# 内存泄漏检测
adb shell dumpsys meminfo {package_name} > ./test_output/meminfo_before.log
# 执行操作...
adb shell dumpsys meminfo {package_name} > ./test_output/meminfo_after.log
# 对比 TOTAL PSS
```

## Step 5: 测试报告生成

**数据操作 Skill:** SpecService（追溯关联）、KnowledgeService（历史基线对比）

### 报告格式

```markdown
# 测试验证报告: {Feature/Spec 标识}

## 测试环境
- 设备: {adb devices 输出}
- Android 版本: {getprop ro.build.version.release}
- 包名: {package_name}
- APK 版本: {versionCode/versionName}
- 测试时间: {timestamp}

## 测试范围
| Spec | SHALL 条目 | 测试用例数 |
|------|-----------|-----------|
| rendering-spec.md | SHALL-3.1, SHALL-3.2 | BB-001~BB-005, WB-001~WB-003 |

## 黑盒测试结果

| 用例 ID | 测试场景 | 关联 SHALL | 结果 | 证据 |
|---------|---------|-----------|------|------|
| BB-001 | 游戏启动→蛇可见 | SHALL-3.1 | ✅ PASS | screenshot: bb001.png |
| BB-002 | 菜单按钮可点击 | SHALL-3.2 | ✅ PASS | log: 状态 MENU→PLAYING |
| BB-003 | SPEED=1 移动速度 | Feature 边界 | ❌ FAIL | 帧间隔 0.5s > 预期 0.3s |
| BB-004 | SPEED=4 移动速度 | Feature 边界 | ✅ PASS | 帧间隔 0.07s |
| BB-005 | 快速连续点击 | 异常输入 | ✅ PASS | 无 crash |

**黑盒通过率:** 4/5 (80%)

## 白盒测试结果

| 用例 ID | 测试场景 | 覆盖目标 | 结果 | 覆盖率 |
|---------|---------|---------|------|--------|
| WB-001 | renderSnake true 分支 | CFG 分支覆盖 | ✅ PASS | - |
| WB-002 | renderSnake false 分支 | CFG 分支覆盖 | ✅ PASS | - |
| WB-003 | handleButtonDown MENU | switch-case | ✅ PASS | - |

**分支覆盖率:** 100% (3/3)
**代码覆盖率:** {N}% (来自 coverage.ec)

## Feature 配置验证

| Feature | 预期值 | 实际值 | 结果 |
|---------|--------|--------|------|
| FEATURE_SNAKE_FULL_SCREEN | true | true | ✅ |
| FEATURE_SNAKE_SPEED | 1-4 | 1 | ✅ |

## 性能指标

| 指标 | 测量值 | 基线 | 结果 |
|------|--------|------|------|
| 帧率 (FPS) | {N} | ≥55 | ✅/❌ |
| 启动时间 | {N}ms | <2000ms | ✅/❌ |
| 内存 PSS | {N}MB | <100MB | ✅/❌ |

## 失败用例详情

### BB-003: SPEED=1 移动速度 ❌ FAIL

**预期:** 帧间隔 0.3s（SPEED=1 对应 speed=0.30）
**实际:** 帧间隔 0.5s
**证据:**
```
12:00:01.000 updateGame called
12:00:01.500 updateGame called  ← 间隔 0.5s
```

**根因:** `Renderer.cpp` 中 `speed` 初始值未正确从 Feature 读取
**关联 Spec:** rendering-spec SHALL-3.1
**修复建议:** 检查 `Model` 构造函数中 `speed` 初始化逻辑

## 结论

| 维度 | 结果 | 说明 |
|------|------|------|
| 黑盒测试 | 80% (4/5) | BB-003 未通过 |
| 白盒测试 | 100% (3/3) | 全部分支覆盖 |
| Feature 配置 | 100% (2/2) | 全部正确 |
| 性能 | {结果} | {说明} |

**总判定:** {PASS/CONDITIONAL_PASS/FAIL}

## 建议下一步
1. 修复 BB-003: 检查 speed 初始化逻辑
2. 修复后重新执行 `/test-verify`
```

---

## Agent 编排

| Agent | 使用场景 | 调用的数据操作 Skill |
|-------|---------|---------------------|
| **TE** | apply 阶段 TEST-1/TEST-2 的设备端执行 | SpecService（需求基准）→ FeatureService（边界条件）→ CodeGraphService（覆盖率） |
| **DE** | 开发自测阶段的设备端验证 | SpecService（验收标准）→ FeatureService（配置验证）→ CodeGraphService（分支覆盖） |
| **PM** | 人工确认阶段的设备端演示 | SpecService（追溯）→ 测试报告汇总 |

## 数据操作 Skill 依赖

| 数据操作 Skill | 用途 | 调用时机 |
|---------------|------|---------|
| **SpecService** | 读取 SHALL/GWT 作为测试基准、追溯关联 | Step 1, Step 5 |
| **FeatureService** | 读取 Feature 配置（类型/范围/默认值/依赖） | Step 1, Step 2 |
| **KnowledgeService** | 查询历史测试基线、已知问题模式 | Step 5 |
| **CodeGraphService** | L1 CFG/分支/路径 → 白盒用例派生、覆盖率目标 | Step 2, Step 4 |

## 异常处理

| 问题 | 处理 |
|------|------|
| 设备未连接 | 提示 `adb devices` 检查连接 |
| APK 未安装 | 执行 `adb install -r` 安装 |
| 应用 crash | 捕获 crash log → 归入失败用例 → 关联 Spec |
| ANR | 捕获 ANR trace → 归入失败用例 → 关联 Spec |
| 覆盖率文件缺失 | 降级为日志分支统计（BRANCH: 埋点） |
| 设备不支持 instrumented test | 降级为纯 adb input + logcat 验证 |

## 禁止事项

- 禁止在生产设备上执行 `adb logcat -c`（清除日志）
- 禁止执行 `adb shell pm clear`（清除应用数据，除非测试用例明确要求）
- 禁止修改设备系统设置（`adb shell settings put`）
- 禁止直接操作数据层（必须通过数据操作 Skill）

## 使用方式

```
/test-verify feature: FEATURE_SNAKE_FULL_SCREEN
/test-verify spec: rendering-spec
/test-verify spec: game-logic-spec --black-box --white-box
```

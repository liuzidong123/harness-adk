# SnakeShot Button-Menu 白盒测试用例规格

**版本:** v1
**状态:** draft
**负责人:** TE
**依据规格:** specs/button-menu-spec.md v3, specs/game-logic-spec.md v2
**覆盖目标:**
- 行覆盖 ≥ 80%
- 分支覆盖 ≥ 70%
- 异常覆盖 ≥ 50%

---

## 1. 命中检测 — hitTest()

**源文件:** Renderer.cpp:587-592  
**函数签名:** `bool Renderer::hitTest(const ButtonDef &btn, float nx, float ny) const`

### 测试原理
```
命中区域 = [x - w/2, x + w/2] × [y - h/2, y + h/2]
命中条件: nx ∈ [x-w/2, x+w/2] ∧ ny ∈ [y-h/2, y+h/2]
```

| TC-ID | 场景 | 输入 (btn, nx, ny) | 期望结果 | 分支覆盖 |
|-------|------|-------------------|---------|---------|
| HT-01 | 中心点击 | ({0.5,0.5,0.3,0.1}, 0.5, 0.5) | true | 4/4 边界全命中 |
| HT-02 | 左边缘 | ({0.5,0.5,0.3,0.1}, 0.35, 0.5) | true | nx下限边界 |
| HT-03 | 右边缘 | ({0.5,0.5,0.3,0.1}, 0.65, 0.5) | true | nx上限边界 |
| HT-04 | 下边缘 | ({0.5,0.5,0.3,0.1}, 0.5, 0.45) | true | ny下限边界 |
| HT-05 | 上边缘 | ({0.5,0.5,0.3,0.1}, 0.5, 0.55) | true | ny上限边界 |
| HT-06 | 左外侧 1px | ({0.5,0.5,0.3,0.1}, 0.349, 0.5) | false | nx < left |
| HT-07 | 右外侧 1px | ({0.5,0.5,0.3,0.1}, 0.651, 0.5) | false | nx > right |
| HT-08 | 下外侧 1px | ({0.5,0.5,0.3,0.1}, 0.5, 0.449) | false | ny < bottom |
| HT-09 | 上外侧 1px | ({0.5,0.5,0.3,0.1}, 0.5, 0.551) | false | ny > top |
| HT-10 | 远距离偏移 | ({0.5,0.5,0.3,0.1}, 0.0, 0.0) | false | 全miss |
| HT-11 | 矩形非对称: 窄高按钮 | ({0.15,0.08,0.14,0.07}, 0.15, 0.08) | true | PAUSE按钮中心 |
| HT-12 | 矩形非对称: 窄高按钮 miss | ({0.15,0.08,0.14,0.07}, 0.22, 0.08) | true | x在half/外边界 |
| HT-13 | 矩形非对称: 窄高按钮 miss | ({0.15,0.08,0.14,0.07}, 0.23, 0.08) | false | x超出right |

### 分支覆盖率: HT-01~10 覆盖全部 4 个比较分支的正反两侧

---

## 2. 状态机转换 — handleButtonDown()

**源文件:** Renderer.cpp:594-641  
**输入:** (nx, ny) 归一化触摸坐标  
**输出:** gameState_ 状态变更 + 可选 initSnake()

### 2.1 MENU 状态

| TC-ID | 场景 | 状态前置 | 点击区域 | 期望 gameState_ | initSnake? |
|-------|------|---------|---------|-----------------|-----------|
| SM-01 | START 按钮 | MENU | (0.50, 0.50) | PLAYING | 是 |
| SM-02 | MENU 标签无效果 | MENU | (0.50, 0.88) | MENU (不变) | 否 |
| SM-03 | 空白区域 | MENU | (0.10, 0.10) | MENU (不变) | 否 |

### 2.2 PLAYING 状态

| TC-ID | 场景 | 状态前置 | 点击区域 | 期望 gameState_ |
|-------|------|---------|---------|-----------------|
| SP-01 | PAUSE 按钮 | PLAYING | (0.15, 0.08) | PAUSED |
| SP-02 | STOP 按钮 | PLAYING | (0.85, 0.08) | GAME_OVER |
| SP-03 | PLAYING 标签 | PLAYING | (0.50, 0.88) | PLAYING (不变) |
| SP-04 | 网格区域方向输入 | PLAYING | 见方向测试 | PLAYING (不变) |

### 2.3 PAUSED 状态

| TC-ID | 场景 | 状态前置 | 点击区域 | 期望 gameState_ |
|-------|------|---------|---------|-----------------|
| SQ-01 | RESUME 按钮 | PAUSED | (0.50, 0.36) | PLAYING |
| SQ-02 | MENU 按钮 | PAUSED | (0.50, 0.48) | MENU |
| SQ-03 | STOP 按钮 | PAUSED | (0.50, 0.60) | GAME_OVER |
| SQ-04 | PAUSED 标签 | PAUSED | (0.50, 0.88) | PAUSED (不变) |

### 2.4 GAME_OVER 状态

| TC-ID | 场景 | 状态前置 | 点击区域 | 期望 gameState_ | initSnake? |
|-------|------|---------|---------|-----------------|-----------|
| SG-01 | AGAIN 按钮 | GAME_OVER | (0.50, 0.42) | PLAYING | 是 |
| SG-02 | MENU 按钮 | GAME_OVER | (0.50, 0.54) | MENU | 否 |
| SG-03 | GAME_OVER 标签 | GAME_OVER | (0.50, 0.88) | GAME_OVER (不变) | 否 |

### 状态转换总图验证
```
MENU ──[START]──→ PLAYING
PLAYING ──[PAUSE]──→ PAUSED
PLAYING ──[STOP]──→ GAME_OVER
PAUSED ──[RESUME]──→ PLAYING
PAUSED ──[MENU]──→ MENU
PAUSED ──[STOP]──→ GAME_OVER
GAME_OVER ──[AGAIN]──→ PLAYING
GAME_OVER ──[MENU]──→ MENU
```
所有 8 条转换路径已覆盖。

---

## 3. 速度控制 — Speed Control

**源文件:** Renderer.cpp:594-611, Renderer.h:72-78

### 3.1 速度调整

| TC-ID | 场景 | speedLevel_ 前置 | 点击 | 期望 speedLevel_ | 期望 moveInterval_ |
|-------|------|-----------------|------|-----------------|-------------------|
| SC-01 | 加速(3→4) | 3 | "+" (0.72, 0.32) | 4 | 0.07 |
| SC-02 | 加速(2→3) | 2 | "+" (0.72, 0.32) | 3 | 0.12 |
| SC-03 | 减速(2→1) | 2 | "-" (0.28, 0.32) | 1 | 0.30 |
| SC-04 | 减速(3→2) | 3 | "-" (0.28, 0.32) | 2 | 0.20 |

### 3.2 边界限幅

| TC-ID | 场景 | speedLevel_ 前置 | 点击 | 期望 speedLevel_ | 说明 |
|-------|------|-----------------|------|-----------------|------|
| SC-05 | 最低限幅 | 1 | "-" (0.28, 0.32) | 1 | clamp(1-1, 1, 4) = 1 |
| SC-06 | 最高限幅 | 4 | "+" (0.72, 0.32) | 4 | clamp(4+1, 1, 4) = 4 |

### 3.3 速度命中区域边界

| TC-ID | 场景 | 点击 | 期望 |
|-------|------|------|------|
| SC-07 | "-" 左边界 | (0.24, 0.32) | 命中 |
| SC-08 | "-" 左外侧 | (0.239, 0.32) | 未命中 |
| SC-09 | "+" 右边界 | (0.76, 0.32) | 命中 |
| SC-10 | "+" 右外侧 | (0.761, 0.32) | 未命中 |
| SC-11 | "-" 下边界 | (0.28, 0.28) | 命中 |
| SC-12 | "-" 下外侧 | (0.28, 0.279) | 未命中 |

---

## 4. 方向输入 — Direction Input

**源文件:** Renderer.cpp:625-638

**条件:** gameState_ == PLAYING && 未命中任何按钮

### 4.1 方向判定 (蛇头 (10,10) → UI坐标 (0.13+(10+0.5)*0.037, 0.14+(10+0.5)*0.037) = (0.5185, 0.5285))

| TC-ID | 场景 | 触摸点 (nx, ny) | dx | dy | |dx|>|dy|? | 期望 dir |
|-------|------|----------------|----|----|-----------|----------|
| DI-01 | 向右 | (0.70, 0.5285) | +0.1815 | 0 | 水平 | RIGHT |
| DI-02 | 向左 | (0.30, 0.5285) | -0.2185 | 0 | 水平 | LEFT |
| DI-03 | 向上 | (0.5185, 0.70) | 0 | +0.1715 | 垂直 | UP |
| DI-04 | 向下 | (0.5185, 0.30) | 0 | -0.2285 | 垂直 | DOWN |
| DI-05 | 水平优先: dx>dy | (0.60, 0.55) | +0.0815 | +0.0215 | 水平 | RIGHT |
| DI-06 | 垂直优先: dy>dx | (0.52, 0.60) | +0.0015 | +0.0715 | 垂直 | UP |
| DI-07 | 水平优先: dy=dx | (0.60, 0.60) | +0.0815 | +0.0715 | 水平 | RIGHT |

---

## 5. 可见性过滤 — Visibility Filtering

**源文件:** Renderer.cpp:132-137

| TC-ID | 场景 | gameState_ | buttons_ 过滤条件 | 可见按钮数 |
|-------|------|-----------|------------------|-----------|
| VF-01 | MENU 状态 | MENU | visibleIn == MENU | 2 (START + SNAKESHOT标签) |
| VF-02 | PLAYING 状态 | PLAYING | visibleIn == PLAYING | 3 (PAUSE + STOP + PLAYING标签) |
| VF-03 | PAUSED 状态 | PAUSED | visibleIn == PAUSED | 4 (RESUME + MENU + STOP + PAUSED标签) |
| VF-04 | GAME_OVER 状态 | GAME_OVER | visibleIn == GAME_OVER | 3 (AGAIN + MENU + GAME_OVER标签) |

---

## 6. 缩放逻辑 — Font Scale

**源文件:** Renderer.cpp:265

| TC-ID | 场景 | width_ | 期望 fontScale |
|-------|------|-------|---------------|
| FS-01 | 小屏 ≤960px | 720 | 2 |
| FS-02 | 小屏 =960px | 960 | 2 |
| FS-03 | 中屏 960-1440 | 1280 | 3 |
| FS-04 | 大屏 ≥1440px | 1440 | 4 |
| FS-05 | 大屏 >1440px | 1920 | 4 |

---

## 7. 网格坐标转换

**源文件:** Renderer.cpp:477-483

```
gridToUIX(gx) = 0.13 + (gx + 0.5) * 0.037
gridToUIY(gy) = 0.14 + (gy + 0.5) * 0.037
```

| TC-ID | 场景 | 输入(gx/gy) | 期望 UI X | 期望 UI Y |
|-------|------|------------|-----------|-----------|
| GC-01 | 原点(0,0) | (0, 0) | 0.13 + 0.5*0.037 = 0.1485 | 0.14 + 0.5*0.037 = 0.1585 |
| GC-02 | 中点(10,10) | (10, 10) | 0.13 + 10.5*0.037 = 0.5185 | 0.14 + 10.5*0.037 = 0.5285 |
| GC-03 | 末点(19,19) | (19, 19) | 0.13 + 19.5*0.037 = 0.8515 | 0.14 + 19.5*0.037 = 0.8615 |

---

## 8. 初始蛇状态 — initSnake()

**源文件:** Renderer.cpp:391-403

| TC-ID | 场景 | 期望 |
|-------|------|------|
| IS-01 | 蛇段数 | 3 (头(10,10) + 身(9,10) + 尾(8,10)) |
| IS-02 | 初始方向 | RIGHT |
| IS-03 | nextDir | RIGHT (与 snakeDir 一致) |
| IS-04 | moveTimer | 0.0 |
| IS-05 | moveInterval | kMoveIntervals[speedLevel_-1] |
| IS-06 | gameOver | false |
| IS-07 | foodPos 非空 | 食物不在蛇身上 |

---

## 9. 碰撞检测

**源文件:** Renderer.cpp:451-464

| TC-ID | 场景 | 输入 | 期望 |
|-------|------|------|------|
| CC-01 | 墙壁碰撞: x<0 | head=(-1,10) | gameState_ = GAME_OVER |
| CC-02 | 墙壁碰撞: x≥20 | head=(20,10) | gameState_ = GAME_OVER |
| CC-03 | 墙壁碰撞: y<0 | head=(10,-1) | gameState_ = GAME_OVER |
| CC-04 | 墙壁碰撞: y≥20 | head=(10,20) | gameState_ = GAME_OVER |
| CC-05 | 自身碰撞: 与第1节 | head=(10,10) + body=(10,10) | gameState_ = GAME_OVER |
| CC-06 | 无碰撞 | head=(11,10), body 无此坐标 | 正常移动, push_front |

---

## 测试环境要求

```
框架: Google Test (gtest)
编译器: C++17
Android EGL/GLES 依赖: 测试时 mock/stub
独立编译目标: test_button_menu
```

### CMake 集成建议

```cmake
find_package(GTest REQUIRED)
add_executable(test_button_menu test_button_menu.cpp)
target_link_libraries(test_button_menu GTest::GTest GTest::Main)
target_compile_features(test_button_menu PUBLIC cxx_std_17)
```

### 通过标准

```
所有 TC: PASS
覆盖率: 行 ≥80%, 分支 ≥70%
无内存泄漏 (valgrind/ASAN)

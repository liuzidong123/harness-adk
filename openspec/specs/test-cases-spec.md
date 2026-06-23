# SnakeShot 测试用例规格

**版本:** v2.1
**状态:** draft
**负责人:** TE
**依据规格:** openspec/specs/button-menu-spec.md v3, openspec/specs/scoring-spec.md v1
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

## 10. 音效播放 — Audio SFX

**源文件:** AudioEngine.cpp, Renderer.cpp
**依据规格:** openspec/specs/audio-sfx-spec.md v1

| TC-ID | 场景 | 操作 | 期望结果 | SHALL 映射 |
|-------|------|------|---------|-----------|
| AU-01 | 开始音效 | MENU 下点击 START 按钮 | `audioEngine_->playStart()` 被调用 | SHALL-001 |
| AU-02 | 暂停音效 | PLAYING 下点击 PAUSE 按钮 | `audioEngine_->playPause()` 被调用 | SHALL-002 |
| AU-03 | 吃食物音效 | 蛇头移动到食物位置 | `audioEngine_->playEat()` 被调用 | SHALL-003 |
| AU-04 | 碰撞 GameOver 音效 | 蛇头撞击墙壁或自身 | `audioEngine_->playGameOver()` 被调用 | SHALL-004 |
| AU-05 | STOP按钮 GameOver 音效 | PLAYING 下点击 STOP 按钮 | `audioEngine_->playGameOver()` 被调用 | SHALL-004 |
| AU-06 | 异步播放不阻塞 | 音效播放时，主循环帧率不受影响 | 播放在 `std::thread` 中进行，不阻塞 | SHALL-005 |
| AU-07 | WAV 资源加载 | AudioEngine::init() 执行 | 4 个 WAV 文件从 assets 正确加载 | SHALL-006 |
| AU-08 | WAV 解析失败容错 | 提供无效 WAV 文件 | `loadWav` 返回空 SoundBuffer，不崩溃 | - |
| AU-09 | 无音效时优雅降级 | AudioEngine 未初始化时调用 play* 方法 | 无崩溃，无副作用 | - |

---

## 11. 计分逻辑 — Scoring

**源文件:** Renderer.cpp:455-480  
**依据规格:** openspec/specs/scoring-spec.md v1

### 11.1 食物加分

| TC-ID | 场景 | 操作 | 期望结果 | SHALL 映射 |
|-------|------|------|---------|-----------|
| SC-01 | 吃食物加分 | 蛇头移动到食物所在格子 | score_ 增加 1 | SHALL-001 |
| SC-02 | 非食物不加分 | 蛇头移动到无食物格子 | score_ 不变 | SHALL-001 |

### 11.2 碰墙减分 + 方向调转

| TC-ID | 场景 | 操作 | 期望结果 | SHALL 映射 |
|-------|------|------|---------|-----------|
| SC-03 | 碰墙减分 | 蛇头超出左边界 (x<0) | score_ 减少 1 | SHALL-002 |
| SC-04 | 碰墙上边界 | 蛇头超出上边界 (y>=20) | score_ 减少 1 | SHALL-002 |
| SC-05 | 碰墙方向调转(UP→DOWN) | 蛇头超出上边界 | snakeDir_ 变为 DOWN | SHALL-002 |
| SC-06 | 碰墙方向调转(DOWN→UP) | 蛇头超出下边界 | snakeDir_ 变为 UP | SHALL-002 |
| SC-07 | 碰墙方向调转(LEFT→RIGHT) | 蛇头超出左边界 | snakeDir_ 变为 RIGHT | SHALL-002 |
| SC-08 | 碰墙方向调转(RIGHT→LEFT) | 蛇头超出右边界 | snakeDir_ 变为 LEFT | SHALL-002 |
| SC-09 | 碰墙不触发 GameOver | 蛇头超出任意边界 | gameState_ 不变 (仍为 PLAYING) | SHALL-002 |
| SC-10 | 碰墙蛇头不移动 | 蛇头超出左边界 | 不执行 push_front，蛇头位置不变 | SHALL-002 |

### 11.3 GameOver 计分清零

| TC-ID | 场景 | 操作 | 期望结果 | SHALL 映射 |
|-------|------|------|---------|-----------|
| SC-11 | 自碰计分清零 | 蛇头撞到自身身体 | score_ = 0, gameState_ = GAME_OVER | SHALL-003 |

### 11.4 分数显示

| TC-ID | 场景 | 操作 | 期望结果 | SHALL 映射 |
|-------|------|------|---------|-----------|
| SC-12 | 正分显示 | score_ = 42 | 文本显示 "SCORE:  42" | SHALL-004 |
| SC-13 | 负分显示 | score_ = -7 | 文本显示 "SCORE: -7" | SHALL-005 |
| SC-14 | 零分显示 | score_ = 0 | 文本显示 "SCORE:  0" | SHALL-004 |
| SC-15 | 三位数显示 | score_ = 123 | 文本显示 "SCORE: 123" | SHALL-004 |

### 11.5 追溯矩阵（Scoring）

| SHALL | 测试用例 |
|-------|---------|
| SHALL-001 | SC-01, SC-02 |
| SHALL-002 | SC-03 ~ SC-10 |
| SHALL-003 | SC-11 |
| SHALL-004 | SC-12, SC-14, SC-15 |
| SHALL-005 | SC-13 |

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1 | - | 初始版本（Button-Menu 测试用例） |
| v2 | 2026-06-24 | 新增音效播放测试用例（AU-01~09） |
| v2.1 | 2026-06-24 | 新增计分逻辑测试用例（SC-01~15） |

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

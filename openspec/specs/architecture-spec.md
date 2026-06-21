# SnakeShot 系统架构规格

**版本:** v2
**状态:** draft
**负责人:** SA

---

## 1. 概述

SnakeShot 是一款 Android 原生贪吃蛇游戏，使用 **GameActivity** + **OpenGL ES 3.0** 原生渲染，无 Android View 系统依赖。

### 设计目标
- 全屏沉浸式游戏体验
- 纯原生 C++ 渲染管线（60fps）
- 触摸输入驱动
- 无第三方游戏引擎依赖

## 2. 系统层次架构

```mermaid
graph TB
    subgraph Kotlin层
        MA[MainActivity<br/>extends GameActivity]
        MA -->|System.loadLibrary| SO[libsnakeshot.so]
    end

    subgraph 原生入口层
        MC[main.cpp]
        MC -->|handle_cmd| LC[生命周期回调]
        MC -->|ALooper_pollOnce| GL[游戏主循环]
        MC -->|motion_event_filter| IF[输入过滤<br/>Pointer/Joystick]
    end

    subgraph 渲染器核心层
        R[Renderer.h/.cpp]
        R -->|owns| EGL[EGL 上下文]
        R -->|调度| RP[渲染管线]
        R -->|处理| IP[输入处理]
        R -->|驱动| SG[蛇游戏逻辑]
        R -->|管理| SM[游戏状态机<br/>MENU/PLAYING/PAUSED/GAME_OVER]
    end

    subgraph 图形子系统
        S[Shader] -->|编译执行| GLSL[GLSL 300 es]
        M[Model] -->|几何数据| VBO[顶点/索引 Buffer]
        T[TextureAsset] -->|纹理来源| PNG[PNG 解码]
        T -->|纹理来源| CT[纯色生成]
        T -->|纹理来源| TXT[文字渲染]
        BF[BitmapFont] -->|位图渲染| TXT
        UT[Utility] -->|矩阵运算| MP[正交投影]
        UT -->|错误检查| GE[glGetError]
    end

    subgraph 调试子系统
        AO[AndroidOut] -->|logcat 输出| ADB[adb logcat]
    end

    MA --> MC
    LC --> R
    GL --> R
    RP --> S
    RP --> M
    M --> T
    T --> BF
```

## 3. 模块依赖关系

```mermaid
graph LR
    main.cpp --> Renderer
    main.cpp --> AndroidOut
    Renderer --> Shader
    Renderer --> Model
    Renderer --> TextureAsset
    Renderer --> Utility
    Renderer --> AndroidOut
    Shader --> Model
    Shader --> Utility
    TextureAsset --> BitmapFont
    TextureAsset --> Model

    style main.cpp fill:#4a9eff
    style Renderer fill:#ff6b6b
    style Shader fill:#51cf66
    style Model fill:#fcc419
    style TextureAsset fill:#cc5de8
    style BitmapFont fill:#ff922b
    style Utility fill:#20c997
    style AndroidOut fill:#868e96
```

## 4. 模块职责速查

| 模块 | 文件 | 职责一句话 |
|------|------|-----------|
| **MainActivity** | `MainActivity.kt` | 加载 native 库 + 沉浸全屏标志 |
| **main** | `main.cpp` | ALooper 事件循环 + 生命周期回调分发 |
| **Renderer** | `Renderer.h/.cpp` | 核心：EGL/渲染/UI/蛇逻辑/输入 — 全部汇聚于此 |
| **Shader** | `Shader.h/.cpp` | GLSL 编译 + 通过 attrib pointer 绘制 Model |
| **Model** | `Model.h` | 顶点/索引/纹理 三元组容器 |
| **TextureAsset** | `TextureAsset.h/.cpp` | 三种纹理来源：PNG / 纯色 / 文字 |
| **BitmapFont** | `BitmapFont.h/.cpp` | 内嵌 96 字符 8×8 位图 → RGBA buffer |
| **Utility** | `Utility.h/.cpp` | GL 错误检查 + 正交矩阵构建 |
| **AndroidOut** | `AndroidOut.h/.cpp` | `std::ostream` → `__android_log_print` |

## 5. 数据流

### 主循环 (60fps)

```mermaid
sequenceDiagram
    participant AL as ALooper
    participant LC as lifecycle_cmd
    participant R as Renderer
    participant IN as Input
    participant GP as GPU

    Note over AL: 非阻塞 pollOnce(0)
    AL->>LC: APP_CMD_INIT_WINDOW
    LC->>R: new Renderer(pApp)
    R->>R: initRenderer()

    Note over AL,R: 每帧循环
    AL->>R: ALooper_pollOnce 完成
    R->>IN: handleInput()
    IN->>IN: android_app_swap_input_buffers
    IN->>IN: motion → handleButtonDown()

    R->>R: render()
    R->>R: updateRenderArea()
    R->>R: 设置正交投影
    R->>GP: glClear(CORNFLOWER_BLUE)
    R->>GP: drawModel(game models)
    R->>R: 切换 UI 投影
    R->>R: updateSnake(dt) [仅 PLAYING]
    R->>R: renderSnake()
    R->>GP: drawModel(UI buttons)
    R->>GP: drawModel(speed controls) [仅 MENU]
    R->>GP: eglSwapBuffers()
```

### EGL 初始化流程

```mermaid
flowchart TD
    A[eglGetDisplay] --> B[eglInitialize]
    B --> C[eglChooseConfig<br/>RGBA8 + Depth24 + ES3]
    C --> D[eglCreateWindowSurface<br/>app_->window]
    D --> E[eglCreateContext<br/>ES 3.0]
    E --> F[eglMakeCurrent]
    F --> G[Shader::loadShader<br/>vertex + fragment]
    G --> H[glClearColor<br/>glEnable BLEND]
    H --> I[createModels<br/>createUI<br/>createSnakeAssets<br/>createSpeedControls]
    I --> J[initSnake]
```

## 6. 投影矩阵切换

```mermaid
flowchart LR
    subgraph 游戏投影
        GMP[正交投影<br/>halfHeight=2<br/>aspect=w/h<br/>near=-1 far=1]
    end
    subgraph UI 投影
        UIP[[2 0 0 -1<br/>0 2 0 -1<br/>0 0 -1 0<br/>0 0 0 1]]
    end

    GMP -->|游戏 Model| DRAW[glDrawElements]
    UIP -->|UI Model| DRAW
```

### 矩阵结构 (column-major)
```
游戏投影: buildOrthographicMatrix(2.0, width/height, -1.0, 1.0)
UI 投影:  [2.0] [0]   [0]  [-1.0]
          [0]   [2.0] [0]  [-1.0]
          [0]   [0]   [-1] [0]
          [0]   [0]   [0]  [1.0]
```

## 7. 渲染管线 (单帧)

```mermaid
flowchart TB
    A[updateRenderArea<br/>尺寸变化？更新 viewport+投影] --> B[切换游戏投影]
    B --> C[glClear COLOR_BUFFER_BIT]
    C --> D[for each model in models_<br/>→ shader->drawModel]
    D --> E[切换 UI 投影]
    E --> F{gameState == PLAYING?}
    F -->|Yes| G[updateSnake(dt)]
    F -->|No| H[跳过]
    G --> H
    H --> I{snakeSegments_ 非空}
    I -->|Yes| J[renderSnake<br/>网格背景→食物→蛇身]
    I -->|No| K[跳过]
    J --> K
    K --> L[for each button<br/>visibleIn==gameState<br/>→ drawModel]
    L --> M{gameState == MENU?}
    M -->|Yes| N[draw speed controls]
    M -->|No| O[跳过]
    N --> O
    O --> P[eglSwapBuffers]
```

## 8. GLSL Shader 概要

```
Vertex Shader (#version 300 es)
  Input:  vec3 inPosition, vec2 inUV
  Output: vec2 fragUV
  Uniform: mat4 uProjection
  → gl_Position = uProjection * vec4(inPosition, 1.0)

Fragment Shader (#version 300 es, mediump float)
  Input:  vec2 fragUV
  Uniform: sampler2D uTexture
  → outColor = texture(uTexture, fragUV)
```

## 9. 构建配置

```
Build System: Gradle + CMake 3.22.1
Android SDK: API 36
NDK ABIs: armeabi-v7a | arm64-v8a | x86 | x86_64
Native Libs: game-activity_static | EGL | GLESv3 | jnigraphics | android | log
Kotlin: androidx.games-activity:4.0.0
```

## 10. 文件关系图

```mermaid
flowchart TB
    subgraph 构建
        CMake[CMakeLists.txt] -->|编译| SO[libsnakeshot.so]
        GC[build.gradle.kts] -->|打包| APK
    end

    subgraph C++源码
        direction TB
        MC[main.cpp]-->RH[Renderer.h]
        RH-->RC[Renderer.cpp]
        SH[Shader.h]-->SC[Shader.cpp]
        MH[Model.h]
        TH[TextureAsset.h]-->TC[TextureAsset.cpp]
        BFH[BitmapFont.h]-->BFC[BitmapFont.cpp]
        UH[Utility.h]-->UC[Utility.cpp]
        AOH[AndroidOut.h]-->AOC[AndroidOut.cpp]
    end

    subgraph Kotlin
        MAK[MainActivity.kt]
    end

    CMake --> MC & SH & TH & BFH & UH & AOH
    RC --> SH & MH & TH & UH & AOH
    SC --> MH & UH
    TC --> BFH & MH

    style CMake fill:#e7f5ff
    style GC fill:#e7f5ff
    style MAK fill:#d3f9d8
```

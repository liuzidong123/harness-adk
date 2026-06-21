# Android 游戏内存与资源管理标准

> 行业标准参考：Android Memory Management、NDK 内存指南、Google Play 资源规范

---

## 1. 内存架构模型

```mermaid
graph TB
    subgraph Native Heap [Native 堆]
        A[纹理数据] -->|glTexImage2D| AG[GPU 显存]
        B[网格/顶点] -->|VBO/IBO| AG
        C[音频样本] -->|OpenSL| AD[音频缓冲区]
        D[Physics/Logic] --> N[Native 碎片]
    end

    subgraph Java Heap [Java 堆]
        E[Activity/View] -->|GC 管理| J[Java 连续空间]
        F[Bitmap] -->|Android 8+ 移到 Native| N
        G[网络/JSON] --> J
    end

    subgraph Shared Memory
        H[Ashmem<br/>匿名共享]
        I[MemoryFile<br/>大文件映射]
    end
```

### Android 内存分配演进

```
Android 版本        Bitmap         Large Objects     GC 行为
API 26 (8.0):      NativeHeap       NativeHeap       Concurrent
API 29 (10):       NativeHeap       NativeHeap       Concurrent + Compaction
API 31 (12):       NativeHeap       NativeHeap       Low Pause (<= 2ms)
API 34 (14):       NativeHeap       NativeHeap       可配置 GC 模式

核心结论:
  - Bitmap 已从 Java Heap 移出，大纹理无 GC 压力
  - 但大量小对象 (Java) 仍需关注 GC
  - 推荐使用 Native (C++) 管理游戏内存
```

## 2. 纹理内存管理

### 纹理内存预算

```
纹理内存 = 宽度 × 高度 × 像素字节 × Mip 链系数

格式内存计算:
  RGBA8888:  w × h × 4          bytes
  ETC2:      w × h × 0.5        bytes (= 4:1)
  ASTC 4x4:  w × h × 0.5        bytes (= 4:1)
  ASTC 6x6:  w × h × 0.22       bytes (= 9:1)
  ASTC 8x8:  w × h × 0.125      bytes (= 16:1)

Mip 链系数:
  Mip Levels = floor(log2(max(w, h))) + 1
  完整 Mip:  总内存 ≈ 1.33 × base_level  (几何级数 1 + 1/4 + 1/16 + ...)
```

### 纹理池化策略

```mermaid
flowchart LR
    LOAD[加载纹理] --> POOL{Texture Pool<br/>中已存在}
    POOL -->|命中| REF[引用计数+1]
    POOL -->|未命中| BUDGET{预算充足}

    BUDGET -->|是| NEW[创建新纹理]
    BUDGET -->|否| EVICT{有可淘汰<br/>引用=0}

    EVICT -->|是| REMOVE[LRU 淘汰]
    REMOVE --> BUDGET
    EVICT -->|否| DOWNSCALE[降质量加载<br/>小尺寸/低Mip]

    NEW --> POOL
    DOWNSCALE --> POOL

    REF --> UNLOAD[引用归0]
    UNLOAD --> POOL
```

## 3. 资源打包标准

### APK 结构建议

```
app.apk
├── lib/
│   ├── arm64-v8a/libgame.so
│   ├── armeabi-v7a/libgame.so
│   └── x86_64/libgame.so       # 可选：仅模拟器
├── assets/
│   ├── textures/
│   │   ├── scene1.astc          # ASTC 纹理图集
│   │   └── ui.rgba              # UI 纹理 (uncompressed)
│   ├── models/
│   │   └── characters.glb       # glTF 模型
│   ├── shaders/
│   │   └── core.spv             # SPIR-V Shader
│   └── config/
│       └── game_settings.json   # 运行时配置
└── res/
    └── raw/
        ├── bgm.ogg              # 背景音乐
        └── sfx_pack.ogg         # 音效组合文件
```

### Play Asset Delivery 策略

```mermaid
flowchart TB
    PAD[Play Asset Delivery] -->|安装时分发| INSTALL[install-time<br/>核心资源<br/>~50MB]
    PAD -->|首次启动下载| FT[fast-follow<br/>Level 1-3<br/>~100MB]
    PAD -->|按需下载| ODEM[on-demand<br/>Level 4+<br/>~150MB]
    PAD -->|运行时流式| STREAM[streaming<br/>高清纹理<br/>~200MB]

    INSTALL --> DONE[可运行]
    FT --> DONE
    ODEM -->|加载中| PLACEHOLDER[使用低清占位]
    STREAM -->|按需| STREAM_LOAD[逐步替换]
```

## 4. 对象池模式

```mermaid
flowchart TB
    subgraph Object Pool
        P[Pool 容器] -->|acquire| A[从空闲队列取]
        P -->|release| R[放回空闲队列]

        A -->|如果空| GROW[allocate new]
        A -->|如果有| REUSE[重用]
    end

    subgraph 适用对象
        PROJ[Projectile 抛射物]
        PART[Particle 粒子]
        FX[Visual Effect 特效]
        ENEMY[Enemy 敌人]
    end
```

```cpp
// 标准对象池接口
template<typename T>
class ObjectPool {
    std::vector<T> pool_;       // 连续内存
    std::vector<size_t> free_;  // 空闲索引栈

    T* acquire() {
        if (free_.empty()) {
            pool_.emplace_back();
            return &pool_.back();
        }
        auto idx = free_.back();
        free_.pop_back();
        return &pool_[idx];
    }

    void release(T* obj) {
        auto idx = obj - pool_.data();
        free_.push_back(idx);
    }
};
```

## 5. Android onTrimMemory 响应

```mermaid
flowchart TB
    OS[系统内存紧张] -->|onTrimMemory 级别| LEVEL{级别}

    LEVEL -->|TRIM_MEMORY_RUNNING_CRITICAL| CRIT[释放所有非必需资源<br/>‣ 高 Mip 级别<br/>‣ 音频缓存<br/>‣ Shader 缓存]
    LEVEL -->|TRIM_MEMORY_RUNNING_LOW| LOW[释放 LRU 资源<br/>‣ 上一场景资源<br/>‣ 预加载缓存]
    LEVEL -->|TRIM_MEMORY_RUNNING_MODERATE| MOD[释放冷数据<br/>‣ 未使用的纹理]
    LEVEL -->|TRIM_MEMORY_UI_HIDDEN| HID[完全释放场景<br/>进入后台模式<br/>保存状态后释放大部分内存]
```

## 6. Shader 缓存策略

### Shader 编译卡顿问题

```mermaid
xychart-beta
    title "Shader 编译时间分布"
    x-axis ["首次", "缓存后", "驱动更新"]
    y-axis "编译耗时 (ms)" 0 --> 300
    bar [280, 5, 260]
    bar [250, 3, 240]
    bar [200, 2, 200]
```

### 缓存策略

```
1. 离线编译
   - 使用 Android GPU Inspector 收集机型 Shader 缓存
   - 打包时包含常见 GPU 的缓存

2. 运行时缓存
   - glGetProgramBinary → 存到 filesDir
   - 下次启动直接 glProgramBinary 加载

3. 后台编译
   - 首次编译在 loading 界面完成
   - 场景切换时预变异

4. 回退方案
   - 简单备用 Shader (无特效)
   - 渐进式特效开启
```

## 7. 资源引用计数

```yaml
资源生命周期策略:
  强引用:   游戏中正在使用 → 不可卸载
  弱引用:   引用计数为 0 → LRU 候选
  预加载:   提前 3 秒开始加载 → 进入温区
  流式:     距离阈值触发 → 逐步加载高 Mip

引用计数规则:
  场景加载:       +1
  场景活跃:       ≥1
  场景卸载:       -1 (计数归 0 → 可淘汰)
  特效播放:       +1 (临时)
  特效结束:       -1

注意:
  - 避免循环引用 (使用 weak_ptr)
  - 引用图定期巡检 (每 10s 检查孤立资源)
```

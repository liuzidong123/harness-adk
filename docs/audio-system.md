# Android 游戏音频系统标准

> 行业标准参考：OpenSL ES 规范、AAudio API、Wwise/FMOD 集成指南

---

## 1. API 选型

```mermaid
flowchart TB
    Q{目标 API?} -->|API 16+| O[OpenSL ES<br/>最低延迟 ~20ms]
    Q -->|API 27+| A[AAudio<br/>最低延迟 ~10ms]
    Q -->|AudioTrack| T[AudioTrack<br/>延迟 30-100ms<br/>最简单]
    Q -->|Oboe| OB[Oboe 库<br/>AAudio 封装<br/>API 16+ 兼容]

    O -->|复杂| EX1[3D 音效<br/>多路混音]
    A -->|低延迟| EX2[实时交互音乐]
    T -->|简单| EX3[背景音乐播放]
    OB -->|跨版本| EX4[推荐方案]
```

### 推荐：Oboe 库

```
Oboe 优点:
  - API 16+ 统一接口 (底层自动选择 AAudio/OpenSL)
  - 低延迟 (AAudio 路径 < 10ms)
  - 官方维护 (Google)
  - C++ 原生接口

集成:
  dependencies:
    - implementation("com.google.oboe:oboe:1.9.0")

  CMake:
    find_library(oboe oboe)
    target_link_libraries(game oboe)
```

## 2. 音频架构

```mermaid
graph TB
    subgraph 游戏逻辑
        EVENT[GameEvent<br/>碰撞/射击/踏板]
        COMMAND[AudioCommand<br/>播放/停止/参数]
    end

    subgraph 音频引擎
        MIXER[AudioMixer<br/>多通道混音]
        BUS[AudioBus 系统<br/>SFX / Music / Voice / Ambience]
        VOICE[VoiceManager<br/>声道池]
        DSP[DSP 效果<br/>Reverb / EQ / Doppler]
    end

    subgraph 平台层
        AA[AAudio / OpenSL ES]
        STR[StreamManager<br/>流管理]
    end

    subgraph 外设
        SPK[扬声器]
        HP[耳机]
        BT[蓝牙耳机]
    end

    EVENT --> COMMAND
    COMMAND --> MIXER
    COMMAND --> VOICE
    MIXER --> BUS
    BUS --> DSP
    DSP --> STR
    STR --> AA
    AA --> SPK
    AA --> HP
    AA --> BT
```

## 3. 声音资源管理

### 音频格式选择

| 格式 | 比特率 | 场景 | 压缩 | 延迟 |
|------|--------|------|------|------|
| AAC | 128-320kbps | 背景音乐 | 有损 | 高 (需解码) |
| MP3 | 128-320kbps | 兼容 | 有损 | 高 (需解码) |
| OGG Vorbis | 96-256kbps | 音乐+SFX | 有损 | 中 |
| Opus | 32-128kbps | 语音 | 有损 | 低 |
| WAV/PCM | 1411kbps (16bit) | SFX/UI | 无损 | 极低 |
| ADPCM | 352kbps | SFX 压缩 | 无损/压缩 | 低 |

### 音频资源配置

```yaml
资源分类:
  BGM:         背景音乐 (OGG, 128kbps, 立体声, 44.1kHz)
  SFX:         音效 (WAV/ADPCM, 单声道, 22.05kHz)
  VOICE:       语音 (Opus, 32kbps, 单声道, 16kHz)
  UI:          UI 音效 (WAV/ADPCM, 单声道, 22.05kHz)

内存预算:
  同时加载:
    BGM:  1 首 ≈ 5-20MB (流式解码 500KB buffer)
    SFX:  50-100 个 ≈ 5-10MB
    VOICE: 10-20 条 ≈ 1-2MB

  同时播放:
    SFX:     ≤ 16 voices
    BGM:     ≤ 1 voice
    VOICE:   ≤ 2 voices
    UI:      ≤ 4 voices
```

## 4. 3D 音频规格

### 监听器与声源

```mermaid
flowchart LR
    subgraph 声学场景
        L[Listener<br/>~ 摄像机]
        S1[Source 1<br/>敌人枪声]
        S2[Source 2<br/>环境爆炸]
        S3[Source 3<br/>脚步]
    end

    subgraph 衰减模型
        LN[线性: dist_max ≈ 50m]
        INV[逆距离: realistic]
        EXP[指数: game feel]
    end

    L -->|距离| ATTN[衰减计算]
    S1 --> ATTN
    S2 --> ATTN
    S3 --> ATTN
    ATTN --> PAN[立体声定位]
    ATTN --> VOL[音量衰减]
    ATTN --> DOP[多普勒效应]
```

### 衰减参数

```
线性衰减:   gain = 1 - clamp(distance/range, 0, 1)
逆距离:     gain = 1 / (1 + distance * rolloff)
指数:       gain = pow(1/distance, exponent)
多普勒:     pitch_shift = (v_source + v_listener) / speed_of_sound

推荐默认:
  最小距离:    1m     (全音量)
  最大距离:    50m    (静音)
  滚降系数:    1.0    (线性)
  多普勒:      1.0    (物理正确或按需)
```

## 5. 动态混音总线

```mermaid
graph TB
    subgraph 混音总线
        BUS_MAST[Master Bus<br/>0 dB]
        BUS_BGM[BGM Bus<br/>-6 dB]
        BUS_SFX[SFX Bus<br/>0 dB]
        BUS_VOICE[Voice Bus<br/>-3 dB]
        BUS_UI[UI Bus<br/>-3 dB]
        BUS_AMB[Ambience Bus<br/>-6 dB]
    end

    subgraph 音效声道
        S1[Gunshot] --> BUS_SFX
        S2[Footstep] --> BUS_SFX
        S3[Explosion] --> BUS_SFX
        BGM[BGM Track] --> BUS_BGM
        DL[Dialog Line] --> BUS_VOICE
        CL[Click] --> BUS_UI
        ENV[Wind/Bird] --> BUS_AMB
    end

    BUS_BGM --> BUS_MAST
    BUS_SFX --> BUS_MAST
    BUS_VOICE --> BUS_MAST
    BUS_UI --> BUS_MAST
    BUS_AMB --> BUS_MAST

    BUS_MAST --> DUCK[Ducking<br/>语音时 BGM 降 6dB]
```

### Ducking 规则

```
触发: Voice 总线有活动
     → BGM 总线音量 -6dB (衰减时间 100ms)
     → 恢复: 语音停止后 500ms 恢复 (恢复时间 200ms)

优先级: Voice > SFX > BGM
```

## 6. 音频延迟标准

### 端到端延迟

```
Oboe (AAudio 路径):
  播放延迟:   ~10-15ms
  录制延迟:   ~15-20ms

OpenSL ES:
  播放延迟:   ~20-40ms
  录制延迟:   ~30-50ms

AudioTrack:
  播放延迟:   ~30-100ms

目标:
  触觉反馈音效:   < 20ms (和触摸同步)
  UI 音效:         < 30ms
  环境/BGM:        < 100ms (容忍度高)
```

### 低延迟配置

```
Oboe 首选设置:
  sampleRate:      设备原生采样率 (query via AAudioStreamBuilder)
  samplesPerFrame: 2 (立体声) / 1 (单声道)
  format:          16-bit PCM (最低功耗)
  sharingMode:     EXCLUSIVE (独占低延迟)
  performanceMode: LOW_LATENCY
  bufferCapacity:  双缓冲 (最小安全值)

回调:
  AudioStreamDataCallback → 填充下一个 buffer
  确保回调内无锁/无分配
```

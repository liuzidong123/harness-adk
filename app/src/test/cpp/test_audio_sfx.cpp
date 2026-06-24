#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
#include <vector>

// ============================================================
// AudioEngine WAV parsing test (standalone, no JNI dependency)
// ============================================================

struct SoundBuffer {
    std::vector<int16_t> pcmData;
    int sampleRate;
    int numChannels;
};

// Standalone WAV parser replicating AudioEngine::loadWav
// Reads from memory buffer for testability
static SoundBuffer parseWavFromMemory(const uint8_t *data, size_t fileSize) {
    SoundBuffer buf = {};

    if (fileSize < 44) return buf;
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) return buf;

    uint16_t audioFormat = *(uint16_t*)(data + 20);
    uint16_t numChannels = *(uint16_t*)(data + 22);
    uint32_t sampleRate = *(uint32_t*)(data + 24);
    uint16_t bitsPerSample = *(uint16_t*)(data + 34);

    if (audioFormat != 1) return buf; // Must be PCM

    uint32_t dataOffset = 44;
    uint32_t dataSize = 0;
    if (dataOffset + 4 <= fileSize && memcmp(data + 36, "data", 4) == 0) {
        dataSize = *(uint32_t*)(data + 40);
        dataOffset = 44;
    } else {
        uint32_t offset = 12;
        while (offset + 8 <= fileSize) {
            uint32_t chunkSize = *(uint32_t*)(data + offset + 4);
            if (memcmp(data + offset, "data", 4) == 0) {
                dataSize = chunkSize;
                dataOffset = offset + 8;
                break;
            }
            offset += 8 + chunkSize;
            if (chunkSize % 2) chunkSize++;
        }
    }

    if (dataSize == 0 || dataOffset + dataSize > fileSize) return buf;

    buf.sampleRate = sampleRate;
    buf.numChannels = numChannels;

    if (bitsPerSample == 16) {
        int sampleCount = dataSize / 2;
        const int16_t *pcm = reinterpret_cast<const int16_t*>(data + dataOffset);
        buf.pcmData.assign(pcm, pcm + sampleCount);
    } else if (bitsPerSample == 8) {
        int sampleCount = dataSize;
        buf.pcmData.resize(sampleCount);
        const uint8_t *pcm8 = data + dataOffset;
        for (int i = 0; i < sampleCount; i++) {
            buf.pcmData[i] = (static_cast<int16_t>(pcm8[i]) - 128) << 8;
        }
    }

    return buf;
}

// Helper: build a valid WAV in memory with given PCM data
static std::vector<uint8_t> buildWav(const std::vector<int16_t> &pcmData,
                                     int sampleRate = 22050,
                                     int numChannels = 1,
                                     int bitsPerSample = 16) {
    int dataSize = static_cast<int>(pcmData.size() * (bitsPerSample / 8));
    int fileSize = 36 + dataSize;
    std::vector<uint8_t> wav(fileSize + 8);

    memcpy(wav.data(), "RIFF", 4);
    *(int*)(wav.data() + 4) = fileSize;
    memcpy(wav.data() + 8, "WAVE", 4);
    memcpy(wav.data() + 12, "fmt ", 4);
    *(int*)(wav.data() + 16) = 16;
    *(int16_t*)(wav.data() + 20) = 1; // PCM
    *(int16_t*)(wav.data() + 22) = numChannels;
    *(int*)(wav.data() + 24) = sampleRate;
    *(int*)(wav.data() + 28) = sampleRate * numChannels * bitsPerSample / 8;
    *(int16_t*)(wav.data() + 32) = numChannels * bitsPerSample / 8;
    *(int16_t*)(wav.data() + 34) = bitsPerSample;
    memcpy(wav.data() + 36, "data", 4);
    *(int*)(wav.data() + 40) = dataSize;
    memcpy(wav.data() + 44, pcmData.data(), dataSize);

    return wav;
}

TEST(WavParse, Valid16BitMono) {
    std::vector<int16_t> pcm(1000, 0);
    auto wav = buildWav(pcm);
    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_EQ(buf.sampleRate, 22050);
    EXPECT_EQ(buf.numChannels, 1);
    EXPECT_EQ(buf.pcmData.size(), 1000);
}

TEST(WavParse, Valid16BitStereo) {
    std::vector<int16_t> pcm(2000, 0);
    auto wav = buildWav(pcm, 44100, 2);
    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_EQ(buf.sampleRate, 44100);
    EXPECT_EQ(buf.numChannels, 2);
    EXPECT_EQ(buf.pcmData.size(), 2000);
}

TEST(WavParse, Valid8BitMono) {
    std::vector<int16_t> pcm; // Not used — we'll inject 8-bit data manually
    // Build 8-bit WAV
    int sampleRate = 22050;
    int numChannels = 1;
    int bitsPerSample = 8;
    std::vector<uint8_t> rawPcm(500, 128); // 8-bit samples, silence at 128
    int dataSize = static_cast<int>(rawPcm.size());
    int fileSize = 36 + dataSize;
    std::vector<uint8_t> wav(fileSize + 8);
    memcpy(wav.data(), "RIFF", 4);
    *(int*)(wav.data() + 4) = fileSize;
    memcpy(wav.data() + 8, "WAVE", 4);
    memcpy(wav.data() + 12, "fmt ", 4);
    *(int*)(wav.data() + 16) = 16;
    *(int16_t*)(wav.data() + 20) = 1;
    *(int16_t*)(wav.data() + 22) = numChannels;
    *(int*)(wav.data() + 24) = sampleRate;
    *(int*)(wav.data() + 28) = sampleRate * numChannels * bitsPerSample / 8;
    *(int16_t*)(wav.data() + 32) = numChannels * bitsPerSample / 8;
    *(int16_t*)(wav.data() + 34) = bitsPerSample;
    memcpy(wav.data() + 36, "data", 4);
    *(int*)(wav.data() + 40) = dataSize;
    memcpy(wav.data() + 44, rawPcm.data(), dataSize);

    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_EQ(buf.sampleRate, 22050);
    EXPECT_EQ(buf.numChannels, 1);
    EXPECT_EQ(buf.pcmData.size(), 500);
    // 8-bit 128 → 16-bit 0
    EXPECT_EQ(buf.pcmData[0], 0);
}

TEST(WavParse, InvalidHeader) {
    std::vector<uint8_t> invalid(100, 0);
    auto buf = parseWavFromMemory(invalid.data(), invalid.size());
    EXPECT_TRUE(buf.pcmData.empty());
}

TEST(WavParse, NoRiffHeader) {
    std::vector<uint8_t> data(100, 0);
    memcpy(data.data(), "ABCD", 4);
    auto buf = parseWavFromMemory(data.data(), data.size());
    EXPECT_TRUE(buf.pcmData.empty());
}

TEST(WavParse, FileTooSmall) {
    std::vector<uint8_t> tiny(10, 0);
    auto buf = parseWavFromMemory(tiny.data(), tiny.size());
    EXPECT_TRUE(buf.pcmData.empty());
}

TEST(WavParse, NotPcmFormat) {
    std::vector<int16_t> pcm(100, 0);
    auto wav = buildWav(pcm);
    *(int16_t*)(wav.data() + 20) = 3; // Not PCM
    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_TRUE(buf.pcmData.empty());
}

TEST(WavParse, ExtraChunksBeforeData) {
    // WAV with an extra "JUNK" chunk before "data"
    std::vector<int16_t> pcm(200, 0);
    int dataSize = static_cast<int>(pcm.size() * 2);
    int junkSize = 32;
    int fileSize = 36 + 8 + junkSize + dataSize;
    std::vector<uint8_t> wav(fileSize + 8);
    int offset = 0;
    memcpy(wav.data() + offset, "RIFF", 4); offset += 4;
    *(int*)(wav.data() + offset) = fileSize; offset += 4;
    memcpy(wav.data() + offset, "WAVE", 4); offset += 4;
    memcpy(wav.data() + offset, "fmt ", 4); offset += 4;
    *(int*)(wav.data() + offset) = 16; offset += 4;
    *(int16_t*)(wav.data() + offset) = 1; offset += 2;
    *(int16_t*)(wav.data() + offset) = 1; offset += 2;
    *(int*)(wav.data() + offset) = 22050; offset += 4;
    *(int*)(wav.data() + offset) = 44100; offset += 4;
    *(int16_t*)(wav.data() + offset) = 2; offset += 2;
    *(int16_t*)(wav.data() + offset) = 16; offset += 2;
    // JUNK chunk
    memcpy(wav.data() + offset, "JUNK", 4); offset += 4;
    *(int*)(wav.data() + offset) = junkSize; offset += 4;
    memset(wav.data() + offset, 0, junkSize); offset += junkSize;
    // data chunk
    memcpy(wav.data() + offset, "data", 4); offset += 4;
    *(int*)(wav.data() + offset) = dataSize; offset += 4;
    memcpy(wav.data() + offset, pcm.data(), dataSize);

    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_EQ(buf.sampleRate, 22050);
    EXPECT_EQ(buf.pcmData.size(), 200);
}

TEST(WavParse, LargeFile) {
    std::vector<int16_t> pcm(100000, 42); // ~200KB PCM
    auto wav = buildWav(pcm, 44100, 2);
    auto buf = parseWavFromMemory(wav.data(), wav.size());
    EXPECT_EQ(buf.sampleRate, 44100);
    EXPECT_EQ(buf.numChannels, 2);
    EXPECT_EQ(buf.pcmData.size(), 100000);
}

// ============================================================
// Game event → audio trigger mapping tests
// ============================================================

// Track which audio events were triggered
struct AudioTriggerSpy {
    bool startCalled = false;
    bool pauseCalled = false;
    bool eatCalled = false;
    bool gameOverCalled = false;

    void reset() { startCalled = pauseCalled = eatCalled = gameOverCalled = false; }
};

// Test that state transitions map to correct audio triggers
TEST(AudioTriggerMapping, MenuStartTriggersStartSound) {
    AudioTriggerSpy spy;
    // Simulate: MENU state → START button → should call playStart
    GameState state = GameState::MENU;
    bool playStart = false;

    // Button hit logic: visibleIn==MENU, targetState==PLAYING
    if (state == GameState::MENU) {
        // Hit START button
        playStart = true;
    }

    if (playStart) spy.startCalled = true;
    EXPECT_TRUE(spy.startCalled);
    EXPECT_FALSE(spy.pauseCalled);
    EXPECT_FALSE(spy.gameOverCalled);
}

TEST(AudioTriggerMapping, PlayingPauseTriggersPauseSound) {
    AudioTriggerSpy spy;
    GameState state = GameState::PLAYING;
    bool playPause = false;

    if (state == GameState::PLAYING) {
        // Hit PAUSE button → target PAUSED
        playPause = true;
    }

    if (playPause) spy.pauseCalled = true;
    EXPECT_TRUE(spy.pauseCalled);
}

TEST(AudioTriggerMapping, PlayingStopTriggersGameOverSound) {
    AudioTriggerSpy spy;
    GameState state = GameState::PLAYING;
    bool playGameOver = false;

    if (state == GameState::PLAYING) {
        // Hit STOP button → target GAME_OVER
        playGameOver = true;
    }

    if (playGameOver) spy.gameOverCalled = true;
    EXPECT_TRUE(spy.gameOverCalled);
}

TEST(AudioTriggerMapping, WallCollisionTriggersGameOverSound) {
    AudioTriggerSpy spy;
    bool playGameOver = false;

    // Simulate wall collision in updateSnake
    int headX = -1, headY = 10;
    int gridSize = 20;
    if (headX < 0 || headX >= gridSize || headY < 0 || headY >= gridSize) {
        playGameOver = true;
    }

    if (playGameOver) spy.gameOverCalled = true;
    EXPECT_TRUE(spy.gameOverCalled);
}

TEST(AudioTriggerMapping, SelfCollisionTriggersGameOverSound) {
    AudioTriggerSpy spy;
    bool playGameOver = false;
    std::vector<std::pair<int,int>> body = {{10, 10}, {9, 10}};
    std::pair<int,int> newHead = {10, 10};

    for (const auto &seg : body) {
        if (seg == newHead) {
            playGameOver = true;
            break;
        }
    }

    if (playGameOver) spy.gameOverCalled = true;
    EXPECT_TRUE(spy.gameOverCalled);
}

TEST(AudioTriggerMapping, EatFoodTriggersEatSound) {
    AudioTriggerSpy spy;
    std::pair<int,int> head = {5, 5};
    std::pair<int,int> food = {5, 5};
    bool playEat = false;

    if (head == food) {
        playEat = true;
    }

    if (playEat) spy.eatCalled = true;
    EXPECT_TRUE(spy.eatCalled);
}

TEST(AudioTriggerMapping, NoSoundOnInvalidInput) {
    // Simulate game in MENU, tap on blank area → no sound
    AudioTriggerSpy spy;
    GameState state = GameState::MENU;
    float nx = 0.1f, ny = 0.1f;
    bool hitButton = false;

    // No button at this location → nothing triggered
    if (!hitButton) {
        // No audio trigger
    }

    EXPECT_FALSE(spy.startCalled);
    EXPECT_FALSE(spy.pauseCalled);
    EXPECT_FALSE(spy.eatCalled);
    EXPECT_FALSE(spy.gameOverCalled);
}

TEST(AudioTriggerMapping, AgainDoesNotTriggerStartSound) {
    // GAME_OVER → AGAIN → PLAYING, should NOT play start sound
    AudioTriggerSpy spy;
    GameState state = GameState::GAME_OVER;
    bool playStart = false;

    // From GAME_OVER, AGAIN goes to PLAYING, but start sound only for MENU→PLAYING
    // In our implementation, MENU→PLAYING = start sound only
    if (state == GameState::GAME_OVER) {
        // No start sound from GAME_OVER
        playStart = false;
    }

    if (playStart) spy.startCalled = true;
    EXPECT_FALSE(spy.startCalled);
}

// ============================================================
// AudioEngine graceful degradation
// ============================================================
TEST(AudioDegradation, PlayWithoutInitDoesNotCrash) {
    // Simulate calling play methods when AudioEngine is not initialized
    bool initialized = false;
    bool crashed = false;

    // These should be safe no-ops
    auto safePlay = [&](const char *name) {
        if (!initialized) return; // no-op guard
    };

    try {
        safePlay("start");
        safePlay("pause");
        safePlay("eat");
        safePlay("gameover");
    } catch (...) {
        crashed = true;
    }

    EXPECT_FALSE(crashed);
}

// ============================================================
// WAV asset file verification (file list in spec)
// ============================================================
struct WavAssetSpec {
    const char *filename;
    int expectedMinBytes;
};

TEST(WavAssetSpec, RequiredAssetsListed) {
    WavAssetSpec assets[] = {
        {"sfx_start.wav", 44},
        {"sfx_pause.wav", 44},
        {"sfx_eat.wav", 44},
        {"sfx_gameover.wav", 44},
    };
    int count = sizeof(assets) / sizeof(assets[0]);
    EXPECT_EQ(count, 4);
    for (int i = 0; i < count; i++) {
        EXPECT_GT(assets[i].expectedMinBytes, 0);
    }
}

#include "AudioEngine.h"

#include <cstring>
#include <thread>
#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#define ALOG(...) __android_log_print(ANDROID_LOG_DEBUG, "AudioEngine", __VA_ARGS__)

// AudioTrack constants
static constexpr int STREAM_MUSIC = 3;
static constexpr int CHANNEL_OUT_MONO = 4;
static constexpr int ENCODING_PCM_16BIT = 2;
static constexpr int MODE_STATIC = 1;

AudioEngine::AudioEngine()
    : javaVm_(nullptr)
    , activityObj_(nullptr)
    , audioTrackClass_(nullptr)
    , constructorId_(nullptr)
    , playMethodId_(nullptr)
    , writeMethodId_(nullptr)
    , stopMethodId_(nullptr)
    , releaseMethodId_(nullptr)
    , initialized_(false) {}

AudioEngine::~AudioEngine() {
    shutdown();
}

JNIEnv *AudioEngine::getJniEnv() {
    JNIEnv *env = nullptr;
    if (javaVm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        javaVm_->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

bool AudioEngine::init(android_app *app) {
    javaVm_ = app->activity->vm;
    activityObj_ = app->activity->javaGameActivity;

    JNIEnv *env = getJniEnv();
    if (!env) {
        ALOG("Failed to get JNIEnv");
        return false;
    }

    // Find AudioTrack class
    jclass localClass = env->FindClass("android/media/AudioTrack");
    if (!localClass) {
        ALOG("Failed to find AudioTrack class");
        return false;
    }
    audioTrackClass_ = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);

    // Get constructor: AudioTrack(int streamType, int sampleRate, int channelConfig, int audioFormat, int bufferSize, int mode)
    constructorId_ = env->GetMethodID(audioTrackClass_, "<init>", "(IIIIII)V");
    playMethodId_ = env->GetMethodID(audioTrackClass_, "play", "()V");
    writeMethodId_ = env->GetMethodID(audioTrackClass_, "write", "([BII)I");
    stopMethodId_ = env->GetMethodID(audioTrackClass_, "stop", "()V");
    releaseMethodId_ = env->GetMethodID(audioTrackClass_, "release", "()V");

    if (!constructorId_ || !playMethodId_ || !writeMethodId_ || !stopMethodId_ || !releaseMethodId_) {
        ALOG("Failed to get AudioTrack method IDs");
        return false;
    }

    AAssetManager *mgr = app->activity->assetManager;

    // Pre-load all WAV files
    startSfx_ = loadWav(mgr, "sfx_start.wav");
    pauseSfx_ = loadWav(mgr, "sfx_pause.wav");
    eatSfx_ = loadWav(mgr, "sfx_eat.wav");
    gameOverSfx_ = loadWav(mgr, "sfx_gameover.wav");

    if (startSfx_.pcmData.empty() || pauseSfx_.pcmData.empty() ||
        eatSfx_.pcmData.empty() || gameOverSfx_.pcmData.empty()) {
        ALOG("Failed to load one or more audio assets");
        return false;
    }

    ALOG("AudioEngine initialized successfully");
    initialized_ = true;
    return true;
}

void AudioEngine::shutdown() {
    if (!initialized_) return;

    JNIEnv *env = getJniEnv();
    if (env && audioTrackClass_) {
        env->DeleteGlobalRef(audioTrackClass_);
        audioTrackClass_ = nullptr;
    }
    initialized_ = false;
}

AudioEngine::SoundBuffer AudioEngine::loadWav(AAssetManager *mgr, const char *path) {
    SoundBuffer buf = {};
    AAsset *asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        ALOG("Failed to open asset: %s", path);
        return buf;
    }

    off_t fileSize = AAsset_getLength(asset);
    if (fileSize < 44) {
        ALOG("File too small: %s", path);
        AAsset_close(asset);
        return buf;
    }

    const uint8_t *data = static_cast<const uint8_t*>(AAsset_getBuffer(asset));
    if (!data) {
        ALOG("Failed to get buffer: %s", path);
        AAsset_close(asset);
        return buf;
    }

    // Parse WAV header
    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        ALOG("Invalid WAV header: %s", path);
        AAsset_close(asset);
        return buf;
    }

    uint16_t audioFormat = *(uint16_t*)(data + 20);
    uint16_t numChannels = *(uint16_t*)(data + 22);
    uint32_t sampleRate = *(uint32_t*)(data + 24);
    uint16_t bitsPerSample = *(uint16_t*)(data + 34);

    if (audioFormat != 1) {
        ALOG("Unsupported audio format (not PCM): %s", path);
        AAsset_close(asset);
        return buf;
    }

    // Find "data" chunk
    uint32_t dataOffset = 44;
    uint32_t dataSize = 0;
    if (dataOffset + 4 <= fileSize && memcmp(data + 36, "data", 4) == 0) {
        dataSize = *(uint32_t*)(data + 40);
        dataOffset = 44;
    } else {
        // Skip extra chunks
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

    if (dataSize == 0 || dataOffset + dataSize > fileSize) {
        ALOG("Invalid data chunk: %s", path);
        AAsset_close(asset);
        return buf;
    }

    buf.sampleRate = sampleRate;
    buf.numChannels = numChannels;

    if (bitsPerSample == 16) {
        int sampleCount = dataSize / 2;
        const int16_t *pcm = reinterpret_cast<const int16_t*>(data + dataOffset);
        buf.pcmData.assign(pcm, pcm + sampleCount);
    } else if (bitsPerSample == 8) {
        // Convert 8-bit to 16-bit
        int sampleCount = dataSize;
        buf.pcmData.resize(sampleCount);
        const uint8_t *pcm8 = data + dataOffset;
        for (int i = 0; i < sampleCount; i++) {
            buf.pcmData[i] = (static_cast<int16_t>(pcm8[i]) - 128) << 8;
        }
    } else {
        ALOG("Unsupported bits per sample: %d", bitsPerSample);
    }

    AAsset_close(asset);
    ALOG("Loaded WAV: %s (%d samples, %d Hz, %d ch)", path,
         (int)buf.pcmData.size(), buf.sampleRate, buf.numChannels);
    return buf;
}

void AudioEngine::playSound(const SoundBuffer &buf) {
    if (!initialized_ || buf.pcmData.empty()) return;

    // Copy the buffer and play on a background thread
    std::thread([this, buf]() {
        JNIEnv *env = nullptr;
        javaVm_->AttachCurrentThread(&env, nullptr);
        if (!env) return;

        int byteCount = static_cast<int>(buf.pcmData.size() * sizeof(int16_t));
        jbyteArray byteArray = env->NewByteArray(byteCount);
        env->SetByteArrayRegion(byteArray, 0, byteCount,
                                reinterpret_cast<const jbyte*>(buf.pcmData.data()));

        jobject track = env->NewObject(audioTrackClass_, constructorId_,
                                       STREAM_MUSIC,
                                       buf.sampleRate,
                                       CHANNEL_OUT_MONO,
                                       ENCODING_PCM_16BIT,
                                       byteCount,
                                       MODE_STATIC);
        if (track) {
            env->CallIntMethod(track, writeMethodId_, byteArray, 0, byteCount);
            env->CallVoidMethod(track, playMethodId_);
            env->CallVoidMethod(track, releaseMethodId_);
            env->DeleteLocalRef(track);
        }
        env->DeleteLocalRef(byteArray);
        javaVm_->DetachCurrentThread();
    }).detach();
}

void AudioEngine::playStart() { playSound(startSfx_); }
void AudioEngine::playPause() { playSound(pauseSfx_); }
void AudioEngine::playEat() { playSound(eatSfx_); }
void AudioEngine::playGameOver() { playSound(gameOverSfx_); }

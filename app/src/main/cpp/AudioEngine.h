#ifndef SNAKESHOT_AUDIOENGINE_H
#define SNAKESHOT_AUDIOENGINE_H

#include <cstdint>
#include <vector>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

struct android_app;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init(android_app *app);
    void shutdown();

    void playStart();
    void playPause();
    void playEat();
    void playGameOver();

private:
    struct SoundBuffer {
        std::vector<int16_t> pcmData;
        int sampleRate;
        int numChannels;
    };

    SoundBuffer loadWav(AAssetManager *mgr, const char *path);
    void playSound(const SoundBuffer &buf);

    JNIEnv *getJniEnv();

    JavaVM *javaVm_;
    jobject activityObj_;
    jclass audioTrackClass_;
    jmethodID constructorId_;
    jmethodID playMethodId_;
    jmethodID writeMethodId_;
    jmethodID stopMethodId_;
    jmethodID releaseMethodId_;

    SoundBuffer startSfx_;
    SoundBuffer pauseSfx_;
    SoundBuffer eatSfx_;
    SoundBuffer gameOverSfx_;

    bool initialized_;
};

#endif

#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace soft3d {

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    virtual bool Start(int sampleRate, int channels) = 0;
    virtual void Stop() = 0;
    virtual bool Submit(const int16_t* samples, std::size_t sampleCount) = 0;
};

class s3dAudioDevice {
public:
    static constexpr int voiceCount = 16;
    static constexpr int mixSampleCount = 400;
    static constexpr int sampleRate = 16000;

    explicit s3dAudioDevice(std::unique_ptr<AudioBackend> backend);
    ~s3dAudioDevice();

    s3dAudioDevice(const s3dAudioDevice&) = delete;
    s3dAudioDevice& operator=(const s3dAudioDevice&) = delete;

    bool Start();
    void Stop();
    void Pause();
    void Resume();
    void Update();

    void StartVoice(const int16_t* pcm, uint32_t pcmSampleCount, int voice, int volume,
                    bool looping);
    void StopVoice(int voice);
    void StopAllVoices();
    bool IsVoiceFinished(int voice) const;
    void SetVoiceLooping(int voice, bool looping);
    bool GetVoiceLooping(int voice) const;
    void SetVoiceVolume(int voice, int volume);
    int GetVoiceVolume(int voice) const;
    void SetVoiceVolumeMode(int voice, bool direct);
    void SetMasterVolume(int volume);
    int GetMasterVolume() const;
    uint32_t GetVoicePosition(int voice) const;

    const std::array<int16_t, mixSampleCount>& MixBlock();
    bool IsRunning() const;

private:
    static int VolumeToShift(int volume);
    void Run();
    void SubmitBlock();
    void RecordStreamError();

    mutable std::mutex stateMutex;
    int masterShift = 7;
    std::array<const int16_t*, voiceCount> pcm{};
    std::array<uint32_t, voiceCount> pcmSampleCounts{};
    std::array<uint32_t, voiceCount> positions{};
    std::array<int, voiceCount> volumes{};
    std::array<uint32_t, voiceCount> looping{};
    std::array<uint32_t, voiceCount> finished{};
    std::array<uint32_t, voiceCount> directVolume{};
    std::array<int32_t, mixSampleCount> accumulation{};
    std::array<int16_t, mixSampleCount> output{};
    int streamError = 0;
    int recoveryCountdown = 0;

    std::unique_ptr<AudioBackend> backend;
    std::thread worker;
    std::mutex commandMutex;
    std::condition_variable commandChanged;
    int command = 0;
    std::atomic<bool> active{true};
    std::atomic<bool> streamOpen{false};
};

} // namespace soft3d

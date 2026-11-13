#pragma once

#include "soft3d/AudioDevice.hpp"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace soft3d::platform {

class DummyAudioBackend final : public AudioBackend {
public:
    bool Start(int sampleRate, int channels) override;
    void Stop() override;
    bool Submit(const int16_t* samples, std::size_t sampleCount) override;

    void SetStartResult(bool result);
    void SetSubmitResult(bool result);
    bool IsRunning() const;
    int GetStartCount() const;
    int GetStopCount() const;
    uint64_t GetSubmittedSampleCount() const;
    std::vector<int16_t> GetLastBlock() const;

private:
    mutable std::mutex mutex;
    bool running = false;
    bool startResult = true;
    bool submitResult = true;
    int startCount = 0;
    int stopCount = 0;
    uint64_t submittedSamples = 0;
    std::vector<int16_t> lastBlock;
};

} // namespace soft3d::platform

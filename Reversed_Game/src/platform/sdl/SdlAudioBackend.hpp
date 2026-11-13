#pragma once

#include "soft3d/AudioDevice.hpp"

#include <SDL3/SDL.h>

namespace soft3d::platform {

class SdlAudioBackend final : public AudioBackend {
public:
    SdlAudioBackend() = default;
    ~SdlAudioBackend() override;

    bool Start(int sampleRate, int channels) override;
    void Stop() override;
    bool Submit(const int16_t* samples, std::size_t sampleCount) override;

private:
    SDL_AudioStream* stream = nullptr;
};

} // namespace soft3d::platform

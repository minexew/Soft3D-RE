#include "platform/sdl/SdlAudioBackend.hpp"

namespace soft3d::platform {

SdlAudioBackend::~SdlAudioBackend()
{
    Stop();
}

bool SdlAudioBackend::Start(int sampleRate, int channels)
{
    if (stream != nullptr) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return false;
    }

    SDL_AudioSpec specification{};
    specification.format = SDL_AUDIO_S16;
    specification.channels = channels;
    specification.freq = sampleRate;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification,
                                       nullptr, nullptr);
    if (stream == nullptr) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    if (!SDL_ResumeAudioStreamDevice(stream)) {
        Stop();
        return false;
    }
    return true;
}

void SdlAudioBackend::Stop()
{
    if (stream != nullptr) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

bool SdlAudioBackend::Submit(const int16_t* samples, std::size_t sampleCount)
{
    return stream != nullptr && SDL_PutAudioStreamData(
        stream, samples, static_cast<int>(sampleCount * sizeof(int16_t)));
}

} // namespace soft3d::platform

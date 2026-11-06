#include "app/EngineStubs.hpp"

#include <algorithm>

namespace app {

AudioStub::~AudioStub()
{
    Stop();
}

bool AudioStub::Start()
{
    if (stream != nullptr) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return false;
    }

    SDL_AudioSpec specification{};
    specification.format = SDL_AUDIO_S16;
    specification.channels = 2;
    specification.freq = 22050;
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification,
                                       nullptr, nullptr);
    if (stream == nullptr) {
        return false;
    }
    return SDL_ResumeAudioStreamDevice(stream);
}

void AudioStub::Stop()
{
    if (stream != nullptr) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool AudioStub::SubmitPcm16(const int16_t* samples, std::size_t bytes)
{
    return stream != nullptr && SDL_PutAudioStreamData(stream, samples, static_cast<int>(bytes));
}

bool AudioStub::IsRunning() const
{
    return stream != nullptr;
}

void ControlsStub::ProcessEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
        return;
    }

    const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
    switch (event.key.key) {
    case SDLK_UP:
    case SDLK_W:
        state.up = pressed;
        break;
    case SDLK_DOWN:
    case SDLK_S:
        state.down = pressed;
        break;
    case SDLK_LEFT:
    case SDLK_A:
        state.left = pressed;
        break;
    case SDLK_RIGHT:
    case SDLK_D:
        state.right = pressed;
        break;
    case SDLK_RETURN:
    case SDLK_SPACE:
        state.action = pressed;
        break;
    default:
        break;
    }
}

const ControlState& ControlsStub::State() const
{
    return state;
}

void TimerStub::Reset()
{
    previousNanoseconds = SDL_GetTicksNS();
}

double TimerStub::NextDeltaSeconds()
{
    const uint64_t now = NowNanoseconds();
    if (previousNanoseconds == 0) {
        previousNanoseconds = now;
        return 1.0 / 60.0;
    }
    const uint64_t elapsed = now - previousNanoseconds;
    previousNanoseconds = now;
    return static_cast<double>(elapsed) / 1000000000.0;
}

uint64_t TimerStub::NowNanoseconds() const
{
    return SDL_GetTicksNS();
}

void SoftwareFrame::Resize(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;
    pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
}

uint32_t* SoftwareFrame::Pixels()
{
    return pixels.data();
}

const uint32_t* SoftwareFrame::Pixels() const
{
    return pixels.data();
}

int SoftwareFrame::Pitch() const
{
    return width * static_cast<int>(sizeof(uint32_t));
}

int SoftwareFrame::Width() const
{
    return width;
}

int SoftwareFrame::Height() const
{
    return height;
}

} // namespace app

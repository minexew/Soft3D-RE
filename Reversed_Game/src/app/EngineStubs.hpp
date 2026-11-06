#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace app {

struct ControlState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool action = false;
};

class AudioStub {
public:
    ~AudioStub();

    bool Start();
    void Stop();
    bool SubmitPcm16(const int16_t* samples, std::size_t bytes);

    bool IsRunning() const;

private:
    SDL_AudioStream* stream = nullptr;
};

class ControlsStub {
public:
    void ProcessEvent(const SDL_Event& event);

    const ControlState& State() const;

private:
    ControlState state;
};

class TimerStub {
public:
    void Reset();
    double NextDeltaSeconds();
    uint64_t NowNanoseconds() const;

private:
    uint64_t previousNanoseconds = 0;
};

class SoftwareFrame {
public:
    void Resize(int width, int height);

    uint32_t* Pixels();
    const uint32_t* Pixels() const;
    int Pitch() const;
    int Width() const;
    int Height() const;

private:
    int width = 0;
    int height = 0;
    std::vector<uint32_t> pixels;
};

} // namespace app

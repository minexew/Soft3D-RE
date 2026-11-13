#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>

namespace app {

struct ControlState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool action = false;
};

class ControlsStub {
public:
    void ProcessEvent(const SDL_Event& event);

    const ControlState& State() const;
    int ConsumeMenuSelection();

private:
    ControlState state;
    int menuSelection = 0;
};

class TimerStub {
public:
    void Reset();
    double NextDeltaSeconds();
    uint64_t NowNanoseconds() const;

private:
    uint64_t previousNanoseconds = 0;
};

} // namespace app

#include "app/EngineStubs.hpp"

#include <algorithm>

namespace app {

void ControlsStub::ProcessEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
        return;
    }

    const bool pressed = event.type == SDL_EVENT_KEY_DOWN;
    if (pressed && !event.key.repeat) {
        if (event.key.key == SDLK_LEFT || event.key.key == SDLK_A ||
            event.key.key == SDLK_UP || event.key.key == SDLK_W) {
            menuSelection = -1;
        } else if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_D ||
                   event.key.key == SDLK_DOWN || event.key.key == SDLK_S) {
            menuSelection = 1;
        }
    }
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

int ControlsStub::ConsumeMenuSelection()
{
    if (menuSelection != 0) {
        const int result = menuSelection;
        menuSelection = 0;
        return result;
    }
    if (state.left || state.up) {
        return -1;
    }
    return state.right || state.down ? 1 : 0;
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

} // namespace app

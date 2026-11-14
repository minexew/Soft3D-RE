#include "platform/sdl/SdlClock.hpp"

#include <SDL3/SDL.h>

namespace soft3d::platform {

uint32_t SdlClock::NowMicroseconds()
{
    return static_cast<uint32_t>(SDL_GetTicksNS() / 1000);
}

void SdlClock::SleepMicroseconds(uint32_t microseconds)
{
    SDL_DelayNS(static_cast<uint64_t>(microseconds) * 1000);
}

} // namespace soft3d::platform

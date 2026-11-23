#include "platform/sdl/SdlVibrationBackend.hpp"

#include <SDL3/SDL_init.h>

namespace soft3d::platform {

SdlVibrationBackend::SdlVibrationBackend()
{
    if (!SDL_InitSubSystem(SDL_INIT_HAPTIC)) {
        return;
    }
    int count = 0;
    SDL_HapticID* devices = SDL_GetHaptics(&count);
    if (devices == nullptr) {
        return;
    }
    for (int index = 0; index < count && haptic == nullptr; ++index) {
        SDL_Haptic* candidate = SDL_OpenHaptic(devices[index]);
        if (candidate != nullptr) {
            if (SDL_InitHapticRumble(candidate)) {
                haptic = candidate;
            } else {
                SDL_CloseHaptic(candidate);
            }
        }
    }
    SDL_free(devices);
}

SdlVibrationBackend::~SdlVibrationBackend()
{
    if (haptic != nullptr) {
        SDL_StopHapticRumble(haptic);
        SDL_CloseHaptic(haptic);
    }
    SDL_QuitSubSystem(SDL_INIT_HAPTIC);
}

void SdlVibrationBackend::SetEnabled(bool value)
{
    enabled = value;
    if (!enabled && haptic != nullptr) {
        SDL_StopHapticRumble(haptic);
    }
}

bool SdlVibrationBackend::Vibrate(uint32_t durationMilliseconds)
{
    return enabled && haptic != nullptr &&
        SDL_PlayHapticRumble(haptic, 1.0f, durationMilliseconds);
}

} // namespace soft3d::platform

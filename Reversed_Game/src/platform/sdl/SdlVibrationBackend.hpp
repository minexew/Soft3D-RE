#pragma once

#include "platform/PlatformServices.hpp"

#include <SDL3/SDL_haptic.h>

namespace soft3d::platform {

class SdlVibrationBackend final : public VibrationBackend {
public:
    SdlVibrationBackend();
    ~SdlVibrationBackend() override;

    bool IsSupported() const override { return haptic != nullptr; }
    void SetEnabled(bool enabled) override;
    bool Vibrate(uint32_t durationMilliseconds) override;

private:
    SDL_Haptic* haptic = nullptr;
    bool enabled = false;
};

} // namespace soft3d::platform

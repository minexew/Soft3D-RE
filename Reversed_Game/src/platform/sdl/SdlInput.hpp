#pragma once

#include "soft3d/Input.hpp"

#include <SDL3/SDL.h>

namespace soft3d::platform {

Key TranslateSdlKey(SDL_Keycode key, int controlOrientation = 0);
void ProcessSdlKeyEvent(Input& input, const SDL_Event& event, int controlOrientation = 0);

} // namespace soft3d::platform

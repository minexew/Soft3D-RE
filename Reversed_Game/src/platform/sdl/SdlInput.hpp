#pragma once

#include "soft3d/Input.hpp"

#include <SDL3/SDL.h>

namespace soft3d::platform {

Key TranslateSdlKey(SDL_Keycode key);
void ProcessSdlKeyEvent(Input& input, const SDL_Event& event);

} // namespace soft3d::platform

#include "platform/sdl/SdlInput.hpp"

namespace soft3d::platform {

Key TranslateSdlKey(SDL_Keycode key)
{
    switch (key) {
    case SDLK_LEFT:
    case SDLK_A:
        return Key::Left;
    case SDLK_RIGHT:
    case SDLK_D:
        return Key::Right;
    case SDLK_UP:
    case SDLK_W:
        return Key::Up;
    case SDLK_DOWN:
    case SDLK_S:
        return Key::Down;
    case SDLK_1:
        return Key::Number1;
    case SDLK_2:
        return Key::Number2;
    case SDLK_3:
        return Key::Number3;
    case SDLK_4:
        return Key::Number4;
    case SDLK_5:
        return Key::Number5;
    case SDLK_6:
        return Key::Number6;
    case SDLK_7:
        return Key::Number7;
    case SDLK_8:
        return Key::Number8;
    case SDLK_9:
        return Key::Number9;
    case SDLK_0:
        return Key::Number0;
    case SDLK_Q:
        return Key::SoftLeft;
    case SDLK_E:
        return Key::SoftRight;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE:
        return Key::Accept;
    default:
        return Key::None;
    }
}

void ProcessSdlKeyEvent(Input& input, const SDL_Event& event)
{
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
        return;
    }
    const Key key = TranslateSdlKey(event.key.key);
    if (event.type == SDL_EVENT_KEY_DOWN) {
        input.KeyDown(key, true);
    } else {
        input.KeyUp(key, true);
    }
}

} // namespace soft3d::platform

#include "platform/sdl/SdlInput.hpp"

namespace soft3d::platform {

Key TranslateSdlKey(SDL_Keycode key, int controlOrientation)
{
    Key translated;
    switch (key) {
    case SDLK_LEFT:
    case SDLK_A:
        translated = Key::Left;
        break;
    case SDLK_RIGHT:
    case SDLK_D:
        translated = Key::Right;
        break;
    case SDLK_UP:
    case SDLK_W:
        translated = Key::Up;
        break;
    case SDLK_DOWN:
    case SDLK_S:
        translated = Key::Down;
        break;
    case SDLK_1:
        translated = Key::Number1;
        break;
    case SDLK_2:
        translated = Key::Number2;
        break;
    case SDLK_3:
        translated = Key::Number3;
        break;
    case SDLK_4:
        translated = Key::Number4;
        break;
    case SDLK_5:
        translated = Key::Number5;
        break;
    case SDLK_6:
        translated = Key::Number6;
        break;
    case SDLK_7:
        translated = Key::Number7;
        break;
    case SDLK_8:
        translated = Key::Number8;
        break;
    case SDLK_9:
        translated = Key::Number9;
        break;
    case SDLK_0:
        translated = Key::Number0;
        break;
    case SDLK_ASTERISK:
    case SDLK_KP_MULTIPLY:
    case SDLK_M:
        translated = Key::Asterisk;
        break;
    case SDLK_Q:
        translated = Key::SoftLeft;
        break;
    case SDLK_E:
        translated = Key::SoftRight;
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE:
        translated = Key::Accept;
        break;
    default:
        return Key::None;
    }

    if (controlOrientation == 1) {
        switch (translated) {
        case Key::Up:
            return Key::Left;
        case Key::Right:
            return Key::Up;
        case Key::Down:
            return Key::Right;
        case Key::Left:
            return Key::Down;
        default:
            break;
        }
    } else if (controlOrientation == 2) {
        switch (translated) {
        case Key::Up:
            return Key::Right;
        case Key::Right:
            return Key::Down;
        case Key::Down:
            return Key::Left;
        case Key::Left:
            return Key::Up;
        default:
            break;
        }
    }
    return translated;
}

void ProcessSdlKeyEvent(Input& input, const SDL_Event& event, int controlOrientation)
{
    if (event.type != SDL_EVENT_KEY_DOWN && event.type != SDL_EVENT_KEY_UP) {
        return;
    }
    const Key key = event.key.key == SDLK_8 && (event.key.mod & SDL_KMOD_SHIFT) != 0
        ? Key::Asterisk : TranslateSdlKey(event.key.key, controlOrientation);
    if (event.type == SDL_EVENT_KEY_DOWN) {
        input.KeyDown(key, true);
    } else {
        input.KeyUp(key, true);
    }
}

} // namespace soft3d::platform

#include "platform/sdl/SdlInput.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

SDL_Event KeyEvent(SDL_EventType type, SDL_Keycode key, bool repeat = false)
{
    SDL_Event event{};
    event.type = type;
    event.key.key = key;
    event.key.repeat = repeat;
    return event;
}

} // namespace

int main()
{
    try {
        using soft3d::Key;
        using soft3d::platform::TranslateSdlKey;
        CHECK(TranslateSdlKey(SDLK_LEFT) == Key::Left);
        CHECK(TranslateSdlKey(SDLK_A) == Key::Left);
        CHECK(TranslateSdlKey(SDLK_RIGHT) == Key::Right);
        CHECK(TranslateSdlKey(SDLK_D) == Key::Right);
        CHECK(TranslateSdlKey(SDLK_UP) == Key::Up);
        CHECK(TranslateSdlKey(SDLK_W) == Key::Up);
        CHECK(TranslateSdlKey(SDLK_DOWN) == Key::Down);
        CHECK(TranslateSdlKey(SDLK_S) == Key::Down);
        CHECK(TranslateSdlKey(SDLK_0) == Key::Number0);
        CHECK(TranslateSdlKey(SDLK_9) == Key::Number9);
        CHECK(TranslateSdlKey(SDLK_Q) == Key::SoftLeft);
        CHECK(TranslateSdlKey(SDLK_E) == Key::SoftRight);
        CHECK(TranslateSdlKey(SDLK_RETURN) == Key::Accept);
        CHECK(TranslateSdlKey(SDLK_KP_ENTER) == Key::Accept);
        CHECK(TranslateSdlKey(SDLK_SPACE) == Key::Accept);
        CHECK(TranslateSdlKey(SDLK_ESCAPE) == Key::None);

        soft3d::Input input;
        SDL_Event event = KeyEvent(SDL_EVENT_KEY_DOWN, SDLK_LEFT);
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        event = KeyEvent(SDL_EVENT_KEY_DOWN, SDLK_1);
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        CHECK(input.IsKeyFirstDown(Key::Left));
        CHECK(input.IsKeyFirstDown(Key::Number1));
        for (int repeat = 0; repeat < 5; ++repeat) {
            event = KeyEvent(SDL_EVENT_KEY_DOWN, SDLK_LEFT, true);
            soft3d::platform::ProcessSdlKeyEvent(input, event);
        }
        CHECK(!input.IsKeyPress(Key::Left));
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        CHECK(input.IsKeyPress(Key::Left));
        event = KeyEvent(SDL_EVENT_KEY_UP, SDLK_LEFT);
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        CHECK(input.IsKeyClick(Key::Left));
        CHECK(input.IsKeyDown(Key::Number1));
        event = KeyEvent(SDL_EVENT_KEY_DOWN, SDLK_ESCAPE);
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        CHECK(!input.IsKeyDown(Key::Accept));
        event = KeyEvent(SDL_EVENT_WINDOW_FOCUS_LOST, SDLK_SPACE);
        soft3d::platform::ProcessSdlKeyEvent(input, event);
        CHECK(!input.IsKeyDown(Key::Accept));
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "SDL input tests passed\n";
    return 0;
}

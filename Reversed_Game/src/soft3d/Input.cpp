#include "soft3d/Input.hpp"

namespace soft3d {

Key TranslateSymbianKey(int scanCode)
{
    switch (scanCode) {
    case 0x2a:
        return Key::Asterisk;
    case 0x7f:
        return Key::Hash;
    case 0x30:
    case 0x4d:
        return Key::Number1;
    case 0x31:
    case 0x52:
        return Key::Number2;
    case 0x32:
    case 0x54:
        return Key::Number3;
    case 0x33:
    case 0x59:
        return Key::Number4;
    case 0x34:
    case 0x46:
        return Key::Number5;
    case 0x35:
    case 0x47:
        return Key::Number6;
    case 0x36:
    case 0x48:
        return Key::Number7;
    case 0x37:
    case 0x56:
        return Key::Number8;
    case 0x38:
    case 0x42:
        return Key::Number9;
    case 0x39:
    case 0x4e:
        return Key::Number0;
    case 0x0e:
        return Key::Left;
    case 0x0f:
        return Key::Right;
    case 0x10:
        return Key::Up;
    case 0x11:
        return Key::Down;
    case 0xa4:
        return Key::SoftLeft;
    case 0xa5:
        return Key::SoftRight;
    case 0xa7:
        return Key::Accept;
    default:
        return Key::None;
    }
}

Input::Input()
{
    ClearKey();
}

void Input::ClearKey()
{
    keyStates.fill(0);
    repeatCounts.fill(0);
}

void Input::Update()
{
    for (std::size_t key = 1; key < keySlotCount; ++key) {
        keyStates[key] &= 0xf5;
    }
}

bool Input::IsValid(Key key)
{
    const auto index = static_cast<uint8_t>(key);
    return index > 0 && index < keySlotCount;
}

bool Input::AcceptsEvent(Key key, bool hardwareEvent) const
{
    const auto index = static_cast<uint8_t>(key);
    return IsValid(key) &&
           (!hardwareEvent || !anyKeyDown || (index >= 0x11 && index <= 0x13));
}

void Input::KeyDown(Key key, bool hardwareEvent)
{
    if (!AcceptsEvent(key, hardwareEvent)) {
        return;
    }
    const auto index = static_cast<std::size_t>(key);
    uint8_t& state = keyStates[index];
    if ((state & keyDown) == 0) {
        state = keyDown | keyFirstDown;
        return;
    }
    if ((state & keyPress) == 0 && ++repeatCounts[index] > 5) {
        state |= keyPress;
        repeatCounts[index] = 0;
    }
}

void Input::KeyPress(Key key, bool hardwareEvent)
{
    if (AcceptsEvent(key, hardwareEvent)) {
        keyStates[static_cast<std::size_t>(key)] |= keyPress;
    }
}

void Input::KeyUp(Key key, bool hardwareEvent)
{
    if (!AcceptsEvent(key, hardwareEvent)) {
        return;
    }
    const auto index = static_cast<std::size_t>(key);
    keyStates[index] = (keyStates[index] & keyDown) != 0 ? keyClick : keyUp;
    repeatCounts[index] = 0;
}

bool Input::IsKeyFirstDown(Key key) const
{
    return (GetKeyState(key) & keyFirstDown) != 0;
}

bool Input::IsKeyDown(Key key) const
{
    return (GetKeyState(key) & keyDown) != 0;
}

bool Input::IsKeyPress(Key key) const
{
    return (GetKeyState(key) & keyPress) != 0;
}

bool Input::IsKeyUp(Key key) const
{
    return (GetKeyState(key) & keyDown) == 0;
}

bool Input::IsKeyClick(Key key) const
{
    return (GetKeyState(key) & keyClick) != 0;
}

bool Input::IsKeyDownOrClick(Key key) const
{
    return IsKeyDown(key) || IsKeyClick(key);
}

bool Input::IsAnyKeyClick() const
{
    return anyKeyDown;
}

uint8_t Input::GetKeyState(Key key) const
{
    return IsValid(key) ? keyStates[static_cast<std::size_t>(key)] : uint8_t{keyUp};
}

uint8_t Input::GetRepeatCount(Key key) const
{
    return IsValid(key) ? repeatCounts[static_cast<std::size_t>(key)] : 0;
}

} // namespace soft3d

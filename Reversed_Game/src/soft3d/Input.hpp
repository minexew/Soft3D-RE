#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace soft3d {

enum class Key : uint8_t {
    None = 0,
    Asterisk = 1,
    Hash = 2,
    Number1 = 3,
    Number2 = 4,
    Number3 = 5,
    Number4 = 6,
    Number5 = 7,
    Number6 = 8,
    Number7 = 9,
    Number8 = 10,
    Number9 = 11,
    Number0 = 12,
    Left = 13,
    Right = 14,
    Up = 15,
    Down = 16,
    SoftLeft = 17,
    SoftRight = 18,
    Accept = 19,
};

enum KeyState : uint8_t {
    keyUp = 0,
    keyDown = 1,
    keyFirstDown = 2,
    keyPress = 4,
    keyClick = 8,
};

Key TranslateSymbianKey(int scanCode);

class Input {
public:
    static constexpr std::size_t keySlotCount = 24;
    static constexpr std::size_t keyCount = keySlotCount - 1;

    Input();

    void ClearKey();
    void Update();
    void SetAnyKeyDown(uint8_t value);
    void KeyDown(Key key, bool hardwareEvent = false);
    void KeyPress(Key key, bool hardwareEvent = false);
    void KeyUp(Key key, bool hardwareEvent = false);

    bool IsKeyFirstDown(Key key) const;
    bool IsKeyDown(Key key) const;
    bool IsKeyPress(Key key) const;
    bool IsKeyUp(Key key) const;
    bool IsKeyClick(Key key) const;
    bool IsKeyDownOrClick(Key key) const;
    bool IsAnyKeyFirstDown() const;
    bool IsAnyKeyDown() const;
    bool IsAnyKeyPress() const;
    bool IsAnyKeyUp() const;
    bool IsAnyKeyClick() const;
    bool IsAnyKeyDownOrClick() const;
    uint8_t GetKeyState(Key key) const;
    uint8_t GetRepeatCount(Key key) const;

private:
    static bool IsValid(Key key);
    bool AcceptsEvent(Key key, bool hardwareEvent) const;

    uint8_t anyKeyDown = 0;
    std::array<uint8_t, keySlotCount> keyStates{};
    std::array<uint8_t, keySlotCount> repeatCounts{};
};

} // namespace soft3d

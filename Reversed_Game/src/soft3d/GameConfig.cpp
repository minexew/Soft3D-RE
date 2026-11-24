#include "soft3d/GameConfig.hpp"

#include <algorithm>
#include <cstring>

namespace soft3d {
namespace {

template<typename T>
void AppendWord(std::array<uint8_t, gameConfigSerializedSize>& output, size_t& offset,
                const T& value)
{
    static_assert(sizeof(T) == 4);
    std::memcpy(output.data() + offset, &value, sizeof(value));
    offset += sizeof(value);
}

uint32_t ReadWord(const uint8_t*& input)
{
    uint32_t value;
    std::memcpy(&value, input, sizeof(value));
    input += sizeof(value);
    return value;
}

bool ValidAction(int action)
{
    return action > 0 && action < 8;
}

} // namespace

std::array<uint8_t, gameConfigSerializedSize> SerializeGameConfiguration(
    const GameConfiguration& configuration)
{
    std::array<uint8_t, gameConfigSerializedSize> output{};
    size_t offset = 0;
    for (uint32_t binding : configuration.controlBindings) {
        AppendWord(output, offset, binding);
    }
    AppendWord(output, offset, configuration.masterVolume);
    AppendWord(output, offset, configuration.vibrationEnabled);
    for (uint32_t unlocked : configuration.unlocks.models) {
        AppendWord(output, offset, unlocked);
    }
    for (uint32_t unlocked : configuration.unlocks.documents) {
        AppendWord(output, offset, unlocked);
    }
    for (uint32_t unlocked : configuration.unlocks.photos) {
        AppendWord(output, offset, unlocked);
    }
    AppendWord(output, offset, configuration.unlocks.extraWeapon0);
    AppendWord(output, offset, configuration.unlocks.extraWeapon1);
    AppendWord(output, offset, configuration.unlocks.newGameOptions);
    return output;
}

result DeserializeGameConfiguration(const void* data, size_t size,
                                    GameConfiguration& configuration)
{
    if (data == nullptr || size < gameConfigSerializedSize) {
        return S3D_FORMAT;
    }
    const uint8_t* input = static_cast<const uint8_t*>(data);
    for (uint32_t& binding : configuration.controlBindings) {
        binding = ReadWord(input);
    }
    configuration.masterVolume = ReadWord(input);
    configuration.vibrationEnabled = ReadWord(input);
    for (uint32_t& unlocked : configuration.unlocks.models) {
        unlocked = ReadWord(input);
    }
    for (uint32_t& unlocked : configuration.unlocks.documents) {
        unlocked = ReadWord(input);
    }
    for (uint32_t& unlocked : configuration.unlocks.photos) {
        unlocked = ReadWord(input);
    }
    configuration.unlocks.extraWeapon0 = ReadWord(input);
    configuration.unlocks.extraWeapon1 = ReadWord(input);
    configuration.unlocks.newGameOptions = ReadWord(input);
    return S3D_OK;
}

ControlMap::ControlMap(GameConfiguration& value)
    : configuration(value)
{
    actionEnabled.fill(true);
    SetOrientation(1);
}

void ControlMap::SetOrientation(int value)
{
    orientation = value;
    if (value == 1) {
        primaryBindings = {0, 13, 14, 16, 15, 0, 0, 0};
        configuration.controlBindings = {0, 7, 9, 11, 5, 0, 1, 3};
    } else if (value == 2) {
        primaryBindings = {0, 14, 13, 15, 16, 0, 0, 0};
        configuration.controlBindings = {0, 9, 7, 5, 11, 0, 1, 3};
    } else {
        primaryBindings = {0, 15, 16, 13, 14, 0, 0, 0};
        configuration.controlBindings = {0, 5, 11, 7, 9, 0, 1, 3};
    }
}

int ControlMap::GetOrientation() const
{
    return orientation;
}

uint32_t ControlMap::GetBinding(int action) const
{
    return ValidAction(action) ? configuration.controlBindings[action] : 0;
}

uint32_t ControlMap::GetPrimaryBinding(int action) const
{
    return ValidAction(action) ? primaryBindings[action] : 0;
}

void ControlMap::SetBinding(int action, uint32_t key)
{
    if (ValidAction(action)) {
        configuration.controlBindings[action] = key;
    }
}

void ControlMap::SetEnabled(bool value, Input* input)
{
    if (enabled != value && input != nullptr) {
        input->ClearKey();
    }
    enabled = value;
}

void ControlMap::SetActionEnabled(int action, bool value)
{
    if (ValidAction(action)) {
        actionEnabled[action] = value;
    }
}

bool ControlMap::IsEnabled() const
{
    return enabled;
}

bool ControlMap::ActionFirstDown(const Input& input, int action, bool primaryOnly) const
{
    if (!enabled || !ValidAction(action) || !actionEnabled[action]) {
        return false;
    }
    const Key primary = static_cast<Key>(primaryBindings[action]);
    if (primary != Key::None && input.IsKeyFirstDown(primary)) {
        return true;
    }
    const Key secondary = static_cast<Key>(configuration.controlBindings[action]);
    return !primaryOnly && secondary != Key::None && input.IsKeyFirstDown(secondary);
}

bool ControlMap::ActionDown(const Input& input, int action, bool primaryOnly) const
{
    if (!enabled || !ValidAction(action) || !actionEnabled[action]) {
        return false;
    }
    const Key primary = static_cast<Key>(primaryBindings[action]);
    if (primary != Key::None && input.IsKeyDown(primary)) {
        return true;
    }
    const Key secondary = static_cast<Key>(configuration.controlBindings[action]);
    return !primaryOnly && secondary != Key::None && input.IsKeyDown(secondary);
}

bool ControlMap::ActionPress(const Input& input, int action, bool primaryOnly) const
{
    if (!enabled || !ValidAction(action) || !actionEnabled[action]) {
        return false;
    }
    const Key primary = static_cast<Key>(primaryBindings[action]);
    if (primary != Key::None && input.IsKeyPress(primary)) {
        return true;
    }
    const Key secondary = static_cast<Key>(configuration.controlBindings[action]);
    return !primaryOnly && secondary != Key::None && input.IsKeyPress(secondary);
}

bool ControlMap::ActionUp(const Input& input, int action, bool primaryOnly) const
{
    if (!enabled || !ValidAction(action) || !actionEnabled[action]) {
        return false;
    }
    const Key primary = static_cast<Key>(primaryBindings[action]);
    if (primary != Key::None && !input.IsKeyUp(primary)) {
        return false;
    }
    const Key secondary = static_cast<Key>(configuration.controlBindings[action]);
    return primaryOnly || secondary == Key::None || input.IsKeyUp(secondary);
}

bool ControlMap::ActionClick(const Input& input, int action, bool primaryOnly) const
{
    if (!enabled || !ValidAction(action) || !actionEnabled[action]) {
        return false;
    }
    const Key primary = static_cast<Key>(primaryBindings[action]);
    if (primary != Key::None && input.IsKeyClick(primary)) {
        return true;
    }
    const Key secondary = static_cast<Key>(configuration.controlBindings[action]);
    return !primaryOnly && secondary != Key::None && input.IsKeyClick(secondary);
}

} // namespace soft3d

#pragma once

#include "soft3d/Input.hpp"
#include "soft3d/s3dCommon.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace soft3d {

constexpr size_t gameUnlockCount = 100;
constexpr size_t gameConfigSerializedSize = 1252;

struct GameUnlocks {
    std::array<uint32_t, gameUnlockCount> models{};
    std::array<uint32_t, gameUnlockCount> documents{};
    std::array<uint32_t, gameUnlockCount> photos{};
    uint32_t extraWeapon0 = 0;
    uint32_t extraWeapon1 = 0;
    uint32_t newGameOptions = 0;
};

struct GameConfiguration {
    std::array<uint32_t, 8> controlBindings{};
    uint32_t masterVolume = 255;
    uint32_t vibrationEnabled = 0;
    GameUnlocks unlocks;
};

std::array<uint8_t, gameConfigSerializedSize> SerializeGameConfiguration(
    const GameConfiguration& configuration);
result DeserializeGameConfiguration(const void* data, size_t size,
                                    GameConfiguration& configuration);

class ControlMap {
public:
    explicit ControlMap(GameConfiguration& configuration);

    void SetOrientation(int value);
    int GetOrientation() const;
    uint32_t GetBinding(int action) const;
    void SetBinding(int action, uint32_t key);
    bool ActionClick(const Input& input, int action, bool primaryOnly = true) const;
    bool ActionDown(const Input& input, int action, bool primaryOnly = true) const;

private:
    GameConfiguration& configuration;
    std::array<uint32_t, 8> primaryBindings{};
    int orientation = 1;
};

} // namespace soft3d

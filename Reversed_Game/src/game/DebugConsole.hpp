#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/Font.hpp"

#include <array>
#include <cstdint>

namespace game {

class Scene;

class DebugConsole {
public:
    explicit DebugConsole(Scene& scene);

    int32_t Update();
    void Render();
    bool LoadMapSpawn(int32_t mapId);

    bool IsActive() const { return active; }
    int32_t GetSelection() const { return selection; }
    int32_t GetEnteredCount() const { return enteredCount; }
    int32_t GetFrameCounter() const { return frameCounter; }
    const std::array<char16_t, 8>& GetEnteredCharacters() const { return entered; }
    const soft3d::Vec3& GetSpawnPosition() const { return spawnPosition; }
    const soft3d::Vec3& GetSpawnDirection() const { return spawnDirection; }

private:
    void Reset();
    bool IsDigitSequence(int32_t start) const;
    int32_t ParseUnsigned(int32_t start) const;

    Scene& scene;
    bool active = false;
    int32_t selection = 0;
    std::array<char16_t, 11> choices{};
    int32_t enteredCount = 0;
    int32_t textX = 32;
    int32_t textY = 0;
    int32_t frameCounter = 0;
    soft3d::VisString display;
    std::array<char16_t, 8> entered{};
    soft3d::Vec3 spawnPosition;
    soft3d::Vec3 spawnDirection;
};

} // namespace game

#pragma once

#include "game/GameData.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/Units.hpp"

namespace soft3d {
class GameEngine;
}

namespace game {

class CameraController;

class MyGameUnitMgr : public soft3d::GameUnitManager {
public:
    explicit MyGameUnitMgr(const soft3d::UnitManagerContext& context = {});

    soft3d::result Initialize(soft3d::GameEngine& engine);
    soft3d::result LoadDayContent(int day, soft3d::GameEngine& engine);
    void Reset();
    void FinishMapLoad(bool restoreSavedState);
    void SetMapId(int32_t value) { mapId = value; }
    void SetPaused(bool value) { paused = value; }
    bool IsPaused() const { return paused; }
    bool IsMapReady() const { return mapReady; }
    int32_t GetDay() const { return day; }
    int32_t GetMapId() const { return mapId; }

    void ConfigureScene(CameraController* cameraController, soft3d::Creature* player);
    void HandleMapEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp);
    uint32_t NextRandom();
    uint32_t* GetRandomState() { return &randomState; }
    void SetRandomSeed(uint32_t seed) { randomState = seed; }
    const std::array<soft3d::Fixed, 6>& GetMapHeaderValues() const { return mapHeaderValues; }

    GameData& GetData();
    const GameData& GetData() const;

private:
    GameData data;
    CameraController* cameraController = nullptr;
    soft3d::Creature* player = nullptr;
    std::array<soft3d::Fixed, 6> mapHeaderValues{};
    uint32_t randomState = 0;
    int32_t day = 0;
    int32_t mapId = 0;
    bool mapReady = false;
    bool paused = false;
};

} // namespace game

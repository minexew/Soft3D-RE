#pragma once

#include "game/GameData.hpp"
#include "soft3d/Units.hpp"

namespace soft3d {
class GameEngine;
}

namespace game {

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

    GameData& GetData();
    const GameData& GetData() const;

private:
    GameData data;
    int32_t day = 0;
    int32_t mapId = 0;
    bool mapReady = false;
    bool paused = false;
};

} // namespace game

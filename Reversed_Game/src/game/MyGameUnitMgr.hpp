#pragma once

#include "game/GameData.hpp"
#include "soft3d/Units.hpp"

namespace soft3d {
class GameEngine;
}

namespace game {

class MyGameUnitMgr : public soft3d::GameUnitManager {
public:
    soft3d::result Initialize(soft3d::GameEngine& engine);
    soft3d::result LoadDayContent(int day, soft3d::GameEngine& engine);

    GameData& GetData();
    const GameData& GetData() const;

private:
    GameData data;
};

} // namespace game

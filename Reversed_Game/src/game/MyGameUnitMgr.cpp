#include "game/MyGameUnitMgr.hpp"

#include "soft3d/GameEngine.hpp"

namespace game {

soft3d::result MyGameUnitMgr::Initialize(soft3d::GameEngine& engine)
{
    engine.GetAudioManager().SetSoundNameTable(data.GetSoundNameTable());
    return data.LoadItemData(*engine.GetResourceManager().GetFilePak(), engine.GetContent());
}

soft3d::result MyGameUnitMgr::LoadDayContent(int day, soft3d::GameEngine& engine)
{
    return data.LoadDayContent(day, *engine.GetResourceManager().GetFilePak());
}

GameData& MyGameUnitMgr::GetData()
{
    return data;
}

const GameData& MyGameUnitMgr::GetData() const
{
    return data;
}

} // namespace game

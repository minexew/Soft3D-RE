#include "game/MyGameUnitMgr.hpp"

#include "soft3d/GameEngine.hpp"

namespace game {

MyGameUnitMgr::MyGameUnitMgr(const soft3d::UnitManagerContext& context)
    : GameUnitManager(context)
{
}

soft3d::result MyGameUnitMgr::Initialize(soft3d::GameEngine& engine)
{
    engine.GetAudioManager().SetSoundNameTable(data.GetSoundNameTable());
    return data.LoadItemData(*engine.GetResourceManager().GetFilePak(), engine.GetContent());
}

soft3d::result MyGameUnitMgr::LoadDayContent(int value, soft3d::GameEngine& engine)
{
    const soft3d::result status = data.LoadDayContent(value, *engine.GetResourceManager().GetFilePak());
    if (status == soft3d::S3D_OK) {
        day = value;
    }
    return status;
}

void MyGameUnitMgr::Reset()
{
    ResetBuckets();
    mapReady = false;
    paused = false;
}

void MyGameUnitMgr::FinishMapLoad(bool restoreSavedState)
{
    (void)restoreSavedState;
    mapReady = true;
    paused = false;
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

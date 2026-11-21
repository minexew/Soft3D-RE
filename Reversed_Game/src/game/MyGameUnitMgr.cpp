#include "game/MyGameUnitMgr.hpp"

#include "game/CameraController.hpp"
#include "game/Triggers.hpp"
#include "soft3d/GameEngine.hpp"

#include <stdexcept>

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
    for (int bucket : {1, 2, 4, 5, 6, 7}) {
        ClearBucket(bucket, true);
    }
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

void MyGameUnitMgr::ConfigureScene(CameraController* newCameraController,
                                   soft3d::Creature* newPlayer)
{
    cameraController = newCameraController;
    player = newPlayer;
}

void MyGameUnitMgr::HandleMapEntity(const soft3d::BspEntityRecord& record,
                                    const soft3d::Bsp& bsp)
{
    if (record.type == 10) {
        auto trigger = std::make_unique<CameraTrigger>(cameraController, player);
        const soft3d::result status = trigger->LoadEntity(record, bsp, *GetContext().resources);
        if (status != soft3d::S3D_OK) {
            throw std::runtime_error("CameraTrigger_LoadEntity failed with Soft3D result " +
                                     std::to_string(status));
        }
        AddUnit(trigger.release(), 6);
    } else if (record.type == 11) {
        auto trigger = std::make_unique<LightTrigger>(player);
        const soft3d::result status = trigger->LoadEntity(record, bsp);
        if (status != soft3d::S3D_OK) {
            throw std::runtime_error("LightTrigger_LoadEntity failed with Soft3D result " +
                                     std::to_string(status));
        }
        AddUnit(trigger.release(), 7);
    } else if (record.type == 102) {
        if (record.brushIndex < 0 ||
            static_cast<std::size_t>(record.brushIndex) >= bsp.GetEntityBrushes().size()) {
            throw std::runtime_error("Map header entity has no brush record");
        }
        const auto& values = bsp.GetEntityBrushes()[static_cast<std::size_t>(record.brushIndex)].values;
        for (std::size_t index = 0; index < mapHeaderValues.size(); ++index) {
            mapHeaderValues[index] = soft3d::ShiftLeftFixed(values[index], 16);
        }
    }
}

uint32_t MyGameUnitMgr::NextRandom()
{
    randomState = randomState * 0x343fdU + 0x269ec3U;
    return (randomState >> 16) & 0x7fffU;
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

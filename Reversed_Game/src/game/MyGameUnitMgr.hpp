#pragma once

#include "game/GameData.hpp"
#include "game/MapUnits.hpp"
#include "game/Triggers.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/Effects.hpp"
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
    int32_t GetHealthPackCount() const { return healthPackCount; }
    void SetHealthPackCount(int32_t value) { healthPackCount = value; }
    soft3d::GameUnit* GetPushTarget() const { return pushTarget; }
    void SetPushTarget(soft3d::GameUnit* value) { pushTarget = value; }

    void ConfigureScene(CameraController* cameraController, soft3d::Creature* player);
    void HandleMapEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp);
    soft3d::Things* AddThing(int32_t unitId, int8_t thingType, int8_t variant, bool collisionEnabled,
                     const char16_t* modelName, const soft3d::Vec3& position,
                     const soft3d::Vec3& axisX, const soft3d::Vec3& axisY,
                     const soft3d::Vec3& axisZ);
    Trap* AddTrap(int32_t unitId, const soft3d::ObjectBounds& bounds,
                  const soft3d::Vec3& impulse);
    EventTrigger* AddEventTrigger(int32_t unitId, int16_t count, int32_t textId,
                                  const soft3d::ObjectBounds& bounds);
    PickTrigger* AddPickTrigger(int32_t unitId, int32_t thingId, int32_t textId,
                                const soft3d::ObjectBounds& bounds);
    DoorTrigger* AddDoorTrigger(int32_t unitId, int32_t requiredItem, int32_t lockedText,
                                int32_t doorResourceIndex, int32_t nextMap, int32_t nextEpisode,
                                const soft3d::Vec3& spawnPosition,
                                const soft3d::Vec3& spawnDirection,
                                const soft3d::ObjectBounds& bounds);
    ActTrigger* AddActTrigger(int32_t unitId, const soft3d::ObjectBounds& bounds);
    HideTrigger* AddHideTrigger(int32_t unitId, int32_t maskIndex,
                                const soft3d::Vec3& hidePosition,
                                const soft3d::Vec3& cameraDirection,
                                const soft3d::ObjectBounds& bounds);
    ItemTrigger* AddItemTrigger(int32_t unitId, int32_t itemId,
                                const soft3d::ObjectBounds& bounds);
    TalkTrigger* AddTalkTrigger(int32_t unitId, const soft3d::ObjectBounds& bounds,
                                bool fadeAtEnd);
    StayBox* RestoreStayBox();
    StayBox* FindStayBox(int32_t unitId);
    void SetStayBoxHost(StayBoxHost* value) { stayBoxHost = value; }
    void AttachStayBoxEnemy(soft3d::GameUnit& enemy, StayBox& box);
    void SpawnMapItems();
    InteractionState& GetInteractionState() { return interactionState; }
    const InteractionState& GetInteractionState() const { return interactionState; }
    void SetInteractionUi(TriggerUiHost* value) { interactionUi = value; }
    int32_t GetDoorState(int32_t doorId) const;
    void SetDoorState(int32_t doorId, int32_t value);
    const std::array<int32_t, 1024>& GetDoorStates() const { return doorStates; }
    void Update();
    void SubmitRenderables(soft3d::RenderQueues& queues);
    std::size_t GetLensflareCount() const { return lensflareCount; }
    soft3d::Lensflare* GetLensflare(std::size_t index) const;
    uint32_t NextRandom();
    uint32_t* GetRandomState() { return &randomState; }
    void SetRandomSeed(uint32_t seed) { randomState = seed; }
    const std::array<soft3d::Fixed, 6>& GetMapHeaderValues() const { return mapHeaderValues; }

    GameData& GetData();
    const GameData& GetData() const;

private:
    GameData data;
    InteractionState interactionState;
    TriggerUiHost* interactionUi = &interactionState;
    CameraController* cameraController = nullptr;
    soft3d::Creature* player = nullptr;
    StayBoxHost* stayBoxHost = nullptr;
    std::array<soft3d::Fixed, 6> mapHeaderValues{};
    std::array<int32_t, 1024> doorStates{};
    uint32_t randomState = 0;
    int32_t day = 0;
    int32_t mapId = 0;
    int32_t healthPackCount = 0;
    soft3d::GameUnit* pushTarget = nullptr;
    std::array<std::unique_ptr<soft3d::Lensflare>, 16> lensflares{};
    std::array<StayBox, 100> stayBoxes{};
    std::size_t lensflareCount = 0;
    int32_t stayBoxCount = 0;
    bool mapReady = false;
    bool paused = false;
};

} // namespace game

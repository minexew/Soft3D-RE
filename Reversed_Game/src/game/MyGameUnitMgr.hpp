#pragma once

#include "game/Enemy.hpp"
#include "game/GameData.hpp"
#include "game/MapUnits.hpp"
#include "game/Triggers.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/Effects.hpp"
#include "soft3d/Units.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace soft3d {
class GameEngine;
}

namespace game {

class CameraController;

class MyGameUnitMgr : public soft3d::GameUnitManager, public StayBoxHost {
public:
    explicit MyGameUnitMgr(const soft3d::UnitManagerContext& context = {});

    soft3d::result Initialize(soft3d::GameEngine& engine);
    soft3d::result LoadDayContent(int day, soft3d::GameEngine& engine);
    void Reset();
    void FinishMapLoad(bool restoreSavedState);
    void SetMapId(int32_t value);
    void SetPaused(bool value) { paused = value; }
    bool IsPaused() const { return paused; }
    bool IsMapReady() const { return mapReady; }
    int32_t GetDay() const { return day; }
    int32_t GetMapId() const { return mapId; }
    int32_t GetHealthPackCount() const { return healthPackCount; }
    void SetHealthPackCount(int32_t value) { healthPackCount = value; }
    void RestoreDayAndMap(int32_t savedDay, int32_t savedMap) { day = savedDay; mapId = savedMap; }
    soft3d::result SaveState(soft3d::OutputStream& stream) const;
    soft3d::result LoadUnitState(soft3d::InputStream& stream);
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

    bool AddEnemy(int32_t unitId, int32_t enemyType, int32_t spawnState,
                  soft3d::Vec3& position, const soft3d::Vec3& direction,
                  bool fromEnemyInfo = false);
    Enemy* FindEnemy(int32_t unitId) const;
    Enemy* GetPooledEnemy(int32_t enemyType, std::size_t slot) const;
    void RemoveEnemy(Enemy* enemy);
    void RemoveEnemiesByType(int32_t enemyType);
    void RemoveAllEnemies();
    void PrepareDayEnemies(int32_t day);
    void SpawnMapEnemies();
    void UpdateEnemySpawnRecords();
    EnemySpawnRecord* CreateEnemySpawnRecord(Enemy* enemy);

    StayBox* AddStayBox(int32_t unitId, const soft3d::ObjectBounds& bounds);
    StayBox* RestoreStayBox();
    StayBox* FindStayBox(int32_t unitId);
    void SetStayBoxHost(StayBoxHost* value) { stayBoxHost = value; }
    void AttachStayBoxEnemy(soft3d::GameUnit& enemy, StayBox& box);
    soft3d::GameUnit* AddCollisionUnit(int32_t unitId, const soft3d::ObjectBounds& bounds);

    int SpawnStayBoxEnemy(uint8_t unitId, uint8_t type, uint8_t active,
                          const soft3d::Vec3& position, const soft3d::Vec3& direction) override;
    soft3d::GameUnit* FindStayBoxEnemy(uint8_t unitId) override;
    const soft3d::Vec3& GetStayBoxEnemySpawnPosition(
        const soft3d::GameUnit* enemy) const override;
    int GetStayBoxEnemyState(const soft3d::GameUnit* enemy) const override;
    uint8_t GetStayBoxEnemyType(const soft3d::GameUnit* enemy) const override;
    uint8_t GetStayBoxEnemyActive(const soft3d::GameUnit* enemy) const override;
    uint8_t GetStayBoxEnemySpawnState(const soft3d::GameUnit* enemy) const override;
    void SetStayBoxEnemySpawnState(soft3d::GameUnit* enemy, uint8_t value) override;
    void BindStayBoxEnemy(soft3d::GameUnit* enemy, StayBox* stayBox,
                          const soft3d::Vec3& returnPosition) override;
    void AttachStayBoxEnemy(soft3d::GameUnit* enemy, StayBox* stayBox) override;
    void RemoveStayBoxEnemy(soft3d::GameUnit* enemy) override;

    void SpawnMapItems();
    InteractionState& GetInteractionState() { return interactionState; }
    const InteractionState& GetInteractionState() const { return interactionState; }
    void SetInteractionUi(TriggerUiHost* value) { interactionUi = value; }
    int32_t GetDoorState(int32_t doorId) const;
    void SetDoorState(int32_t doorId, int32_t value);
    const std::array<int32_t, 1024>& GetDoorStates() const { return doorStates; }
    void StartScriptTimer(int32_t index);
    int32_t GetScriptTimerElapsed(int32_t index) const;
    void StopScriptTimer(int32_t index);
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
    static constexpr std::size_t enemyTypeCount = 17;
    static constexpr std::size_t enemyPoolStride = 8;

    GameData data;
    InteractionState interactionState;
    TriggerUiHost* interactionUi = &interactionState;
    CameraController* cameraController = nullptr;
    soft3d::Creature* player = nullptr;
    StayBoxHost* stayBoxHost = this;
    std::array<soft3d::Fixed, 6> mapHeaderValues{};
    std::array<int32_t, 1024> doorStates{};
    std::array<std::array<int32_t, 2>, 32> scriptTimers{};
    std::array<std::array<std::unique_ptr<Enemy>, enemyPoolStride>, enemyTypeCount> enemyPools{};
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

#pragma once

#include "game/Player.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/Units.hpp"

#include <cstdint>
#include <vector>

namespace game {

class Breakable final : public soft3d::Things {
public:
    Breakable(soft3d::s3dResManager* resources = nullptr, int cacheLevel = 3,
              soft3d::Renderer* renderer = nullptr, soft3d::AudioManager* audio = nullptr,
              soft3d::Bsp* bsp = nullptr);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void Configure(Player* player, int32_t requiredWeapon);
    void SetBreakSound(const char16_t* name);
    bool ConsumeBreakEvent();

private:
    soft3d::AudioData* breakSound = nullptr;
    Player* player = nullptr;
    int32_t requiredWeapon = 0;
    int16_t breakEvent = 0;
    int16_t hitLatched = 0;
};

class Pushable final : public soft3d::Things, public PushableTarget {
public:
    Pushable(soft3d::s3dResManager* resources = nullptr, int cacheLevel = 3,
             soft3d::Renderer* renderer = nullptr, soft3d::AudioManager* audio = nullptr,
             soft3d::Bsp* bsp = nullptr);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    bool CanBePushed() const override { return pushEnabled; }
    soft3d::AudioData* GetPushSound() const override { return pushSound; }
    void SetPusher(Player* player) override { pusher = player; }
    void SetPushEnabled(bool enabled) { pushEnabled = enabled; }
    void SetPushSound(const char16_t* name);
    Player* GetPusher() const { return pusher; }

private:
    bool pushEnabled = true;
    soft3d::AudioData* pushSound = nullptr;
    Player* pusher = nullptr;
};

class Pickable final : public soft3d::Things {
public:
    Pickable(soft3d::s3dResManager* resources = nullptr, int cacheLevel = 3,
             soft3d::Renderer* renderer = nullptr, soft3d::AudioManager* audio = nullptr,
             soft3d::Bsp* bsp = nullptr, soft3d::GameUnitManager* manager = nullptr);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void SetItemId(int32_t value) { itemId = value; }
    int32_t GetItemId() const { return itemId; }
    void SetSpawnRecord(int32_t* value) { spawnItemId = value; }
    void SetPickupSound(const char16_t* name);
    void PickUp(Player* player);

private:
    soft3d::AudioData* pickupSound = nullptr;
    int32_t itemId = -1;
    int32_t* spawnItemId = nullptr;
};

class BspUnit final : public soft3d::BspEntity, public soft3d::GameUnit {
public:
    BspUnit();

    void LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp);
    void Submit(soft3d::RenderQueues& queues) override;
    void Update() override {}
    void PostUpdate() override {}

    const soft3d::Animation* GetMapAnimation() const { return mapAnimation; }
    const soft3d::SojVertices* GetSecondaryVertices() const { return secondaryVertices; }

private:
    const soft3d::Animation* mapAnimation = nullptr;
    const soft3d::SojVertices* secondaryVertices = nullptr;
};

class ElectricSaw final : public soft3d::VisUnit {
public:
    ElectricSaw(soft3d::s3dResManager* resources = nullptr, int cacheLevel = 3,
                soft3d::Renderer* renderer = nullptr, soft3d::AudioManager* audio = nullptr);

    void Update() override;
};

class Trap final : public soft3d::GameUnit {
public:
    explicit Trap(Player* player = nullptr);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void SetPlayer(Player* value) { player = value; }
    void SetImpulse(const soft3d::Vec3& value) { impulse = value; }
    const soft3d::Vec3& GetImpulse() const { return impulse; }

private:
    Player* player;
    soft3d::Vec3 impulse;
};

class StayBox;

class StayBoxHost {
public:
    virtual ~StayBoxHost() = default;
    virtual int SpawnStayBoxEnemy(uint8_t unitId, uint8_t type, uint8_t active,
                                  const soft3d::Vec3& position, const soft3d::Vec3& direction) = 0;
    virtual soft3d::GameUnit* FindStayBoxEnemy(uint8_t unitId) = 0;
    virtual const soft3d::Vec3& GetStayBoxEnemySpawnPosition(
        const soft3d::GameUnit* enemy) const = 0;
    virtual int GetStayBoxEnemyState(const soft3d::GameUnit* enemy) const = 0;
    virtual uint8_t GetStayBoxEnemyType(const soft3d::GameUnit* enemy) const = 0;
    virtual uint8_t GetStayBoxEnemyActive(const soft3d::GameUnit* enemy) const = 0;
    virtual uint8_t GetStayBoxEnemySpawnState(const soft3d::GameUnit* enemy) const = 0;
    virtual void SetStayBoxEnemySpawnState(soft3d::GameUnit* enemy, uint8_t value) = 0;
    virtual void BindStayBoxEnemy(soft3d::GameUnit* enemy, StayBox* stayBox,
                                  const soft3d::Vec3& returnPosition) = 0;
    virtual void RemoveStayBoxEnemy(soft3d::GameUnit* enemy) = 0;
};

class StayBox final : public soft3d::GameUnit {
public:
    struct SpawnRecord {
        soft3d::Vec3 position;
        uint8_t unitId = 0;
        uint8_t type = 0;
        uint8_t active = 0;
        uint8_t spawnState = 0;
    };

    StayBox();

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void Configure(soft3d::GameUnit* enemy, StayBoxHost* host);
    void AddAllowedRegion(const soft3d::ObjectBounds& bounds);
    void AddDeniedRegion(const soft3d::ObjectBounds& bounds);
    void AddSpawnRecord(const soft3d::GameUnit& enemy);
    void AddSpawnRecord(const SpawnRecord& record);

    bool IsEnemyOutside() const { return enemyOutside != 0; }
    bool HasSpawnedEnemies() const { return spawned != 0; }
    const std::vector<SpawnRecord>& GetSpawnRecords() const { return spawnRecords; }
    std::vector<SpawnRecord>& GetSpawnRecords() { return spawnRecords; }

private:
    bool OverlapsAny(const std::vector<soft3d::ObjectBounds>& regions) const;
    void SpawnEnemies();
    void DespawnEnemies();

    soft3d::GameUnit* enemy = nullptr;
    StayBoxHost* host = nullptr;
    int32_t enemyOutside = 1;
    int32_t spawned = 0;
    std::vector<soft3d::ObjectBounds> allowedRegions;
    std::vector<soft3d::ObjectBounds> deniedRegions;
    std::vector<SpawnRecord> spawnRecords;
};

} // namespace game

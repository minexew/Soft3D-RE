#pragma once

#include "game/GameData.hpp"
#include "game/MapUnits.hpp"
#include "game/Player.hpp"
#include "soft3d/Units.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace game {

class MyGameUnitMgr;
class Kate;

enum EnemyState : int32_t {
    EnemyIdle = 0,
    EnemyAlert = 1,
    EnemyChase = 2,
    EnemyAttack = 3,
    EnemyHurt = 4,
    EnemyKnockback = 5,
    EnemyDying = 15,
    EnemyDead = 16,
    EnemyRemoved = 17,
};

enum EnemyMoveMode : int32_t {
    EnemyNoMovement = 0,
    EnemyMoveToTarget = 1,
    EnemyFollowPlayer = 2,
};

struct EnemyCommonConfig {
    int32_t unitType = 0;
    int32_t maxHealth = 0;
    int32_t attackPower = 0;
    soft3d::Fixed hitDistance = 0;
    soft3d::Fixed attackRange = 0;
    int16_t attackInterval = 0;
    int16_t attackStart = 0;
    int16_t attackEnd = 0;
    int16_t alertRecoveryDelay = 0;
    int16_t idleAlertDelay = 0;
    soft3d::Fixed alertSpeed = 0;
    soft3d::Fixed chaseSpeed = 0;
    int32_t knockbackChance = 0;
    soft3d::Fixed knockbackSpeed = 0;
    soft3d::Fixed perceptionRange = 0;
    int32_t gunExperienceReward = 0;
    bool positionalAudio = true;
};

const EnemyCommonConfig& GetEnemyCommonConfig(int32_t enemyType);
bool IsSingletonEnemyType(int32_t enemyType);

class Enemy : public soft3d::Creature, public CombatTarget {
public:
    Enemy(soft3d::s3dResManager* resources = nullptr, soft3d::Renderer* renderer = nullptr,
          soft3d::AudioManager* audio = nullptr, soft3d::Bsp* bsp = nullptr);
    ~Enemy() override = default;

    soft3d::GameUnit* QueryPoint(const soft3d::Vec3& point) override;
    soft3d::GameUnit* QuerySegment(const soft3d::Vec3& start,
                                   const soft3d::Vec3& end) override;
    soft3d::GameUnit* QueryBounds(const soft3d::ObjectBounds& bounds) override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;
    void QueryUnitCollision() override;
    void SetNavigationEnabled(bool enabled) override;
    void SetAnimation(const soft3d::Animation* resource, int32_t direction = 1,
                      bool loop = true) override;

    virtual void LoadResources(const char16_t* modelName,
                               const soft3d::Animation* const* animationNames,
                               soft3d::AudioData* const* soundNames);
    virtual void Reset();
    virtual bool UpdatePerception();
    virtual void Damage(int32_t amount);
    virtual void Die();
    virtual void InitializeAnimations(const soft3d::Animation* const* values);
    virtual void InitializeSounds(soft3d::AudioData* const* values);
    virtual void StopSound();
    virtual void Spawn(const soft3d::Vec3& value);
    virtual void UpdateIdle();
    virtual void UpdateChase();
    virtual void UpdateAlertDelay();
    virtual void UpdateAlert();
    virtual int32_t SelectAttack();
    virtual void BeginAttack();
    virtual bool TestAttackHit();
    virtual void UpdateAttack();
    virtual bool ReceivePlayerAttack();
    virtual void UpdateHurt();
    virtual void UpdateKnockback();
    virtual void UpdateDeath();
    virtual void SetAnimationByIndex(int32_t index, int32_t direction = 1, bool loop = true);

    bool IsAttackable() const override;
    bool IsAttacking() const override;
    int32_t GetAttackPower() const override;
    soft3d::Fixed GetPlayerDistance() const override;

    void ConfigureCommonDefaults(const EnemyCommonConfig& config);
    void SnapshotMovementConfig();
    void SetManager(MyGameUnitMgr* value);
    void SetPlayer(Player* value) { player = value; }
    void SetEnemyType(int32_t value) { enemyType = value; }
    void SetHealth(int32_t value);
    void SetAttackPower(int32_t value) { attackPower = value; }
    void SetMaxHealth(int32_t value) { maxHealth = value; }
    void SetStopped(bool value);
    void SetSpawnState(int32_t value) { stopped = value; }
    void FollowPlayer() { moveMode = EnemyFollowPlayer; }
    bool MoveTo(const soft3d::Vec3& target);
    bool RequestAttack();
    void AttachStayBox(StayBox* value);
    void SetStayBoxReturnPosition(const soft3d::Vec3& value) { stayBoxReturnPosition = value; }
    void SetPerceptionRange(soft3d::Fixed value) { perceptionRange = value; }
    void SetAlertSpeed(soft3d::Fixed value) { alertSpeed = value; }
    void SetWaypointLoop(bool value) { waypointLoop = value ? 1 : 0; }
    void AddWaypoint(const soft3d::Vec3& position, int32_t waitTime,
                     const soft3d::Vec3& direction);
    void ClearWaypoints();
    void SetAnimationResource(int32_t index, const char16_t* name);
    void SetSoundResource(int32_t index, const char16_t* name);
    void EnsureSpawnRecord();
    void SetSpawnRecord(EnemySpawnRecord* value) { spawnRecord = value; }
    void CheckGroundForSpawn() { CheckGround(); }

    int32_t GetEnemyType() const { return enemyType; }
    int32_t GetState() const { return state; }
    int32_t GetHealth() const { return health; }
    int32_t GetMaxHealth() const { return maxHealth; }
    int32_t GetMoveMode() const { return moveMode; }
    int32_t GetSpawnState() const { return stopped; }
    bool IsRemoved() const { return state == EnemyDead || state == EnemyRemoved; }
    bool IsPlayerVisible() const { return playerVisible != 0; }
    bool IsStopped() const { return stopped != 0; }
    bool HasCurrentWaypoint() const { return currentWaypoint >= 0; }
    std::size_t GetWaypointCount() const { return waypoints.size(); }
    int32_t GetCurrentWaypointIndex() const { return currentWaypoint; }
    int16_t GetWaypointDirection() const { return waypointDirection; }
    int16_t GetWaypointLoop() const { return waypointLoop; }
    int16_t GetWaitTimer() const { return waitTimer; }
    int16_t GetAttackCooldown() const { return attackCooldown; }
    int16_t GetAttackInterval() const { return attackInterval; }
    int16_t GetHitCooldown() const { return hitCooldown; }
    int16_t GetDeathCountdown() const { return deathCountdown; }
    int16_t GetStateTime() const { return stateTime; }
    int16_t GetAttackStart() const { return attackStart; }
    int16_t GetAttackEnd() const { return attackEnd; }
    int16_t GetAlertRecoveryDelay() const { return alertRecoveryDelay; }
    int16_t GetIdleAlertDelay() const { return idleAlertDelay; }
    int32_t GetKnockbackChance() const { return knockbackChance; }
    bool IsAttackHitDone() const { return attackHitDone != 0; }
    bool IsKnockbackPending() const { return knockbackPending != 0; }
    const soft3d::Vec3& GetMoveTarget() const { return moveTarget; }
    const soft3d::Vec3& GetStayBoxReturnPosition() const { return stayBoxReturnPosition; }
    StayBox* GetStayBox() const { return stayBox; }
    EnemySpawnRecord* GetSpawnRecord() const { return spawnRecord; }
    const soft3d::Animation* GetAnimationResource(std::size_t index) const;
    soft3d::AudioData* GetSoundResource(std::size_t index) const;
    soft3d::Fixed GetHitDistance() const { return hitDistance; }
    soft3d::Fixed GetAttackRange() const { return attackRange; }
    soft3d::Fixed GetPerceptionRange() const { return perceptionRange; }
    soft3d::Fixed GetAlertSpeed() const { return alertSpeed; }
    soft3d::Fixed GetChaseSpeed() const { return chaseSpeed; }
    soft3d::Fixed GetKnockbackSpeed() const { return knockbackSpeed; }
    int32_t GetGunExperienceReward() const { return gunExperienceReward; }
    int32_t GetBloodLife() const { return blood.life; }
    const soft3d::Vec3& GetBloodPosition() const { return blood.position; }
    const soft3d::Vec3& GetBloodVelocity() const { return blood.velocity; }

protected:
    friend class Kate;

    struct BloodSplat {
        void LoadResources(soft3d::s3dResManager* manager);
        void Trigger(const soft3d::Vec3& position, const soft3d::Vec3& spread,
                     MyGameUnitMgr* manager);
        void Update(MyGameUnitMgr* manager, soft3d::Bsp* bsp);

        soft3d::s3dResManager* resources = nullptr;
        soft3d::TextureRes* resourceA = nullptr;
        soft3d::TextureRes* resourceB = nullptr;
        soft3d::Texture* textureA = nullptr;
        soft3d::Texture* textureB = nullptr;
        int32_t life = 0;
        soft3d::Vec3 position;
        soft3d::Vec3 velocity;
    } blood;

    void StopMotion();
    void BeginChase();
    bool UpdateWaitTimer();
    bool AdvanceWaypoint();
    const soft3d::Vec3& GetForward() const;
    void SetStateTime(int32_t value) { stateTime = static_cast<int16_t>(value); }
    void SetAnimationFinished(bool value) { animationFinished = value; }
    bool TestAttackHitVolume(int32_t firstStart, int32_t firstEnd,
                             int32_t secondStart = -1, int32_t secondEnd = -1,
                             soft3d::Fixed widthBonus = 0);

    int32_t enemyType = 0;
    int32_t maxHealth = 0;
    int32_t health = 0;
    int32_t attackPower = 0;
    int32_t state = EnemyRemoved;
    soft3d::Fixed playerDistance = 0;
    Player* player = nullptr;
    soft3d::Fixed hitDistance = 0;
    soft3d::Fixed attackRange = 0;
    int32_t knockbackChance = 0;
    soft3d::Fixed knockbackSpeed = 0;
    std::array<const soft3d::Animation*, 6> animations{};
    std::array<soft3d::AudioData*, 6> sounds{};
    int16_t attackStart = 0;
    int16_t attackEnd = 0;
    int16_t hitCooldown = 0;
    int16_t deathCountdown = 0;
    int16_t stateTime = 0;
    int16_t unused7ca = 0;
    std::vector<EnemyWaypoint> waypoints;
    int32_t currentWaypoint = -1;
    int16_t waypointLoop = 0;
    int16_t waypointDirection = 1;
    int16_t alertRecoveryDelay = 0;
    int16_t idleAlertDelay = 0;
    int16_t waitTimer = -1;
    int16_t attackHitDone = 0;
    soft3d::Fixed alertSpeed = 0;
    soft3d::Fixed chaseSpeed = 0;
    int32_t playerVisible = 0;
    soft3d::Fixed perceptionRange = 0;
    int32_t knockbackPending = 0;
    int16_t attackCooldown = 0;
    int16_t attackInterval = 0;
    int32_t stopped = 0;
    int16_t stayBoxAttached = 0;
    int16_t stayBoxOutside = 0;
    StayBox* stayBox = nullptr;
    soft3d::Vec3 stayBoxReturnPosition;
    EnemySpawnRecord* spawnRecord = nullptr;
    std::array<const soft3d::Animation*, 3> defaultAnimations{};
    std::array<soft3d::AudioData*, 3> defaultSounds{};
    soft3d::Fixed defaultAlertSpeed = 0;
    soft3d::Fixed defaultPerceptionRange = 0;
    int32_t gunExperienceReward = 0;
    const soft3d::Animation* savedAnimation = nullptr;
    int32_t savedAnimationTime = 0;
    int32_t moveMode = EnemyNoMovement;
    soft3d::Vec3 moveTarget;
};

} // namespace game

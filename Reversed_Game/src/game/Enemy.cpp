#include "game/Enemy.hpp"

#include "game/GameData.hpp"
#include "game/MyGameUnitMgr.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/TextureRes.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <climits>
#include <limits>
#include <stdexcept>

namespace game {
namespace {

const std::array<EnemyCommonConfig, 17> commonConfigs{{
    {10, 50, 15, 0x90000, 0x88000, 24, 3, 6, 64, 24, 0x2666, 0x9999, 250, 0xb333, 0xa0000, 0, true},
    {11, 999999999, 30, 0x80000, 0x78000, 24, 11, 14, 64, 32, 0x8000, 0x8000, 800, 0x8000, 0x27100000, 0, false},
    {12, 200, 15, 0x80000, 0x80000, 8, 11, 14, 64, 32, 0x8000, 0x8000, 800, 0x8000, 0x27100000, 0, false},
    {13, 9999, 25, 0x88000, 0x80000, 48, 8, 10, 0, 0, 0x6666, 0x6666, 1000, 0, 0x27100000, 0, true},
    {14, 1, 15, 0x78000, 0x80000, 32, 8, 10, 0, 0, 0, 0, 0, 0, 0x80000, 0, false},
    {15, 80, 15, 0xb0000, 0xb0000, 24, 7, 11, 36, 96, 0x3333, 0x3333, 500, 0x8000, 0xa0000, 0, true},
    {16, 9999, 25, 0x90000, 0x88000, 24, 8, 10, 32, 96, 0x4000, 0x4000, 999, 0, 0xb0000, 0, true},
    {17, 50, 15, 0x78000, 0x70000, 16, 3, 6, 16, 80, 0x3333, 0x9999, 100, 0xb333, 0xa0000, 0, true},
    {18, 240, 0, 0xb0000, 0xa0000, 8, 14, 16, 0, 0, 0x4ccc, 0x4ccc, 1000, 0, 0x27100000, 0, false},
    {19, 60, 20, 0x70000, 0x68000, 24, 7, 9, 32, 90, 0x1999, 0x1999, 500, 0x9999, 0x90000, 0, true},
    {20, 1, 5, 0, 0x80000, 32, 255, 0, 8, 24, 0x3333, 0x3333, 0, 0, 0x80000, 0, false},
    {21, 60, 0, 0, 0, 255, 0, 0, 16, 64, 0x3333, 0x3333, 0, 0xb333, 0, 0, true},
    {22, 150, 30, 0x80000, 0x78000, 32, 7, 9, 64, 32, 0x8000, 0xb333, 800, 0x6666, 0x140000, 8, true},
    {23, 260, 25, 0x98000, 0x98000, 40, 6, 7, 64, 32, 0x5999, 0x5999, 1000, 0x6666, 0x27100000, 30, false},
    {24, 40, 35, 0, 0x1e0000, 40, 0, 0, 0, 0, 0x13333, 0x13333, 1000, 0, 0x3e80000, 0, false},
    {25, 1, 0, 0, 0, 255, 0, 0, 9999, 1, 0x4ccc, 0x4ccc, 0, 0, 0, 10, true},
    {26, 9999, 0, 0, 0x40000, 255, 0, 0, 0, 0, 0x8ccc, 0x8ccc, 0, 0, 0x27100000, 0, true},
}};

class CommonPoolEnemy final : public Enemy {
public:
    CommonPoolEnemy(const soft3d::UnitManagerContext& context, const GameData& gameData,
                    int32_t type)
        : Enemy(context.resources, context.renderer, context.audio, context.bsp)
        , data(gameData)
    {
        enemyType = type;
        ConfigureCommonDefaults(GetEnemyCommonConfig(type));
        SetPositionalAudio(GetEnemyCommonConfig(type).positionalAudio);
    }

    void InitializeAnimations(const soft3d::Animation* const*) override
    {
        static constexpr std::array<int16_t, 6> zero{};
        const std::array<int16_t, 6>* names = &zero;
        static constexpr std::array<int16_t, 6> maid{87, 88, 89, 90, 91, 92};
        static constexpr std::array<int16_t, 6> bully{93, 94, 95, 96, 97, 98};
        static constexpr std::array<int16_t, 6> lamper{110, 111, 112, 113, 114, 115};
        static constexpr std::array<int16_t, 6> weed{116, 117, 118, 119, 120, 121};
        static constexpr std::array<int16_t, 6> fatMaid{122, 123, 124, 125, 126, 127};
        static constexpr std::array<int16_t, 6> maid2{130, 131, 132, 133, 134, 135};
        static constexpr std::array<int16_t, 6> maid3{184, 185, 186, 187, 188, 189};
        static constexpr std::array<int16_t, 6> worm{190, 191, 192, 193, 194, 195};
        static constexpr std::array<int16_t, 6> prisoner{196, 199, 200, 201, 202, 203};
        static constexpr std::array<int16_t, 6> victor{204, 205, 206, 207, 208, 209};
        static constexpr std::array<int16_t, 6> sam{234, 234, -1, -1, -1, -1};
        static constexpr std::array<int16_t, 6> kate{235, 236, 237, -1, -1, -1};
        switch (enemyType) {
        case 0: names = &maid; break;
        case 1: names = &bully; break;
        case 4: names = &lamper; break;
        case 5: names = &weed; break;
        case 6: names = &fatMaid; break;
        case 7: names = &maid2; break;
        case 9: names = &maid3; break;
        case 10: names = &worm; break;
        case 11: names = &prisoner; break;
        case 12: names = &victor; break;
        case 15: names = &sam; break;
        case 16: names = &kate; break;
        default: break;
        }
        for (std::size_t index = 0; index < names->size(); ++index) {
            const int16_t name = (*names)[index];
            SetAnimationResource(static_cast<int32_t>(index),
                                 name < 0 ? nullptr : data.GetAnimationName(name));
        }
    }

    void InitializeSounds(soft3d::AudioData* const*) override
    {
        static constexpr std::array<int16_t, 6> zero{};
        const std::array<int16_t, 6>* names = &zero;
        static constexpr std::array<int16_t, 6> maid{0, 33, 34, 35, 36, 37};
        static constexpr std::array<int16_t, 6> bully{0, 39, 40, 41, 42, 43};
        static constexpr std::array<int16_t, 6> ghost{0, 45, 46, 47, 48, 49};
        static constexpr std::array<int16_t, 6> lamper{0, 52, 53, 54, 55, 56};
        static constexpr std::array<int16_t, 6> weed{0, 58, 59, 60, 61, 62};
        static constexpr std::array<int16_t, 6> fatMaid{0, 64, 65, 66, 67, 68};
        static constexpr std::array<int16_t, 6> maid2{0, 71, 72, 73, 74, 75};
        static constexpr std::array<int16_t, 6> maid3{0, 77, 78, 79, 80, 81};
        static constexpr std::array<int16_t, 6> worm{0, 83, 84, 85, 86, 87};
        static constexpr std::array<int16_t, 6> prisoner{0, 89, 90, 91, 92, 93};
        static constexpr std::array<int16_t, 6> victor{110, 111, 112, 113, 114, 115};
        static constexpr std::array<int16_t, 6> sam{0, 107, 0, 0, 0, 108};
        static constexpr std::array<int16_t, 6> kate{0, 109, 109, 0, 0, 0};
        switch (enemyType) {
        case 0: names = &maid; break;
        case 1: names = &bully; break;
        case 3: names = &ghost; break;
        case 4: names = &lamper; break;
        case 5: names = &weed; break;
        case 6: names = &fatMaid; break;
        case 7: names = &maid2; break;
        case 9: names = &maid3; break;
        case 10: names = &worm; break;
        case 11: names = &prisoner; break;
        case 12: names = &victor; break;
        case 15: names = &sam; break;
        case 16: names = &kate; break;
        default: break;
        }
        for (std::size_t index = 0; index < names->size(); ++index) {
            const int16_t name = (*names)[index];
            SetSoundResource(static_cast<int32_t>(index),
                             name <= 0 ? nullptr : data.GetSoundName(name));
        }
    }

private:
    const GameData& data;
};

int16_t DecrementPositive(int16_t value)
{
    return value > 0 ? static_cast<int16_t>(value - 1) : value;
}

} // namespace

const EnemyCommonConfig& GetEnemyCommonConfig(int32_t enemyType)
{
    if (enemyType < 0 || static_cast<std::size_t>(enemyType) >= commonConfigs.size()) {
        throw std::out_of_range("Enemy type");
    }
    return commonConfigs[static_cast<std::size_t>(enemyType)];
}

bool IsSingletonEnemyType(int32_t enemyType)
{
    switch (enemyType) {
    case 1:
    case 2:
    case 3:
    case 8:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        return true;
    default:
        return false;
    }
}

std::unique_ptr<Enemy> CreateCommonPoolEnemy(const soft3d::UnitManagerContext& context,
                                             const GameData& data, int32_t enemyType)
{
    if (enemyType < 0 || enemyType >= 17) {
        return nullptr;
    }
    return std::make_unique<CommonPoolEnemy>(context, data, enemyType);
}

void Enemy::BloodSplat::LoadResources(soft3d::s3dResManager* manager)
{
    resources = manager;
    if (resources == nullptr) {
        return;
    }
    resourceA = dynamic_cast<soft3d::TextureRes*>(resources->GetResource(
        soft3d::ResourceTexture, 0, u"zv_jianxue_a", u".\\common\\"));
    resourceB = dynamic_cast<soft3d::TextureRes*>(resources->GetResource(
        soft3d::ResourceTexture, 0, u"zv_jianxue_b", u".\\common\\"));
    textureA = resourceA != nullptr ? resourceA->GetTexture() : nullptr;
    textureB = resourceB != nullptr ? resourceB->GetTexture() : nullptr;
}

void Enemy::BloodSplat::Trigger(const soft3d::Vec3& value, const soft3d::Vec3& spread,
                                MyGameUnitMgr* manager)
{
    position = value;
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    velocity = soft3d::Scale(spread, random % 0xffff - 0x7fff);
    velocity.y = 0;
    life = 9;
}

void Enemy::BloodSplat::Update(MyGameUnitMgr* manager, soft3d::Bsp* bsp)
{
    if (life <= 0) {
        return;
    }
    position = soft3d::Add(position, velocity);
    velocity.x = soft3d::FixedMultiply(velocity.x, 0xe147);
    velocity.z = soft3d::FixedMultiply(velocity.z, 0xe147);
    velocity.y = soft3d::SubtractFixed(velocity.y, 0x428f);
    --life;
    if (life != 1 || bsp == nullptr) {
        return;
    }
    const bool flipU = manager != nullptr && manager->NextRandom() % 1000 < 501;
    const bool flipV = manager != nullptr && manager->NextRandom() % 1000 < 501;
    const bool selectB = manager == nullptr || manager->NextRandom() % 1000 < 556;
    soft3d::Texture* texture = selectB ? textureB : textureA;
    soft3d::Vec3 start = position;
    start.y = soft3d::AddFixed(start.y, 0x80000);
    soft3d::Vec3 end = soft3d::Add(position, velocity);
    end.y = soft3d::SubtractFixed(end.y, 0x60000);
    bsp->BlendLightmapSplat(texture, start, end, flipU, flipV);
}

Enemy::Enemy(soft3d::s3dResManager* resources, soft3d::Renderer* renderer,
             soft3d::AudioManager* audio, soft3d::Bsp* map)
    : Creature(resources, 2, renderer, audio, map)
{
    unitType = 8;
    blood.resources = resources;
    SetPositionalAudio(true);
}

void Enemy::ConfigureCommonDefaults(const EnemyCommonConfig& config)
{
    unitType = config.unitType;
    maxHealth = config.maxHealth;
    attackPower = config.attackPower;
    hitDistance = config.hitDistance;
    attackRange = config.attackRange;
    attackInterval = config.attackInterval;
    attackStart = config.attackStart;
    attackEnd = config.attackEnd;
    alertRecoveryDelay = config.alertRecoveryDelay;
    idleAlertDelay = config.idleAlertDelay;
    alertSpeed = config.alertSpeed;
    chaseSpeed = config.chaseSpeed;
    knockbackChance = config.knockbackChance;
    knockbackSpeed = config.knockbackSpeed;
    perceptionRange = config.perceptionRange;
    gunExperienceReward = config.gunExperienceReward;
    SnapshotMovementConfig();
}

void Enemy::SnapshotMovementConfig()
{
    defaultAlertSpeed = alertSpeed;
    defaultPerceptionRange = perceptionRange;
}

void Enemy::SetManager(MyGameUnitMgr* value)
{
    SetUnitManager(value);
}

bool Enemy::IsAttackable() const
{
    return !IsRemoved() && state != EnemyDying;
}

bool Enemy::IsAttacking() const
{
    return state < EnemyDying && playerVisible != 0;
}

int32_t Enemy::GetAttackPower() const { return attackPower; }
soft3d::Fixed Enemy::GetPlayerDistance() const { return playerDistance; }

const soft3d::Vec3& Enemy::GetForward() const
{
    static constexpr soft3d::Vec3 zero{};
    soft3d::Object* object = GetObject(0);
    return object != nullptr ? object->GetAxisZ() : zero;
}

soft3d::GameUnit* Enemy::QueryPoint(const soft3d::Vec3& point)
{
    return state < EnemyDying ? GameUnit::QueryPoint(point) : nullptr;
}

soft3d::GameUnit* Enemy::QuerySegment(const soft3d::Vec3& start, const soft3d::Vec3& end)
{
    return state < EnemyDying ? GameUnit::QuerySegment(start, end) : nullptr;
}

soft3d::GameUnit* Enemy::QueryBounds(const soft3d::ObjectBounds& bounds)
{
    return state < EnemyDying ? GameUnit::QueryBounds(bounds) : nullptr;
}

void Enemy::SubmitRenderables(soft3d::RenderQueues& queues)
{
    if (state != EnemyRemoved) {
        VisUnit::SubmitRenderables(queues);
    }
}

void Enemy::QueryUnitCollision()
{
    PhysicUnit::QueryUnitCollision();
    if (player != nullptr && player->QueryBounds(collisionBounds) != nullptr &&
        unitQueryResult.count < static_cast<int32_t>(unitQueryResult.units.size())) {
        unitQueryResult.units[static_cast<std::size_t>(unitQueryResult.count++)] = player;
        unitCollision = true;
    }
}

void Enemy::SetAnimation(const soft3d::Animation* resource, int32_t direction, bool loop)
{
    PhysicUnit::SetAnimation(resource, direction, loop);
}

void Enemy::SetAnimationByIndex(int32_t index, int32_t direction, bool loop)
{
    if (index >= 0 && static_cast<std::size_t>(index) < animations.size()) {
        PhysicUnit::SetAnimation(animations[static_cast<std::size_t>(index)], direction, loop);
    }
}

void Enemy::SetAnimationResource(int32_t index, const char16_t* name)
{
    if (index < 0 || static_cast<std::size_t>(index) >= animations.size()) {
        return;
    }
    animations[static_cast<std::size_t>(index)] = nullptr;
    if (name != nullptr && resourceManager != nullptr) {
        animations[static_cast<std::size_t>(index)] = dynamic_cast<const soft3d::Animation*>(
            resourceManager->GetResource(soft3d::ResourceAnimation, 2, name));
    }
    if (state == index) {
        SetAnimationByIndex(index, 1, true);
    }
}

void Enemy::SetSoundResource(int32_t index, const char16_t* name)
{
    if (index < 0 || static_cast<std::size_t>(index) >= sounds.size()) {
        return;
    }
    sounds[static_cast<std::size_t>(index)] = nullptr;
    if (name != nullptr && resourceManager != nullptr) {
        sounds[static_cast<std::size_t>(index)] = dynamic_cast<soft3d::AudioData*>(
            resourceManager->GetResource(soft3d::ResourceAudio, 2, name, u".\\audio\\"));
    }
}

const soft3d::Animation* Enemy::GetAnimationResource(std::size_t index) const
{
    return index < animations.size() ? animations[index] : nullptr;
}

soft3d::AudioData* Enemy::GetSoundResource(std::size_t index) const
{
    return index < sounds.size() ? sounds[index] : nullptr;
}

void Enemy::LoadResources(const char16_t* modelName,
                          const soft3d::Animation* const* animationValues,
                          soft3d::AudioData* const* soundValues)
{
    LoadObjects(modelName, nullptr, 1);
    LoadMaterials();
    InitializeAnimations(animationValues);
    InitializeSounds(soundValues);
    EnableShadow(true);
    blood.LoadResources(resourceManager);
    std::copy_n(animations.begin(), defaultAnimations.size(), defaultAnimations.begin());
    std::copy_n(sounds.begin(), defaultSounds.size(), defaultSounds.begin());
}

void Enemy::InitializeAnimations(const soft3d::Animation* const*)
{
    animations.fill(nullptr);
}

void Enemy::InitializeSounds(soft3d::AudioData* const*)
{
    sounds.fill(nullptr);
}

void Enemy::Reset()
{
    state = EnemyRemoved;
    health = maxHealth;
    Creature::SetNavigationEnabled(false);
    spawnRecord = nullptr;
    StopSound();
    currentWaypoint = -1;
    waypoints.clear();
    animation = nullptr;
    savedAnimation = nullptr;
}

void Enemy::Spawn(const soft3d::Vec3& value)
{
    std::copy(defaultAnimations.begin(), defaultAnimations.end(), animations.begin());
    std::copy(defaultSounds.begin(), defaultSounds.end(), sounds.begin());
    position = value;
    SetDirection(soft3d::Subtract(player != nullptr ? player->GetPosition() : soft3d::Vec3{},
                                  position));
    previousPosition = position;
    position.y = soft3d::AddFixed(position.y, 0x40000);
    SetAnimationByIndex(0, 1, true);
    if (bsp != nullptr) {
        soft3d::Vec3 end = position;
        end.y = soft3d::SubtractFixed(end.y, 0xc0000);
        *groundTrace = bsp->Trace(position, end, localBounds);
        if (groundTrace->startSolid) {
            position = groundTrace->endPosition;
        }
    } else {
        *groundTrace = soft3d::BspTraceResult{};
    }
    state = EnemyIdle;
    velocity = {};
    health = maxHealth;
    hitCooldown = 0;
    attackHitDone = 0;
    Creature::SetNavigationEnabled(false);
    moveMode = EnemyNoMovement;
    stopped = 0;
    stayBoxAttached = 0;
    stayBoxOutside = 0;
    currentWaypoint = -1;
    waypoints.clear();
    waypointDirection = 1;
    soft3d::Vec3 distancePoint = position;
    distancePoint.y = player != nullptr ? player->GetPosition().y : position.y;
    playerDistance = player != nullptr
        ? soft3d::Length(soft3d::Subtract(distancePoint, player->GetPosition())) : 0;
    deathCountdown = 0;
    stateTime = 0;
    unused7ca = 0;
    waitTimer = -1;
    attackCooldown = 0;
    playerVisible = 0;
    collisionEnabled = true;
    ResetNavigationPath();
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetRenderMode(soft3d::ObjectRenderMode::Normal);
    }
    StopSound();
    physicsFlags = 0xb;
    groundTrace->startSolid = false;
    alertSpeed = defaultAlertSpeed;
    perceptionRange = defaultPerceptionRange;
    spawnRecord = nullptr;
    knockbackPending = 0;
    savedAnimation = nullptr;
    if (primaryObject != nullptr && bsp != nullptr) {
        primaryObject->SetObjectIntensity(
            soft3d::SubtractFixed(bsp->GetLightIntensity(position), 0x4ccc));
    }
}

void Enemy::StopSound()
{
    VisUnit::StopSound();
}

void Enemy::StopMotion()
{
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    physicsFlags &= ~soft3d::PhysicStepOffset;
    velocity = {};
    SetAnimationByIndex(0, 1, true);
    StopSound();
    state = EnemyIdle;
}

void Enemy::SetNavigationEnabled(bool enabled)
{
    if (state >= EnemyDying) {
        return;
    }
    if (!IsNavigationEnabled() && enabled) {
        StopSound();
        savedAnimation = animation;
        savedAnimationTime = GetObject(0) != nullptr ? GetObject(0)->GetAnimationTime() : 0;
        if (state != EnemyIdle) {
            StopMotion();
        }
        waitTimer = -1;
    }
    if (IsNavigationEnabled() && !enabled) {
        if (state == EnemyAlert) {
            PlaySound(sounds[1], true);
        }
        if (state == EnemyChase) {
            PlaySound(sounds[2], true);
        }
    }
    Creature::SetNavigationEnabled(enabled);
}

void Enemy::SetHealth(int32_t value)
{
    if (value < 0) {
        health = maxHealth;
    } else {
        health = value;
        if (value == 0) {
            Die();
        }
    }
}

void Enemy::SetStopped(bool value)
{
    stopped = value ? 1 : 0;
    if (value) {
        moveMode = EnemyNoMovement;
        playerVisible = 0;
    }
}

void Enemy::AttachStayBox(StayBox* value)
{
    stayBox = value;
    stayBoxAttached = 1;
}

void Enemy::AddWaypoint(const soft3d::Vec3& waypointPosition, int32_t waitTime,
                        const soft3d::Vec3& direction)
{
    EnemyWaypoint waypoint;
    waypoint.position = waypointPosition;
    waypoint.waitTime = waitTime;
    waypoint.direction = direction;
    waypoints.push_back(waypoint);
    currentWaypoint = 0;
}

void Enemy::ClearWaypoints()
{
    waypoints.clear();
    currentWaypoint = -1;
}

void Enemy::EnsureSpawnRecord()
{
    if (spawnRecord == nullptr && unitManager != nullptr) {
        spawnRecord = static_cast<MyGameUnitMgr*>(unitManager)->CreateEnemySpawnRecord(this);
    }
}

bool Enemy::MoveTo(const soft3d::Vec3& target)
{
    if (moveMode != EnemyNoMovement || state >= EnemyDying) {
        return false;
    }
    if (stopped == 0) {
        moveMode = EnemyMoveToTarget;
        moveTarget = target;
        soft3d::Vec3 destination = position;
        destination.y = target.y;
        const soft3d::Fixed distance = soft3d::SquaredLength(soft3d::Subtract(target, destination));
        if (distance < 0 || distance >= chaseSpeed) {
            return false;
        }
        moveMode = EnemyNoMovement;
        StopMotion();
        position = destination;
        return true;
    }
    moveMode = EnemyNoMovement;
    StopMotion();
    return true;
}

void Enemy::BeginChase()
{
    if (stopped != 0) {
        return;
    }
    if (moveMode == EnemyNoMovement && playerDistance <= attackRange) {
        const int32_t selection = SelectAttack();
        if (selection > 0) {
            BeginAttack();
            return;
        }
        if (selection == -1) {
            return;
        }
    }
    physicsFlags &= ~soft3d::PhysicKeepDirection;
    physicsFlags &= ~0xe0U;
    physicsFlags |= soft3d::PhysicSimpleMovement | soft3d::PhysicStepOffset;
    state = EnemyChase;
    SetAnimationByIndex(2, 1, true);
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    if (unitManager != nullptr) {
        for (soft3d::GameUnit* unit = unitManager->GetFirstUnit(0); unit != nullptr;
             unit = unit->GetNextUnit()) {
            auto* other = dynamic_cast<Enemy*>(unit);
            if (other != nullptr && other->enemyType == enemyType &&
                other->soundVoice != -1 && other->state == EnemyChase) {
                return;
            }
        }
    }
    PlaySound(sounds[2], true);
}

bool Enemy::UpdateWaitTimer()
{
    if (waitTimer == -1) {
        const int32_t delay = alertRecoveryDelay;
        const int32_t remainder = delay != 0 && unitManager != nullptr
            ? static_cast<int32_t>(static_cast<MyGameUnitMgr*>(unitManager)->NextRandom() % delay)
            : 0;
        stateTime = static_cast<int16_t>(remainder);
        waitTimer = static_cast<int16_t>(delay + remainder);
    }
    if (waitTimer != 0) {
        return false;
    }
    StopMotion();
    waitTimer = -1;
    stateTime = 0;
    return true;
}

void Enemy::UpdateAlertDelay()
{
    if (currentWaypoint < 0 || stopped != 0) {
        return;
    }
    if (waitTimer == -1) {
        const int32_t delay = idleAlertDelay;
        const int32_t remainder = delay != 0 && unitManager != nullptr
            ? static_cast<int32_t>(static_cast<MyGameUnitMgr*>(unitManager)->NextRandom() % delay)
            : 0;
        stateTime = static_cast<int16_t>(remainder);
        waitTimer = static_cast<int16_t>(delay + remainder);
    }
    if (waitTimer != 0) {
        return;
    }
    physicsFlags |= soft3d::PhysicSimpleMovement | soft3d::PhysicStepOffset;
    state = EnemyAlert;
    SetAnimationByIndex(1, 1, true);
    waitTimer = -1;
    stateTime = 0;
    PlaySound(sounds[1], true);
}

bool Enemy::AdvanceWaypoint()
{
    if (currentWaypoint < 0 || static_cast<std::size_t>(currentWaypoint) >= waypoints.size()) {
        return true;
    }
    const EnemyWaypoint& current = waypoints[static_cast<std::size_t>(currentWaypoint)];
    soft3d::Vec3 target = current.position;
    target.y = position.y;
    const soft3d::Fixed distance = soft3d::SquaredLength(soft3d::Subtract(position, target));
    if (distance < 0 || distance > soft3d::fixedOne) {
        moveTarget = current.position;
        return false;
    }
    if (waypoints.size() > 1) {
        int32_t next = -1;
        if (waypointDirection != 0 && currentWaypoint + 1 < static_cast<int32_t>(waypoints.size())) {
            next = currentWaypoint + 1;
        } else if (waypointDirection == 0 && currentWaypoint > 0) {
            next = currentWaypoint - 1;
        }
        if (next < 0) {
            if (waypointLoop == 0) {
                waypointDirection = waypointDirection == 0 ? 1 : 0;
                next = currentWaypoint + (waypointDirection != 0 ? 1 : -1);
            } else {
                next = waypointDirection != 0 ? 0 : static_cast<int32_t>(waypoints.size()) - 1;
            }
        }
        SetDirection(current.direction);
        currentWaypoint = next;
        waitTimer = static_cast<int16_t>(waypoints[static_cast<std::size_t>(next)].waitTime);
        return true;
    }
    SetDirection(current.direction);
    return true;
}

bool Enemy::UpdatePerception()
{
    if (player == nullptr) {
        return false;
    }
    if (player->IsUsedItemHealing()) {
        playerVisible = 0;
        return false;
    }
    if (playerVisible != 0) {
        return true;
    }
    if (state >= EnemyDying || player->IsHiddenOrFPS()) {
        return false;
    }
    if (playerDistance < perceptionRange) {
        playerVisible = 1;
        return true;
    }
    soft3d::Object* playerObject = player->GetObject(0);
    if (playerObject != nullptr && playerObject->GetObjectIntensity() < -0x1999) {
        return false;
    }
    const soft3d::Vec3 direction = soft3d::Normalize(
        soft3d::Subtract(player->GetPosition(), position));
    if (soft3d::Dot(direction, GetForward()) < 0xb333) {
        return false;
    }
    soft3d::Vec3 eye = position;
    eye.y = soft3d::AddFixed(eye.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    eye.y = soft3d::SubtractFixed(eye.y, soft3d::fixedOne);
    if (bsp != nullptr && bsp->Trace(eye, player->GetPosition()).startSolid) {
        soft3d::Vec3 playerEye = player->GetPosition();
        playerEye.y = soft3d::AddFixed(playerEye.y,
            soft3d::SubtractFixed(player->GetLocalBounds().maximum.y,
                                  player->GetLocalBounds().minimum.y) >> 1);
        playerEye.y = soft3d::SubtractFixed(playerEye.y, soft3d::fixedOne);
        if (bsp->Trace(eye, playerEye).startSolid) {
            return false;
        }
    }
    playerVisible = 1;
    return true;
}

int32_t Enemy::SelectAttack()
{
    if (stopped != 0 || moveMode != EnemyNoMovement || !IsAttacking() ||
        playerDistance > attackRange) {
        return 0;
    }
    if (attackCooldown > 0) {
        return -1;
    }
    if (player == nullptr || bsp == nullptr) {
        return 1;
    }
    return bsp->Trace(position, player->GetPosition()).startSolid ? 0 : 1;
}

bool Enemy::RequestAttack()
{
    if (SelectAttack() == 0) {
        return false;
    }
    BeginAttack();
    return true;
}

void Enemy::BeginAttack()
{
    StopSound();
    waitTimer = -1;
    stateTime = 0;
    unused7ca = 0;
    state = EnemyAttack;
    velocity = {};
    SetAnimationByIndex(3, 1, false);
    PlaySound(sounds[3], false);
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    attackHitDone = 0;
}

bool Enemy::TestAttackHit()
{
    if (attackHitDone != 0 || stateTime < attackStart || stateTime > attackEnd) {
        return false;
    }
    const soft3d::Vec3 start = position;
    const soft3d::Vec3 end = soft3d::Add(start, soft3d::Scale(GetForward(), hitDistance));
    soft3d::ObjectBounds bounds{{INT32_MAX, INT32_MAX, INT32_MAX},
                                {INT32_MIN, INT32_MIN, INT32_MIN}};
    auto expand = [&bounds](const soft3d::Vec3& point) {
        bounds.minimum.x = std::min(bounds.minimum.x, point.x);
        bounds.minimum.y = std::min(bounds.minimum.y, point.y);
        bounds.minimum.z = std::min(bounds.minimum.z, point.z);
        bounds.maximum.x = std::max(bounds.maximum.x, point.x);
        bounds.maximum.y = std::max(bounds.maximum.y, point.y);
        bounds.maximum.z = std::max(bounds.maximum.z, point.z);
    };
    expand(start);
    expand(end);
    const bool hit = player != nullptr && player->IsBoundsInside(bounds);
    if (hit) {
        attackHitDone = 1;
        player->DamageFrom(this);
    }
    return hit;
}

bool Enemy::ReceivePlayerAttack()
{
    if (hitCooldown != 0 || player == nullptr) {
        return false;
    }
    const int32_t damage = player->GetAttackDamage(this);
    if (damage == 0) {
        return false;
    }
    Damage(damage);
    return true;
}

void Enemy::Damage(int32_t amount)
{
    StopSound();
    health -= amount;
    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    if (health < 1) {
        Die();
    } else {
        if (knockbackChance != 0 && unitManager != nullptr &&
            static_cast<int32_t>(static_cast<MyGameUnitMgr*>(unitManager)->NextRandom() % 250) <=
                knockbackChance) {
            knockbackPending = 1;
            state = EnemyKnockback;
            SetAnimationByIndex(4, 1, false);
            SetAnimationSpeed(0x4b0);
            PlaySound(sounds[4], false);
        } else {
            state = EnemyHurt;
            SetAnimationByIndex(4, 1, false);
            PlaySound(sounds[4], false);
        }
    }
    stateTime = 0;
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
        velocity = soft3d::Scale(GetForward(),
            -soft3d::FixedMultiply(player->GetAttackBlend(), knockbackSpeed));
        if (knockbackPending != 0) {
            velocity = soft3d::Scale(velocity, 0x8000);
        }
        const soft3d::Vec3 bloodPosition = soft3d::Subtract(position,
            soft3d::Scale(GetForward(), 0x30000));
        blood.Trigger(bloodPosition, player->GetSword()->GetHitVelocity(),
                      static_cast<MyGameUnitMgr*>(unitManager));
    }
    hitCooldown = 8;
}

void Enemy::Die()
{
    if (state == EnemyDying || IsRemoved()) {
        return;
    }
    if (player != nullptr) {
        player->NotifyEnemyKilled(gunExperienceReward);
    }
    StopSound();
    SetAnimationByIndex(5, 1, false);
    PlaySound(sounds[5], false);
    deathCountdown = 12;
    state = EnemyDying;
    if (spawnRecord != nullptr) {
        spawnRecord->active = 0;
    }
}

void Enemy::UpdateIdle()
{
    if (player == nullptr) {
        return;
    }
    if (soft3d::AbsFixed(soft3d::SubtractFixed(player->GetPosition().y, position.y)) > 0x50000 ||
        ReceivePlayerAttack()) {
        return;
    }
    if (stopped != 0) {
        StopSound();
    } else if (moveMode != EnemyNoMovement) {
        BeginChase();
    } else if (stayBoxOutside == 0) {
        if (UpdatePerception()) {
            BeginChase();
        } else {
            UpdateAlertDelay();
        }
    }
}

void Enemy::UpdateAlert()
{
    SetNavigationSpeed(alertSpeed);
    if (ReceivePlayerAttack()) {
        return;
    }
    if (stopped != 0) {
        StopMotion();
        waitTimer = 0;
    } else if (moveMode != EnemyNoMovement) {
        BeginChase();
    } else if (!UpdateWaitTimer()) {
        if (UpdatePerception()) {
            BeginChase();
        } else if (AdvanceWaypoint()) {
            StopMotion();
            stateTime = 0;
        } else {
            NavigateTo(moveTarget, chaseSpeed);
        }
    }
}

void Enemy::UpdateChase()
{
    SetNavigationSpeed(chaseSpeed);
    if (ReceivePlayerAttack()) {
        return;
    }
    if (stopped != 0) {
        StopMotion();
    } else if (moveMode == EnemyMoveToTarget) {
        if (NavigateTo(moveTarget, chaseSpeed)) {
            StopMotion();
        }
    } else if (moveMode == EnemyFollowPlayer) {
        if (player != nullptr) {
            NavigateTo(player->GetPosition(), chaseSpeed);
        }
        if (UpdatePerception()) {
            moveMode = EnemyNoMovement;
        }
    } else if (stayBoxOutside == 0 && player != nullptr) {
        if (playerDistance <= attackRange) {
            const int32_t selection = SelectAttack();
            if (selection > 0) {
                StopMotion();
                BeginAttack();
                return;
            }
            if (selection == -1) {
                StopMotion();
                return;
            }
        }
        soft3d::Vec3 target = player->GetPosition();
        const soft3d::Fixed enemyHeight = soft3d::SubtractFixed(localBounds.maximum.y,
                                                                localBounds.minimum.y);
        const soft3d::Fixed playerHeight = soft3d::SubtractFixed(player->GetLocalBounds().maximum.y,
                                                                 player->GetLocalBounds().minimum.y);
        target.y = soft3d::AddFixed(target.y,
            soft3d::SubtractFixed(enemyHeight, playerHeight) >> 1);
        NavigateTo(target, chaseSpeed);
    }
}

void Enemy::UpdateAttack()
{
    if (knockbackPending == 0 && ReceivePlayerAttack()) {
        return;
    }
    stateTime = static_cast<int16_t>(stateTime + 1);
    TestAttackHit();
    if (IsAnimationFinished()) {
        stateTime = 0;
        SetAnimationByIndex(0, 1, true);
        attackCooldown = attackInterval;
        state = EnemyIdle;
        knockbackPending = 0;
    }
}

void Enemy::UpdateHurt()
{
    stateTime = static_cast<int16_t>(stateTime + 1);
    if (stateTime > 5) {
        velocity = {};
    }
    if (IsAnimationFinished()) {
        stateTime = 0;
        velocity = {};
        physicsFlags &= ~soft3d::PhysicKeepDirection;
        physicsFlags |= soft3d::PhysicSimpleMovement;
        SetAnimationByIndex(0, 1, true);
        state = EnemyIdle;
    }
}

void Enemy::UpdateKnockback()
{
    stateTime = static_cast<int16_t>(stateTime + 1);
    if (stateTime > 4) {
        velocity = {};
    }
    if (IsAnimationFinished()) {
        stateTime = 0;
        velocity = {};
        physicsFlags &= ~soft3d::PhysicKeepDirection;
        physicsFlags |= soft3d::PhysicSimpleMovement;
        SetAnimationByIndex(0, 1, true);
        state = EnemyIdle;
    }
}

void Enemy::UpdateDeath()
{
    stateTime = static_cast<int16_t>(stateTime + 1);
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetRenderMode(soft3d::ObjectRenderMode::SecondaryStreams);
    }
    if (stateTime > 7) {
        velocity = {};
    }
    if (IsAnimationFinished()) {
        deathCountdown = static_cast<int16_t>(deathCountdown - 1);
    }
    if (deathCountdown == 0) {
        stateTime = 0;
        collisionEnabled = false;
        state = EnemyDead;
    }
}

void Enemy::Update()
{
    if (state > EnemyDying) {
        StopSound();
        return;
    }
    if (IsNavigationEnabled()) {
        if (objectGroup != nullptr) {
            if (GetObject(0) != nullptr) {
                GetObject(0)->SetPosition(position);
            }
            for (std::size_t index = 0; index < GetObjectCount(); ++index) {
                GetObject(index)->UpdateTransform();
            }
        }
        if (shadowEnabled) {
            UpdateShadow();
        }
        return;
    }
    if (savedAnimation != nullptr) {
        SetAnimation(savedAnimation, 1, true);
        SetAnimationTime(savedAnimationTime);
        savedAnimation = nullptr;
    }
    if (player != nullptr) {
        soft3d::Vec3 distancePoint = position;
        distancePoint.y = player->GetPosition().y;
        playerDistance = soft3d::Length(soft3d::Subtract(distancePoint, player->GetPosition()));
    }
    if (stayBoxAttached != 0 && state != EnemyDying && stayBox != nullptr) {
        stayBoxOutside = static_cast<int16_t>(stayBox->IsEnemyOutside());
        if (stayBoxOutside != 0) {
            soft3d::Vec3 target = stayBoxReturnPosition;
            target.y = position.y;
            const soft3d::Fixed distance = soft3d::SquaredLength(
                soft3d::Subtract(position, target));
            if (distance >= 0 && distance < soft3d::fixedOne) {
                if (state == EnemyChase) {
                    moveMode = EnemyNoMovement;
                    StopMotion();
                }
            } else if (moveMode == EnemyNoMovement) {
                moveTarget = stayBoxReturnPosition;
                moveMode = EnemyMoveToTarget;
                BeginChase();
            }
        } else if (moveMode != EnemyNoMovement) {
            moveMode = EnemyNoMovement;
            StopMotion();
        }
    }
    switch (state) {
    case EnemyIdle: UpdateIdle(); break;
    case EnemyAlert: UpdateAlert(); break;
    case EnemyChase: UpdateChase(); break;
    case EnemyAttack: UpdateAttack(); break;
    case EnemyHurt: UpdateHurt(); break;
    case EnemyKnockback: UpdateKnockback(); break;
    case EnemyDying: UpdateDeath(); break;
    default: break;
    }
    PhysicUnit::Update();
    blood.Update(static_cast<MyGameUnitMgr*>(unitManager), bsp);
    attackCooldown = DecrementPositive(attackCooldown);
    waitTimer = DecrementPositive(waitTimer);
    hitCooldown = DecrementPositive(hitCooldown);
}

void Enemy::SaveState(soft3d::OutputStream& stream) const
{
    VisUnit::SaveState(stream);
    stream.WriteInt32(enemyType);
    stream.WriteInt32(IsNavigationEnabled() ? 1 : 0);
    stream.WriteInt32(health);
    stream.WriteInt32(state);
}

void Enemy::LoadState(soft3d::InputStream& stream)
{
    VisUnit::LoadState(stream);
    stream.ReadInt32(enemyType);
    int32_t enabled = 0;
    stream.ReadInt32(enabled);
    RestoreNavigationEnabled(enabled != 0);
    stream.ReadInt32(health);
    stream.ReadInt32(state);
}

} // namespace game

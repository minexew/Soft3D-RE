#include "game/NamedEnemies.hpp"

#include "game/Lord.hpp"
#include "game/MaidBully.hpp"
#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <numeric>

namespace game {
namespace {

void Configure(Enemy& enemy, int32_t type)
{
    enemy.SetEnemyType(type);
    enemy.ConfigureCommonDefaults(GetEnemyCommonConfig(type));
    enemy.SetPositionalAudio(GetEnemyCommonConfig(type).positionalAudio);
}

const soft3d::Animation* LoadAnimationResource(soft3d::s3dResManager* resources, const GameData& data,
                                       int32_t name, int32_t cacheLevel)
{
    return resources != nullptr && name >= 0
        ? dynamic_cast<const soft3d::Animation*>(resources->GetResource(
              soft3d::ResourceAnimation, cacheLevel, data.GetAnimationName(
                  static_cast<std::size_t>(name))))
        : nullptr;
}

soft3d::AudioData* LoadSound(soft3d::s3dResManager* resources, const GameData& data,
                             int32_t name, int32_t cacheLevel)
{
    return resources != nullptr && name > 0
        ? dynamic_cast<soft3d::AudioData*>(resources->GetResource(
              soft3d::ResourceAudio, cacheLevel, data.GetSoundName(
                  static_cast<std::size_t>(name)), u".\\audio\\"))
        : nullptr;
}

template <std::size_t N>
void LoadAnimationResources(Enemy& enemy, const GameData& data, const std::array<int32_t, N>& names)
{
    for (std::size_t index = 0; index < names.size(); ++index) {
        enemy.SetAnimationResource(static_cast<int32_t>(index),
            names[index] < 0 ? nullptr : data.GetAnimationName(static_cast<std::size_t>(names[index])));
    }
}

template <std::size_t N>
void LoadSounds(Enemy& enemy, const GameData& data, const std::array<int32_t, N>& names)
{
    for (std::size_t index = 0; index < names.size(); ++index) {
        enemy.SetSoundResource(static_cast<int32_t>(index),
            names[index] <= 0 ? nullptr : data.GetSoundName(static_cast<std::size_t>(names[index])));
    }
}

soft3d::ObjectBounds Translate(const soft3d::ObjectBounds& bounds, const soft3d::Vec3& offset)
{
    return {soft3d::Add(bounds.minimum, offset), soft3d::Add(bounds.maximum, offset)};
}

bool Overlaps(const soft3d::ObjectBounds& left, const soft3d::ObjectBounds& right)
{
    return right.maximum.x >= left.minimum.x && right.maximum.y >= left.minimum.y &&
           right.maximum.z >= left.minimum.z && right.minimum.x <= left.maximum.x &&
           right.minimum.y <= left.maximum.y && right.minimum.z <= left.maximum.z;
}

bool Contains(const soft3d::ObjectBounds& bounds, const soft3d::Vec3& point)
{
    return point.x >= bounds.minimum.x && point.y >= bounds.minimum.y &&
           point.z >= bounds.minimum.z && point.x <= bounds.maximum.x &&
           point.y <= bounds.maximum.y && point.z <= bounds.maximum.z;
}

soft3d::Vec3 Center(const soft3d::ObjectBounds& bounds)
{
    return soft3d::ShiftRight(soft3d::Add(bounds.minimum, bounds.maximum), 1);
}

uint32_t Random(MyGameUnitMgr* manager)
{
    return manager != nullptr ? manager->NextRandom() : 0;
}

} // namespace

Victor::Victor(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 12);
}

void Victor::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimationResources(*this, data, std::array<int32_t, 6>{204, 205, 206, 207, 208, 209});
    for (std::size_t index = 0; index < leapAnimations.size(); ++index) {
        leapAnimations[index] = LoadAnimationResource(resourceManager, data, 210 + static_cast<int32_t>(index),
                                              cacheLevel);
    }
}

void Victor::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{110, 111, 112, 113, 114, 115});
    leapSounds[0] = LoadSound(resourceManager, data, 116, cacheLevel);
    leapSounds[1] = LoadSound(resourceManager, data, 117, cacheLevel);
    leapSounds[2] = leapSounds[1];
}

const soft3d::Animation* Victor::GetLeapAnimation(std::size_t index) const
{
    return index < leapAnimations.size() ? leapAnimations[index] : nullptr;
}

soft3d::AudioData* Victor::GetLeapSound(std::size_t index) const
{
    return index < leapSounds.size() ? leapSounds[index] : nullptr;
}

void Victor::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    leapPhase = 0;
    StopMotion();
    stateTime = 0;
}

void Victor::QueryUnitCollision()
{
    PhysicUnit::QueryUnitCollision();
    if (leapPhase != 1 && player != nullptr && player->QueryBounds(collisionBounds) != nullptr &&
        unitQueryResult.count < static_cast<int32_t>(unitQueryResult.units.size())) {
        unitQueryResult.units[static_cast<std::size_t>(unitQueryResult.count++)] = player;
        unitCollision = true;
    }
}

void Victor::Update()
{
    if (state >= EnemyDead) {
        StopSound();
        return;
    }
    if (IsNavigationEnabled()) {
        if (GetObject(0) != nullptr) {
            GetObject(0)->SetPosition(position);
        }
        if (objectGroup != nullptr) {
            for (std::size_t index = 0; index < GetObjectCount(); ++index) {
                GetObject(index)->UpdateTransform();
            }
        }
        UpdateShadow();
        return;
    }
    if (state == 6) {
        UpdateLeap();
    }
    Enemy::Update();
}

void Victor::BeginAttack()
{
    Enemy::BeginAttack();
    attackStart = 7;
    attackEnd = 9;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    if (Random(manager) % 10000 > 6000) {
        SetAnimationSpeed(600);
        attackStart = static_cast<int16_t>(static_cast<uint16_t>(attackStart) >> 1);
        attackEnd = static_cast<int16_t>(static_cast<uint16_t>(attackEnd) >> 1);
    }
    if (Random(manager) % 10000 > 7000) {
        attackCooldown = static_cast<int16_t>(attackInterval >> 1);
    }
}

bool Victor::TestAttackHit()
{
    return TestAttackHitVolume(attackStart, attackEnd);
}

bool Victor::ReceivePlayerAttack()
{
    if (hitCooldown != 0 || player == nullptr) {
        return false;
    }
    const int32_t damage = player->GetAttackDamage(this);
    if (damage == 0) {
        return false;
    }
    StopSound();
    health -= damage;
    if (health < 1) {
        Die();
        return true;
    }
    blood.Trigger(soft3d::Subtract(position, soft3d::Scale(GetForward(), 0x30000)),
                  player->GetSword()->GetHitVelocity(), static_cast<MyGameUnitMgr*>(unitManager));
    hitCooldown = 8;
    PlaySound(sounds[4], false);
    if (state == 6 && leapPhase != 0) {
        return false;
    }
    stateTime = 0;
    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    state = EnemyHurt;
    SetAnimationByIndex(4, 1, false);
    SetDirection(soft3d::Subtract(player->GetPosition(), position));
    velocity = soft3d::Scale(GetForward(),
        -soft3d::FixedMultiply(player->GetAttackBlend(), knockbackSpeed));
    return true;
}

bool Victor::BeginLeapAtPlayer(int32_t selection)
{
    if (selection <= 7000 || playerDistance < minimumLeapDistance ||
        playerDistance > maximumLeapDistance) {
        return false;
    }
    StopMotion();
    state = 6;
    leapPhase = 0;
    stateTime = 18;
    PlaySound(leapSounds[1], false);
    SetAnimation(leapAnimations[0], 1, false);
    SetAnimationSpeed(600);
    physicsFlags &= ~soft3d::PhysicKeepDirection;
    return true;
}

void Victor::UpdateIdle()
{
    if (ReceivePlayerAttack()) {
        stateTime = 0;
        return;
    }
    if (stateTime > 0) {
        --stateTime;
        return;
    }
    Enemy::UpdateIdle();
    if (playerVisible == 0) {
        return;
    }
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t selection = static_cast<int32_t>(Random(manager) % 10000);
    if (BeginLeapAtPlayer(selection)) {
        return;
    }
    if (selection < 5001) {
        if (SelectAttack() < 1) {
            stateTime = static_cast<int16_t>(Random(manager) % 16 + 16);
        }
    } else {
        BeginChase();
    }
}

void Victor::UpdateChase()
{
    Enemy::UpdateChase();
    if (moveMode != EnemyNoMovement || playerVisible == 0) {
        return;
    }
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t selection = static_cast<int32_t>(Random(manager) % 10000);
    if (BeginLeapAtPlayer(selection)) {
        return;
    }
    if (selection > 9800 && SelectAttack() < 1) {
        StopMotion();
        stateTime = static_cast<int16_t>(Random(manager) % 16 + 16);
    }
}

void Victor::UpdateLeap()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (leapPhase == 0) {
        --stateTime;
        if (stateTime > 0 && player != nullptr) {
            leapTarget = player->GetPosition();
            if (bsp != nullptr) {
                const soft3d::BspTraceResult trace = bsp->Trace(position, leapTarget, localBounds);
                if (trace.startSolid && !trace.allSolid) {
                    leapTarget = soft3d::Add(leapTarget, soft3d::Scale(trace.planeNormal, 0x28000));
                }
            }
            SetDirection(soft3d::Subtract(leapTarget, position));
        }
        if (!IsAnimationFinished()) {
            return;
        }
        SetDirection(soft3d::Subtract(leapTarget, position));
        leapPhase = 1;
        PlaySound(leapSounds[2], false);
        SetAnimation(leapAnimations[1], 1, false);
        physicsFlags &= ~soft3d::PhysicSimpleMovement;
        physicsFlags &= ~soft3d::PhysicStepOffset;
        groundTrace->startSolid = false;
    }
    if (leapPhase == 1) {
        velocity = soft3d::Add(velocity, soft3d::Scale(GetForward(), 0x6666));
        const soft3d::ObjectBounds current = Translate(localBounds, position);
        soft3d::ObjectBounds next = Translate(current, velocity);
        next.minimum.x = soft3d::SubtractFixed(next.minimum.x, soft3d::fixedOne);
        next.minimum.z = soft3d::SubtractFixed(next.minimum.z, soft3d::fixedOne);
        next.maximum.x = soft3d::AddFixed(next.maximum.x, soft3d::fixedOne);
        next.maximum.z = soft3d::AddFixed(next.maximum.z, soft3d::fixedOne);
        if (player != nullptr && player->QueryBounds(next) != nullptr) {
            player->DamageHurt2();
            const soft3d::Vec3 target = soft3d::Add(position,
                soft3d::Scale(GetForward(), 0x960000));
            soft3d::BspTraceResult trace;
            if (bsp != nullptr) {
                trace = bsp->Trace(position, target, localBounds);
            }
            soft3d::ObjectBounds impact = Translate(localBounds, trace.endPosition);
            impact.minimum.x = soft3d::SubtractFixed(impact.minimum.x, 0x11999);
            impact.minimum.z = soft3d::SubtractFixed(impact.minimum.z, 0x11999);
            impact.maximum.x = soft3d::AddFixed(impact.maximum.x, 0x11999);
            impact.maximum.z = soft3d::AddFixed(impact.maximum.z, 0x11999);
            if (Overlaps(impact, player->GetCollisionBounds())) {
                StopSound();
                velocity = {};
                leapPhase = 2;
                PlaySound(leapSounds[0], false);
                SetAnimation(leapAnimations[2], 1, false);
                return;
            }
        }
        if (groundTrace->startSolid || unitCollision) {
            StopSound();
            velocity = {};
            leapPhase = 2;
            PlaySound(leapSounds[0], false);
            SetAnimation(leapAnimations[2], 1, false);
        }
    } else if (leapPhase == 2 && IsAnimationFinished()) {
        leapPhase = 0;
        stateTime = 24;
        attackCooldown = attackInterval;
        StopMotion();
    }
}

Sam::Sam(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 15);
}

void Sam::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimationResources(*this, data, std::array<int32_t, 6>{234, 234, -1, -1, -1, -1});
}

void Sam::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 107, 0, 0, 0, 108});
}

bool Sam::UpdatePerception() { return false; }
int32_t Sam::SelectAttack() { return 0; }

void Sam::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetMaterialBlendMode(0, 0);
    }
}

void Sam::Damage(int32_t amount)
{
    Enemy::Damage(amount);
    blood.life = 0;
}

void Sam::Die()
{
    if (player != nullptr) {
        player->NotifyEnemyKilled(gunExperienceReward);
    }
    StopSound();
    PlaySound(sounds[5], false);
    deathCountdown = 16;
    state = EnemyDying;
}

void Sam::UpdateDeath()
{
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetRenderMode(soft3d::ObjectRenderMode::SecondaryStreams);
    }
    deathCountdown = static_cast<int16_t>(deathCountdown - 1);
    if (GetObject(0) != nullptr) {
        uint32_t mode = 0;
        if (deathCountdown < 5) mode = 1;
        else if (deathCountdown < 9) mode = 2;
        else if (deathCountdown < 13) mode = 3;
        GetObject(0)->SetMaterialBlendMode(0, mode);
    }
    if (deathCountdown == 0) {
        stateTime = 0;
        collisionEnabled = false;
        if (unitManager != nullptr) {
            static_cast<MyGameUnitMgr*>(unitManager)->RemoveEnemy(this);
        }
    }
}

Prisoner::Prisoner(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 11);
}

void Prisoner::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimationResources(*this, data, std::array<int32_t, 6>{196, 199, 200, 201, 202, 203});
    for (std::size_t index = 0; index < idleAnimations.size(); ++index) {
        idleAnimations[index] = LoadAnimationResource(resourceManager, data, 196 + static_cast<int32_t>(index),
                                              cacheLevel);
    }
}

void Prisoner::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 89, 90, 91, 92, 93});
}

bool Prisoner::UpdatePerception() { return false; }
int32_t Prisoner::SelectAttack() { return 0; }

void Prisoner::Update()
{
    const int32_t previousState = state;
    Enemy::Update();
    if ((previousState != state && state == EnemyIdle) ||
        (state == EnemyIdle && IsAnimationFinished())) {
        auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
        const int32_t selection = static_cast<int32_t>(Random(manager) % 30000);
        idleVariant = selection < 11000 ? 0 : selection < 19000 ? 1 : 2;
        animations[0] = idleAnimations[static_cast<std::size_t>(idleVariant)];
        SetAnimation(animations[0], 1, true);
    }
}

void Prisoner::UpdateDeath()
{
    Enemy::UpdateDeath();
    if (IsAnimationFinished()) {
        position.y = soft3d::SubtractFixed(position.y, 0x1999);
        physicsFlags &= ~soft3d::PhysicSlideMovement;
    }
    if (deathCountdown == 0 && unitManager != nullptr) {
        static_cast<MyGameUnitMgr*>(unitManager)->RemoveEnemy(this);
    }
}

Kate::Kate(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 16);
}

void Kate::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimationResources(*this, data, std::array<int32_t, 6>{235, 236, 237, -1, -1, -1});
}

void Kate::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 109, 109, 0, 0, 0});
}

bool Kate::UpdatePerception() { return true; }
int32_t Kate::SelectAttack() { return -1; }

void Kate::UpdateIdle()
{
    if (stopped == 0) {
        if (moveMode == EnemyNoMovement) {
            if (UpdatePerception() && playerDistance > 0x50000) {
                BeginChase();
            }
        } else {
            BeginChase();
        }
    } else {
        VisUnit::StopSound();
    }
}

void Kate::AlertLinkedBully()
{
    if (linkedBully == nullptr) {
        return;
    }
    const soft3d::Fixed distance = soft3d::Length(soft3d::Subtract(
        linkedBully->GetPosition(), position));
    if (distance < linkedBully->GetPerceptionRange()) {
        linkedBully->playerVisible = 1;
        return;
    }
    if (GetObject(0) != nullptr && GetObject(0)->GetObjectIntensity() < -0x1999) {
        return;
    }
    const soft3d::Vec3 direction = soft3d::Normalize(soft3d::Subtract(
        position, linkedBully->GetPosition()));
    soft3d::Object* bullyObject = linkedBully->GetObject(0);
    const soft3d::Vec3 forward = bullyObject != nullptr ? bullyObject->GetAxisZ() : soft3d::Vec3{};
    if (soft3d::Dot(direction, forward) < 0xb333) {
        return;
    }
    soft3d::Vec3 eye = linkedBully->GetPosition();
    const soft3d::ObjectBounds& bounds = linkedBully->GetLocalBounds();
    eye.y = soft3d::AddFixed(eye.y,
        soft3d::SubtractFixed(bounds.maximum.y, bounds.minimum.y) >> 1);
    eye.y = soft3d::SubtractFixed(eye.y, soft3d::fixedOne);
    if (bsp == nullptr || !bsp->Trace(eye, position).startSolid) {
        linkedBully->playerVisible = 1;
        return;
    }
    soft3d::Vec3 kateEye = position;
    kateEye.y = soft3d::AddFixed(kateEye.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    kateEye.y = soft3d::SubtractFixed(kateEye.y, soft3d::fixedOne);
    if (!bsp->Trace(eye, kateEye).startSolid) {
        linkedBully->playerVisible = 1;
    }
}

void Kate::Update()
{
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    Enemy::Update();
    if (player != nullptr && !player->IsUsedItemHealing() && linkedBully != nullptr &&
        !linkedBully->IsAttacking()) {
        AlertLinkedBully();
    }
}

DarkKen::DarkKen(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 14);
    cacheLevel = 3;
    gun = std::make_unique<soft3d::Object>(context.renderer);
    gunSecondary = std::make_unique<soft3d::Object>(context.renderer);
    collisionEnabled = false;
}

void DarkKen::InitializeAnimations(const soft3d::Animation* const*)
{
    cacheLevel = 2;
    LoadAnimationResources(*this, data, std::array<int32_t, 6>{25, 26, 26, 30, 33, 35});
    cacheLevel = 3;
    attackAnimations[0] = LoadAnimationResource(resourceManager, data, 31, 2);
    attackAnimations[1] = LoadAnimationResource(resourceManager, data, 32, 2);
    static constexpr std::array<int32_t, 8> extra{68, 69, 69, 73, 76, 78, 74, 75};
    for (std::size_t index = 0; index < extraAnimations.size(); ++index) {
        extraAnimations[index] = LoadAnimationResource(resourceManager, data, extra[index], 2);
    }
}

void DarkKen::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 21, 21, 29, 30, 31});
    attackSounds[0] = LoadSound(resourceManager, data, 24, cacheLevel);
    attackSounds[1] = LoadSound(resourceManager, data, 28, cacheLevel);
}

const soft3d::Animation* DarkKen::GetExtraAnimation(std::size_t index) const
{
    return index < extraAnimations.size() ? extraAnimations[index] : nullptr;
}

soft3d::AudioData* DarkKen::GetExtraSound(std::size_t index) const
{
    return index < attackSounds.size() ? attackSounds[index] : nullptr;
}

soft3d::Object* DarkKen::GetGunObject(std::size_t index) const
{
    return index == 0 ? gun.get() : index == 1 ? gunSecondary.get() : nullptr;
}

void DarkKen::QueryUnitCollision()
{
    PhysicUnit::QueryUnitCollision();
}

void DarkKen::InitializeRegions()
{
    static constexpr std::array<soft3d::ObjectBounds, 16> initial{{
        {{-0x4c0000, 0x10000, 0xf0000}, {-0x2a0000, 0x20000, 0x310000}},
        {{-0x80000, 0x10000, 0xf0000}, {0x1a0000, 0x20000, 0x310000}},
        {{-0x4c0000, 0x10000, -0x350000}, {-0x2a0000, 0x20000, -0x130000}},
        {{-0x80000, 0x10000, -0x350000}, {0x1a0000, 0x20000, -0x130000}},
        {{-0x2a0000, 0x10000, -0x20000}, {-0x190000, 0x20000, 0xf0000}},
        {{-0x190000, 0x10000, -0x20000}, {-0x80000, 0x20000, 0xf0000}},
        {{-0x2a0000, 0x10000, -0x130000}, {-0x190000, 0x20000, -0x20000}},
        {{-0x190000, 0x10000, -0x130000}, {-0x80000, 0x20000, -0x20000}},
        {{-0x2a0000, 0x10000, 0xf0000}, {-0x190000, 0x20000, 0x310000}},
        {{-0x190000, 0x10000, 0xf0000}, {-0x80000, 0x20000, 0x310000}},
        {{-0x4c0000, 0x10000, -0x20000}, {-0x2a0000, 0x20000, 0xf0000}},
        {{-0x4c0000, 0x10000, -0x130000}, {-0x2a0000, 0x20000, -0x20000}},
        {{-0x80000, 0x10000, -0x20000}, {0x1a0000, 0x20000, 0xf0000}},
        {{-0x80000, 0x10000, -0x130000}, {0x1a0000, 0x20000, -0x20000}},
        {{-0x2a0000, 0x10000, -0x350000}, {-0x190000, 0x20000, -0x130000}},
        {{-0x190000, 0x10000, -0x350000}, {-0x80000, 0x20000, -0x130000}},
    }};
    regions = initial;
    for (std::size_t region = 0; region < regions.size(); ++region) {
        struct Neighbor { soft3d::Fixed distance; int32_t index; };
        std::array<Neighbor, 15> neighbors{};
        std::size_t count = 0;
        for (std::size_t other = 0; other < regions.size(); ++other) {
            if (other != region) {
                neighbors[count++] = {soft3d::Length(soft3d::Subtract(
                    Center(regions[region]), Center(regions[other]))), static_cast<int32_t>(other)};
            }
        }
        std::sort(neighbors.begin(), neighbors.end(), [](const Neighbor& left, const Neighbor& right) {
            return left.distance < right.distance;
        });
        for (std::size_t index = 0; index < neighbors.size(); ++index) {
            regionNeighbors[region][index] = static_cast<uint8_t>(neighbors[index].index);
        }
    }
}

void DarkKen::Spawn(const soft3d::Vec3& value)
{
    if (player != nullptr) {
        player->LoadWeaponAnimations(data, 1);
        player->LoadAlternateObject(u"r_ken_shadow_a");
    }
    LoadResources(u"r_ken_a", nullptr, nullptr);
    if (resourceManager != nullptr) {
        gun->Load(resourceManager, u"ow_qiang_a", 2);
        gun->LoadMaterials(resourceManager, 2);
        gunSecondary->Load(resourceManager, u"ow_qiang_a", 2);
    }
    gunSecondary->SetTransformMode(2);
    Enemy::Spawn(value);
    for (std::size_t index = 0; index < GetObjectCount(); ++index) {
        GetObject(index)->LoadTextureOverride(0, u"dj_evilken_b", cacheLevel);
    }
    InitializeRegions();
    currentRegion = -1;
    recentRegions.fill(-1);
    regionTarget = position;
    attackCooldown = 0;
    tauntCooldown = 16;
    stateTime = 0;
    unused7ca = 0;
}

const soft3d::ObjectBounds& DarkKen::GetRegion(std::size_t index) const
{
    static constexpr soft3d::ObjectBounds empty{};
    return index < regions.size() ? regions[index] : empty;
}

int32_t DarkKen::GetRegionNeighbor(std::size_t region, std::size_t index) const
{
    return region < regionNeighbors.size() && index < regionNeighbors[region].size()
        ? regionNeighbors[region][index] : -1;
}

void DarkKen::SetRegionTarget(int32_t index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= regions.size()) {
        return;
    }
    const soft3d::Vec3 center = Center(regions[static_cast<std::size_t>(index)]);
    regionTarget = {center.x, position.y, center.z};
}

void DarkKen::ChooseNextRegion()
{
    soft3d::Vec3 point = position;
    point.y = soft3d::AddFixed(point.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    int32_t enemyRegion = 0;
    while (enemyRegion < 16 && !Contains(regions[static_cast<std::size_t>(enemyRegion)], point)) {
        ++enemyRegion;
    }
    if (player == nullptr) {
        return;
    }
    point = player->GetPosition();
    point.y = soft3d::AddFixed(point.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    int32_t playerRegion = 0;
    while (playerRegion < 16 && !Contains(regions[static_cast<std::size_t>(playerRegion)], point)) {
        ++playerRegion;
    }
    if (playerRegion >= 16) {
        return;
    }
    int32_t rank = 0;
    while (rank < 15) {
        const int32_t candidate = regionNeighbors[static_cast<std::size_t>(playerRegion)]
                                                 [static_cast<std::size_t>(rank)];
        if (candidate == enemyRegion) {
            return;
        }
        if (std::find(recentRegions.begin(), recentRegions.end(), candidate) == recentRegions.end()) {
            SetRegionTarget(candidate);
            if (currentRegion != candidate) {
                recentRegions[2] = recentRegions[1];
                recentRegions[1] = recentRegions[0];
                recentRegions[0] = currentRegion;
                currentRegion = candidate;
            }
            return;
        }
        ++rank;
    }
    SetRegionTarget(regionNeighbors[static_cast<std::size_t>(playerRegion)][0]);
}

void DarkKen::Update()
{
    if (state >= EnemyDead) {
        StopSound();
        return;
    }
    if (IsNavigationEnabled()) {
        if (GetObject(0) != nullptr) GetObject(0)->SetPosition(position);
        if (objectGroup != nullptr) {
            for (std::size_t index = 0; index < GetObjectCount(); ++index) {
                GetObject(index)->UpdateTransform();
            }
        }
        UpdateShadow();
        return;
    }
    UpdatePerception();
    ChooseNextRegion();
    const int32_t previousState = state;
    Enemy::Update();
    if (previousState != EnemyIdle && state == EnemyIdle) {
        auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
        tauntCooldown = static_cast<int32_t>((Random(manager) % 4096) >> 8) + 16;
    }
    if (gun != nullptr && gunSecondary != nullptr && GetObject(0) != nullptr) {
        for (soft3d::Object* object : {gun.get(), gunSecondary.get()}) {
            object->SetPosition(position);
            object->SetAxisX(GetObject(0)->GetAxisX());
            object->SetAxisY(GetObject(0)->GetAxisY());
            object->SetAxisZ(GetObject(0)->GetAxisZ());
        }
        gun->UpdateTransform();
        gunSecondary->SetModelMatrix(gun->GetModelMatrix());
    }
}

void DarkKen::PostUpdate()
{
    VisUnit::PostUpdate();
    if (GetObject(0) != nullptr) {
        gun->AdvanceAnimation();
        gunSecondary->AdvanceAnimation();
    }
}

void DarkKen::SubmitRenderables(soft3d::RenderQueues& queues)
{
    Enemy::SubmitRenderables(queues);
    if (gun == nullptr || gunSecondary == nullptr || GetObject(0) == nullptr) return;
    gun->sortKey = GetObject(0)->sortKey;
    gunSecondary->sortKey = GetObject(0)->sortKey;
    gunSecondary->SetSecondaryTransform(GetObject(0)->GetSecondaryTransform());
    gun->UpdateFrustumVisibility();
    if (!gun->IsFrustumCulled()) {
        queues.Submit(gun.get(), 3);
        queues.Submit(gunSecondary.get(), 2);
    }
}

void DarkKen::SetNavigationEnabled(bool enabled)
{
    if (state < EnemyDying) {
        Creature::SetNavigationEnabled(enabled);
    }
}

void DarkKen::UpdateIdle()
{
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    if (ReceivePlayerAttack()) {
        return;
    }
    if (SelectAttack() > 0) {
        BeginAttack();
        return;
    }
    const soft3d::Fixed distance = soft3d::SquaredLength(soft3d::Subtract(position, regionTarget));
    if (distance < 0 || distance >= chaseSpeed) {
        BeginChase();
    }
    if (--tauntCooldown < 1) {
        PlaySound(sounds[0], false);
        auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
        tauntCooldown = static_cast<int32_t>((Random(manager) % 3072) >> 8) + 20;
    }
}

void DarkKen::UpdateChase()
{
    SetNavigationSpeed(chaseSpeed);
    if (ReceivePlayerAttack()) {
        return;
    }
    if (SelectAttack() > 0) {
        StopMotion();
        BeginAttack();
    } else if (NavigateTo(regionTarget, chaseSpeed)) {
        StopMotion();
    }
}

int32_t DarkKen::SelectAttack()
{
    if (playerDistance < attackRange || attackCooldown > 0 || player == nullptr) {
        return 0;
    }
    soft3d::Vec3 eye = position;
    eye.y = soft3d::SubtractFixed(eye.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    soft3d::Vec3 target = player->GetPosition();
    target.y = eye.y;
    return bsp == nullptr || !bsp->Trace(eye, target).startSolid ? 1 : 0;
}

bool DarkKen::TestAttackHit()
{
    if (attackHitDone != 0 || stateTime != 1 || unused7ca != 0 || player == nullptr) {
        return false;
    }
    soft3d::Vec3 start = position;
    const soft3d::Fixed halfHeight =
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1;
    start.y = soft3d::SubtractFixed(start.y, soft3d::SubtractFixed(halfHeight, 0x18000));
    const soft3d::Vec3 forward = GetForward();
    const soft3d::Vec3 near = soft3d::Add(start, soft3d::Scale(forward,
        soft3d::FixedMultiply(0x20000, playerDistance)));
    if (!player->IsSegmentInside(start, near)) {
        return false;
    }
    const soft3d::Vec3 far = soft3d::Add(start, soft3d::Scale(forward,
        soft3d::AddFixed(playerDistance, 0x50000)));
    if (bsp != nullptr && bsp->Trace(start, far).startSolid) {
        return false;
    }
    attackHitDone = 1;
    player->DamageFrom(this);
    return true;
}

void DarkKen::UpdateAttack()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (stateTime == 0 && player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    TestAttackHit();
    if (!IsAnimationFinished()) {
        return;
    }
    if (stateTime == 0) {
        PlaySound(attackSounds[0], false);
        SetAnimation(attackAnimations[0], 1, false);
        gun->SetAnimation(extraAnimations[6]);
        gun->SetAnimationPlayback(1, false);
        gunSecondary->SetAnimation(extraAnimations[6]);
        gunSecondary->SetAnimationPlayback(1, false);
        stateTime = 1;
    } else if (stateTime == 1 && unused7ca == 0) {
        SetAnimation(attackAnimations[1], 1, false);
        gun->SetAnimation(extraAnimations[7]);
        gun->SetAnimationPlayback(1, false);
        gunSecondary->SetAnimation(extraAnimations[7]);
        gunSecondary->SetAnimationPlayback(1, false);
        unused7ca = 1;
    } else if (stateTime == 1 && unused7ca == 1) {
        stateTime = 0;
        SetAnimationByIndex(0, 1, true);
        attackCooldown = attackInterval;
        state = EnemyIdle;
        knockbackPending = 0;
    }
}

void DarkKen::SetAnimationByIndex(int32_t index, int32_t direction, bool loop)
{
    Enemy::SetAnimationByIndex(index, direction, loop);
    if (index >= 0 && static_cast<std::size_t>(index) < extraAnimations.size()) {
        gun->SetAnimation(extraAnimations[static_cast<std::size_t>(index)]);
        gun->SetAnimationPlayback(direction, loop);
        gunSecondary->SetAnimation(extraAnimations[static_cast<std::size_t>(index)]);
        gunSecondary->SetAnimationPlayback(direction, loop);
    }
    if (index == 2) {
        SetAnimationSpeed(450);
        gun->SetAnimationSpeed(450);
        gunSecondary->SetAnimationSpeed(450);
    }
}

std::unique_ptr<Enemy> CreateNamedPoolEnemy(const soft3d::UnitManagerContext& context,
                                            const GameData& data, int32_t enemyType)
{
    switch (enemyType) {
    case 8: return std::make_unique<Lord>(context, data);
    case 11: return std::make_unique<Prisoner>(context, data);
    case 12: return std::make_unique<Victor>(context, data);
    case 14: return std::make_unique<DarkKen>(context, data);
    case 15: return std::make_unique<Sam>(context, data);
    case 16: return std::make_unique<Kate>(context, data);
    default: return nullptr;
    }
}

} // namespace game

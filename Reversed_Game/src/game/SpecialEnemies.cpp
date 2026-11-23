#include "game/SpecialEnemies.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <climits>

namespace game {
namespace {

void Configure(Enemy& enemy, int32_t type)
{
    enemy.SetEnemyType(type);
    enemy.ConfigureCommonDefaults(GetEnemyCommonConfig(type));
    enemy.SetPositionalAudio(GetEnemyCommonConfig(type).positionalAudio);
}

template <std::size_t N>
void LoadAnimations(Enemy& enemy, const GameData& data,
                    const std::array<int32_t, N>& names)
{
    for (std::size_t index = 0; index < names.size(); ++index) {
        enemy.SetAnimationResource(static_cast<int32_t>(index),
                                   names[index] < 0 ? nullptr : data.GetAnimationName(
                                       static_cast<std::size_t>(names[index])));
    }
}

template <std::size_t N>
void LoadSounds(Enemy& enemy, const GameData& data, const std::array<int32_t, N>& names)
{
    for (std::size_t index = 0; index < names.size(); ++index) {
        enemy.SetSoundResource(static_cast<int32_t>(index),
                               names[index] <= 0 ? nullptr : data.GetSoundName(
                                   static_cast<std::size_t>(names[index])));
    }
}

void ExpandBounds(soft3d::ObjectBounds& bounds, const soft3d::Vec3& point)
{
    bounds.minimum.x = std::min(bounds.minimum.x, point.x);
    bounds.minimum.y = std::min(bounds.minimum.y, point.y);
    bounds.minimum.z = std::min(bounds.minimum.z, point.z);
    bounds.maximum.x = std::max(bounds.maximum.x, point.x);
    bounds.maximum.y = std::max(bounds.maximum.y, point.y);
    bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

} // namespace

Worm::Worm(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 10);
}

void Worm::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimations(*this, data, std::array<int32_t, 6>{190, 191, 192, 193, 194, 195});
}

void Worm::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 83, 84, 85, 86, 87});
}

void Worm::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    attackPathClear = false;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const uint32_t speedRandom = manager != nullptr ? manager->NextRandom() : 0;
    chaseSpeed = 0x3333 + static_cast<int32_t>(speedRandom % 0x3333);
    const uint32_t scaleRandom = manager != nullptr ? manager->NextRandom() : 0;
    const soft3d::Fixed scale = 0x10000 + static_cast<int32_t>(scaleRandom % 80000);
    if (primaryObject != nullptr) {
        primaryObject->ResetTransform();
        primaryObject->Scale(scale, soft3d::fixedOne, scale);
        primaryObject->UpdateTransform();
    }
    const uint32_t colorRandom = manager != nullptr ? manager->NextRandom() : 0;
    const int32_t selection = static_cast<int32_t>(colorRandom % 30000);
    colorVariant = selection < 11000 ? 0 : selection < 19000 ? 1 : 2;
    const char16_t* texture = colorVariant == 0 ? u"dk_bug_a" : colorVariant == 1 ? u"dk_bug_b" : u"dk_bug_c";
    for (std::size_t index = 0; index < GetObjectCount(); ++index) {
        GetObject(index)->LoadTextureOverride(0, texture, cacheLevel);
    }
}

void Worm::Update()
{
    Enemy::Update();
    if (IsNavigationEnabled()) {
        return;
    }
    attackPathClear = true;
    if (player == nullptr) {
        attackPathClear = false;
        return;
    }
    const soft3d::Fixed vertical = soft3d::AbsFixed(
        soft3d::SubtractFixed(player->GetPosition().y, position.y));
    if (vertical > 0x50000) {
        attackPathClear = false;
        return;
    }
    if (bsp != nullptr && bsp->Trace(position, player->GetPosition(), localBounds).startSolid) {
        attackPathClear = false;
        return;
    }

    soft3d::ObjectBounds bounds = collisionBounds;
    const soft3d::Fixed enemyHalfWidth =
        soft3d::SubtractFixed(localBounds.maximum.x, localBounds.minimum.x) >> 1;
    const soft3d::ObjectBounds& playerBounds = player->GetLocalBounds();
    const soft3d::Fixed playerHalfWidth =
        soft3d::SubtractFixed(playerBounds.maximum.x, playerBounds.minimum.x) >> 1;
    const soft3d::Fixed behindDistance = soft3d::SubtractFixed(
        soft3d::AddFixed(enemyHalfWidth, playerHalfWidth), soft3d::fixedOne);
    const soft3d::Vec3 behind = soft3d::Subtract(player->GetPosition(),
        soft3d::Scale(GetForward(), behindDistance));
    ExpandBounds(bounds, soft3d::Add(playerBounds.minimum, behind));
    ExpandBounds(bounds, soft3d::Add(playerBounds.maximum, behind));
    if (unitManager != nullptr) {
        if (unitManager->QueryBounds(bounds, 0, this).count > 0 ||
            unitManager->QueryBounds(bounds, 2, this).count > 0) {
            attackPathClear = false;
        }
    }
}

void Worm::UpdateAlert()
{
    if (playerDistance <= hitDistance && attackPathClear) {
        attackCooldown = 0;
    }
    Enemy::UpdateAlert();
}

int32_t Worm::SelectAttack()
{
    if (stopped != 0 || moveMode != EnemyNoMovement || !IsAttacking() || !attackPathClear ||
        playerDistance > attackRange) {
        return 0;
    }
    return attackCooldown > 0 ? -1 : 1;
}

void Worm::BeginAttack()
{
    Enemy::BeginAttack();
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicStepOffset;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
}

bool Worm::TestAttackHit()
{
    if (attackHitDone != 0) {
        return false;
    }
    const bool hit = unitQueryResult.count > 0 &&
        unitQueryResult.units[static_cast<std::size_t>(unitQueryResult.count - 1)] == player;
    if (hit) {
        attackHitDone = 1;
    }
    return hit;
}

void Worm::UpdateAttack()
{
    if (ReceivePlayerAttack()) {
        velocity = {};
        SnapToGround();
        physicsFlags &= ~soft3d::PhysicKeepDirection;
        physicsFlags &= ~soft3d::PhysicSeparateAxisCollision;
        physicsFlags |= soft3d::PhysicStepOffset | soft3d::PhysicSimpleMovement;
        return;
    }

    stateTime = static_cast<int16_t>(stateTime + 1);
    if (stateTime == 14) {
        velocity = soft3d::Scale(GetForward(), 0x20000);
        velocity.y = 0xcccc;
        physicsFlags |= soft3d::PhysicSeparateAxisCollision;
        groundTrace->startSolid = false;
        return;
    }
    if (unused7ca == 0 && primaryObject != nullptr && primaryObject->GetAnimationTime() > 0x12c0) {
        primaryObject->SetAnimationTime(0x12c0);
    }
    if (TestAttackHit()) {
        velocity.x = soft3d::FixedMultiply(-GetForward().x, 0x1999);
        velocity.z = soft3d::FixedMultiply(-GetForward().z, 0x1999);
        if (player != nullptr) {
            player->DamageFrom(this);
        }
    }
    if (stateTime > 14 && velocity.y <= 0 && groundTrace->startSolid) {
        velocity = {};
        physicsFlags &= ~soft3d::PhysicSeparateAxisCollision;
        if (unused7ca == 0 && primaryObject != nullptr) {
            primaryObject->SetAnimationTime(0x13ec);
        }
        unused7ca = static_cast<int16_t>(unused7ca + 1);
    }
    if (IsAnimationFinished()) {
        physicsFlags &= ~soft3d::PhysicKeepDirection;
        physicsFlags |= soft3d::PhysicStepOffset | soft3d::PhysicSimpleMovement;
        SetAnimationByIndex(0, 1, true);
        state = EnemyIdle;
        attackCooldown = attackInterval;
    }
}

void Worm::UpdateDeath()
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

Ghost::Ghost(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 3);
}

void Ghost::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 45, 46, 47, 48, 49});
    proximitySound = resourceManager != nullptr
        ? dynamic_cast<soft3d::AudioData*>(resourceManager->GetResource(
              soft3d::ResourceAudio, cacheLevel, data.GetSoundName(50), u".\\audio\\"))
        : nullptr;
}

void Ghost::SubmitRenderables(soft3d::RenderQueues&)
{
}

void Ghost::StopProximitySound()
{
    if (proximityVoice != -1) {
        if (audioManager != nullptr) {
            audioManager->StopVoice(proximityVoice);
        }
        proximityVoice = -1;
    }
}

void Ghost::StopSound()
{
    StopProximitySound();
    Enemy::StopSound();
}

void Ghost::SetNavigationEnabled(bool enabled)
{
    if (!IsNavigationEnabled() && enabled) {
        StopProximitySound();
    }
    Enemy::SetNavigationEnabled(enabled);
}

void Ghost::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    proximityVoice = -1;
}

void Ghost::Update()
{
    if (state > EnemyDying) {
        StopProximitySound();
        return;
    }
    if (player == nullptr) {
        Enemy::Update();
        return;
    }
    localBounds = player->GetLocalBounds();
    localBounds.minimum.x = -soft3d::fixedOne;
    localBounds.maximum.x = soft3d::fixedOne;
    localBounds.minimum.z = -soft3d::fixedOne;
    localBounds.maximum.z = soft3d::fixedOne;
    Enemy::Update();
    if (IsNavigationEnabled() || player->GetScene() == nullptr ||
        player->GetScene()->GetFadeShader()->IsActive()) {
        return;
    }

    Scene* scene = player->GetScene();
    soft3d::TwistShader* twist = scene->GetTwistShader();
    if (playerDistance <= 0xa0000) {
        scene->GetEngine().GetRenderer().SelectShader(twist);
        twist->SetAmplitude(2);
    } else if (playerDistance <= 0x140000) {
        scene->GetEngine().GetRenderer().SelectShader(twist);
        twist->SetAmplitude(1);
        if (proximityVoice == -1 && audioManager != nullptr) {
            proximityVoice = audioManager->PlaySound(proximitySound, true);
        }
    } else {
        scene->GetEngine().GetRenderer().SelectShader(nullptr);
        StopProximitySound();
    }
}

bool Ghost::ReceivePlayerAttack()
{
    return false;
}

void Ghost::UpdateAttack()
{
    stateTime = static_cast<int16_t>(stateTime + 1);
    TestAttackHit();
    if (stateTime == 20) {
        stateTime = 0;
        attackCooldown = attackInterval;
        state = EnemyIdle;
    }
}

Lamper::Lamper(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 4);
}

void Lamper::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimations(*this, data, std::array<int32_t, 6>{110, 111, 112, 113, 114, 115});
}

void Lamper::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 52, 53, 54, 55, 56});
}

void Lamper::Spawn(const soft3d::Vec3& value)
{
    position = value;
    previousPosition = position;
    SetAnimationByIndex(0, 1, true);
    SetAnimationByIndex(0, 1, true);
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
    waypoints.clear();
    currentWaypoint = -1;
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
    collisionEnabled = false;
    ResetNavigationPathNodes();
    if (primaryObject != nullptr) {
        primaryObject->SetRenderMode(soft3d::ObjectRenderMode::Normal);
    }
    VisUnit::StopSound();
    physicsFlags = 0xb;
    groundTrace->startSolid = false;
}

bool Lamper::UpdatePerception()
{
    if (state >= EnemyDying || player == nullptr || player->IsHiddenOrFPS() ||
        playerDistance >= perceptionRange) {
        playerVisible = 0;
        return false;
    }
    playerVisible = 1;
    return true;
}

void Lamper::UpdateIdle()
{
    UpdatePerception();
    if (!ReceivePlayerAttack() && SelectAttack() > 0) {
        BeginAttack();
    }
}

void Lamper::BeginAttack()
{
    VisUnit::StopSound();
    waitTimer = -1;
    stateTime = 0;
    unused7ca = 0;
    state = EnemyAttack;
    velocity = {};
    SetAnimationByIndex(3, 1, false);
    PlaySound(sounds[3], false);
    attackHitDone = 0;
}

bool Lamper::TestAttackHit()
{
    if (attackHitDone != 0 || stateTime < static_cast<int16_t>(attackStart) ||
        stateTime > static_cast<int16_t>(attackEnd)) {
        return false;
    }
    soft3d::Vec3 point = position;
    point.y = soft3d::AddFixed(point.y,
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1);
    const bool hit = player != nullptr && player->IsPointInside(point);
    if (hit) {
        attackHitDone = 1;
        player->DamageFrom(this);
    }
    return hit;
}

bool Lamper::ReceivePlayerAttack()
{
    if (hitCooldown != 0 || player == nullptr) {
        return false;
    }
    collisionEnabled = true;
    const int32_t damage = player->GetAttackDamage(this);
    collisionEnabled = false;
    if (damage == 0) {
        return false;
    }
    VisUnit::StopSound();
    health -= damage;
    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    Die();
    stateTime = 0;
    velocity = {};
    return true;
}

void Lamper::Damage(int32_t amount)
{
    Enemy::Damage(amount);
    blood.life = 0;
}

Weed::Weed(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    Configure(*this, 5);
}

void Weed::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadAnimations(*this, data, std::array<int32_t, 6>{116, 117, 118, 119, 120, 121});
}

void Weed::InitializeSounds(soft3d::AudioData* const*)
{
    LoadSounds(*this, data, std::array<int32_t, 6>{0, 58, 59, 60, 61, 62});
}

bool Weed::UpdatePerception()
{
    if (playerVisible != 0) {
        return true;
    }
    if (state >= EnemyDying || player == nullptr || player->IsHiddenOrFPS() ||
        playerDistance >= perceptionRange) {
        return false;
    }
    playerVisible = 1;
    return true;
}

void Weed::Damage(int32_t amount)
{
    Enemy::Damage(amount);
    blood.life = 0;
}

bool Weed::ReceivePlayerAttack()
{
    if (hitCooldown != 0 || player == nullptr) {
        return false;
    }
    const int32_t damage = player->GetAttackDamage(this);
    if (damage == 0) {
        return false;
    }
    VisUnit::StopSound();
    health -= damage;
    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    if (health < 1) {
        Die();
    } else {
        auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
        const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom() % 1000) : 0;
        if (knockbackChance < random) {
            state = EnemyHurt;
            SetAnimationByIndex(4, 1, false);
            PlaySound(sounds[4], false);
        } else {
            knockbackPending = 1;
            state = EnemyKnockback;
            SetAnimationByIndex(4, 1, false);
            SetAnimationSpeed(0x4b0);
            PlaySound(sounds[4], false);
        }
    }
    stateTime = 0;
    SetDirection(soft3d::Subtract(player->GetPosition(), position));
    velocity = soft3d::Scale(GetForward(),
        -soft3d::FixedMultiply(player->GetAttackBlend(), knockbackSpeed));
    if (knockbackPending != 0) {
        velocity = soft3d::Scale(velocity, 0x8000);
    }
    hitCooldown = 8;
    return true;
}

std::unique_ptr<Enemy> CreateSpecialPoolEnemy(const soft3d::UnitManagerContext& context,
                                              const GameData& data, int32_t enemyType)
{
    switch (enemyType) {
    case 3: return std::make_unique<Ghost>(context, data);
    case 4: return std::make_unique<Lamper>(context, data);
    case 5: return std::make_unique<Weed>(context, data);
    case 10: return std::make_unique<Worm>(context, data);
    default: return nullptr;
    }
}

} // namespace game

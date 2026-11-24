#include "game/MaidBully.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/s3dResManager.hpp"

#include <climits>

namespace game {
namespace {

} // namespace

Maid::Maid(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 0);
}

void Maid::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{87, 88, 89, 90, 91, 92});
}

void Maid::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 33, 34, 35, 36, 37});
}

Maid2::Maid2(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 7);
}

void Maid2::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{130, 131, 132, 133, 134, 135});
}

void Maid2::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 71, 72, 73, 74, 75});
}

Maid3::Maid3(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 9);
}

void Maid3::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{184, 185, 186, 187, 188, 189});
}

void Maid3::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 77, 78, 79, 80, 81});
}

void Maid3::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    chaseSpeed = 0x1999;
    if (unitManager != nullptr) {
        chaseSpeed += static_cast<int32_t>(static_cast<MyGameUnitMgr*>(unitManager)->NextRandom() %
                                           0x199a);
    }
}

void Maid3::UpdateDeath()
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

FatMaid::FatMaid(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 6);
}

void FatMaid::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{122, 123, 124, 125, 128, 129});
    throwAnimation = LoadEnemyAnimationResource(resourceManager, data, 126, cacheLevel);
    projectileAnimation = LoadEnemyAnimationResource(resourceManager, data, 127, cacheLevel);
}

void FatMaid::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 64, 65, 66, 67, 68});
    projectileSound = LoadEnemySoundResource(resourceManager, data, 69, cacheLevel);
}

void FatMaid::LoadResources(const char16_t* modelName,
                            const soft3d::Animation* const* animationNames,
                            soft3d::AudioData* const* soundNames)
{
    Enemy::LoadResources(modelName, animationNames, soundNames);
    projectile = std::make_unique<soft3d::Object>(renderer);
    projectile->SetTransformMode(0);
    if (resourceManager != nullptr) {
        projectile->Load(resourceManager, u"ow_shuiguo_a", 2);
        projectile->LoadMaterials(resourceManager, 2);
    }
    projectile->SetLightingEnabled(false);
}

void FatMaid::SubmitRenderables(soft3d::RenderQueues& queues)
{
    Enemy::SubmitRenderables(queues);
    if (projectileState != 0 && projectile != nullptr) {
        queues.Submit(projectile.get(), 3);
    }
}

void FatMaid::Update()
{
    if (state > EnemyDying) {
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
    UpdatePerception();
    if (state == 6) {
        UpdateThrow();
    }
    Enemy::Update();
    if (projectileState != 0) {
        UpdateProjectile();
    }
}

void FatMaid::PostUpdate()
{
    VisUnit::PostUpdate();
    if (projectileState != 0 && projectile != nullptr) {
        projectileAnimationFinished = projectile->AdvanceAnimation() ? 1 : 0;
    }
}

void FatMaid::Spawn(const soft3d::Vec3& value)
{
    Enemy::Spawn(value);
    throwCooldown = 0;
    projectileState = 0;
}

void FatMaid::Damage(int32_t)
{
}

bool FatMaid::ReceivePlayerAttack()
{
    return false;
}

bool FatMaid::CanThrowProjectile() const
{
    if (player == nullptr || projectile == nullptr) {
        return false;
    }
    if (playerDistance < soft3d::FixedMultiply(hitDistance, 0x18000) ||
        playerDistance > projectileRange) {
        return false;
    }
    soft3d::Vec3 start = position;
    start = soft3d::Subtract(start, soft3d::Scale(projectile->GetAxisZ(), 0x23333));
    start = soft3d::Add(start, soft3d::Scale(projectile->GetAxisY(), 0x1cccc));
    start = soft3d::Add(start, soft3d::Scale(projectile->GetAxisX(), soft3d::fixedOne));
    if (bsp == nullptr) {
        return true;
    }
    return !bsp->Trace(start, player->GetPosition()).startSolid;
}

void FatMaid::UpdateChase()
{
    SetNavigationSpeed(chaseSpeed);
    if (ReceivePlayerAttack()) {
        return;
    }
    if (stopped != 0 || stayBoxAttached != 0) {
        StopMotion();
        return;
    }
    if (moveMode != EnemyNoMovement) {
        if (NavigateTo(moveTarget, chaseSpeed)) {
            StopMotion();
        }
        return;
    }
    if (stayBoxOutside != 0 || player == nullptr) {
        return;
    }
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
    if (throwCooldown > 0) {
        --throwCooldown;
    } else if (CanThrowProjectile()) {
        auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
        const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
        if (random % 1000 > 400) {
            StopMotion();
            state = 6;
            SetAnimation(throwAnimation, 1, false);
            throwCooldown = 0;
            return;
        }
        const int32_t delay = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
        throwCooldown = ((delay % 3072) >> 8) + 12;
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

void FatMaid::UpdateThrow()
{
    if (throwCooldown <= 16 && player != nullptr && projectile != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
        projectileStart = position;
        projectileStart = soft3d::Subtract(projectileStart,
            soft3d::Scale(projectile->GetAxisZ(), 0x23333));
        projectileStart = soft3d::Add(projectileStart,
            soft3d::Scale(projectile->GetAxisY(), 0x1cccc));
        projectileStart = soft3d::Add(projectileStart,
            soft3d::Scale(projectile->GetAxisX(), soft3d::fixedOne));
        projectileTarget = player->GetPosition();
    }
    ++throwCooldown;
    if (throwCooldown == 22 && projectile != nullptr) {
        PlaySound(sounds[3], false);
        projectileState = 1;
        projectile->SetPosition(projectileStart);
        projectileVelocity = soft3d::Subtract(projectileTarget, projectileStart);
        projectileVelocity.y = 0;
        projectileVelocity = soft3d::Scale(soft3d::Normalize(projectileVelocity), 0x2cccc);
        projectile->SetAnimation(projectileAnimation);
        projectile->SetAnimationPlayback(1, false);
        projectile->SetAnimationSpeed(0);
    }
    if (IsAnimationFinished()) {
        StopMotion();
        throwCooldown = 0;
    }
}

void FatMaid::FinishProjectile()
{
    if (projectile != nullptr) {
        projectile->SetAnimationSpeed(300);
    }
    projectileState = 2;
}

void FatMaid::UpdateProjectile()
{
    if (projectile == nullptr) {
        projectileState = 0;
        return;
    }
    if (projectileState == 2) {
        if (projectileAnimationFinished == 1) {
            projectileState = 0;
        }
        return;
    }

    const soft3d::Vec3 oldPosition = projectile->GetPosition();
    projectile->SetPosition(soft3d::Add(oldPosition, projectileVelocity));
    projectileVelocity = soft3d::Scale(projectileVelocity, 0xf333);
    projectileVelocity.y = soft3d::SubtractFixed(projectileVelocity.y, 0x1999);
    projectile->UpdateTransform();
    const soft3d::ObjectBounds bounds = Translate(projectile->GetPrimaryBounds(),
                                                  projectile->GetPosition());
    if (player != nullptr && player->QueryBounds(bounds) != nullptr) {
        PlaySound(projectileSound, false);
        FinishProjectile();
        attackPower = 15;
        player->DamageFrom(this);
        attackPower = 25;
        return;
    }

    if (bsp != nullptr) {
        const soft3d::BspTraceResult trace = bsp->Trace(oldPosition, projectile->GetPosition(),
                                                        projectile->GetPrimaryBounds());
        if (trace.startSolid) {
            PlaySound(projectileSound, false);
            projectile->SetPosition(trace.endPosition);
            FinishProjectile();
            auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
            const bool flipU = manager != nullptr && manager->NextRandom() % 1000 <= 500;
            const bool flipV = manager != nullptr && manager->NextRandom() % 1000 <= 500;
            const bool selectB = manager == nullptr || manager->NextRandom() % 1000 <= 555;
            bsp->BlendLightmapSplat(selectB ? blood.textureB : blood.textureA, oldPosition,
                soft3d::Add(oldPosition, soft3d::Scale(projectileVelocity, 0x40000)), flipU, flipV);
            return;
        }
    }

    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    if (manager != nullptr) {
        for (int bucket : {0, 2}) {
            for (soft3d::GameUnit* unit = manager->GetFirstUnit(bucket); unit != nullptr;
                 unit = unit->GetNextUnit()) {
                if (unit != this && unit->QueryBounds(bounds) != nullptr) {
                    PlaySound(projectileSound, false);
                    FinishProjectile();
                    return;
                }
            }
        }
    }
}

Bully::Bully(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 1);
}

void Bully::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{93, 94, 95, 96, 97, 98});
}

void Bully::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 39, 40, 41, 42, 43});
}

bool Bully::TestAttackHit()
{
    return TestAttackHitVolume(attackStart, attackEnd, secondAttackStart, secondAttackEnd,
                               soft3d::fixedOne);
}

BullyDay3::BullyDay3(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 2);
    cacheLevel = 3;
    SetPositionalAudio(false);
}

void BullyDay3::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{93, 94, 95, 96, 97, 98});
    static constexpr std::array<int32_t, 11> names{99, 100, 101, 102, 103, 104,
                                                   105, 106, 107, 108, 109};
    for (std::size_t index = 0; index < names.size(); ++index) {
        extraAnimations[index] = LoadEnemyAnimationResource(resourceManager, data, names[index], cacheLevel);
    }
}

void BullyDay3::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{0, 39, 40, 41, 42, 43});
    for (std::size_t index = 0; index < extraSounds.size(); ++index) {
        extraSounds[index] = LoadEnemySoundResource(resourceManager, data, 119 + static_cast<int32_t>(index),
                                       cacheLevel);
    }
}

const soft3d::Animation* BullyDay3::GetExtraAnimation(std::size_t index) const
{
    return index < extraAnimations.size() ? extraAnimations[index] : nullptr;
}

soft3d::AudioData* BullyDay3::GetExtraSound(std::size_t index) const
{
    return index < extraSounds.size() ? extraSounds[index] : nullptr;
}

void BullyDay3::FacePlayer()
{
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
}

void BullyDay3::SetControlsDisabled(bool disabled)
{
    if (player != nullptr && player->GetScene() != nullptr) {
        soft3d::ControlMap* controls = player->GetScene()->GetControlMap();
        if (controls->IsEnabled() == disabled) {
            player->GetScene()->GetEngine().GetInput().ClearKey();
        }
        controls->SetEnabled(!disabled);
    }
}

void BullyDay3::Spawn(const soft3d::Vec3& value)
{
    fakeShadow = std::make_unique<soft3d::FakeShadow>(renderer);
    fakeShadow->Initialize(0x20000);
    LoadResources(u"r_bully_a", nullptr, nullptr);
    Enemy::Spawn(value);
    for (std::size_t index = 0; index < GetObjectCount(); ++index) {
        GetObject(index)->LoadTextureOverride(0, u"dj_bully_b", cacheLevel);
    }
    encounterMode = 0;
    chargePhase = 0;
    qtePhase = 0;
    StopMotion();
    stateTime = 0;
    if (player != nullptr) {
        player->LoadBullyQteAnimations(data);
    }
    if (qteObject == nullptr) {
        qteObject = std::make_unique<soft3d::VisUnit>(resourceManager, cacheLevel, renderer,
            player != nullptr && player->GetScene() != nullptr
                ? &player->GetScene()->GetEngine().GetAudioManager() : nullptr);
    }
    qteObject->LoadObjects(u"oszv_anjian_a", nullptr, 1);
    qteObject->LoadMaterials();
    if (soft3d::Object* object = qteObject->GetObject(0)) {
        object->SetObjectIntensity(0x2666);
        object->SetMaterialIntensityBounce(0, 0x2666, 0x7333, 8);
        object->SetMaterialIntensityBounce(1, 0x2666, 0x7333, 8);
    }
}

void BullyDay3::SubmitRenderables(soft3d::RenderQueues& queues)
{
    Enemy::SubmitRenderables(queues);
    if (state == 7 && qtePhase == 1 && qteObject != nullptr) {
        qteObject->SubmitRenderables(queues);
    }
}

void BullyDay3::PostUpdate()
{
    VisUnit::PostUpdate();
    if (state == 7 && qtePhase == 1 && qteObject != nullptr) {
        qteObject->PostUpdate();
    }
}

void BullyDay3::QueryUnitCollision()
{
    PhysicUnit::QueryUnitCollision();
    if (chargePhase != 1 && player != nullptr && player->QueryBounds(collisionBounds) != nullptr &&
        unitQueryResult.count < static_cast<int32_t>(unitQueryResult.units.size())) {
        unitQueryResult.units[static_cast<std::size_t>(unitQueryResult.count++)] = player;
        unitCollision = true;
    }
}

void BullyDay3::SetNavigationEnabled(bool enabled)
{
    if (state < EnemyDying) {
        Creature::SetNavigationEnabled(enabled);
    }
}

void BullyDay3::Update()
{
    if (state > EnemyDying) {
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
    UpdatePerception();
    knockbackChance = encounterMode == 0 ? 700 : 1000;
    if (state == 6) {
        UpdateCharge();
    } else if (state == 7) {
        UpdateQte();
    }
    Enemy::Update();
    if (state == 7 && qtePhase == 1 && qteObject != nullptr) {
        qteObject->Update();
    }
}

void BullyDay3::UpdateIdle()
{
    if (ReceivePlayerAttack()) {
        stateTime = 0;
        return;
    }
    if (stateTime > 0) {
        --stateTime;
        return;
    }
    const int32_t rushThreshold = encounterMode == 0 ? 6500 : 3000;
    const int32_t attackThreshold = encounterMode == 0 ? 4500 : 0;
    const int32_t chaseThreshold = encounterMode == 0 ? 1000 : 0;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    const int32_t selection = random % 10000;
    if (selection > rushThreshold) {
        state = 6;
        chargePhase = 0;
        PlaySound(extraSounds[1], false);
        SetAnimation(extraAnimations[1], 1, false);
        if (encounterMode != 0) {
            SetAnimationSpeed(600);
        }
        physicsFlags &= ~soft3d::PhysicKeepDirection;
        FacePlayer();
        stateTime = 12;
    } else if (selection > attackThreshold) {
        BeginAttack();
    } else if (selection > chaseThreshold) {
        const int32_t delay = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
        stateTime = static_cast<int16_t>(delay % 32 + 32);
        BeginChase();
    } else if (SelectAttack() < 1) {
        const int32_t delay = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
        stateTime = static_cast<int16_t>(delay % 16 + 16);
    }
}

void BullyDay3::UpdateChase()
{
    --stateTime;
    if (stateTime < 1) {
        StopMotion();
    } else {
        Enemy::UpdateChase();
    }
}

void BullyDay3::BeginAttack()
{
    StopSound();
    waitTimer = -1;
    stateTime = 0;
    unused7ca = 0;
    state = EnemyAttack;
    velocity = {};
    FacePlayer();
    attackHitDone = 0;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    if (random % 10000 < 5001) {
        attackKind = 0;
        attackPower = 20;
        PlaySound(sounds[3], false);
        SetAnimation(animations[3], 1, false);
    } else {
        attackKind = 1;
        attackPower = 25;
        PlaySound(extraSounds[0], false);
        SetAnimation(extraAnimations[0], 1, false);
    }
}

bool BullyDay3::TestAttackHit()
{
    if (attackKind == 0) {
        return TestAttackHitVolume(attackStart, attackEnd, secondAttackStart, secondAttackEnd);
    }
    return TestAttackHitVolume(alternateAttackStart, alternateAttackEnd);
}

void BullyDay3::BeginQte(bool unlocked)
{
    SetAnimation(extraAnimations[6], 1, false);
    velocity = {};
    if (player == nullptr) {
        return;
    }
    player->GetScene()->GetEngine().GetInput().ClearKey();
    if (unlocked) {
        player->GetScene()->GetCameraController()->SetFrozen(true);
        state = 7;
        qtePhase = 6;
        encounterMode = 1;
        savedPlayerPosition = player->GetPosition();
        return;
    }
    state = 7;
    qtePhase = 0;
    player->SetAttacksDisabled(true);
    if (qteObject != nullptr) {
        const auto* resource = dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
            soft3d::ResourceAnimation, cacheLevel, u"oszv_anjian_a"));
        qteObject->SetAnimation(resource, 1, true);
        soft3d::Vec3 objectPosition = position;
        objectPosition.y = soft3d::AddFixed(objectPosition.y, 0x28000);
        qteObject->SetPosition(objectPosition);
        CameraController* camera = player->GetScene()->GetCameraController();
        soft3d::Vec3 direction = soft3d::Subtract(camera->GetPosition(), camera->GetTarget());
        direction.y = 0;
        qteObject->SetDirection(direction);
    }
}

bool BullyDay3::ReceivePlayerAttack()
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
    blood.Trigger(soft3d::Subtract(position, soft3d::Scale(GetForward(), 0x30000)),
                  player->GetSword()->GetHitVelocity(), static_cast<MyGameUnitMgr*>(unitManager));
    stateTime = 0;
    hitCooldown = 8;
    PlaySound(sounds[4], false);
    const bool unlocked = player->GetScene()->GetEventFlag(1002);
    if (unlocked && health < 1) {
        BeginQte(true);
        return true;
    }
    if (!unlocked && ((encounterMode == 0 && health <= 100) ||
                      (encounterMode != 0 && health < 1))) {
        BeginQte(false);
        return true;
    }
    if ((state == 6 && chargePhase <= 1) || state == 7) {
        return false;
    }

    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    if (random % 1000 < knockbackChance) {
        state = EnemyHurt;
        SetAnimationByIndex(4, 1, false);
    } else {
        knockbackPending = 1;
        state = EnemyKnockback;
        SetAnimationByIndex(4, 1, false);
        SetAnimationSpeed(0x4b0);
    }
    FacePlayer();
    velocity = soft3d::Scale(GetForward(),
        -soft3d::FixedMultiply(player->GetAttackBlend(), knockbackSpeed));
    if (knockbackPending != 0) {
        velocity = soft3d::Scale(velocity, 0x8000);
    }
    return true;
}

void BullyDay3::Die()
{
    if (state == EnemyDying || IsRemoved()) {
        return;
    }
    if (player != nullptr) {
        player->NotifyEnemyKilled(0);
    }
    StopSound();
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetRenderMode(soft3d::ObjectRenderMode::SecondaryStreams);
    }
    stateTime = 0;
    animation = nullptr;
    state = EnemyDead;
}

void BullyDay3::UpdateCharge()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (chargePhase == 0) {
        --stateTime;
        if (stateTime > 0 && player != nullptr) {
            chargeTarget = player->GetPosition();
            if (bsp != nullptr) {
                const soft3d::BspTraceResult trace = bsp->Trace(position, chargeTarget, localBounds);
                if (trace.startSolid && !trace.allSolid) {
                    chargeTarget = soft3d::Add(chargeTarget,
                                               soft3d::Scale(trace.planeNormal, 0x28000));
                }
            }
            SetDirection(soft3d::Subtract(chargeTarget, position));
        }
        if (!IsAnimationFinished()) {
            return;
        }
        SetDirection(soft3d::Subtract(chargeTarget, position));
        chargePhase = 1;
        PlaySound(extraSounds[2], true);
        SetAnimation(extraAnimations[2], 1, true);
        physicsFlags &= ~soft3d::PhysicSimpleMovement;
        physicsFlags &= ~soft3d::PhysicStepOffset;
        velocity = soft3d::Scale(GetForward(), encounterMode == 0 ? 0x3333 : 0x6666);
        groundTrace->startSolid = false;
    }
    if (chargePhase == 1) {
        velocity = soft3d::Add(velocity, soft3d::Scale(GetForward(), 0x4ccc));
        const soft3d::ObjectBounds current = Translate(localBounds, position);
        soft3d::ObjectBounds next = Translate(current, velocity);
        next.minimum.x = soft3d::SubtractFixed(next.minimum.x, soft3d::fixedOne);
        next.minimum.z = soft3d::SubtractFixed(next.minimum.z, soft3d::fixedOne);
        next.maximum.x = soft3d::AddFixed(next.maximum.x, soft3d::fixedOne);
        next.maximum.z = soft3d::AddFixed(next.maximum.z, soft3d::fixedOne);
        if (player != nullptr && player->QueryBounds(next) != nullptr) {
            player->DamageHurt2();
            const soft3d::Vec3 target = soft3d::Add(position,
                soft3d::Scale(GetForward(), 0x500000));
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
                chargePhase = 2;
                PlaySound(extraSounds[3], false);
                SetAnimation(extraAnimations[3], 1, false);
                player->GetScene()->GetCameraController()->Shake(12, 0x8000);
                return;
            }
        }
        if (groundTrace->startSolid) {
            StopSound();
            velocity = {};
            chargePhase = 2;
            PlaySound(extraSounds[3], false);
            SetAnimation(extraAnimations[3], 1, false);
            if (player != nullptr) {
                player->GetScene()->GetCameraController()->Shake(12, 0x8000);
            }
        }
    } else if (chargePhase == 2) {
        if (IsAnimationFinished()) {
            chargePhase = 3;
            PlaySound(extraSounds[4], false);
            SetAnimation(extraAnimations[4], 1, true);
            if (encounterMode != 0) {
                SetAnimationSpeed(600);
                stateTime = 20;
            } else {
                stateTime = 40;
            }
        }
    } else if (chargePhase == 3) {
        --stateTime;
        if (stateTime < 1) {
            StopMotion();
            stateTime = 12;
        }
    }
}

void BullyDay3::UpdateQte()
{
    velocity = {};
    if (player == nullptr) {
        return;
    }
    Scene* scene = player->GetScene();
    CameraController* camera = scene->GetCameraController();
    if (qtePhase == 0) {
        scene->GetEngine().GetInput().ClearKey();
        SetControlsDisabled(true);
        if (!IsAnimationFinished()) {
            return;
        }
        SetControlsDisabled(false);
        qtePhase = 1;
        stateTime = 64;
        SetAnimation(extraAnimations[7], 1, true);
        return;
    }
    if (qtePhase == 1) {
        --stateTime;
        if (stateTime < 1) {
            health = encounterMode == 0 ? 150 : 50;
            stateTime = 0;
            StopMotion();
            player->SetAttacksDisabled(false);
        }
        if (!scene->GetControlMap()->ActionClick(scene->GetEngine().GetInput(), 5, false)) {
            return;
        }
        soft3d::ObjectBounds bounds = collisionBounds;
        bounds.minimum.x = soft3d::SubtractFixed(bounds.minimum.x, 0x40000);
        bounds.minimum.z = soft3d::SubtractFixed(bounds.minimum.z, 0x40000);
        bounds.maximum.x = soft3d::AddFixed(bounds.maximum.x, 0x40000);
        bounds.maximum.z = soft3d::AddFixed(bounds.maximum.z, 0x40000);
        if (player->QueryBounds(bounds) == nullptr) {
            return;
        }
        qtePhase = 2;
        scene->GetEngine().GetInput().ClearKey();
        scene->GetFadeShader()->StartFade(1, 0, 7, 1);
        SetControlsDisabled(true);
        stateTime = 8;
        return;
    }
    if (qtePhase == 2) {
        --stateTime;
        if (stateTime > 0) {
            return;
        }
        PlaySound(extraSounds[5], false);
        qtePhase = 3;
        SetAnimation(extraAnimations[8], 1, false);
        savedPlayerPosition = player->GetPosition();
        player->SetDirection(GetForward());
        player->SetPosition(position);
        player->SnapToGround();
        player->SetAction(12, 1, false);
        scene->GetFadeShader()->StartFade(0, 0, 7, 1);
        const soft3d::Vec3 direction = soft3d::Normalize(
            soft3d::Subtract(position, camera->GetPosition()));
        const soft3d::Vec3 cameraPosition = soft3d::Subtract(position,
            soft3d::Scale(direction, 0x140000));
        camera->SetMode(CameraController::FixedMode, cameraPosition, {}, 0, nullptr, 0, true, 0);
        camera->SetFrozen(true);
        return;
    }
    if (qtePhase == 3) {
        if (!IsAnimationFinished()) {
            return;
        }
        PlaySound(extraSounds[6], true);
        qtePhase = 4;
        SetAnimation(extraAnimations[9], 1, true);
        player->SetAction(13, 1, true);
        qteSuccessCount = 0;
        if (encounterMode != 0) {
            player->BeginQTE(12, 28, 5);
        } else {
            player->BeginQTE(16, 36, 5);
        }
        return;
    }
    if (qtePhase == 4) {
        if (player->GetQteResult() == 0) {
            StopSound();
            qtePhase = 5;
            PlaySound(extraSounds[7], false);
            SetAnimation(extraAnimations[10], 1, false);
            player->SetAction(14, 1, false);
            return;
        }
        if (player->GetQteResult() != 1) {
            return;
        }
        ++qteSuccessCount;
        if (qteSuccessCount == 2) {
            StopSound();
            qtePhase = 6;
            PlaySound(encounterMode == 0 ? extraSounds[4] : extraSounds[8], false);
            SetAnimation(extraAnimations[10], 1, false);
            player->SetAction(15, 1, false);
        } else if (encounterMode != 0) {
            player->BeginQTE(12, 28, 5);
        } else {
            player->BeginQTE(16, 36, 5);
        }
        return;
    }
    if (qtePhase == 5 || qtePhase == 6) {
        qteOutcome = qtePhase == 6 ? 1 : 0;
        if (!IsAnimationFinished()) {
            return;
        }
        scene->GetFadeShader()->StartFade(1, 0, 7, 1);
        SetControlsDisabled(true);
        stateTime = 8;
        qtePhase = 7;
        return;
    }
    if (qtePhase != 7) {
        return;
    }
    --stateTime;
    if (stateTime > 0) {
        return;
    }
    scene->GetFadeShader()->StartFade(0, 0, 7, 1);
    SetControlsDisabled(false);
    if (encounterMode == 0 || qteOutcome == 0) {
        camera->SetFrozen(false);
        scene->ResetCameraWarmup();
        camera->SetInstant(true);
    }
    player->SetPosition(savedPlayerPosition);
    player->SnapToGround();
    player->SetNavigationEnabled(false);
    player->SetDirection(soft3d::Subtract(position, savedPlayerPosition));
    SetControlsDisabled(false);
    player->SetAttacksDisabled(false);
    stateTime = 12;
    if (qteOutcome == 0) {
        health = encounterMode == 0 ? 150 : 50;
        StopMotion();
    } else if (encounterMode == 0) {
        encounterMode = 1;
        StopMotion();
    } else {
        Die();
    }
}

BullySaw::BullySaw(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : ElectricSaw(context.resources, 3, context.renderer, context.audio), data(gameData)
{
}

void BullySaw::InitializeAnimations()
{
    static constexpr std::array<int32_t, 11> names{223, 224, 224, 225, 226, 227,
                                                   228, 229, 230, 231, 232};
    for (std::size_t index = 0; index < names.size(); ++index) {
        animations[index] = LoadEnemyAnimationResource(resourceManager, data, names[index], cacheLevel);
    }
}

void BullySaw::SetAnimationByIndex(int32_t index, int32_t direction, bool loop)
{
    if (index >= 0 && static_cast<std::size_t>(index) < animations.size()) {
        SetAnimation(animations[static_cast<std::size_t>(index)], direction, loop);
    }
}

const soft3d::Animation* BullySaw::GetAnimationResource(std::size_t index) const
{
    return index < animations.size() ? animations[index] : nullptr;
}

BullyDay5::BullyDay5(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 13);
    cacheLevel = 3;
    SetPositionalAudio(false);
    saw = std::make_unique<BullySaw>(context, data);
}

void BullyDay5::InitializeAnimations(const soft3d::Animation* const*)
{
    LoadEnemyAnimationResources(*this, data, std::array<int32_t, 6>{213, 214, 214, 216, 217, 218});
    static constexpr std::array<int32_t, 4> names{219, 220, 222, 221};
    for (std::size_t index = 0; index < names.size(); ++index) {
        extraAnimations[index] = LoadEnemyAnimationResource(resourceManager, data, names[index], cacheLevel);
    }
}

void BullyDay5::InitializeSounds(soft3d::AudioData* const*)
{
    LoadEnemySoundResources(*this, data, std::array<int32_t, 6>{128, 129, 130, 131, 132, 133});
    static constexpr std::array<int32_t, 9> names{134, 135, 121, 122, 136, 137, 138, 139, 140};
    for (std::size_t index = 0; index < names.size(); ++index) {
        extraSounds[index] = LoadEnemySoundResource(resourceManager, data, names[index], cacheLevel);
    }
}

const soft3d::Animation* BullyDay5::GetExtraAnimation(std::size_t index) const
{
    return index < extraAnimations.size() ? extraAnimations[index] : nullptr;
}

soft3d::AudioData* BullyDay5::GetExtraSound(std::size_t index) const
{
    return index < extraSounds.size() ? extraSounds[index] : nullptr;
}

void BullyDay5::LoadResources(const char16_t* modelName,
                              const soft3d::Animation* const* animationNames,
                              soft3d::AudioData* const* soundNames)
{
    Enemy::LoadResources(modelName, animationNames, soundNames);
    saw->LoadObjects(u"ow_dianju_a", nullptr, 1);
    saw->LoadAlternateObject(u"ow_dianju_a");
    saw->EnableShadow(true);
    saw->LoadMaterials();
    saw->InitializeAnimations();
}

void BullyDay5::Spawn(const soft3d::Vec3& value)
{
    fakeShadow = std::make_unique<soft3d::FakeShadow>(renderer);
    fakeShadow->Initialize(0x20000);
    LoadResources(u"r_bullyx_a", nullptr, nullptr);
    Enemy::Spawn(value);
    chargePhase = 0;
    saw->SetPosition(value);
    StopMotion();
    if (player != nullptr) {
        player->LoadSpecialAnimations(data, player->GetScene()->GetEventFlag(1002));
    }
    stateTime = 16;
    slamCooldown = 220;
}

void BullyDay5::SubmitRenderables(soft3d::RenderQueues& queues)
{
    Enemy::SubmitRenderables(queues);
    saw->SubmitRenderables(queues);
}

void BullyDay5::PostUpdate()
{
    VisUnit::PostUpdate();
    saw->PostUpdate();
}

void BullyDay5::QueryUnitCollision()
{
    PhysicUnit::QueryUnitCollision();
    if (chargePhase != 2 && player != nullptr && player->QueryBounds(collisionBounds) != nullptr &&
        unitQueryResult.count < static_cast<int32_t>(unitQueryResult.units.size())) {
        unitQueryResult.units[static_cast<std::size_t>(unitQueryResult.count++)] = player;
        unitCollision = true;
    }
}

void BullyDay5::SetNavigationEnabled(bool enabled)
{
    if (state < EnemyDying) {
        Creature::SetNavigationEnabled(enabled);
    }
}

void BullyDay5::SetAnimation(const soft3d::Animation* resource, int32_t direction, bool loop)
{
    Enemy::SetAnimation(resource, direction, loop);
}

void BullyDay5::FacePlayer()
{
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
}

void BullyDay5::Update()
{
    if (state > EnemyDying) {
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
    if (slamCooldown > 0) {
        --slamCooldown;
    }
    UpdatePerception();
    switch (state) {
    case 6: UpdateThrow(); break;
    case 7: UpdateChargeAttack(); break;
    case 8: UpdatePickup(); break;
    case 9: UpdateSlam(); break;
    default: break;
    }
    Enemy::Update();
    UpdateThrownSaw();
    if (sawState != 0) {
        saw->SetPosition(position);
        saw->SetDirection(GetForward());
    }
    saw->Update();
}

void BullyDay5::UpdateIdle()
{
    if (ReceivePlayerAttack()) {
        stateTime = 0;
        return;
    }
    if (sawState == 0) {
        FacePlayer();
        velocity = {};
        chargePhase = 1;
        state = 7;
        return;
    }
    if (SelectAttack() != 0) {
        BeginAttack();
        return;
    }
    if (stateTime > 0) {
        FacePlayer();
        --stateTime;
        return;
    }
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    const int32_t selection = random % 10000;
    if (player != nullptr && !player->IsHurt4Requested()) {
        if (playerDistance >= chargeDistance && selection > 8000) {
            BeginThrowAttack();
            return;
        }
        if (slamCooldown <= 0 && playerDistance <= slamRange) {
            BeginSlamAttack();
            return;
        }
        if (selection <= 2000) {
            PlaySound(extraSounds[8], false);
            const int32_t delay = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
            stateTime = static_cast<int16_t>(delay % 8 + 8);
        } else {
            PlaySound(extraSounds[7], false);
            BeginChase();
        }
        return;
    }
    if (playerDistance >= chargeDistance && selection > 6500) {
        BeginThrowAttack();
        return;
    }
    PlaySound(extraSounds[7], false);
    BeginChase();
}

void BullyDay5::UpdateChase()
{
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    const int32_t random = manager != nullptr ? static_cast<int32_t>(manager->NextRandom()) : 0;
    const int32_t selection = random % 10000;
    if (playerDistance >= chargeDistance && selection > 8500) {
        BeginThrowAttack();
    } else if (slamCooldown <= 0 && player != nullptr && !player->IsHurt4Requested() &&
               playerDistance <= slamRange && SelectAttack() == 0) {
        BeginSlamAttack();
    } else {
        Enemy::UpdateChase();
    }
}

bool BullyDay5::IsSegmentInside(const soft3d::Vec3& start, const soft3d::Vec3& end)
{
    if (throwPhase == 2 && player != nullptr && player->GetWeapon() > 2 &&
        player->GetAttackPhase() == 2 && saw->QuerySegment(start, end) != nullptr) {
        impactSoundPending = 1;
    }
    return Creature::IsSegmentInside(start, end);
}

bool BullyDay5::TestAttackHit()
{
    return TestAttackHitVolume(attackStart, attackEnd);
}

bool BullyDay5::ReceivePlayerAttack()
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
        EndThrow(true);
        Die();
        return true;
    }
    blood.Trigger(soft3d::Subtract(position, soft3d::Scale(GetForward(), 0x30000)),
                  player->GetSword()->GetHitVelocity(), static_cast<MyGameUnitMgr*>(unitManager));
    hitCooldown = 8;
    PlaySound(sounds[4], false);
    if ((state == 6 && chargePhase == 0) || state == 9) {
        return false;
    }
    stateTime = 0;
    knockbackPending = 1;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    state = EnemyHurt;
    SetAnimationByIndex(4, 1, false);
    FacePlayer();
    velocity = soft3d::Scale(GetForward(),
        -soft3d::FixedMultiply(player->GetAttackBlend(), knockbackSpeed));
    return true;
}

void BullyDay5::BeginThrowAttack()
{
    StopMotion();
    FacePlayer();
    state = 6;
    throwPhase = 0;
}

void BullyDay5::BeginSlamAttack()
{
    StopMotion();
    FacePlayer();
    state = 9;
    slamPhase = 0;
    slamCooldown = 220;
}

void BullyDay5::EndThrow(bool fast)
{
    throwPhase = 3;
    saw->SetAnimationByIndex(9, 1, false);
    if (fast) {
        saw->SetAnimationSpeed(600);
        throwVelocity = {};
    }
}

void BullyDay5::UpdateThrow()
{
    if (throwPhase == 0 && player != nullptr) {
        sawState = 0;
        throwPhase = 1;
        SetAnimation(extraAnimations[1], 1, false);
        saw->SetAnimationByIndex(8, 1, false);
        PlaySound(extraSounds[0], false);
        throwTarget = player->GetPosition();
    }
    if (throwPhase == 1 && saw->IsAnimationFinished()) {
        state = 7;
        throwPhase = 2;
        saw->SetPosition(soft3d::Add(position, soft3d::Scale(GetForward(), 0x20000)));
        saw->SetAnimationByIndex(9, 1, true);
        throwTimer = 10;
        throwVelocity = soft3d::Scale(soft3d::Normalize(
            soft3d::Subtract(throwTarget, saw->GetPosition())), throwSpeed);
        UpdateThrownSaw();
    }
}

void BullyDay5::UpdateChargeAttack()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (chargePhase == 0) {
        return;
    }
    if (chargePhase == 1) {
        SetDirection(soft3d::Subtract(saw->GetPosition(), position));
        chargePhase = 2;
        PlaySound(extraSounds[2], true);
        SetAnimation(extraAnimations[2], 1, true);
        physicsFlags &= ~soft3d::PhysicSimpleMovement;
        physicsFlags &= ~soft3d::PhysicStepOffset;
        groundTrace->startSolid = false;
    }
    if (chargePhase != 2) {
        return;
    }
    SetDirection(soft3d::Subtract(saw->GetPosition(), position));
    velocity = soft3d::Add(velocity, soft3d::Scale(GetForward(), 0x4ccc));
    const soft3d::ObjectBounds current = Translate(localBounds, position);
    soft3d::ObjectBounds next = Translate(current, velocity);
    next.minimum.x = soft3d::SubtractFixed(next.minimum.x, soft3d::fixedOne);
    next.minimum.z = soft3d::SubtractFixed(next.minimum.z, soft3d::fixedOne);
    next.maximum.x = soft3d::AddFixed(next.maximum.x, soft3d::fixedOne);
    next.maximum.z = soft3d::AddFixed(next.maximum.z, soft3d::fixedOne);
    if (player != nullptr && player->QueryBounds(next) != nullptr) {
        player->DamageHurt2();
        const soft3d::Vec3 target = soft3d::Add(position,
            soft3d::Scale(GetForward(), 0xc80000));
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
            chargePhase = 0;
            state = 8;
            player->GetScene()->GetCameraController()->Shake(12, 0x7ae);
            return;
        }
        if (player->QueryBounds(Translate(localBounds, saw->GetPosition())) != nullptr) {
            StopSound();
            velocity = {};
            chargePhase = 0;
            state = 8;
            return;
        }
    }
    if (soft3d::SquaredLength(velocity) >
        soft3d::SquaredLength(soft3d::Subtract(saw->GetPosition(), position))) {
        StopSound();
        velocity = {};
        chargePhase = 0;
        state = 8;
        return;
    }
    if (groundTrace->startSolid) {
        StopSound();
        PlaySound(extraSounds[1], false);
        velocity = {};
        chargePhase = 0;
        state = 8;
        if (player != nullptr) {
            player->GetScene()->GetCameraController()->Shake(12, 0x7ae);
        }
    }
    if (unitCollision) {
        StopSound();
        PlaySound(extraSounds[1], false);
        velocity = {};
        chargePhase = 0;
        state = 8;
        if (player != nullptr) {
            player->GetScene()->GetCameraController()->Shake(12, 0x7ae);
        }
    }
}

void BullyDay5::UpdatePickup()
{
    if (sawState == 0) {
        PlaySound(extraSounds[7], false);
        SetAnimation(extraAnimations[3], 1, false);
        saw->SetAnimationByIndex(10, 1, false);
        sawState = 1;
    }
    if (sawState == 1 && IsAnimationFinished()) {
        sawState = 2;
        StopMotion();
        stateTime = 20;
    }
}

void BullyDay5::UpdateSlam()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (slamPhase == 0) {
        SetAnimation(extraAnimations[0], 1, false);
        saw->SetAnimationByIndex(6, 1, false);
        slamPhase = 1;
    }
    if (slamPhase == 1) {
        if (GetObject(0) != nullptr && GetObject(0)->GetAnimationTime() == 0xf3c &&
            player != nullptr) {
            PlaySound(extraSounds[1], false);
            player->GetScene()->GetCameraController()->Shake(12, 0x28f);
            player->RequestHurt4();
        }
        if (IsAnimationFinished()) {
            slamPhase = 0;
            StopMotion();
        }
    }
}

void BullyDay5::UpdateThrownSaw()
{
    if (throwPhase == 2) {
        if (throwTimer > 0 && --throwTimer < 1) {
            chargePhase = 1;
            unitCollision = false;
            groundTrace->startSolid = false;
        }
        if (impactSoundPending != 0) {
            PlaySound(extraSounds[4], false);
            EndThrow(true);
            impactSoundPending = 0;
        }
        const soft3d::ObjectBounds current = Translate(saw->GetObject(0)->GetPrimaryBounds(), saw->GetPosition());
        const soft3d::ObjectBounds next = Translate(current, throwVelocity);
        if (player != nullptr && player->QueryBounds(next) != nullptr) {
            saw->SetPosition(soft3d::Add(saw->GetPosition(), throwVelocity));
            player->Damage(30);
            PlaySound(extraSounds[5], false);
            EndThrow(true);
        }
        if (bsp != nullptr) {
            const soft3d::BspTraceResult trace = bsp->Trace(saw->GetPosition(),
                soft3d::Add(saw->GetPosition(), throwVelocity), saw->GetObject(0)->GetPrimaryBounds());
            if (trace.startSolid) {
                PlaySound(extraSounds[4], false);
                EndThrow(true);
            }
        }
        if (unitManager != nullptr) {
            const auto& hits = static_cast<MyGameUnitMgr*>(unitManager)->QueryBounds(next, -1, this);
            if (hits.count > 0) {
                PlaySound(extraSounds[4], false);
                EndThrow(true);
            }
        }
        saw->SetPosition(soft3d::Add(saw->GetPosition(), throwVelocity));
    } else if (throwPhase == 3 && IsAnimationFinished()) {
        throwPhase = 0;
        if (chargePhase == 0) {
            chargePhase = 1;
            unitCollision = false;
            groundTrace->startSolid = false;
        }
        throwVelocity = {};
    }
}

std::unique_ptr<Enemy> CreateMaidBullyPoolEnemy(const soft3d::UnitManagerContext& context,
                                                const GameData& data, int32_t enemyType)
{
    switch (enemyType) {
    case 0: return std::make_unique<Maid>(context, data);
    case 1: return std::make_unique<Bully>(context, data);
    case 2: return std::make_unique<BullyDay3>(context, data);
    case 6: return std::make_unique<FatMaid>(context, data);
    case 7: return std::make_unique<Maid2>(context, data);
    case 9: return std::make_unique<Maid3>(context, data);
    case 13: return std::make_unique<BullyDay5>(context, data);
    default: return nullptr;
    }
}

} // namespace game

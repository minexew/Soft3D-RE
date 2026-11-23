#include "game/Player.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>

namespace game {
namespace {

} // namespace

PlayerStateSet::PlayerStateSet() = default;

Player::Player(soft3d::s3dResManager* resources, soft3d::Renderer* renderer, soft3d::Bsp* map,
               Scene* owner, soft3d::AudioManager* audio)
    : Creature(resources, 2, renderer, audio, map)
    , StateMachine<Player>(&states)
    , scene(owner)
    , controls(&owner->GetEngine().GetControlMap())
    , sword(std::make_unique<Sword>(resources, renderer))
    , gunFire(renderer, &owner->GetEngine().GetCamera())
{
    unitType = 9;
    SetNavigationSpeed(0xcccc);
}

soft3d::result Player::Initialize(const GameData& data)
{
    states.InitializeStates(this);
    LoadAdultModel(false);
    if (GetObject(0) == nullptr) {
        return soft3d::S3D_FILE;
    }
    LoadDefaultAnimations(data);
    return soft3d::S3D_OK;
}

void Player::LoadDefaultAnimations(const GameData& data)
{
    animations[0].fill(nullptr);
    static constexpr int actions[] = {0, 1, 8, 9, 10, 11};
    static constexpr int names[] = {0, 1, 2, 3, 4, 34};
    for (std::size_t index = 0; index < std::size(actions); ++index) {
        animations[0][actions[index]] = dynamic_cast<const soft3d::Animation*>(
            resourceManager->GetResource(soft3d::ResourceAnimation, 0,
                                         data.GetAnimationName(names[index])));
    }
    defaultStand = animations[0][0];
    defaultRun = animations[0][1];
    defaultHurt2 = animations[0][11];
}

void Player::LoadWeaponAnimations(const GameData& data, int type)
{
    if (type < 1 || type > 4) {
        return;
    }
    static constexpr int ordinaryNames[4][12] = {
        {0, 1, 5, 6, 7, 8, 9, 10, 2, 3, 4, 19},
        {0, 11, 12, 13, 14, 15, 16, 17, 18, 20, 4, 19},
        {25, 26, 27, 28, 29, 30, 31, 32, 33, 35, 4, 19},
        {0, 1, -1, -1, -1, 37, 38, 39, 2, 3, 4, 19},
    };
    auto& table = animations[static_cast<std::size_t>(type)];
    for (std::size_t action = 0; action < std::size(ordinaryNames[type - 1]); ++action) {
        const int name = ordinaryNames[type - 1][action];
        table[action] = name < 0 ? nullptr : dynamic_cast<const soft3d::Animation*>(
            resourceManager->GetResource(soft3d::ResourceAnimation, 2,
                                         data.GetAnimationName(name)));
    }
}

void Player::PrepareWeapon(const GameData& data, int type)
{
    static constexpr const char16_t* modelNames[] = {
        nullptr, u"ow_dao_a", u"ow_xiyangjian_a", u"ow_qiang_a", u"ow_shaying_a"
    };
    if (type < 1 || type > 4) {
        return;
    }
    sword->LoadModel(type, modelNames[type]);
    sword->LoadAnimations(data, type);
    LoadWeaponAnimations(data, type);
    if (type == 3 || type == 4) {
        gunFire.LoadTextures(resourceManager);
    }
}

void Player::LoadBullyQteAnimations(const GameData& data)
{
    if (scene->GetEventFlag(1002)) {
        return;
    }
    for (int action = 12; action <= 15; ++action) {
        animations[2][static_cast<std::size_t>(action)] =
            dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
                soft3d::ResourceAnimation, 3, data.GetAnimationName(21 + action - 12)));
    }
    sword->LoadQteAnimations(data, 2, false);
}

void Player::LoadSpecialAnimations(const GameData& data, bool alternate)
{
    const auto load = [&data, this](int name) {
        return dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
            soft3d::ResourceAnimation, 3, data.GetAnimationName(name)));
    };
    animations[0][12] = load(34);
    if (!alternate) {
        animations[3][12] = load(36);
        sword->LoadQteAnimations(data, 3, false);
        sword->UseStandForSpecial(3);
    } else {
        animations[4][12] = load(36);
        sword->LoadQteAnimations(data, 4, true);
        sword->UseStandForSpecial(4);
    }
}

const soft3d::Animation* Player::GetActionAnimation(int type, int action) const
{
    if (type < 0 || type >= 5 || action < 0 || action >= 20) {
        return nullptr;
    }
    return animations[static_cast<std::size_t>(type)][static_cast<std::size_t>(action)];
}

void Player::LoadAdultModel(bool snap)
{
    LoadObjects(u"r_ken_a", nullptr, 1);
    LoadAlternateObject(u"r_ken_shadow_a");
    EnableShadow(true);
    LoadMaterials();
    const int outfit = scene->GetNewGameOptions().outfit;
    if (outfit == 1 || outfit == 2) {
        for (std::size_t index = 0; index < GetObjectCount(); ++index) {
            GetObject(index)->LoadTextureOverride(0, outfit == 1 ? u"dj_ken_ab" : u"dj_ken_ac", 2);
        }
    }
    animations[0][0] = defaultStand;
    animations[0][1] = defaultRun;
    animations[0][11] = defaultHurt2;
    young = 0;
    SetNavigationSpeed(0xcccc);
    StateMachine<Player>::Exit(this);
    EnterState(PlayerStand);
    currentAction = 0;
    currentDirection = 1;
    currentLoop = 1;
    if (snap) {
        SnapToGround();
    }
}

void Player::LoadYoungModel()
{
    LoadObjects(u"r_xiaoken_a", nullptr, 1);
    LoadAlternateObject(u"r_xiaoken_shadow_a");
    EnableShadow(true);
    LoadMaterials();
    animations[0][0] = defaultStand;
    animations[0][1] = defaultRun;
    animations[0][11] = defaultHurt2;
    const GameData& data = scene->GetUnitManager()->GetData();
    animations[0][0] = dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
        soft3d::ResourceAnimation, 2, data.GetAnimationName(41)));
    animations[0][1] = dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
        soft3d::ResourceAnimation, 2, data.GetAnimationName(42)));
    animations[0][10] = dynamic_cast<const soft3d::Animation*>(resourceManager->GetResource(
        soft3d::ResourceAnimation, 2, data.GetAnimationName(43)));
    young = 1;
    SetNavigationSpeed(0x8000);
    StateMachine<Player>::Exit(this);
    EnterState(PlayerStand);
    currentAction = 0;
    currentDirection = 1;
    currentLoop = 1;
    SnapToGround();
}

void Player::ResetAll(const GameData& data)
{
    inventory.fill(0);
    inventoryDayStart.fill(0);
    notes.fill(0);
    notesDayStart.fill(0);
    healthPackDayStart = 0;
    scene->GetUnitManager()->SetHealthPackCount(0);
    itemUseCount = 0;
    itemTriggerCount = 0;
    killCount = 0;
    savedItemUseCount = 0;
    savedKillCount = 0;
    playTime = 0;
    deathCount = 0;
    selectedNote = 0;
    Reset(data);
}

void Player::Reset(const GameData& data)
{
    (void)data;
    sword->SetType(0);
    LoadAdultModel(false);
    position = {};
    SetDirection({0, 0, soft3d::fixedOne});
    maxHealth = 100;
    health = maxHealth;
    healthTenths = health / 10;
    hurtRequest = 0;
    hurt2Request = 0;
    hurt3Request = 0;
    hurt4Request = 0;
    deathState = 0;
    deathAnimation = 1;
    attacker = nullptr;
    stateTimer = 0;
    stateCounter = 0;
    autoTarget = nullptr;
    externalVelocity = {};
    usedItem = 0;
    sword->Reset();
    frameCounter = 0;
    selectedNote = -1;
    selectedItem = 1;
    qteResult = -1;
    hideReturnFPS = 0;
    hidden = 0;
    fpsActive = 0;
    attackPhase = 0;
    gunAttackActive = 0;
    attacksDisabled = 0;
    pushing = 0;
    physicsFlags = 0xf;
    gunExperience = 0;
    gunLevel = 0;
    Creature::SetNavigationEnabled(false);
    StateMachine<Player>::Reset(this);
}

void Player::ResetForMap()
{
    StateMachine<Player>::Reset(this);
    Creature::SetNavigationEnabled(false);
    navigating = 0;
    usedItemHealing = 0;
    health = maxHealth;
    presentationVisible = 1;
    hidden = 0;
    fpsActive = 0;
    pushing = 0;
    hurtRequest = 0;
    hurt2Request = 0;
    hurt3Request = 0;
    hurt4Request = 0;
    deathState = 0;
    stateTimer = 0;
    stateCounter = 0;
    externalVelocity = {};
    physicsFlags = 0xf;
    ResetNavigationPath();
    SnapToGround();
}

void Player::SetVisible(bool value)
{
    visible = value;
    presentationVisible = value ? 1 : 0;
}

soft3d::ObjectBounds Player::GetInteractionBounds() const
{
    if (pushing == 0) {
        return collisionBounds;
    }
    return {soft3d::Add(pushLocalBounds.minimum, position),
            soft3d::Add(pushLocalBounds.maximum, position)};
}

bool Player::NavigateTo(const soft3d::Vec3& target, soft3d::Fixed radius)
{
    if (navigating == 0) {
        navigating = 1;
    }
    const bool reached = Creature::NavigateTo(target, radius == 0 ? GetNavigationSpeed() : radius);
    if (reached) {
        navigating = 0;
    }
    return reached;
}

void Player::SetNavigationEnabled(bool enabled)
{
    Creature::SetNavigationEnabled(enabled);
    if (controls != nullptr) {
        scene->GetEngine().GetInput().ClearKey();
    }
    sword->SetHitActive(false);
    sword->Reset();
    attackPhase = 0;
    if (currentAction != 0 && deathState == 0 && hurt2Request == 0 && hurt3Request == 0 &&
        hurt4Request == 0 && fpsActive == 0) {
        StateMachine<Player>::Exit(this);
        EnterState(PlayerStand);
        currentAction = 0;
        currentDirection = 1;
        currentLoop = 1;
    }
}

const soft3d::Vec3& Player::GetRight() const
{
    static const soft3d::Vec3 fallback{soft3d::fixedOne, 0, 0};
    return GetObject(0) != nullptr ? GetObject(0)->GetAxisX() : fallback;
}

const soft3d::Vec3& Player::GetUp() const
{
    static const soft3d::Vec3 fallback{0, soft3d::fixedOne, 0};
    return GetObject(0) != nullptr ? GetObject(0)->GetAxisY() : fallback;
}

const soft3d::Vec3& Player::GetForward() const
{
    static const soft3d::Vec3 fallback{0, 0, soft3d::fixedOne};
    return GetObject(0) != nullptr ? GetObject(0)->GetAxisZ() : fallback;
}

int Player::GetCurrentStateId() const
{
    for (int index = 0; index < 20; ++index) {
        if (IsCurrentState(GetState(index))) {
            return index;
        }
    }
    return -1;
}

bool Player::IsAttacking() const
{
    const int state = GetCurrentStateId();
    return state >= PlayerAttackA1 && state <= PlayerAttackC3;
}

bool Player::IsPushingUnit(int32_t id) const
{
    return pushing != 0 && pushTarget != nullptr && pushTarget->GetUnitId() == id;
}

bool Player::ActionDown(int action) const
{
    return controls != nullptr && controls->ActionDown(scene->GetEngine().GetInput(), action, false);
}

bool Player::ActionClick(int action) const
{
    return controls != nullptr && controls->ActionClick(scene->GetEngine().GetInput(), action, false);
}

bool Player::ActionPress(int action) const
{
    if (controls == nullptr || !controls->IsEnabled()) {
        return false;
    }
    const soft3d::Input& input = scene->GetEngine().GetInput();
    return input.IsKeyPress(static_cast<soft3d::Key>(controls->GetPrimaryBinding(action))) ||
           input.IsKeyPress(static_cast<soft3d::Key>(controls->GetBinding(action)));
}

bool Player::ActionUp(int action) const
{
    if (controls == nullptr || !controls->IsEnabled()) {
        return false;
    }
    const soft3d::Input& input = scene->GetEngine().GetInput();
    const soft3d::Key primary = static_cast<soft3d::Key>(controls->GetPrimaryBinding(action));
    const soft3d::Key secondary = static_cast<soft3d::Key>(controls->GetBinding(action));
    return input.IsKeyUp(primary) && input.IsKeyUp(secondary);
}

soft3d::Key Player::QteInputKey() const
{
    return controls != nullptr
        ? static_cast<soft3d::Key>(controls->GetPrimaryBinding(qteKey)) : soft3d::Key::None;
}

void Player::SetAction(int32_t action, int32_t direction, bool loop)
{
    currentAction = action;
    currentDirection = direction;
    currentLoop = loop ? 1 : 0;
    PhysicUnit::SetAnimation(GetActionAnimation(GetWeapon(), action), direction, loop);
    sword->SetAnimation(action, direction, loop, 0);
}

void Player::UpdateMovement(soft3d::Fixed speed)
{
    if (navigating != 0) {
        return;
    }
    soft3d::Vec3 movement;
    CameraController* camera = scene->GetCameraController();
    if (camera->GetMode() == CameraController::FirstPerson) {
        if (ActionDown(3) || ActionDown(4)) {
            SetVelocity({});
            return;
        }
        cachedForward = soft3d::Normalize(soft3d::Subtract(camera->GetTarget(), camera->GetPosition()));
        cachedRight = camera->GetRight();
    } else {
        if (ActionDown(1) || ActionDown(2) || ActionDown(3) || ActionDown(4) || camera->IsInstant()) {
            cachedForward = soft3d::Normalize(soft3d::Subtract(camera->GetTarget(),
                                                               camera->GetPosition()));
            cachedRight = camera->GetRight();
        }
    }

    soft3d::Vec3 forward = cachedForward;
    const soft3d::Vec3 right = cachedRight;
    soft3d::Vec3 up{0, soft3d::fixedOne, 0};
    if (camera->GetMode() != CameraController::FirstPerson) {
        const soft3d::Fixed alignment = soft3d::AbsFixed(soft3d::Dot(forward, up));
        if (alignment > 0xe666) {
            up = soft3d::Cross(forward, right);
            const soft3d::Fixed amount = soft3d::FixedMultiply(
                soft3d::SubtractFixed(alignment, 0xe666), 0xa0000);
            forward = soft3d::Add(soft3d::Scale(forward, soft3d::SubtractFixed(soft3d::fixedOne, amount)),
                                  soft3d::Scale(up, amount));
        }
    }
    forward.y = 0;
    if (forward.x <= 0x4000 && forward.z <= 0x4000) {
        forward = soft3d::ShiftLeft(forward, 8);
    }
    forward = soft3d::Normalize(forward);

    if (ActionDown(1)) {
        movement.x = forward.x;
        movement.z = forward.z;
    }
    if (camera->GetMode() != CameraController::FirstPerson && ActionDown(2)) {
        movement.x = soft3d::NegateFixed(forward.x);
        movement.z = soft3d::NegateFixed(forward.z);
    }
    if (camera->GetMode() != CameraController::FirstPerson) {
        if (ActionDown(3)) {
            movement = soft3d::Subtract(movement, right);
        } else if (ActionDown(4)) {
            movement = soft3d::Add(movement, right);
        }
    }
    movement = soft3d::Scale(movement, speed);
    if (soft3d::AbsFixed(movement.x) < 0x28f) {
        movement.x = 0;
    }
    if (soft3d::AbsFixed(movement.z) < 0x28f) {
        movement.z = 0;
    }
    SetVelocity(movement);
}

soft3d::State<Player>* Player::CommonTransition()
{
    if (deathState != 0) return GetState(PlayerDie);
    if (hurtRequest != 0) return GetState(PlayerHurt);
    if (hurt2Request != 0) return GetState(PlayerHurt2);
    if (hurt3Request != 0) return GetState(PlayerHurt3);
    if (hurt4Request != 0) return GetState(PlayerHurt4);
    return nullptr;
}

void Player::SelectAttackTarget()
{
    if (deathState != 0 || scene->GetUnitManager() == nullptr) {
        return;
    }
    attackRange = GetWeapon() < 3 || gunLevel == 0 ? 0xf0000 : static_cast<soft3d::Fixed>(0x270f0000);
    autoTarget = nullptr;
    soft3d::Fixed nearest = static_cast<soft3d::Fixed>(0x7fff0000);
    for (soft3d::GameUnit* unit = scene->GetUnitManager()->GetFirstUnit(0); unit != nullptr;
         unit = unit->GetNextUnit()) {
        auto* target = dynamic_cast<CombatTarget*>(unit);
        if (target == nullptr || !target->IsAttackable()) {
            continue;
        }
        const soft3d::Fixed distance = target->GetPlayerDistance();
        if (distance < 0 || distance > attackRange || distance >= nearest) {
            continue;
        }
        if (bsp != nullptr && bsp->Trace(position, unit->GetPosition()).startSolid != 0) {
            continue;
        }
        soft3d::Vec3 delta = soft3d::Subtract(unit->GetPosition(), position);
        if (soft3d::Dot(delta, GetForward()) < 0) {
            continue;
        }
        delta = soft3d::Normalize(delta);
        if (soft3d::AbsFixed(soft3d::Dot(delta, GetForward())) < 0x6666) {
            continue;
        }
        nearest = distance;
        autoTarget = unit;
    }
    if (autoTarget != nullptr) {
        SetDirection(soft3d::Subtract(autoTarget->GetPosition(), position));
    }
}

void Player::Update()
{
    if (usedItemHealing != 0) {
        health = maxHealth;
    }
    ++frameCounter;
    if (frameCounter == 16) {
        frameCounter = 0;
        ++playTime;
    }
    if (deathState == 0 && IsNavigationEnabled()) {
        SetVelocity({});
        if (IsShadowEnabled()) {
            UpdateShadow();
        }
        pushing = 0;
        itemTriggerCount = 0;
        return;
    }

    StateMachine<Player>::Update(this);
    const bool keepDirection = (physicsFlags & soft3d::PhysicKeepDirection) == 0;
    if (keepDirection) {
        SetDirection(velocity);
        physicsFlags |= soft3d::PhysicKeepDirection;
    }
    velocity = soft3d::Add(velocity, externalVelocity);
    PhysicUnit::Update();
    if (keepDirection) {
        physicsFlags &= ~soft3d::PhysicKeepDirection;
    }
    sword->SetPosition(GetPosition());
    sword->SetRight(GetRight());
    sword->SetUp(GetUp());
    sword->SetForward(GetForward());
    sword->Update();
    itemTriggerCount = 0;
    UpdateGunTargets();
}

void Player::PostUpdate()
{
    if (presentationVisible != 0) {
        VisUnit::PostUpdate();
        if (pushing == 0 && hurt2Request == 0 && hurt4Request == 0) {
            sword->PostUpdate();
        }
    }
}

void Player::SubmitRenderables(soft3d::RenderQueues& queues)
{
    if (presentationVisible == 0) {
        return;
    }
    VisUnit::SubmitRenderables(queues);
    if (pushing == 0 && hurt2Request == 0 && hurt4Request == 0 && GetWeapon() > 0 &&
        sword->GetObject() != nullptr) {
        if (primaryObject != nullptr) {
            sword->SetObjectIntensity(primaryObject->GetObjectIntensity());
        }
        if (shadowObject != nullptr) {
            sword->SetShadowSecondaryTransform(shadowObject->GetSecondaryTransform());
        }
        sword->SubmitRenderables(queues);
    }
    if (gunFire.GetFramesRemaining() > 0) {
        gunPosition = sword->SampleMorph(0);
        gunFire.SetPosition(gunPosition);
        queues.Submit(&gunFire, 5);
    }
}

void Player::UpdateGunTargets()
{
    gunTargets.fill(nullptr);
    if (GetWeapon() != 3 || attackPhase != 2) {
        return;
    }
    for (soft3d::GameUnit* unit = scene->GetUnitManager()->GetFirstUnit(0); unit != nullptr;
         unit = unit->GetNextUnit()) {
        auto* combat = dynamic_cast<CombatTarget*>(unit);
        if (combat == nullptr || !combat->IsAttackable() || !SelectGunTarget(unit)) {
            continue;
        }
        if (gunTargets[0] == nullptr) {
            gunTargets[0] = unit;
        } else {
            auto* selected = dynamic_cast<CombatTarget*>(gunTargets[0]);
            if (selected->GetPlayerDistance() > combat->GetPlayerDistance()) {
                soft3d::GameUnit* previous = gunTargets[0];
                gunTargets[0] = unit;
                if (gunLevel > 0 && (gunTargets[1] == nullptr ||
                    dynamic_cast<CombatTarget*>(gunTargets[1])->GetPlayerDistance() >
                        selected->GetPlayerDistance())) {
                    gunTargets[1] = previous;
                }
            } else if (gunLevel > 0 && (gunTargets[1] == nullptr ||
                dynamic_cast<CombatTarget*>(gunTargets[1])->GetPlayerDistance() >
                    combat->GetPlayerDistance())) {
                gunTargets[1] = unit;
            }
        }
    }
}

bool Player::SelectGunTarget(soft3d::GameUnit* target) const
{
    auto* combat = dynamic_cast<CombatTarget*>(target);
    if (combat == nullptr || GetWeapon() != 3 || attackPhase != 2) {
        return false;
    }
    const soft3d::Vec3 forward = GetForward();
    auto* creature = dynamic_cast<soft3d::Creature*>(target);
    if (creature == nullptr) {
        return false;
    }
    const soft3d::Vec3 far = soft3d::Add(position,
        soft3d::Scale(forward, soft3d::FixedMultiply(0x20000, combat->GetPlayerDistance())));
    if (!creature->IsWithinRadius(position, far.x)) {
        return false;
    }
    const soft3d::Vec3 end = soft3d::Add(position,
        soft3d::Scale(forward, combat->GetPlayerDistance()));
    return bsp == nullptr || bsp->Trace(position, end).startSolid == 0;
}

int32_t Player::GetGunDamage(int32_t targetIndex, soft3d::GameUnit* target) const
{
    static constexpr soft3d::Fixed thresholds[3] = {0, 0x280000, 0x3c0000};
    static constexpr int32_t damage[3][4] = {
        {27, 27, 15, 15}, {40, 40, 20, 20}, {60, 60, 30, 30}
    };
    const auto* combat = dynamic_cast<const CombatTarget*>(target);
    if (combat == nullptr) {
        return 0;
    }
    (void)targetIndex;
    int32_t result = 0;
    for (int tier = 0; tier < 3; ++tier) {
        if (combat->GetPlayerDistance() < thresholds[tier]) {
            result = damage[std::clamp(gunLevel, 0, 2)][tier];
            break;
        }
    }
    if (result == 0) {
        result = damage[std::clamp(gunLevel, 0, 2)][3];
    }
    return result;
}

int32_t Player::GetAttackDamage(soft3d::GameUnit* target)
{
    auto* creature = dynamic_cast<soft3d::Creature*>(target);
    if (deathState != 0 || creature == nullptr) {
        return 0;
    }
    if (GetWeapon() == 4 && attackPhase == 2) {
        soft3d::Vec3 start = position;
        start.y = soft3d::AddFixed(start.y, 0x18000);
        const auto* combat = dynamic_cast<const CombatTarget*>(target);
        if (combat == nullptr) {
            return 0;
        }
        const soft3d::Vec3 forward = GetForward();
        const soft3d::Vec3 end = soft3d::Add(start, soft3d::Scale(forward,
            soft3d::FixedMultiply(0x20000, combat->GetPlayerDistance())));
        if (!creature->IsWithinRadius(start, end.x)) {
            return 0;
        }
        const soft3d::Vec3 clipped = soft3d::Add(start, soft3d::Scale(forward,
            soft3d::SubtractFixed(combat->GetPlayerDistance(), soft3d::fixedOne)));
        return bsp == nullptr || bsp->Trace(start, clipped).startSolid == 0 ? 125 : 0;
    }
    if (GetWeapon() == 3 && attackPhase == 2) {
        int32_t damage = 0;
        for (int index = 0; index < 2; ++index) {
            if (gunTargets[index] == target) {
                damage += GetGunDamage(index, target);
            }
        }
        return damage;
    }
    if (!creature->IsPointInside(sword->GetHitBounds().minimum)) {
        return 0;
    }
    static constexpr int32_t damage[5][3] = {
        {0, 0, 0}, {10, 20, 20}, {40, 20, 50}, {27, 27, 15}, {15, 40, 40}
    };
    if (GetWeapon() == 0) {
        if (attackPhase == 0) return health;
        if (attackPhase == 1) return maxHealth;
        return healthTenths;
    }
    return damage[std::clamp(GetWeapon(), 0, 4)][std::clamp(attackPhase, 0, 2)];
}

bool Player::HasAttackingEnemy() const
{
    for (soft3d::GameUnit* unit = scene->GetUnitManager()->GetFirstUnit(0); unit != nullptr;
         unit = unit->GetNextUnit()) {
        const auto* target = dynamic_cast<const CombatTarget*>(unit);
        if (target != nullptr && target->IsAttacking()) {
            return true;
        }
    }
    return false;
}

bool Player::HasAnyEnemy() const { return scene->GetUnitManager()->GetBucketCount(0) > 0; }

void Player::SetHealth(int32_t value)
{
    if (value < 0 || value > maxHealth) {
        health = maxHealth;
    } else {
        health = value;
        if (value == 0) {
            deathState = 1;
            deathAnimation = 0;
        }
    }
}

void Player::AddHealth(int32_t value)
{
    health += value;
    health = std::clamp(health, 0, maxHealth);
    if (health == 0) {
        deathState = 1;
        deathAnimation = 0;
    }
}

void Player::SetYoung(bool value)
{
    if (value) {
        LoadYoungModel();
    } else {
        LoadAdultModel(true);
    }
}

void Player::SetCustomAnimation(const soft3d::Animation* animation, bool loop)
{
    currentAction = -1;
    PhysicUnit::SetAnimation(animation, 1, loop);
}

void Player::SetWeapon(int type)
{
    sword->SetType(type);
    sword->SetPosition(GetPosition());
    sword->SetRight(GetRight());
    sword->SetUp(GetUp());
    sword->SetForward(GetForward());
    sword->Update();
    SetAction(currentAction, currentDirection, currentLoop != 0);
}

void Player::StartAttack(bool heavy) { EnterState(heavy ? PlayerAttackC1 : PlayerAttackA1); }

void Player::NotifyEnemyKilled(int32_t gunExperience)
{
    ++killCount;
    AddGunExperience(gunExperience);
}

void Player::DamageFrom(soft3d::GameUnit* source)
{
    if (young != 0 || usedItemHealing != 0) {
        return;
    }
    attacker = source;
    Damage(0);
}

void Player::Damage(int32_t amount)
{
    if (deathState != 0) {
        return;
    }
    if (amount < 1) {
        const auto* source = dynamic_cast<const CombatTarget*>(attacker);
        amount = source != nullptr ? source->GetAttackPower() : 1;
        amount = std::max(amount, 1);
    }
    health -= amount;
    if (health < 1) {
        ++deathCount;
        deathState = 1;
    } else {
        hurtRequest = 1;
    }
}

void Player::DamageHurt2()
{
    if (deathState != 0 || hurt2Request != 0) {
        return;
    }
    health -= 30;
    if (health < 1) {
        ++deathCount;
        hurt2Request = 1;
        deathState = 1;
    } else {
        hurt2Request = 1;
    }
}

void Player::DamageHurt3(soft3d::GameUnit* source, soft3d::Fixed speed)
{
    if (deathState != 0 || hurt3Request != 0) {
        return;
    }
    attacker = source;
    const auto* combat = dynamic_cast<const CombatTarget*>(source);
    const int32_t amount = std::max(combat != nullptr ? combat->GetAttackPower() : 1, 1);
    health -= amount;
    if (health < 1) {
        ++deathCount;
        hurt3Request = 1;
        deathState = 1;
    } else {
        hurt3Request = 1;
    }
    knockbackSpeed = speed;
}

void Player::RequestHurt4()
{
    SetVelocity({});
    hurt4Request = 1;
}

void Player::EnterFPS()
{
    if (deathState == 0) {
        EnterState(PlayerFPS);
    }
}

void Player::BeginQTE(int32_t delay, int32_t duration, int32_t requiredHits)
{
    if (deathState == 0) {
        qteDelay = delay;
        qteDuration = duration;
        qteRequiredHits = requiredHits;
        EnterState(PlayerQTE);
    }
}

void Player::BeginHide(const soft3d::Vec3& newPosition, const soft3d::Vec3& cameraDirection,
                       int32_t maskIndex, soft3d::Fixed horizontalLimit,
                       soft3d::Fixed verticalLimit)
{
    if (deathState != 0) {
        return;
    }
    hideMask = maskIndex;
    hideReturnPosition = position;
    position = newPosition;
    hideCameraDirection = cameraDirection;
    hideHorizontalLimit = horizontalLimit;
    hideVerticalLimit = verticalLimit;
    hideReturnFPS = fpsActive;
    StopSound();
    EnterState(PlayerHide);
}

void Player::RestoreHiddenState()
{
    position = hideReturnPosition;
    physicsFlags |= soft3d::PhysicSlideMovement;
    physicsFlags &= ~(soft3d::PhysicRestoreX | soft3d::PhysicRestoreY | soft3d::PhysicRestoreZ);
    scene->GetCameraController()->SetFrozen(false);
    if (hideMask >= 0) {
        scene->SetHideMask(hideMask, false);
    }
    hidden = 0;
    if (hideReturnFPS == 0) {
        presentationVisible = 1;
    }
}

void Player::ExitHide()
{
    RestoreHiddenState();
    EnterState(hideReturnFPS == 0 ? PlayerStand : PlayerFPS);
}

void Player::AddGunExperience(int32_t amount)
{
    if (scene->GetEventFlag(1002)) {
        return;
    }
    gunExperience += amount;
    if (gunLevel == 2) {
        return;
    }
    if (gunLevel == 0 && gunExperience >= 50) {
        PlaySound(142);
        scene->ShowTutorial(41, 64);
        gunLevel = 1;
    }
    if (gunLevel == 1 && gunExperience >= 100) {
        PlaySound(142);
        scene->ShowTutorial(42, 64);
        gunLevel = 2;
    }
}

int32_t Player::GetHealthPackCount() const
{
    return scene->GetUnitManager()->GetHealthPackCount();
}

bool Player::IsInventoryFull(int32_t itemId) const
{
    for (const int32_t item : inventory) {
        if (item == 0 || (item == itemId && itemId == 1)) {
            return false;
        }
    }
    return true;
}

void Player::AddItem(int32_t itemId, bool playSound)
{
    if (deathState != 0 || itemId == 0) {
        return;
    }
    if (playSound) {
        PlaySound(141);
    }
    auto& unlocks = scene->GetEngine().GetConfiguration().unlocks;
    if (itemId < 1001) {
        if (itemId >= 0 && itemId < 100) {
            unlocks.models[static_cast<std::size_t>(itemId)] = 1;
        }
        if (itemId == 1) {
            scene->GetUnitManager()->SetHealthPackCount(GetHealthPackCount() + 1);
            if (playSound) {
                selectedItem = 0;
            }
            if (inventory[0] != 1) {
                const int32_t previous = inventory[0];
                inventory[0] = 1;
                AddItem(previous, false);
            }
            return;
        }
        for (int slot = 0; slot < 9; ++slot) {
            if (inventory[slot] == 0) {
                inventory[slot] = itemId;
                if (playSound) {
                    selectedItem = slot;
                }
                return;
            }
        }
        return;
    }

    int slot = 0;
    while (slot < 128 && notes[slot] != 0) {
        ++slot;
    }
    if (slot < 128) {
        notes[slot] = itemId;
    }
    selectedNote = slot;
    if (itemId < 2001) {
        const int index = itemId - 1001;
        if (index >= 0 && index < 100) unlocks.documents[index] = 1;
    } else {
        const int index = itemId - 2001;
        if (index >= 0 && index < 100) unlocks.photos[index] = 1;
    }
}

bool Player::HasItem(int32_t itemId) const
{
    if (deathState != 0 || itemId < 0) {
        return false;
    }
    if (itemId < 1001) {
        return std::find(inventory.begin(), inventory.end(), itemId) != inventory.end();
    }
    return std::find(notes.begin(), notes.end(), itemId) != notes.end();
}

void Player::RemoveItem(int32_t itemId)
{
    if (itemId < 1001) {
        for (int32_t& item : inventory) {
            if (item == itemId) {
                item = 0;
                if (itemId == 1) {
                    scene->GetUnitManager()->SetHealthPackCount(0);
                }
                break;
            }
        }
        CompactInventory();
        return;
    }
    for (int32_t& note : notes) {
        if (note == itemId) {
            note = 0;
            return;
        }
    }
}

int32_t Player::UseItem(int32_t slot)
{
    if (slot < 0 || slot >= 9) {
        return 0;
    }
    const int32_t itemId = inventory[slot];
    if (itemId == 1) {
        if (health == maxHealth) {
            return 0;
        }
        ++itemUseCount;
        health = std::min(health + (maxHealth >> 1), maxHealth);
        scene->GetUnitManager()->SetHealthPackCount(GetHealthPackCount() - 1);
        if (GetHealthPackCount() < 1) {
            inventory[slot] = 0;
            CompactInventory();
        }
        return 1;
    }
    for (int32_t index = 0; index < itemTriggerCount; ++index) {
        if (itemTriggers[static_cast<std::size_t>(index)]->AcceptsItem(itemId)) {
            usedItem = itemId;
            CompactInventory();
            return 2;
        }
    }
    return 0;
}

bool Player::CombineItems(int32_t firstSlot, int32_t secondSlot)
{
    if (firstSlot < 0 || firstSlot >= 9 || secondSlot < 0 || secondSlot >= 9 ||
        inventory[firstSlot] == 0 || inventory[secondSlot] == 0) {
        return false;
    }
    const int32_t first = inventory[firstSlot];
    const auto& content = scene->GetEngine().GetContent();
    if (first < 0 || first >= static_cast<int32_t>(content.items.size()) ||
        content.items[first].combineWithItemId != inventory[secondSlot]) {
        return false;
    }
    inventory[firstSlot] = 0;
    inventory[secondSlot] = 0;
    AddItem(content.items[first].combinedItemId, true);
    CompactInventory();
    return true;
}

void Player::CompactInventory()
{
    const auto old = inventory;
    inventory.fill(0);
    inventory[0] = old[0];
    for (int slot = 1; slot < 9; ++slot) {
        if (old[slot] != 0) {
            AddItem(old[slot], false);
        }
    }
}

void Player::RegisterItemTrigger(ItemTrigger* trigger)
{
    if (trigger != nullptr && itemTriggerCount < static_cast<int32_t>(itemTriggers.size())) {
        itemTriggers[static_cast<std::size_t>(itemTriggerCount++)] = trigger;
    }
}

int32_t Player::GetInventoryItem(std::size_t slot) const { return inventory.at(slot); }
int32_t Player::GetNote(std::size_t slot) const { return notes.at(slot); }

void Player::SnapshotDayState()
{
    inventoryDayStart = inventory;
    notesDayStart = notes;
    healthPackDayStart = GetHealthPackCount();
    savedItemUseCount = itemUseCount;
    savedKillCount = killCount;
}

void Player::RestoreDayState()
{
    inventory = inventoryDayStart;
    notes = notesDayStart;
    scene->GetUnitManager()->SetHealthPackCount(healthPackDayStart);
    itemUseCount = savedItemUseCount;
    killCount = savedKillCount;
}

void Player::SetAnimationPaused(bool paused)
{
    for (std::size_t index = 0; index < GetObjectCount(); ++index) {
        GetObject(index)->SetAnimationPaused(paused);
    }
}

void Player::SetAnimationSpeed(int32_t speed)
{
    for (std::size_t index = 0; index < GetObjectCount(); ++index) {
        GetObject(index)->SetAnimationSpeed(speed);
    }
    if (shadowObject != nullptr) {
        shadowObject->SetAnimationSpeed(speed);
    }
}

int32_t Player::GetAnimationTime() const
{
    return GetObject(0) != nullptr ? GetObject(0)->GetAnimationTime() : 0;
}

void Player::SaveState(soft3d::OutputStream& stream) const
{
    VisUnit::SaveState(stream);
    stream.WriteInt32(IsNavigationEnabled() ? 1 : 0);
    stream.WriteInt32(health);
    stream.WriteInt32(maxHealth);
    stream.WriteInt32(hidden);
    stream.WriteInt32(hideMask);
    soft3d::WriteVec3(stream, hideReturnPosition);
    stream.WriteInt32(GetWeapon());
    stream.WriteInt32(gunExperience);
    stream.WriteInt32(gunLevel);
    stream.WriteInt32(fpsActive);
    for (int32_t value : inventory) stream.WriteInt32(value);
    for (int32_t value : inventoryDayStart) stream.WriteInt32(value);
    stream.WriteInt32(GetHealthPackCount());
    for (int32_t value : notes) stream.WriteInt32(value);
    for (int32_t value : notesDayStart) stream.WriteInt32(value);
}

void Player::LoadState(soft3d::InputStream& stream)
{
    Reset(scene->GetUnitManager()->GetData());
    stream.Seek(4, 1);
    VisUnit::LoadState(stream);
    int32_t value = 0;
    stream.ReadInt32(value);
    Creature::SetNavigationEnabled(value != 0);
    stream.ReadInt32(health);
    stream.ReadInt32(maxHealth);
    stream.ReadInt32(hidden);
    stream.ReadInt32(hideMask);
    soft3d::ReadVec3(stream, hideReturnPosition);
    stream.ReadInt32(value);
    SetWeapon(value);
    stream.ReadInt32(gunExperience);
    stream.ReadInt32(gunLevel);
    stream.ReadInt32(fpsActive);
    for (int32_t& item : inventory) stream.ReadInt32(item);
    for (int32_t& item : inventoryDayStart) stream.ReadInt32(item);
    stream.ReadInt32(value);
    scene->GetUnitManager()->SetHealthPackCount(value);
    for (int32_t& note : notes) stream.ReadInt32(note);
    for (int32_t& note : notesDayStart) stream.ReadInt32(note);
    StateMachine<Player>::Reset(this);
}

} // namespace game

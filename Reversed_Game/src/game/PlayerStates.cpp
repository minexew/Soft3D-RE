#include "game/Player.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"

#include <algorithm>

namespace game {
namespace {

bool Moving(Player* player)
{
    return soft3d::AbsFixed(player->GetVelocity().x) >= 0x28f ||
           soft3d::AbsFixed(player->GetVelocity().z) >= 0x28f;
}

bool Turning(Player* player)
{
    return player->GetYawVelocity() != 0;
}

void SetFlag(Player* player, uint32_t flag, bool enabled)
{
    const uint32_t flags = player->GetPhysicsFlags();
    player->SetPhysicsFlags(enabled ? flags | flag : flags & ~flag);
}

void ClearVelocity(Player* player) { player->SetVelocity({}); }

} // namespace

void PlayerStateSet::InitializeStates(Player*)
{
    stand.Register(this, PlayerStand);
    run.Register(this, PlayerRun);
    attackA1.Register(this, PlayerAttackA1);
    attackA2.Register(this, PlayerAttackA2);
    attackA3.Register(this, PlayerAttackA3);
    attackB1.Register(this, PlayerAttackB1);
    attackB2.Register(this, PlayerAttackB2);
    attackB3.Register(this, PlayerAttackB3);
    attackC1.Register(this, PlayerAttackC1);
    attackC2.Register(this, PlayerAttackC2);
    attackC3.Register(this, PlayerAttackC3);
    hurt.Register(this, PlayerHurt);
    hurt2.Register(this, PlayerHurt2);
    hurt3.Register(this, PlayerHurt3);
    hurt4.Register(this, PlayerHurt4);
    die.Register(this, PlayerDie);
    push.Register(this, PlayerPush);
    qte.Register(this, PlayerQTE);
    hide.Register(this, PlayerHide);
    fps.Register(this, PlayerFPS);
}

#define PLAYER_REGISTER(name) \
    void PlayerStateSet::name::Register(soft3d::StateSet<Player>* set, int index) \
    { soft3d::State<Player>::Register(set, index); }

PLAYER_REGISTER(CAttackA1)
PLAYER_REGISTER(CAttackA2)
PLAYER_REGISTER(CAttackA3)
PLAYER_REGISTER(CAttackB1)
PLAYER_REGISTER(CAttackB2)
PLAYER_REGISTER(CAttackB3)
PLAYER_REGISTER(CAttackC1)
PLAYER_REGISTER(CAttackC2)
PLAYER_REGISTER(CAttackC3)
PLAYER_REGISTER(CHurt)
PLAYER_REGISTER(CHurt2)
PLAYER_REGISTER(CHurt3)
PLAYER_REGISTER(CHurt4)
PLAYER_REGISTER(CDie)
PLAYER_REGISTER(CPush)
PLAYER_REGISTER(CQTE)
PLAYER_REGISTER(CHide)
PLAYER_REGISTER(CFPS)
#undef PLAYER_REGISTER

void PlayerStateSet::CStand::Enter(Player* player) { player->SetAction(0, 1, true); }
void PlayerStateSet::CStand::Exit(Player*) {}

soft3d::State<Player>* PlayerStateSet::CStand::Update(Player* player)
{
    player->UpdateMovement(player->GetNavigationSpeed());
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    if (player->attacksDisabled == 0 && player->GetWeapon() > 0) {
        if (player->GetWeapon() == 4) {
            if (player->ActionClick(5)) return player->GetState(PlayerAttackC1);
        } else {
            if (player->ActionClick(5)) return player->GetState(PlayerAttackA1);
            if (player->ActionPress(5)) {
                return player->GetState(player->GetWeapon() == 3 ? PlayerAttackC1 : PlayerAttackB1);
            }
        }
    }
    return Moving(player) || Turning(player) ? player->GetState(PlayerRun) : nullptr;
}

void PlayerStateSet::CRun::Enter(Player* player)
{
    player->SetAction(1, 1, true);
    SetFlag(player, soft3d::PhysicStepOffset, true);
    player->PlaySound(21, true);
}
void PlayerStateSet::CRun::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicStepOffset, false);
    player->StopSound();
}

soft3d::State<Player>* PlayerStateSet::CRun::Update(Player* player)
{
    player->UpdateMovement(player->GetNavigationSpeed());
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    if (player->attacksDisabled == 0 && player->GetWeapon() > 0) {
        if (player->GetWeapon() == 4) {
            if (player->ActionClick(5)) return player->GetState(PlayerAttackC1);
        } else {
            if (player->ActionClick(5)) return player->GetState(PlayerAttackA1);
            if (player->ActionPress(5)) {
                return player->GetState(player->GetWeapon() == 3 ? PlayerAttackC1 : PlayerAttackB1);
            }
        }
    }
    if (!Moving(player) && !Turning(player)) return player->GetState(PlayerStand);

    soft3d::ObjectBounds query = player->GetCollisionBounds();
    query.minimum.x -= 0xfd70;
    query.maximum.x -= 0xfd70;
    query.minimum.z += 0xfd70;
    query.maximum.z += 0xfd70;
    const auto& hits = player->scene->GetUnitManager()->QueryBounds(query, 2, nullptr);
    if (hits.count != 1 || hits.units[0]->GetUnitType() != 0x1d) return nullptr;
    auto* target = dynamic_cast<PushableTarget*>(hits.units[0]);
    if (target == nullptr || !target->CanBePushed()) return nullptr;
    const soft3d::Vec3 delta = soft3d::Subtract(hits.units[0]->GetPosition(), player->GetPosition());
    if (soft3d::AbsFixed(delta.x) >= 0x20000 && soft3d::AbsFixed(delta.z) >= 0x20000) return nullptr;
    const soft3d::Vec3 direction = soft3d::Normalize(player->GetForward());
    const soft3d::Vec3 normalized = soft3d::Normalize(delta);
    if (soft3d::Dot(direction, normalized) <= 0x9999) return nullptr;
    player->pushTarget = hits.units[0];
    player->scene->GetUnitManager()->SetPushTarget(hits.units[0]);
    return player->GetState(PlayerPush);
}

void PlayerStateSet::CAttackA1::Enter(Player* player)
{
    ClearVelocity(player);
    player->SelectAttackTarget();
    ClearVelocity(player);
    player->SetAction(2, 1, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    static constexpr soft3d::Fixed blend[5] = {0, 0x1999, 0x6666, 0x6666, 0};
    player->attackBlend = blend[std::clamp(player->GetWeapon(), 0, 4)];
}
void PlayerStateSet::CAttackA1::Exit(Player* player) { SetFlag(player, soft3d::PhysicKeepDirection, false); }
soft3d::State<Player>* PlayerStateSet::CAttackA1::Update(Player* player)
{
    ClearVelocity(player);
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    return player->IsAnimationFinished() ? player->GetState(PlayerAttackA2) : nullptr;
}

void PlayerStateSet::CAttackB1::Enter(Player* player)
{
    ClearVelocity(player);
    player->SelectAttackTarget();
    ClearVelocity(player);
    player->SetAction(5, 1, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    static constexpr soft3d::Fixed blend[5] = {0, 0x3333, 0xcccc, soft3d::fixedOne, 0};
    player->attackBlend = blend[std::clamp(player->GetWeapon(), 0, 4)];
}
void PlayerStateSet::CAttackB1::Exit(Player* player) { SetFlag(player, soft3d::PhysicKeepDirection, false); }
soft3d::State<Player>* PlayerStateSet::CAttackB1::Update(Player* player)
{
    ClearVelocity(player);
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    return player->IsAnimationFinished() ? player->GetState(PlayerAttackB2) : nullptr;
}

#define ATTACK_SECOND_ENTER(name, action, phase) \
    void PlayerStateSet::name::Enter(Player* player) \
    { \
        ClearVelocity(player); \
        player->attackPhase = phase; \
        player->stateTimer = 1; \
        player->stateCounter = 0; \
        player->SetAction(action, 1, false); \
        player->sword->BeginHitWindow(); \
        SetFlag(player, soft3d::PhysicStepOffset, true); \
        SetFlag(player, soft3d::PhysicKeepDirection, true); \
        SetFlag(player, soft3d::PhysicSimpleMovement, false); \
        if (player->GetWeapon() == 1) player->PlaySound(22); \
        else if (player->GetWeapon() == 2 || (phase == 1 && player->GetWeapon() == 3)) \
            player->PlaySound(23); \
    }

ATTACK_SECOND_ENTER(CAttackA2, 3, 1)
ATTACK_SECOND_ENTER(CAttackB2, 6, 1)
#undef ATTACK_SECOND_ENTER

void PlayerStateSet::CAttackA2::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicStepOffset, false);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    player->sword->SetHitActive(false);
}
void PlayerStateSet::CAttackB2::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicStepOffset, false);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    player->sword->SetHitActive(false);
}

#define ATTACK_SECOND_UPDATE(name, nextState) \
    soft3d::State<Player>* PlayerStateSet::name::Update(Player* player) \
    { \
        ClearVelocity(player); \
        if (soft3d::State<Player>* next = player->CommonTransition()) return next; \
        if (player->stateCounter > 0) { \
            player->sword->SetHitActive(false); \
            player->attackPhase = 0; \
        } \
        if (player->stateTimer != 0 && player->IsAnimationFinished()) { \
            ClearVelocity(player); \
            player->stateTimer = 0; \
            player->stateCounter = 0; \
        } \
        if (player->IsAnimationFinished()) { \
            if (player->stateCounter == 2) return player->GetState(nextState); \
            ++player->stateCounter; \
        } \
        return nullptr; \
    }

ATTACK_SECOND_UPDATE(CAttackA2, PlayerAttackA3)
ATTACK_SECOND_UPDATE(CAttackB2, PlayerAttackB3)
#undef ATTACK_SECOND_UPDATE

#define ATTACK_THIRD_ENTER(name, action) \
    void PlayerStateSet::name::Enter(Player* player) \
    { \
        ClearVelocity(player); \
        player->SetAction(action, 1, false); \
        SetFlag(player, soft3d::PhysicStepOffset, true); \
        SetFlag(player, soft3d::PhysicKeepDirection, true); \
        SetFlag(player, soft3d::PhysicSimpleMovement, false); \
    } \
    void PlayerStateSet::name::Exit(Player* player) \
    { \
        SetFlag(player, soft3d::PhysicStepOffset, false); \
        SetFlag(player, soft3d::PhysicKeepDirection, false); \
        SetFlag(player, soft3d::PhysicSimpleMovement, true); \
    } \
    soft3d::State<Player>* PlayerStateSet::name::Update(Player* player) \
    { \
        ClearVelocity(player); \
        if (soft3d::State<Player>* next = player->CommonTransition()) return next; \
        return player->IsAnimationFinished() ? player->GetState(PlayerStand) : nullptr; \
    }

ATTACK_THIRD_ENTER(CAttackA3, 4)
ATTACK_THIRD_ENTER(CAttackB3, 7)
#undef ATTACK_THIRD_ENTER

void PlayerStateSet::CAttackC1::Enter(Player* player)
{
    ClearVelocity(player);
    player->gunAttackActive = 1;
    ClearVelocity(player);
    player->SetAction(5, 1, false);
    if (player->GetWeapon() == 3 && player->gunLevel == 2) {
        player->SetAnimationSpeed(0x1c2);
        player->sword->SetAnimationSpeed(0x1c2);
    }
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    player->attackBlend = soft3d::fixedOne;
}
void PlayerStateSet::CAttackC1::Exit(Player* player)
{
    player->gunAttackActive = 0;
    SetFlag(player, soft3d::PhysicKeepDirection, false);
}
soft3d::State<Player>* PlayerStateSet::CAttackC1::Update(Player* player)
{
    ClearVelocity(player);
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    if (player->GetWeapon() == 3 && player->ActionUp(5)) return player->GetState(PlayerStand);
    player->SelectAttackTarget();
    return player->IsAnimationFinished() ? player->GetState(PlayerAttackC2) : nullptr;
}

void PlayerStateSet::CAttackC2::Enter(Player* player)
{
    ClearVelocity(player);
    player->gunAttackActive = 1;
    player->attackPhase = 2;
    player->stateTimer = 1;
    player->stateCounter = 0;
    player->SetAction(6, 1, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    SetFlag(player, soft3d::PhysicStepOffset, true);
    player->PlaySound(24);
    player->gunFire.Trigger(player->gunPosition, 1);
}
void PlayerStateSet::CAttackC2::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    SetFlag(player, soft3d::PhysicStepOffset, false);
    player->gunAttackActive = 0;
}
soft3d::State<Player>* PlayerStateSet::CAttackC2::Update(Player* player)
{
    ClearVelocity(player);
    player->attackPhase = 0;
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    if (player->stateTimer != 0 && player->IsAnimationFinished()) {
        player->stateTimer = 0;
        player->stateCounter = 0;
    }
    if (player->IsAnimationFinished()) {
        if (player->stateCounter == 2) return player->GetState(PlayerAttackC3);
        ++player->stateCounter;
    } else {
        ++player->stateTimer;
    }
    return nullptr;
}

void PlayerStateSet::CAttackC3::Enter(Player* player)
{
    ClearVelocity(player);
    player->gunAttackActive = 1;
    player->SetAction(7, 1, false);
    if (player->gunLevel == 2) {
        player->SetAnimationSpeed(0x1c2);
        player->sword->SetAnimationSpeed(0x1c2);
    }
    SetFlag(player, soft3d::PhysicStepOffset, true);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    SetFlag(player, soft3d::PhysicSimpleMovement, false);
}
void PlayerStateSet::CAttackC3::Exit(Player* player)
{
    player->gunAttackActive = 0;
    SetFlag(player, soft3d::PhysicStepOffset, false);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
}
soft3d::State<Player>* PlayerStateSet::CAttackC3::Update(Player* player)
{
    ClearVelocity(player);
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    return player->IsAnimationFinished() ? player->GetState(PlayerStand) : nullptr;
}

void PlayerStateSet::CHurt::Enter(Player* player)
{
    player->FaceAttacker();
    ClearVelocity(player);
    player->SetAction(8, 1, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    player->hurtRequest = 0;
    player->PlaySound(25);
}
void PlayerStateSet::CHurt::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
}
soft3d::State<Player>* PlayerStateSet::CHurt::Update(Player* player)
{
    ClearVelocity(player);
    if (soft3d::State<Player>* next = player->CommonTransition()) return next;
    return player->IsAnimationFinished() ? player->GetState(PlayerStand) : nullptr;
}

void PlayerStateSet::CHurt2::Enter(Player* player)
{
    ClearVelocity(player);
    player->SetAction(11, 1, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    player->physicsFlags |= 0xe0;
    player->PlaySound(25);
}
void PlayerStateSet::CHurt2::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    player->physicsFlags &= ~0xe0U;
    player->hurt2Request = 0;
}
soft3d::State<Player>* PlayerStateSet::CHurt2::Update(Player* player)
{
    ClearVelocity(player);
    return player->IsAnimationFinished() ? player->GetState(PlayerStand) : nullptr;
}

void PlayerStateSet::CHurt3::Enter(Player* player)
{
    player->FaceAttacker();
    const soft3d::Vec3 forward = player->GetForward();
    player->SetVelocity(soft3d::Scale(forward, -player->knockbackSpeed));
    player->knockbackVelocity = player->GetVelocity();
    player->SetAction(12, 1, false);
    SetFlag(player, soft3d::PhysicSimpleMovement, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    player->PlaySound(25);
    player->stateTimer = 0;
}
void PlayerStateSet::CHurt3::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    player->hurt3Request = 0;
}
soft3d::State<Player>* PlayerStateSet::CHurt3::Update(Player* player)
{
    ++player->stateTimer;
    if (player->stateTimer < 5) {
        player->velocity = soft3d::Add(player->velocity,
            soft3d::Scale(player->GetForward(), 0x4000));
        if (soft3d::Dot(player->velocity, player->knockbackVelocity) < 0) {
            ClearVelocity(player);
            player->stateTimer = 5;
        }
    } else {
        ClearVelocity(player);
    }
    return player->IsAnimationFinished() ? player->GetState(PlayerStand) : nullptr;
}

void PlayerStateSet::CHurt4::Enter(Player* player)
{
    ClearVelocity(player);
    player->SetAction(12, 1, true);
    player->stateTimer = 24;
    SetFlag(player, soft3d::PhysicSimpleMovement, false);
    SetFlag(player, soft3d::PhysicKeepDirection, true);
    player->physicsFlags |= 0xe0;
}
void PlayerStateSet::CHurt4::Exit(Player* player)
{
    player->hurt4Request = 0;
    SetFlag(player, soft3d::PhysicSimpleMovement, true);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    player->physicsFlags &= ~0xe0U;
}
soft3d::State<Player>* PlayerStateSet::CHurt4::Update(Player* player)
{
    player->UpdateMovement(player->GetNavigationSpeed());
    if (player->deathState != 0) return player->GetState(PlayerDie);
    if (player->hurtRequest != 0) return player->GetState(PlayerHurt);
    if (player->hurt2Request != 0) return player->GetState(PlayerHurt2);
    if (player->hurt3Request != 0) return player->GetState(PlayerHurt3);
    --player->stateTimer;
    return player->stateTimer < 1 ? player->GetState(PlayerStand) : nullptr;
}

void PlayerStateSet::CDie::Enter(Player* player)
{
    ClearVelocity(player);
    if (player->deathAnimation != 0) {
        player->SetAction(player->hurt2Request != 0 ? 11 : 9, 1, false);
    }
    SetFlag(player, soft3d::PhysicSlideMovement, false);
    player->physicsFlags |= 0xe0;
    if (player->deathAnimation != 0) player->PlaySound(26);
}
void PlayerStateSet::CDie::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicSlideMovement, true);
    player->physicsFlags &= ~0xe0U;
    player->hurt2Request = 0;
    player->hurt3Request = 0;
    player->hurt4Request = 0;
}
soft3d::State<Player>* PlayerStateSet::CDie::Update(Player* player)
{
    ClearVelocity(player);
    if (player->deathAnimation == 0) {
        player->deathState = 2;
        player->GetRenderer()->SelectShader(nullptr);
    } else if (player->hurt2Request == 0) {
        if (player->IsAnimationFinished()) {
            player->deathState = 2;
            player->GetRenderer()->SelectShader(nullptr);
        }
    } else if (player->GetAnimationTime() == 0x1518) {
        player->SetAnimationPaused(true);
        player->deathState = 2;
        player->GetRenderer()->SelectShader(nullptr);
    }
    return nullptr;
}

void PlayerStateSet::CPush::Enter(Player* player) { player->BeginPush(); }
void PlayerStateSet::CPush::Exit(Player* player)
{
    auto* target = dynamic_cast<PushableTarget*>(player->pushTarget);
    if (target != nullptr) target->SetPusher(nullptr);
    if (player->pushTarget != nullptr) player->pushTarget->SetCollisionEnabled(true);
    ClearVelocity(player);
    SetFlag(player, soft3d::PhysicKeepDirection, false);
    player->pushing = 0;
}
soft3d::State<Player>* PlayerStateSet::CPush::Update(Player* player)
{
    if (!player->IsAnimationFinished()) return nullptr;
    player->UpdateMovement(player->GetNavigationSpeed());
    if (!Moving(player) || Turning(player)) return player->GetState(PlayerStand);
    player->velocity = soft3d::Normalize(player->velocity);
    const soft3d::Vec3 forward = player->GetForward();
    if (soft3d::Dot(player->velocity, forward) <= 0x9999) return player->GetState(PlayerRun);
    auto* target = dynamic_cast<PushableTarget*>(player->pushTarget);
    if (target != nullptr && target->GetPushSound() != nullptr) {
        player->PlaySound(target->GetPushSound());
    }
    player->SetAction(10, 1, false);
    player->SetLocalBounds(player->pushCombinedBounds);
    player->SetVelocity(soft3d::Scale(soft3d::Scale(forward, player->GetNavigationSpeed()), 0x4000));
    return nullptr;
}

void PlayerStateSet::CQTE::Enter(Player* player)
{
    ClearVelocity(player);
    SetFlag(player, soft3d::PhysicSlideMovement, false);
    player->physicsFlags |= 0xe0;
    player->stateTimer = player->qteDelay;
    player->stateCounter = player->qteDuration;
    player->qteResult = -1;
}
void PlayerStateSet::CQTE::Exit(Player* player)
{
    SetFlag(player, soft3d::PhysicSlideMovement, true);
    player->physicsFlags &= ~0xe0U;
}
soft3d::State<Player>* PlayerStateSet::CQTE::Update(Player* player)
{
    if (player->deathState != 0) return player->GetState(PlayerDie);
    if (player->qteResult != -1) return nullptr;
    soft3d::Input& input = player->scene->GetEngine().GetInput();
    if (player->stateTimer < 1) {
        if (input.IsAnyKeyClick()) {
            if (!input.IsKeyClick(player->QteInputKey())) {
                if (player->qteRequiredHits == 1) {
                    player->qteResult = 0;
                    player->scene->FailQte();
                    return nullptr;
                }
            } else {
                ++player->qteHits;
                if (player->qteRequiredHits == 1 && player->qteHits == player->qteRequiredHits) {
                    player->qteResult = 1;
                    player->scene->ShowQte(player->qteKey, 0, 0);
                    return nullptr;
                }
            }
        }
        if (player->stateCounter > 0 && --player->stateCounter == 0) {
            if (player->qteHits < player->qteRequiredHits) {
                player->qteResult = 0;
                player->scene->FailQte();
            } else {
                player->qteResult = 1;
                player->scene->ShowQte(player->qteKey, 0, 0);
            }
        }
        return nullptr;
    }

    if (--player->stateTimer == 0) {
        player->qteKey = static_cast<int32_t>(player->scene->GetUnitManager()->NextRandom() % 4) + 1;
        player->qteHits = 0;
        player->scene->ShowQte(player->qteKey, player->qteDuration,
                               player->qteRequiredHits == 1 ? 0 : 2);
    } else {
        player->scene->SetQteMode(player->qteRequiredHits == 1 ? 1 : 2);
    }
    return nullptr;
}

void PlayerStateSet::CHide::Enter(Player* player)
{
    ClearVelocity(player);
    SetFlag(player, soft3d::PhysicSlideMovement, false);
    player->physicsFlags |= 0xe0;
    player->scene->GetCameraController()->SetFrozen(true);
    player->scene->GetCameraController()->SetFirstPerson(player->hideCameraDirection, false, false,
        player->hideHorizontalLimit, player->hideVerticalLimit);
    if (player->hideMask >= 0) player->scene->SetHideMask(player->hideMask, true);
    player->hidden = 1;
    player->presentationVisible = 0;
}
void PlayerStateSet::CHide::Exit(Player* player) { player->RestoreHiddenState(); }
soft3d::State<Player>* PlayerStateSet::CHide::Update(Player* player)
{
    if (!player->ActionClick(5) && !player->ActionClick(7)) return nullptr;
    player->scene->GetEngine().GetInput().ClearKey();
    return player->GetState(player->hideReturnFPS == 0 ? PlayerStand : PlayerFPS);
}

void PlayerStateSet::CFPS::Enter(Player* player)
{
    player->scene->GetCameraController()->SetFrozen(true);
    player->scene->GetCameraController()->SetFirstPerson(player->GetForward(), false, true,
                                                         0x9999, 0xcccc);
    player->presentationVisible = 0;
    player->fpsActive = 1;
    player->localBounds.minimum.y = -soft3d::fixedOne;
    player->localBounds.maximum.y = soft3d::fixedOne;
}
void PlayerStateSet::CFPS::Exit(Player* player)
{
    player->scene->GetCameraController()->SetFrozen(false);
    player->presentationVisible = 1;
    player->fpsActive = 0;
    if (player->GetObject(0) != nullptr) {
        player->SetLocalBounds(player->GetObject(0)->GetPrimaryBounds());
    }
}
soft3d::State<Player>* PlayerStateSet::CFPS::Update(Player* player)
{
    player->UpdateMovement(soft3d::FixedMultiply(player->GetNavigationSpeed(), 0x4ccc));
    return nullptr;
}

void Player::FaceAttacker()
{
    if (attacker != nullptr) {
        SetDirection(soft3d::Subtract(attacker->GetPosition(), position));
    }
}

void Player::StartPush(soft3d::GameUnit* target)
{
    pushTarget = target;
    scene->GetUnitManager()->SetPushTarget(target);
    EnterState(PlayerPush);
}

void Player::BeginPush()
{
    SetAction(10, 1, false);
    SetFlag(this, soft3d::PhysicKeepDirection, true);
    pushing = 1;
    pushLocalBounds = localBounds;
    soft3d::Vec3 adjustedPosition = position;
    auto* target = dynamic_cast<PushableTarget*>(pushTarget);
    auto* visual = dynamic_cast<soft3d::VisUnit*>(pushTarget);
    soft3d::Object* object = visual != nullptr ? visual->GetObject(0) : nullptr;
    if (object == nullptr) {
        return;
    }
    const soft3d::ObjectBounds world = object->GetWorldBounds();
    const soft3d::ObjectBounds targetLocal{
        soft3d::Subtract(world.minimum, pushTarget->GetPosition()),
        soft3d::Subtract(world.maximum, pushTarget->GetPosition())};
    if (target != nullptr && target->GetPushSound() != nullptr) PlaySound(target->GetPushSound());
    soft3d::Vec3 direction;
    pushCombinedBounds = localBounds;
    if (position.x < world.minimum.x) {
        direction = {soft3d::fixedOne, 0, 0};
        adjustedPosition.x = soft3d::SubtractFixed(world.minimum.x, localBounds.maximum.x);
        adjustedPosition.z = pushTarget->GetPosition().z;
        pushCombinedBounds.maximum.x = soft3d::AddFixed(pushCombinedBounds.maximum.x,
            soft3d::SubtractFixed(world.maximum.x, world.minimum.x));
        pushCombinedBounds.minimum.z = targetLocal.minimum.z;
        pushCombinedBounds.maximum.z = targetLocal.maximum.z;
    } else if (position.x > world.maximum.x) {
        direction = {-soft3d::fixedOne, 0, 0};
        adjustedPosition.x = soft3d::SubtractFixed(world.maximum.x, localBounds.minimum.x);
        adjustedPosition.z = pushTarget->GetPosition().z;
        pushCombinedBounds.minimum.x = soft3d::SubtractFixed(pushCombinedBounds.minimum.x,
            soft3d::SubtractFixed(world.maximum.x, world.minimum.x));
        pushCombinedBounds.minimum.z = targetLocal.minimum.z;
        pushCombinedBounds.maximum.z = targetLocal.maximum.z;
    } else if (position.z < world.minimum.z) {
        direction = {0, 0, soft3d::fixedOne};
        adjustedPosition.z = soft3d::SubtractFixed(world.minimum.z, localBounds.maximum.z);
        adjustedPosition.x = pushTarget->GetPosition().x;
        pushCombinedBounds.maximum.z = soft3d::AddFixed(pushCombinedBounds.maximum.z,
            soft3d::SubtractFixed(world.maximum.z, world.minimum.z));
        pushCombinedBounds.minimum.x = targetLocal.minimum.x;
        pushCombinedBounds.maximum.x = targetLocal.maximum.x;
    } else if (position.z > world.maximum.z) {
        direction = {0, 0, -soft3d::fixedOne};
        adjustedPosition.z = soft3d::SubtractFixed(world.maximum.z, localBounds.minimum.z);
        adjustedPosition.x = pushTarget->GetPosition().x;
        pushCombinedBounds.minimum.z = soft3d::SubtractFixed(pushCombinedBounds.minimum.z,
            soft3d::SubtractFixed(world.maximum.z, world.minimum.z));
        pushCombinedBounds.minimum.x = targetLocal.minimum.x;
        pushCombinedBounds.maximum.x = targetLocal.maximum.x;
    }
    SetLocalBounds(pushCombinedBounds);
    SetPosition(adjustedPosition);
    SetDirection(direction);
    if (Moving(this)) {
        SetVelocity(soft3d::Scale(soft3d::Scale(direction, GetNavigationSpeed()), 0x4000));
    }
    if (target != nullptr) target->SetPusher(this);
    pushTarget->SetCollisionEnabled(false);
}

} // namespace game

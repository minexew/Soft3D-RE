#include "game/Boss.hpp"

#include "game/MyGameUnitMgr.hpp"

#include <stdexcept>

namespace game {

Boss::Boss(const soft3d::UnitManagerContext& context)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp)
{
}

void Boss::ConfigureSequences(std::vector<BossActionSequence> values)
{
    for (const BossActionSequence& sequence : values) {
        if (sequence.actions.size() != sequence.delays.size() ||
            sequence.actions.size() != sequence.randomDelays.size()) {
            throw std::invalid_argument("Boss sequence arrays must have equal lengths");
        }
    }
    sequences = std::move(values);
}

void Boss::SetAttackAnimation(int32_t index, const soft3d::Animation* value)
{
    if (index >= 0 && static_cast<std::size_t>(index) < attackAnimations.size()) {
        attackAnimations[static_cast<std::size_t>(index)] = value;
    }
}

bool Boss::StartSequence(int32_t index)
{
    if (index < 0 || static_cast<std::size_t>(index) >= sequences.size()) {
        return false;
    }
    currentSequence = index;
    currentActionIndex = 0;
    delayTimer = -1;
    currentAction = 0;
    return true;
}

void Boss::Spawn(const soft3d::Vec3& value)
{
    attackAnimationIndex = 0;
    currentSequence = -1;
    currentActionIndex = 0;
    delayTimer = -1;
    currentAction = 0;
    if (player != nullptr) {
        soft3d::Vec3 distancePoint = position;
        distancePoint.y = player->GetPosition().y;
        playerDistance = soft3d::Length(soft3d::Subtract(distancePoint, player->GetPosition()));
    }
    Enemy::Spawn(value);
}

bool Boss::CanAdvanceSequence() const
{
    return !IsNavigationEnabled() && moveMode == EnemyNoMovement && state != EnemyAttack &&
           state < EnemyDying;
}

void Boss::SelectNextSequence()
{
}

void Boss::Update()
{
    if (!IsNavigationEnabled()) {
        if (moveMode == EnemyNoMovement) {
            SelectNextSequence();
        } else {
            currentAction = 0;
            currentSequence = -1;
            delayTimer = -1;
        }
        if (!sequences.empty() && CanAdvanceSequence() && currentSequence != -1 &&
            static_cast<std::size_t>(currentSequence) < sequences.size()) {
            BossActionSequence& sequence = sequences[static_cast<std::size_t>(currentSequence)];
            bool advance = true;
            if (currentAction == 2) {
                if (playerDistance <= attackRange) {
                    StopMotion();
                } else {
                    advance = false;
                }
            } else if (delayTimer != -1) {
                --delayTimer;
                if (delayTimer == 0) {
                    delayTimer = -1;
                    StopMotion();
                }
                advance = false;
            }
            if (advance && static_cast<std::size_t>(currentActionIndex) < sequence.actions.size()) {
                const std::size_t index = static_cast<std::size_t>(currentActionIndex);
                delayTimer = sequence.delays[index];
                if (delayTimer != -1 && sequence.randomDelays[index] > 0) {
                    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
                    const uint32_t random = manager != nullptr ? manager->NextRandom() : 0;
                    delayTimer += static_cast<int32_t>(
                        (random % static_cast<uint32_t>(sequence.randomDelays[index] << 8)) >> 8);
                }
                currentAction = sequence.actions[index];
                ++currentActionIndex;
                switch (currentAction) {
                case 0:
                    StopMotion();
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    BeginAttack();
                    break;
                case 7:
                    StopMotion();
                    currentActionIndex = 0;
                    currentSequence = -1;
                    break;
                default:
                    break;
                }
            }
        }
    }
    Enemy::Update();
}

void Boss::UpdateIdle()
{
    if (ReceivePlayerAttack()) {
        return;
    }
    if (moveMode == EnemyNoMovement) {
        if (player != nullptr) {
            SetDirection(soft3d::Subtract(player->GetPosition(), position));
        }
    } else {
        BeginChase();
    }
}

void Boss::BeginAttack()
{
    switch (currentAction) {
    case 3: attackAnimationIndex = 0; break;
    case 4: attackAnimationIndex = 1; break;
    case 5: attackAnimationIndex = 2; break;
    case 6: attackAnimationIndex = 3; break;
    default: break;
    }
    waitTimer = -1;
    stateTime = 0;
    unused7ca = 0;
    state = EnemyAttack;
    velocity = {};
    SetAnimation(attackAnimations[static_cast<std::size_t>(attackAnimationIndex)], 1, false);
    if (player != nullptr) {
        SetDirection(soft3d::Subtract(player->GetPosition(), position));
    }
    attackHitDone = 0;
}

} // namespace game

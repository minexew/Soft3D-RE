#pragma once

#include "game/Enemy.hpp"

namespace game {

struct BossActionSequence {
    std::vector<int32_t> actions;
    std::vector<int32_t> delays;
    std::vector<int32_t> randomDelays;
};

class Boss : public Enemy {
public:
    Boss(const soft3d::UnitManagerContext& context = {});

    void Spawn(const soft3d::Vec3& value) override;
    void Update() override;
    void UpdateIdle() override;
    void BeginAttack() override;
    virtual void SelectNextSequence();

    void ConfigureSequences(std::vector<BossActionSequence> values);
    void SetAttackAnimation(int32_t index, const soft3d::Animation* value);
    bool StartSequence(int32_t index);

    int32_t GetCurrentSequence() const { return currentSequence; }
    int32_t GetCurrentActionIndex() const { return currentActionIndex; }
    int32_t GetCurrentAction() const { return currentAction; }
    int32_t GetDelayTimer() const { return delayTimer; }
    int32_t GetAttackAnimationIndex() const { return attackAnimationIndex; }
    std::size_t GetSequenceCount() const { return sequences.size(); }

private:
    bool CanAdvanceSequence() const;

    std::array<const soft3d::Animation*, 4> attackAnimations{};
    int32_t attackAnimationIndex = 0;
    int32_t currentSequence = -1;
    int32_t currentActionIndex = 0;
    int32_t delayTimer = -1;
    int32_t currentAction = 0;
    std::vector<BossActionSequence> sequences;
};

} // namespace game

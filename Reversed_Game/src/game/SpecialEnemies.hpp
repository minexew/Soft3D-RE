#pragma once

#include "game/Enemy.hpp"

namespace game {

class Worm : public Enemy {
public:
    Worm(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void Spawn(const soft3d::Vec3& value) override;
    void Update() override;
    void UpdateAlert() override;
    int32_t SelectAttack() override;
    void BeginAttack() override;
    bool TestAttackHit() override;
    void UpdateAttack() override;
    void UpdateDeath() override;

    bool IsAttackPathClear() const { return attackPathClear; }
    int32_t GetColorVariant() const { return colorVariant; }
    int16_t GetLandingCount() const { return unused7ca; }

private:
    const GameData& data;
    bool attackPathClear = false;
    int32_t colorVariant = 0;
};

class Ghost : public Enemy {
public:
    Ghost(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeSounds(soft3d::AudioData* const* values) override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void Update() override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateAttack() override;
    bool ReceivePlayerAttack() override;
    void StopSound() override;
    void SetNavigationEnabled(bool enabled) override;

    int32_t GetProximityVoice() const { return proximityVoice; }
    soft3d::AudioData* GetProximitySound() const { return proximitySound; }

private:
    void StopProximitySound();

    const GameData& data;
    soft3d::AudioData* proximitySound = nullptr;
    int32_t proximityVoice = -1;
};

class Lamper : public Enemy {
public:
    Lamper(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateIdle() override;
    void BeginAttack() override;
    bool TestAttackHit() override;
    bool ReceivePlayerAttack() override;
    bool UpdatePerception() override;
    void Damage(int32_t amount) override;

private:
    const GameData& data;
};

class Weed : public Enemy {
public:
    Weed(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    bool UpdatePerception() override;
    void Damage(int32_t amount) override;
    bool ReceivePlayerAttack() override;

private:
    const GameData& data;
};

std::unique_ptr<Enemy> CreateSpecialPoolEnemy(const soft3d::UnitManagerContext& context,
                                              const GameData& data, int32_t enemyType);

} // namespace game

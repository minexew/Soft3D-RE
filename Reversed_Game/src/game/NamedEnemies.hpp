#pragma once

#include "game/Enemy.hpp"

namespace game {

class Victor : public Enemy {
public:
    Victor(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void QueryUnitCollision() override;
    void Spawn(const soft3d::Vec3& value) override;
    void Update() override;
    void UpdateIdle() override;
    void UpdateChase() override;
    void BeginAttack() override;
    bool TestAttackHit() override;
    bool ReceivePlayerAttack() override;

    int32_t GetLeapPhase() const { return leapPhase; }
    const soft3d::Vec3& GetLeapTarget() const { return leapTarget; }
    soft3d::Fixed GetMinimumLeapDistance() const { return minimumLeapDistance; }
    soft3d::Fixed GetMaximumLeapDistance() const { return maximumLeapDistance; }
    const soft3d::Animation* GetLeapAnimation(std::size_t index) const;
    soft3d::AudioData* GetLeapSound(std::size_t index) const;

protected:
    void UpdateLeap();
    bool BeginLeapAtPlayer(int32_t selection);

    const GameData& data;
    std::array<const soft3d::Animation*, 3> leapAnimations{};
    std::array<soft3d::AudioData*, 3> leapSounds{};
    int32_t leapPhase = 0;
    soft3d::Vec3 leapTarget;
    soft3d::Fixed minimumLeapDistance = 0x140000;
    soft3d::Fixed maximumLeapDistance = 0x280000;
};

class Sam : public Enemy {
public:
    Sam(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    bool UpdatePerception() override;
    int32_t SelectAttack() override;
    void Spawn(const soft3d::Vec3& value) override;
    void Damage(int32_t amount) override;
    void Die() override;
    void UpdateDeath() override;

private:
    const GameData& data;
};

class Prisoner : public Enemy {
public:
    Prisoner(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    bool UpdatePerception() override;
    int32_t SelectAttack() override;
    void Update() override;
    void UpdateDeath() override;

    int32_t GetIdleVariant() const { return idleVariant; }

private:
    const GameData& data;
    std::array<const soft3d::Animation*, 3> idleAnimations{};
    int32_t idleVariant = 0;
};

class Bully;

class Kate : public Enemy {
public:
    Kate(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    bool UpdatePerception() override;
    int32_t SelectAttack() override;
    void UpdateIdle() override;
    void Update() override;

    void SetLinkedBully(Bully* value) { linkedBully = value; }
    Bully* GetLinkedBully() const { return linkedBully; }

private:
    void AlertLinkedBully();

    const GameData& data;
    Bully* linkedBully = nullptr;
};

class DarkKen : public Enemy {
public:
    DarkKen(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void QueryUnitCollision() override;
    void Spawn(const soft3d::Vec3& value) override;
    void Update() override;
    void PostUpdate() override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void SetNavigationEnabled(bool enabled) override;
    void UpdateIdle() override;
    void UpdateChase() override;
    int32_t SelectAttack() override;
    bool TestAttackHit() override;
    void UpdateAttack() override;
    void SetAnimationByIndex(int32_t index, int32_t direction = 1, bool loop = true) override;

    int32_t GetCurrentRegion() const { return currentRegion; }
    int32_t GetAttackPhase() const { return stateTime; }
    int32_t GetAttackStep() const { return unused7ca; }
    int32_t GetTauntCooldown() const { return tauntCooldown; }
    const soft3d::Vec3& GetRegionTarget() const { return regionTarget; }
    const soft3d::ObjectBounds& GetRegion(std::size_t index) const;
    int32_t GetRegionNeighbor(std::size_t region, std::size_t index) const;
    const soft3d::Animation* GetExtraAnimation(std::size_t index) const;
    soft3d::AudioData* GetExtraSound(std::size_t index) const;
    soft3d::Object* GetGunObject(std::size_t index) const;

protected:
    void InitializeRegions();
    void ChooseNextRegion();
    void SetRegionTarget(int32_t index);

    const GameData& data;
    std::array<const soft3d::Animation*, 2> attackAnimations{};
    std::array<soft3d::AudioData*, 2> attackSounds{};
    std::array<soft3d::ObjectBounds, 16> regions{};
    std::array<std::array<uint8_t, 15>, 16> regionNeighbors{};
    int32_t currentRegion = -1;
    std::array<int32_t, 3> recentRegions{{-1, -1, -1}};
    soft3d::Vec3 regionTarget;
    std::unique_ptr<soft3d::Object> gun;
    std::unique_ptr<soft3d::Object> gunSecondary;
    std::array<const soft3d::Animation*, 8> extraAnimations{};
    int32_t tauntCooldown = 16;
};

int CompareDarkKenRegionDistanceDescending(const Enemy* const* left, const Enemy* const* right);

std::unique_ptr<Enemy> CreateNamedPoolEnemy(const soft3d::UnitManagerContext& context,
                                            const GameData& data, int32_t enemyType);

} // namespace game

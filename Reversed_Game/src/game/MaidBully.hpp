#pragma once

#include "game/Enemy.hpp"

namespace game {

class Maid : public Enemy {
public:
    Maid(const soft3d::UnitManagerContext& context, const GameData& data);
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;

protected:
    const GameData& data;
};

class Maid2 : public Enemy {
public:
    Maid2(const soft3d::UnitManagerContext& context, const GameData& data);
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;

private:
    const GameData& data;
};

class Maid3 : public Enemy {
public:
    Maid3(const soft3d::UnitManagerContext& context, const GameData& data);
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateDeath() override;

private:
    const GameData& data;
};

class FatMaid : public Enemy {
public:
    FatMaid(const soft3d::UnitManagerContext& context, const GameData& data);

    void LoadResources(const char16_t* modelName, const soft3d::Animation* const* animationNames,
                       soft3d::AudioData* const* soundNames) override;
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void Update() override;
    void PostUpdate() override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateChase() override;
    void Damage(int32_t amount) override;
    bool ReceivePlayerAttack() override;

    int32_t GetThrowCooldown() const { return throwCooldown; }
    int32_t GetProjectileState() const { return projectileState; }
    soft3d::Fixed GetProjectileRange() const { return projectileRange; }
    const soft3d::Vec3& GetProjectileVelocity() const { return projectileVelocity; }
    const soft3d::Animation* GetThrowAnimation() const { return throwAnimation; }
    const soft3d::Animation* GetProjectileAnimation() const { return projectileAnimation; }
    soft3d::AudioData* GetProjectileSound() const { return projectileSound; }
    soft3d::Object* GetProjectileObject() const { return projectile.get(); }

protected:
    bool CanThrowProjectile() const;
    void UpdateThrow();
    void UpdateProjectile();
    void FinishProjectile();

    const GameData& data;
    int32_t throwCooldown = 0;
    int32_t projectileState = 0;
    soft3d::Fixed projectileRange = 0x140000;
    soft3d::Vec3 projectileStart;
    soft3d::Vec3 projectileTarget;
    soft3d::Vec3 projectileVelocity;
    int32_t projectileAnimationFinished = 0;
    std::unique_ptr<soft3d::Object> projectile;
    const soft3d::Animation* projectileAnimation = nullptr;
    const soft3d::Animation* throwAnimation = nullptr;
    soft3d::AudioData* projectileSound = nullptr;
};

class Bully : public Enemy {
public:
    Bully(const soft3d::UnitManagerContext& context, const GameData& data);
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    bool TestAttackHit() override;

    int32_t GetSecondAttackStart() const { return secondAttackStart; }
    int32_t GetSecondAttackEnd() const { return secondAttackEnd; }

private:
    const GameData& data;
    int32_t secondAttackStart = 23;
    int32_t secondAttackEnd = 25;
};

class BullyDay3 : public Enemy {
public:
    BullyDay3(const soft3d::UnitManagerContext& context, const GameData& data);

    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void Update() override;
    void PostUpdate() override;
    void QueryUnitCollision() override;
    void SetNavigationEnabled(bool enabled) override;
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateIdle() override;
    void UpdateChase() override;
    void BeginAttack() override;
    bool TestAttackHit() override;
    bool ReceivePlayerAttack() override;
    void Die() override;

    int32_t GetEncounterMode() const { return encounterMode; }
    int32_t GetAttackKind() const { return attackKind; }
    int32_t GetChargePhase() const { return chargePhase; }
    int32_t GetQtePhase() const { return qtePhase; }
    int32_t GetQteSuccessCount() const { return qteSuccessCount; }
    int32_t GetQteOutcome() const { return qteOutcome; }
    const soft3d::Vec3& GetSavedPlayerPosition() const { return savedPlayerPosition; }
    const soft3d::Animation* GetExtraAnimation(std::size_t index) const;
    soft3d::AudioData* GetExtraSound(std::size_t index) const;
    soft3d::VisUnit* GetQteObject() const { return qteObject.get(); }

protected:
    void UpdateCharge();
    void UpdateQte();
    void BeginQte(bool unlocked);
    void FacePlayer();
    void SetControlsDisabled(bool disabled);

    const GameData& data;
    std::array<const soft3d::Animation*, 11> extraAnimations{};
    std::array<soft3d::AudioData*, 9> extraSounds{};
    int32_t encounterMode = 0;
    int32_t attackKind = 0;
    int32_t chargePhase = 0;
    soft3d::Vec3 chargeTarget;
    std::unique_ptr<soft3d::VisUnit> qteObject;
    int32_t qtePhase = 0;
    int32_t qteSuccessCount = 0;
    soft3d::Vec3 savedPlayerPosition;
    int32_t qteOutcome = 0;
    int32_t secondAttackStart = 23;
    int32_t secondAttackEnd = 25;
    int32_t alternateAttackStart = 10;
    int32_t alternateAttackEnd = 14;
};

class BullySaw final : public ElectricSaw {
public:
    BullySaw(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations();
    void SetAnimationByIndex(int32_t index, int32_t direction, bool loop);
    const soft3d::Animation* GetAnimationResource(std::size_t index) const;

private:
    const GameData& data;
    std::array<const soft3d::Animation*, 11> animations{};
};

class BullyDay5 : public Enemy {
public:
    BullyDay5(const soft3d::UnitManagerContext& context, const GameData& data);

    void LoadResources(const char16_t* modelName, const soft3d::Animation* const* animationNames,
                       soft3d::AudioData* const* soundNames) override;
    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void Update() override;
    void PostUpdate() override;
    void QueryUnitCollision() override;
    void SetNavigationEnabled(bool enabled) override;
    void SetAnimation(const soft3d::Animation* resource, int32_t direction = 1,
                      bool loop = true) override;
    void Spawn(const soft3d::Vec3& value) override;
    void UpdateIdle() override;
    void UpdateChase() override;
    bool IsSegmentInside(const soft3d::Vec3& start, const soft3d::Vec3& end) override;
    bool TestAttackHit() override;
    bool ReceivePlayerAttack() override;

    int32_t GetSawState() const { return sawState; }
    int32_t GetChargePhase() const { return chargePhase; }
    int32_t GetSlamPhase() const { return slamPhase; }
    int32_t GetThrowPhase() const { return throwPhase; }
    int32_t GetImpactSoundPending() const { return impactSoundPending; }
    int32_t GetThrowTimer() const { return throwTimer; }
    int32_t GetSlamCooldown() const { return slamCooldown; }
    soft3d::Fixed GetChargeDistance() const { return chargeDistance; }
    soft3d::Fixed GetSlamRange() const { return slamRange; }
    soft3d::Fixed GetThrowSpeed() const { return throwSpeed; }
    const soft3d::Vec3& GetThrowVelocity() const { return throwVelocity; }
    const soft3d::Animation* GetExtraAnimation(std::size_t index) const;
    soft3d::AudioData* GetExtraSound(std::size_t index) const;
    BullySaw* GetSaw() const { return saw.get(); }

protected:
    void UpdateThrow();
    void UpdateChargeAttack();
    void UpdatePickup();
    void UpdateSlam();
    void UpdateThrownSaw();
    void EndThrow(bool fast);
    void FacePlayer();
    void BeginThrowAttack();
    void BeginSlamAttack();

    const GameData& data;
    std::array<const soft3d::Animation*, 4> extraAnimations{};
    std::array<soft3d::AudioData*, 9> extraSounds{};
    int32_t chargePhase = 0;
    int32_t slamPhase = 0;
    int32_t throwPhase = 0;
    soft3d::Fixed chargeDistance = 0xf0000;
    soft3d::Fixed slamRange = 0x280000;
    int32_t sawState = 2;
    std::unique_ptr<BullySaw> saw;
    int32_t impactSoundPending = 0;
    soft3d::Vec3 throwTarget;
    soft3d::Fixed throwSpeed = 0x38000;
    soft3d::Vec3 throwVelocity;
    int32_t throwTimer = 0;
    int32_t slamCooldown = 220;
};

std::unique_ptr<Enemy> CreateMaidBullyPoolEnemy(const soft3d::UnitManagerContext& context,
                                                const GameData& data, int32_t enemyType);

} // namespace game

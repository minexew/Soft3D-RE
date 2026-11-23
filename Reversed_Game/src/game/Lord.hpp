#pragma once

#include "game/Enemy.hpp"

namespace game {

class Lord : public Enemy {
public:
    Lord(const soft3d::UnitManagerContext& context, const GameData& data);

    void InitializeAnimations(const soft3d::Animation* const* values) override;
    void InitializeSounds(soft3d::AudioData* const* values) override;
    void Spawn(const soft3d::Vec3& value) override;
    void Update() override;
    void PostUpdate() override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void SetNavigationEnabled(bool enabled) override;
    void UpdateIdle() override;
    void UpdateChase() override;
    void BeginAttack() override;
    bool TestAttackHit() override;
    void UpdateAttack() override;
    bool ReceivePlayerAttack() override;
    void Die() override;
    void SetAnimationByIndex(int32_t index, int32_t direction = 1, bool loop = true) override;

    int32_t GetEncounterPhase() const { return encounterPhase; }
    int32_t GetEncounterTimer() const { return encounterTimer; }
    int32_t GetEquipmentStage() const { return equipmentStage; }
    int32_t GetAttackKind() const { return attackKind; }
    int32_t GetLeapPhase() const { return leapPhase; }
    int32_t GetHaste() const { return haste; }
    soft3d::Object* GetSwordObject() const { return sword.get(); }
    soft3d::Object* GetShieldObject() const { return shield.get(); }
    const soft3d::Animation* GetBodyAnimation(std::size_t index) const;
    const soft3d::Animation* GetSwordAnimation(std::size_t index) const;
    const soft3d::Animation* GetShieldAnimation(std::size_t index) const;
    soft3d::AudioData* GetExtraSound(std::size_t index) const;

protected:
    void UpdateEncounter();
    void UpdateLeap();
    bool TryBeginLeap();
    bool CanLeap() const;
    void SetControlsDisabled(bool disabled);
    void SetFixedCamera(const soft3d::Vec3& position, const soft3d::Vec3& direction);
    void RestorePlayerAndCamera(bool controlsEnabled = true);
    void SetEncounter(int32_t phase, int32_t timer = 0);
    void ReplaceCrystal(int32_t id, const soft3d::Vec3& position, int32_t bouncePeriod,
                        const soft3d::Vec3& playerPosition, const soft3d::Vec3& playerDirection);
    void BreakCrystal(int32_t id);
    void SetEquipmentStage(int32_t value);
    void PlayEquipmentAnimation(const soft3d::Animation* body,
                                const soft3d::Animation* swordAnimation,
                                const soft3d::Animation* shieldAnimation, bool loop);

    const GameData& data;
    std::array<const soft3d::Animation*, 19> bodyAnimations{};
    std::array<soft3d::AudioData*, 7> extraSounds{};
    std::unique_ptr<soft3d::Object> sword;
    std::array<const soft3d::Animation*, 16> swordAnimations{};
    std::unique_ptr<soft3d::Object> shield;
    std::array<const soft3d::Animation*, 13> shieldAnimations{};
    int32_t renderActive = 1;
    int32_t equipmentStage = 0;
    int32_t attackKind = 0;
    int32_t leapPhase = 0;
    soft3d::Vec3 leapTarget;
    int32_t haste = 0;
    int32_t alternateAttackStart = 11;
    int32_t alternateAttackEnd = 13;
    soft3d::Fixed primaryHitImpulse = soft3d::fixedOne;
    soft3d::Fixed alternateHitImpulse = 0x30000;
    int32_t encounterTimer = 4;
    int32_t encounterPhase = 1;
};

} // namespace game

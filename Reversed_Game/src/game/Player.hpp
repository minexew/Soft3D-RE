#pragma once

#include "game/Sword.hpp"
#include "soft3d/CameraSpot.hpp"
#include "soft3d/GameConfig.hpp"
#include "soft3d/StateMachine.hpp"
#include "soft3d/Units.hpp"

#include <array>
#include <vector>

namespace game {

class ItemTrigger;
class Scene;
class Player;

class CombatTarget {
public:
    virtual ~CombatTarget() = default;
    virtual bool IsAttackable() const = 0;
    virtual bool IsAttacking() const = 0;
    virtual int32_t GetAttackPower() const = 0;
    virtual soft3d::Fixed GetPlayerDistance() const = 0;
};

class PushableTarget {
public:
    virtual ~PushableTarget() = default;
    virtual bool CanBePushed() const = 0;
    virtual soft3d::AudioData* GetPushSound() const = 0;
    virtual void SetPusher(Player* player) = 0;
};

enum PlayerStateId : int32_t {
    PlayerStand = 0,
    PlayerRun = 1,
    PlayerAttackA1 = 2,
    PlayerAttackA2 = 3,
    PlayerAttackA3 = 4,
    PlayerAttackB1 = 5,
    PlayerAttackB2 = 6,
    PlayerAttackB3 = 7,
    PlayerAttackC1 = 8,
    PlayerAttackC2 = 9,
    PlayerAttackC3 = 10,
    PlayerHurt = 11,
    PlayerHurt2 = 12,
    PlayerHurt3 = 13,
    PlayerHurt4 = 14,
    PlayerDie = 15,
    PlayerPush = 16,
    PlayerQTE = 17,
    PlayerHide = 18,
    PlayerFPS = 19,
};

class Player;

class PlayerStateSet final : public soft3d::StateSet<Player> {
public:
    class CStand final : public soft3d::State<Player> {
    public:
        void Enter(Player*) override;
        void Exit(Player*) override;
        State<Player>* Update(Player*) override;
    };
    class CRun final : public soft3d::State<Player> {
    public:
        void Enter(Player*) override;
        void Exit(Player*) override;
        State<Player>* Update(Player*) override;
    };
#define SOFT3D_PLAYER_STATE(name) \
    class name final : public soft3d::State<Player> { \
    public: \
        void Enter(Player*) override; \
        void Exit(Player*) override; \
        State<Player>* Update(Player*) override; \
        void Register(soft3d::StateSet<Player>*, int) override; \
    }
    SOFT3D_PLAYER_STATE(CAttackA1);
    SOFT3D_PLAYER_STATE(CAttackA2);
    SOFT3D_PLAYER_STATE(CAttackA3);
    SOFT3D_PLAYER_STATE(CAttackB1);
    SOFT3D_PLAYER_STATE(CAttackB2);
    SOFT3D_PLAYER_STATE(CAttackB3);
    SOFT3D_PLAYER_STATE(CAttackC1);
    SOFT3D_PLAYER_STATE(CAttackC2);
    SOFT3D_PLAYER_STATE(CAttackC3);
    SOFT3D_PLAYER_STATE(CHurt);
    SOFT3D_PLAYER_STATE(CHurt2);
    SOFT3D_PLAYER_STATE(CHurt3);
    SOFT3D_PLAYER_STATE(CHurt4);
    SOFT3D_PLAYER_STATE(CDie);
    SOFT3D_PLAYER_STATE(CPush);
    SOFT3D_PLAYER_STATE(CQTE);
    SOFT3D_PLAYER_STATE(CHide);
    SOFT3D_PLAYER_STATE(CFPS);
#undef SOFT3D_PLAYER_STATE

    PlayerStateSet();
    soft3d::State<Player>* GetInitialState(Player*) override { return GetRegisteredState(0); }
    void InitializeStates(Player*) override;

private:
    CStand stand;
    CRun run;
    CAttackA1 attackA1;
    CAttackA2 attackA2;
    CAttackA3 attackA3;
    CAttackB1 attackB1;
    CAttackB2 attackB2;
    CAttackB3 attackB3;
    CAttackC1 attackC1;
    CAttackC2 attackC2;
    CAttackC3 attackC3;
    CHurt hurt;
    CHurt2 hurt2;
    CHurt3 hurt3;
    CHurt4 hurt4;
    CDie die;
    CPush push;
    CQTE qte;
    CHide hide;
    CFPS fps;
};

class Player final : public soft3d::Creature,
                     public soft3d::StateMachine<Player>,
                     public soft3d::CameraSpot {
public:
    Player(soft3d::s3dResManager* resources, soft3d::Renderer* renderer, soft3d::Bsp* bsp,
           Scene* scene, soft3d::AudioManager* audio);

    soft3d::result Initialize(const GameData& data);
    Scene* GetScene() const { return scene; }
    void ResetAll(const GameData& data);
    void Reset(const GameData& data);
    void ResetForMap();
    void SetVisible(bool value);
    bool NavigateTo(const soft3d::Vec3& target, soft3d::Fixed radius) override;
    void SetNavigationEnabled(bool enabled) override;

    void Update() override;
    void PostUpdate() override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    const soft3d::Vec3& GetPosition() const override { return position; }
    const soft3d::Vec3& GetRight() const override;
    const soft3d::Vec3& GetUp() const override;
    const soft3d::Vec3& GetForward() const override;
    void SetRight(const soft3d::Vec3& value) override { SetAxisX(value); }
    void SetUp(const soft3d::Vec3& value) override { SetAxisY(value); }
    void SetForward(const soft3d::Vec3& value) override { SetAxisZ(value); }

    soft3d::State<Player>* GetState(int id) const { return states.GetRegisteredState(id); }
    int GetCurrentStateId() const;
    bool IsAttacking() const;
    bool IsHiddenOrFPS() const { return hidden != 0 || fpsActive != 0; }
    bool IsFpsActive() const { return fpsActive != 0; }
    bool AreAttacksDisabled() const { return attacksDisabled != 0; }
    soft3d::ObjectBounds GetInteractionBounds() const;
    bool IsDead() const { return deathState != 0; }
    bool IsDeathComplete() const { return deathState == 2; }
    bool IsYoung() const { return young != 0; }
    bool IsPushing() const { return pushing != 0; }
    bool IsPushingUnit(int32_t id) const;
    int32_t GetHealth() const { return health; }
    int32_t GetMaxHealth() const { return maxHealth; }
    int32_t GetWeapon() const { return sword->GetType(); }
    int32_t GetGunExperience() const { return gunExperience; }
    int32_t GetGunLevel() const { return gunLevel; }
    int32_t GetUsedItem() const { return usedItem; }
    int32_t GetItemTriggerCount() const { return itemTriggerCount; }
    int32_t GetQteResult() const { return qteResult; }
    int32_t GetQteKey() const { return qteKey; }
    int32_t GetQteDuration() const { return qteDuration; }
    int32_t GetStateCounter() const { return stateCounter; }
    int32_t GetPlayTime() const { return playTime; }
    int32_t GetSelectedItemSlot() const { return selectedItem; }
    int32_t GetSelectedNoteSlot() const { return selectedNote; }
    void ClearSelectedItem() { selectedItem = -1; }
    void ClearSelectedNote() { selectedNote = -1; }
    int32_t GetItemUseCount() const { return itemUseCount; }
    int32_t GetKillCount() const { return killCount; }
    int32_t GetDeathCount() const { return deathCount; }
    int32_t GetHealthPackCount() const;
    Sword* GetSword() const { return sword.get(); }
    soft3d::GunFire& GetGunFire() { return gunFire; }
    const soft3d::Vec3& GetGunTargetPosition() const { return gunPosition; }
    const soft3d::Vec3& GetPreviousPosition() const { return previousPosition; }
    const soft3d::Vec3& GetExternalVelocity() const { return externalVelocity; }
    void SetExternalVelocity(const soft3d::Vec3& value) { externalVelocity = value; }

    void SetHealth(int32_t value);
    void AddHealth(int32_t value);
    void SetWeapon(int type);
    void PrepareWeapon(const GameData& data, int type);
    void LoadSpecialAnimations(const GameData& data, bool alternate);
    void SetYoung(bool value);
    void SetCustomAnimation(const soft3d::Animation* animation, bool loop);
    void SetAttacksDisabled(bool disabled) { attacksDisabled = disabled ? 1 : 0; }
    void SetUsedItem(int32_t value) { usedItem = value; }
    void ClearUsedItem() { usedItem = 0; }
    void StartAttack(bool heavy);
    void StartHeavyAttack() { EnterState(PlayerAttackC1); }
    void StartLightAttack() { EnterState(PlayerAttackA1); }
    void Damage(int32_t amount);
    void DamageFrom(soft3d::GameUnit* attacker);
    void DamageHurt2();
    void DamageHurt3(soft3d::GameUnit* attacker, soft3d::Fixed speed);
    void RequestHurt4();
    void EnterFPS();
    void BeginQTE(int32_t delay, int32_t duration, int32_t requiredHits);
    void BeginHide(const soft3d::Vec3& position, const soft3d::Vec3& cameraDirection,
                   int32_t maskIndex, soft3d::Fixed horizontalLimit,
                   soft3d::Fixed verticalLimit);
    void ExitHide();
    void AddGunExperience(int32_t amount);

    void AddItem(int32_t itemId, bool playSound);
    bool IsInventoryFull(int32_t itemId) const;
    bool HasItem(int32_t itemId) const;
    void RemoveItem(int32_t itemId);
    int32_t UseItem(int32_t slot);
    bool CombineItems(int32_t firstSlot, int32_t secondSlot);
    void CompactInventory();
    void RegisterItemTrigger(ItemTrigger* trigger);
    int32_t GetInventoryItem(std::size_t slot) const;
    int32_t GetNote(std::size_t slot) const;
    void SnapshotDayState();
    void RestoreDayState();

    void SetAction(int32_t action, int32_t direction, bool loop);
    void UpdateMovement(soft3d::Fixed speed);
    void SelectAttackTarget();
    soft3d::State<Player>* CommonTransition();
    void EnterState(int id) { StateMachine<Player>::EnterState(GetState(id), this); }
    void ChangeState(int id) { StateMachine<Player>::Exit(this); EnterState(id); }
    void StartPush(soft3d::GameUnit* target);
    void SetAnimationPaused(bool paused);
    void SetAnimationSpeed(int32_t speed);
    int32_t GetAnimationTime() const;
    int32_t GetAttackPhase() const { return attackPhase; }
    int32_t GetAttackDamage(soft3d::GameUnit* target);
    bool HasAttackingEnemy() const;
    bool HasAnyEnemy() const;

private:
    friend class PlayerStateSet;
    friend class PlayerStateSet::CStand;
    friend class PlayerStateSet::CRun;
#define SOFT3D_PLAYER_FRIEND(name) friend class PlayerStateSet::name
    SOFT3D_PLAYER_FRIEND(CAttackA1);
    SOFT3D_PLAYER_FRIEND(CAttackA2);
    SOFT3D_PLAYER_FRIEND(CAttackA3);
    SOFT3D_PLAYER_FRIEND(CAttackB1);
    SOFT3D_PLAYER_FRIEND(CAttackB2);
    SOFT3D_PLAYER_FRIEND(CAttackB3);
    SOFT3D_PLAYER_FRIEND(CAttackC1);
    SOFT3D_PLAYER_FRIEND(CAttackC2);
    SOFT3D_PLAYER_FRIEND(CAttackC3);
    SOFT3D_PLAYER_FRIEND(CHurt);
    SOFT3D_PLAYER_FRIEND(CHurt2);
    SOFT3D_PLAYER_FRIEND(CHurt3);
    SOFT3D_PLAYER_FRIEND(CHurt4);
    SOFT3D_PLAYER_FRIEND(CDie);
    SOFT3D_PLAYER_FRIEND(CPush);
    SOFT3D_PLAYER_FRIEND(CQTE);
    SOFT3D_PLAYER_FRIEND(CHide);
    SOFT3D_PLAYER_FRIEND(CFPS);
#undef SOFT3D_PLAYER_FRIEND

    void LoadAdultModel(bool snapToGround);
    void LoadYoungModel();
    void LoadDefaultAnimations(const GameData& data);
    void LoadWeaponAnimations(const GameData& data, int type);
    const soft3d::Animation* GetActionAnimation(int type, int action) const;
    void UpdateGunTargets();
    bool SelectGunTarget(soft3d::GameUnit* target) const;
    int32_t GetGunDamage(int32_t targetIndex, soft3d::GameUnit* target) const;
    void RestoreHiddenState();
    bool ActionDown(int action) const;
    bool ActionClick(int action) const;
    bool ActionPress(int action) const;
    bool ActionUp(int action) const;
    soft3d::Key QteInputKey() const;
    void FaceAttacker();
    void BeginPush();
    void UpdatePush();

    PlayerStateSet states;
    Scene* scene;
    soft3d::ControlMap* controls;
    std::unique_ptr<Sword> sword;
    soft3d::GunFire gunFire;
    std::array<std::array<const soft3d::Animation*, 20>, 5> animations{};
    const soft3d::Animation* defaultStand = nullptr;
    const soft3d::Animation* defaultRun = nullptr;
    const soft3d::Animation* defaultHurt2 = nullptr;
    soft3d::Vec3 cachedForward{0, 0, soft3d::fixedOne};
    soft3d::Vec3 cachedRight{soft3d::fixedOne, 0, 0};
    soft3d::Vec3 externalVelocity;
    soft3d::GameUnit* attacker = nullptr;
    soft3d::GameUnit* autoTarget = nullptr;
    soft3d::GameUnit* pushTarget = nullptr;
    std::array<soft3d::GameUnit*, 2> gunTargets{};
    soft3d::Vec3 gunPosition;
    soft3d::ObjectBounds pushLocalBounds;
    soft3d::ObjectBounds pushCombinedBounds;
    std::array<ItemTrigger*, 50> itemTriggers{};
    int32_t itemTriggerCount = 0;
    std::array<int32_t, 9> inventory{};
    std::array<int32_t, 9> inventoryDayStart{};
    std::array<int32_t, 128> notes{};
    std::array<int32_t, 128> notesDayStart{};
    int32_t healthPackDayStart = 0;
    int32_t itemUseCount = 0;
    int32_t killCount = 0;
    int32_t savedItemUseCount = 0;
    int32_t savedKillCount = 0;
    int32_t playTime = 0;
    int32_t deathCount = 0;
    int32_t frameCounter = 0;
    int32_t usedItemHealing = 0;
    int32_t young = 0;
    int32_t health = 100;
    int32_t maxHealth = 100;
    int32_t healthTenths = 10;
    int32_t gunExperience = 0;
    int32_t gunLevel = 0;
    int32_t currentAction = 0;
    int32_t currentDirection = 1;
    int32_t currentLoop = 1;
    int32_t deathAnimation = 0;
    int32_t deathState = 0;
    soft3d::Fixed knockbackSpeed = 0;
    soft3d::Vec3 knockbackVelocity;
    int32_t stateTimer = 0;
    int32_t stateCounter = 0;
    int32_t attackPhase = 0;
    int32_t gunAttackActive = 0;
    int32_t attacksDisabled = 0;
    soft3d::Fixed attackRange = 0xf0000;
    soft3d::Fixed attackBlend = 0;
    int32_t hurtRequest = 0;
    int32_t hurt2Request = 0;
    int32_t hurt3Request = 0;
    int32_t hurt4Request = 0;
    int32_t navigating = 0;
    int32_t pushing = 0;
    int32_t presentationVisible = 1;
    int32_t fpsActive = 0;
    int32_t hidden = 0;
    int32_t hideMask = -1;
    soft3d::Vec3 hideReturnPosition;
    soft3d::Vec3 hideCameraDirection;
    int32_t hideReturnFPS = 0;
    soft3d::Fixed hideHorizontalLimit = 0;
    soft3d::Fixed hideVerticalLimit = 0;
    int32_t qteDelay = 0;
    int32_t qteDuration = 0;
    int32_t qteRequiredHits = 0;
    int32_t qteHits = 0;
    int32_t qteKey = 0;
    int32_t qteResult = -1;
    int32_t usedItem = 0;
    int32_t selectedNote = -1;
    int32_t selectedItem = 0;
};

} // namespace game

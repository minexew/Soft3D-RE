#pragma once

#include "game/CameraController.hpp"
#include "soft3d/Units.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace game {

class MyGameUnitMgr;
class Scene;
class Player;
class Pickable;

class TriggerUiHost {
public:
    virtual ~TriggerUiHost() = default;
    virtual void SetPrompt(int32_t prompt) = 0;
    virtual void ShowMessage(int32_t textId, int32_t kind) = 0;
    virtual void SetChoiceString(int32_t index, int32_t textId) = 0;
    virtual void ShowChoice(int32_t titleId, int32_t optionCount, bool portrait) = 0;
    virtual int32_t GetDialogResult() const = 0;
};

class InteractionState final : public TriggerUiHost {
public:
    explicit InteractionState(MyGameUnitMgr* manager = nullptr);

    void SetManager(MyGameUnitMgr* manager) { this->manager = manager; }
    void Reset();
    void SetDialogResult(int32_t result);

    void SetPrompt(int32_t prompt) override { this->prompt = prompt; }
    void ShowMessage(int32_t textId, int32_t kind) override;
    void SetChoiceString(int32_t index, int32_t textId) override;
    void ShowChoice(int32_t titleId, int32_t optionCount, bool portrait) override;
    int32_t GetDialogResult() const override { return dialogResult; }

    int32_t GetPrompt() const { return prompt; }
    int32_t GetDialogKind() const { return dialogKind; }
    int32_t GetDialogText() const { return dialogText; }
    int32_t GetDialogOptionCount() const { return dialogOptionCount; }
    int32_t GetDialogOption(int32_t index) const;
    bool IsDialogPortrait() const { return dialogPortrait; }

private:
    MyGameUnitMgr* manager;
    std::array<int32_t, 8> dialogOptions{};
    int32_t prompt = -1;
    int32_t dialogKind = -1;
    int32_t dialogText = -1;
    int32_t dialogOptionCount = 0;
    int32_t dialogResult = -1;
    bool dialogPortrait = false;
};

class EventTrigger final : public soft3d::GameUnit {
public:
    EventTrigger(Player* player = nullptr, TriggerUiHost* ui = nullptr);

    void Configure(Player* player, TriggerUiHost* ui);
    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void SetRemainingCount(int16_t value) { remainingCount = value; }
    void SetTextId(int32_t value) { textId = value; }
    int16_t GetRemainingCount() const { return remainingCount; }
    int32_t GetTextId() const { return textId; }
    bool IsInside() const { return outside == 0; }

private:
    Player* player;
    TriggerUiHost* ui;
    int32_t textId = 0;
    int16_t remainingCount = -1;
    int16_t outside = 1;
};

class PickTrigger final : public soft3d::GameUnit {
public:
    PickTrigger(MyGameUnitMgr* manager = nullptr, Player* player = nullptr,
                soft3d::ControlMap* controls = nullptr, TriggerUiHost* ui = nullptr);

    void Configure(Pickable* pickable, MyGameUnitMgr* manager, Player* player,
                   soft3d::ControlMap* controls, TriggerUiHost* ui);
    void SetEnabled(bool enabled);
    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    bool IsAvailable() const;
    bool IsEnabled() const { return enabled != 0; }
    bool IsPromptActive() const { return promptActive != 0; }
    bool IsDialogActive() const { return dialogActive != 0; }
    int32_t GetDialogTitleId() const { return dialogTitleId; }
    void SetDialogTitleId(int32_t value) { dialogTitleId = value; }
    Pickable* GetPickable() const { return pickable; }

private:
    void TryPickUp();

    MyGameUnitMgr* manager;
    Player* player;
    soft3d::ControlMap* controls;
    TriggerUiHost* ui;
    Pickable* pickable = nullptr;
    int32_t enabled = 1;
    int32_t dialogTitleId = -1;
    int32_t promptActive = 0;
    int32_t dialogActive = 0;
};

class DoorTrigger final : public soft3d::GameUnit {
public:
    DoorTrigger(Scene* scene = nullptr, MyGameUnitMgr* manager = nullptr,
                Player* player = nullptr, soft3d::ControlMap* controls = nullptr,
                TriggerUiHost* ui = nullptr);

    void Configure(Scene* scene, MyGameUnitMgr* manager, Player* player,
                   soft3d::ControlMap* controls, TriggerUiHost* ui);
    void SetEnabled(bool enabled);
    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;
    void OpenDoor();

    void SetDoorResourceIndex(int32_t value) { doorResourceIndex = value; }
    void SetNextMap(int32_t value) { nextMapId = value; }
    void SetNextEpisode(int32_t value) { nextEpisodeId = value; }
    void SetSpawnPosition(const soft3d::Vec3& value) { spawnPosition = value; }
    void SetSpawnDirection(const soft3d::Vec3& value) { spawnDirection = value; }
    void SetRequiredItem(int32_t value) { requiredItem = value; }
    void SetLockedText(int32_t value) { lockedTextId = value; }
    int32_t GetDoorResourceIndex() const { return doorResourceIndex; }
    int32_t GetNextMap() const { return nextMapId; }
    int32_t GetNextEpisode() const { return nextEpisodeId; }
    const soft3d::Vec3& GetSpawnPosition() const { return spawnPosition; }
    const soft3d::Vec3& GetSpawnDirection() const { return spawnDirection; }
    int32_t GetRequiredItem() const { return requiredItem; }
    int32_t GetLockedText() const { return lockedTextId; }
    int32_t GetUnlockTimer() const { return unlockTimer; }
    bool IsUnlocking() const { return unlocking != 0; }
    bool IsEnabled() const { return enabled != 0; }
    bool IsPromptActive() const { return promptActive != 0; }
    bool IsDialogActive() const { return dialogActive != 0; }

private:
    Scene* scene;
    MyGameUnitMgr* manager;
    Player* player;
    soft3d::ControlMap* controls;
    TriggerUiHost* ui;
    int32_t unlocking = 0;
    int32_t unlockTimer = 0;
    int32_t enabled = 1;
    int32_t doorResourceIndex = 0;
    int32_t nextMapId = 0;
    int32_t nextEpisodeId = 0;
    soft3d::Vec3 spawnPosition;
    soft3d::Vec3 spawnDirection;
    int32_t requiredItem = 0;
    int32_t lockedTextId = 0;
    int32_t promptActive = 0;
    int32_t dialogActive = 0;
};

class ActTrigger final : public soft3d::GameUnit {
public:
    ActTrigger(CameraController* camera = nullptr, Scene* scene = nullptr,
               Player* player = nullptr, soft3d::ControlMap* controls = nullptr,
               TriggerUiHost* ui = nullptr);

    void Configure(CameraController* camera, Scene* scene, Player* player,
                   soft3d::ControlMap* controls, TriggerUiHost* ui);
    void SetEnabled(bool enabled);
    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    void SetOptionCount(int32_t value);
    void SetString(int32_t index, int32_t textId);
    void SetCameraVectors(const soft3d::Vec3& position, const soft3d::Vec3& target);
    void ClearSelection() { selection = -1; }
    int32_t GetSelection() const { return selection; }
    int32_t GetOptionCount() const { return optionCount; }
    int32_t GetTitleId() const { return titleId; }
    int32_t GetReserved5c() const { return reserved5c; }
    int32_t GetOptionId(int32_t index) const;
    const soft3d::Vec3& GetCameraPosition() const { return cameraPosition; }
    const soft3d::Vec3& GetCameraTarget() const { return cameraTarget; }
    bool IsEnabled() const { return enabled != 0; }
    bool IsPromptActive() const { return promptActive != 0; }
    bool IsDialogActive() const { return dialogActive != 0; }

private:
    bool IsSuppressed() const;
    bool HasCameraOverride() const;

    CameraController* camera;
    Scene* scene;
    Player* player;
    soft3d::ControlMap* controls;
    TriggerUiHost* ui;
    int32_t enabled = 1;
    int32_t optionCount = 0;
    int32_t reserved5c = 0;
    int32_t titleId = 0;
    std::array<int32_t, 3> optionIds{};
    int32_t dialogActive = 0;
    int32_t selection = 0;
    int32_t promptActive = 0;
    soft3d::Vec3 cameraPosition;
    soft3d::Vec3 cameraTarget;
};

class CameraTrigger final : public soft3d::GameUnit {
public:
    CameraTrigger(CameraController* controller = nullptr, soft3d::Creature* player = nullptr);

    void Configure(CameraController* controller, soft3d::Creature* player);
    soft3d::result LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp,
                              soft3d::s3dResManager& resources);
    void SetMode(int32_t mode, const soft3d::Vec3& position, const soft3d::Vec3& direction,
                 soft3d::Fixed time, std::unique_ptr<soft3d::CameraPathCollection> paths,
                 soft3d::Fixed pathSpeed, bool snap, int32_t targetMode);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    int32_t GetMode() const { return mode; }
    const soft3d::Vec3& GetPosition() const { return cameraPosition; }
    const soft3d::Vec3& GetDirection() const { return direction; }
    soft3d::Fixed GetTime() const { return time; }
    const soft3d::CameraPathCollection* GetPaths() const { return paths.get(); }
    soft3d::Fixed GetPathSpeed() const { return pathSpeed; }
    bool GetSnap() const { return snap; }
    int32_t GetTargetMode() const { return targetMode; }
    bool IsOutside() const { return outside; }

private:
    CameraController* controller;
    soft3d::Creature* player;
    int32_t mode = 0;
    soft3d::Vec3 cameraPosition;
    soft3d::Vec3 direction;
    soft3d::Fixed time = 0;
    std::unique_ptr<soft3d::CameraPathCollection> paths;
    bool snap = false;
    soft3d::Fixed pathSpeed = 0;
    bool active = false;
    bool outside = true;
    int32_t targetMode = 0;
};

class LightTrigger final : public soft3d::GameUnit {
public:
    explicit LightTrigger(soft3d::Creature* player = nullptr);

    soft3d::result LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp);
    void Update() override;

    bool UsesPlayerDirection() const { return usePlayerDirection; }
    const soft3d::Vec3& GetLightDirection() const { return lightDirection; }

private:
    soft3d::Creature* player;
    bool usePlayerDirection = false;
    soft3d::Vec3 lightDirection;
};

} // namespace game

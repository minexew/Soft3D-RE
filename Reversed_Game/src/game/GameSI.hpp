#pragma once

#include "game/Scene.hpp"
#include "soft3d/SimpleScript.hpp"
#include "soft3d/s3dLog.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace game {

inline constexpr std::size_t gameSICallbackCount = 218;
inline constexpr int gameSIFirstCommand = 43;
inline constexpr int gameSILastCommand = 260;

struct GameSIContext {
    Scene* scene = nullptr;
    MyGameUnitMgr* unitManager = nullptr;
    CameraController* cameraController = nullptr;
    Player* player = nullptr;
    soft3d::GameUIDlg* gameUi = nullptr;
    soft3d::AudioManager* audio = nullptr;
    soft3d::s3dResManager* resources = nullptr;
    soft3d::Bsp* bsp = nullptr;
    soft3d::ControlMap* controlMap = nullptr;
};

class GameSIError : public std::runtime_error {
public:
    GameSIError(int commandId, uint32_t armAddress, const std::string& reason);

    int GetCommandId() const { return commandId; }
    uint32_t GetArmAddress() const { return armAddress; }

private:
    int commandId;
    uint32_t armAddress;
};

class GameSI final : public soft3d::ScriptInterface {
public:
    GameSI();

    void Bind(const GameSIContext& context);
    const GameSIContext& GetContext() const { return context; }

    struct Callback {
        uint32_t armAddress;
        void (GameSI::*handler)(soft3d::ScriptCommand&);
        int32_t thisAdjustment;
    };

    const Callback& GetCallback(int commandId) const;
    static uint32_t GetArmHandlerAddress(int commandId);

protected:
    void ExecuteGameCommand(soft3d::ScriptCommand& command) override;

private:
    using Handler = void (GameSI::*)(soft3d::ScriptCommand&);

    void RequireArguments(const soft3d::ScriptCommand& command, std::size_t count) const;
    void RequireContext() const;
    void IsActionFirstDown(soft3d::ScriptCommand& command);
    void IsActionDown(soft3d::ScriptCommand& command);
    void IsActionPress(soft3d::ScriptCommand& command);
    void IsActionUp(soft3d::ScriptCommand& command);
    void IsActionClicked(soft3d::ScriptCommand& command);
    void IsAnyKeyDown(soft3d::ScriptCommand& command);
    void IsAnyKeyClick(soft3d::ScriptCommand& command);
    void LogMessage(soft3d::ScriptCommand& command);
    void SetUseCameraListener(soft3d::ScriptCommand& command);
    void StartScriptTimer(soft3d::ScriptCommand& command);
    void GetScriptTimerElapsed(soft3d::ScriptCommand& command);
    void StopScriptTimer(soft3d::ScriptCommand& command);
    void SetBspScriptVectors(soft3d::ScriptCommand& command);
    void SetPlayerPresentationVisible(soft3d::ScriptCommand& command);
    void SetControlsDisabled(soft3d::ScriptCommand& command);
    void SetPlayerVisible(soft3d::ScriptCommand& command);
    void FadeOut(soft3d::ScriptCommand& command);
    void FadeIn(soft3d::ScriptCommand& command);
    void Fade(soft3d::ScriptCommand& command, bool fadeOut);
    void LoadNextDay(soft3d::ScriptCommand& command);
    void SetLoadMapParams(soft3d::ScriptCommand& command);
    void LoadNextLevel(soft3d::ScriptCommand& command);
    void LoadMap(soft3d::ScriptCommand& command);
    void IsMapLoaded(soft3d::ScriptCommand& command);
    void SetEventFlag(soft3d::ScriptCommand& command);
    void GetEventFlag(soft3d::ScriptCommand& command);
    void CompleteGame(soft3d::ScriptCommand& command);
    void OpenSaveMenu(soft3d::ScriptCommand& command);
    void SetPaused(soft3d::ScriptCommand& command);
    void ShowEventMessage(soft3d::ScriptCommand& command);
    void SetDialogString(soft3d::ScriptCommand& command);
    void ShowChoiceDialog(soft3d::ScriptCommand& command);
    void ShowSimpleMessage(soft3d::ScriptCommand& command);
    void ShowTimedMessage(soft3d::ScriptCommand& command);
    void GetDialogResult(soft3d::ScriptCommand& command);
    void CloseDialog(soft3d::ScriptCommand& command);
    void IsDialogFinished(soft3d::ScriptCommand& command);
    void ShowMap(soft3d::ScriptCommand& command);
    void ShowTimedResultMessage(soft3d::ScriptCommand& command);
    void SetSubtitle(soft3d::ScriptCommand& command);
    void SetActionMask(soft3d::ScriptCommand& command);
    void SetMapMarker(soft3d::ScriptCommand& command);
    void ClearMapMarker(soft3d::ScriptCommand& command);
    void SetGunHudEnabled(soft3d::ScriptCommand& command);
    void GetItemUseCount(soft3d::ScriptCommand& command);
    void GetKillCount(soft3d::ScriptCommand& command);
    void GetPlayTime(soft3d::ScriptCommand& command);
    void GetDeathCount(soft3d::ScriptCommand& command);
    void GetNoteCount(soft3d::ScriptCommand& command);
    void GetSecretCount(soft3d::ScriptCommand& command);
    void SetCameraFrozen(soft3d::ScriptCommand& command);
    void CameraLookAt(soft3d::ScriptCommand& command);
    void MoveCamera(soft3d::ScriptCommand& command);
    void SetCameraMode(soft3d::ScriptCommand& command);
    void IsCameraPathCompleted(soft3d::ScriptCommand& command);
    void BindCameraToPlayer(soft3d::ScriptCommand& command);
    void BindCameraToEnemy(soft3d::ScriptCommand& command);
    void ShakeCamera(soft3d::ScriptCommand& command);
    void SetDirectFollow(soft3d::ScriptCommand& command);
    void PlayMusic(soft3d::ScriptCommand& command);
    void StopMusic(soft3d::ScriptCommand& command);
    void AddEnvironmentSound(soft3d::ScriptCommand& command);
    void ClearEnvironmentChannel(soft3d::ScriptCommand& command);
    void PlaySound(soft3d::ScriptCommand& command);
    void StopAllSounds(soft3d::ScriptCommand& command);
    void CreatePositionalSound(soft3d::ScriptCommand& command);
    void ClearSoundInstances(soft3d::ScriptCommand& command);
    void SetLightStyle(soft3d::ScriptCommand& command);
    void GetLightStyle(soft3d::ScriptCommand& command);
    void SetFog(soft3d::ScriptCommand& command);
    void SelectPresentationShader(soft3d::ScriptCommand& command);
    void AddEventTrigger(soft3d::ScriptCommand& command);
    void SetEventTriggerCount(soft3d::ScriptCommand& command);
    void SetEventTriggerText(soft3d::ScriptCommand& command);
    void IsEventTriggerActive(soft3d::ScriptCommand& command);
    void AddPickTrigger(soft3d::ScriptCommand& command);
    void EnablePickTrigger(soft3d::ScriptCommand& command);
    void AddDoorTrigger(soft3d::ScriptCommand& command);
    void SetDoorTriggerItem(soft3d::ScriptCommand& command);
    void SetDoorTriggerLockedText(soft3d::ScriptCommand& command);
    void EnableDoorTrigger(soft3d::ScriptCommand& command);
    void AddActTrigger(soft3d::ScriptCommand& command);
    void SetActTriggerOptionCount(soft3d::ScriptCommand& command);
    void SetActTriggerString(soft3d::ScriptCommand& command);
    void GetActTriggerSelection(soft3d::ScriptCommand& command);
    void ClearActTriggerSelection(soft3d::ScriptCommand& command);
    void SetActTriggerCamera(soft3d::ScriptCommand& command);
    void EnableActTrigger(soft3d::ScriptCommand& command);
    void AddHideTrigger(soft3d::ScriptCommand& command);
    void SetHideTriggerLimits(soft3d::ScriptCommand& command);
    void EnableHideTrigger(soft3d::ScriptCommand& command);
    void AddItemTrigger(soft3d::ScriptCommand& command);
    void AddItemTriggerItem(soft3d::ScriptCommand& command);
    void EnableItemTrigger(soft3d::ScriptCommand& command);
    void AddTalkTrigger(soft3d::ScriptCommand& command);
    void SetTalkTriggerCameraPosition(soft3d::ScriptCommand& command);
    void AddTalkTriggerCamera(soft3d::ScriptCommand& command);
    void AddTalkTriggerLine(soft3d::ScriptCommand& command);
    void IsTalkTriggerActive(soft3d::ScriptCommand& command);
    void IsTalkTriggerCompleted(soft3d::ScriptCommand& command);
    void EnableTalkTrigger(soft3d::ScriptCommand& command);
    void RemoveInteractionTrigger(soft3d::ScriptCommand& command);
    Enemy* FindEnemy(soft3d::ScriptCommand& command);
    void AddEnemy(soft3d::ScriptCommand& command);
    void PersistEnemy(soft3d::ScriptCommand& command);
    void SetEnemyNormalRendering(soft3d::ScriptCommand& command);
    void SetEnemyAnimation(soft3d::ScriptCommand& command);
    void IsEnemyAnimationFinished(soft3d::ScriptCommand& command);
    void IsEnemyRemoved(soft3d::ScriptCommand& command);
    void GetEnemyHealth(soft3d::ScriptCommand& command);
    void SetEnemyHealth(soft3d::ScriptCommand& command);
    void GetEnemyAttackPower(soft3d::ScriptCommand& command);
    void SetEnemyAttackPower(soft3d::ScriptCommand& command);
    void SetEnemyDirection(soft3d::ScriptCommand& command);
    void TeleportEnemy(soft3d::ScriptCommand& command);
    void MoveEnemyTo(soft3d::ScriptCommand& command);
    void EnemyHearSound(soft3d::ScriptCommand& command);
    void EnemyBoundsInside(soft3d::ScriptCommand& command);
    void FollowEnemy(soft3d::ScriptCommand& command);
    void AttackEnemy(soft3d::ScriptCommand& command);
    void RemoveEnemy(soft3d::ScriptCommand& command);
    void RemoveEnemiesByType(soft3d::ScriptCommand& command);
    void RemoveAllEnemies(soft3d::ScriptCommand& command);
    void PauseEnemy(soft3d::ScriptCommand& command);
    void PauseEnemiesByType(soft3d::ScriptCommand& command);
    void PauseAllEnemies(soft3d::ScriptCommand& command);
    void StopEnemy(soft3d::ScriptCommand& command);
    void AddStayBox(soft3d::ScriptCommand& command);
    void SetStayBoxReturnPosition(soft3d::ScriptCommand& command);
    void AddEnemyWaypoint(soft3d::ScriptCommand& command);
    void SetEnemyWaypointLoop(soft3d::ScriptCommand& command);
    void ClearEnemyWaypoints(soft3d::ScriptCommand& command);
    void SetEnemyPerceptionRange(soft3d::ScriptCommand& command);
    void IsEnemyAlerted(soft3d::ScriptCommand& command);
    void SetEnemyAnimationResource(soft3d::ScriptCommand& command);
    void SetEnemySoundResource(soft3d::ScriptCommand& command);
    void SetEnemyAlertSpeed(soft3d::ScriptCommand& command);
    void AddCollisionUnit(soft3d::ScriptCommand& command);
    void RemoveCollisionUnit(soft3d::ScriptCommand& command);
    void AddStayBoxDeniedRegion(soft3d::ScriptCommand& command);
    void AddStayBoxAllowedRegion(soft3d::ScriptCommand& command);
    void AddStayBoxEnemy(soft3d::ScriptCommand& command);
    void AddStayBoxRegion(soft3d::ScriptCommand& command, bool allowed);
    EventTrigger& RequireEventTrigger(soft3d::ScriptCommand& command);
    PickTrigger& RequirePickTrigger(soft3d::ScriptCommand& command);
    DoorTrigger& RequireDoorTrigger(soft3d::ScriptCommand& command);
    ActTrigger& RequireActTrigger(soft3d::ScriptCommand& command);
    HideTrigger& RequireHideTrigger(soft3d::ScriptCommand& command);
    ItemTrigger& RequireItemTrigger(soft3d::ScriptCommand& command);
    TalkTrigger& RequireTalkTrigger(soft3d::ScriptCommand& command);
    void AddTrap(soft3d::ScriptCommand& command);
    void RemoveTrap(soft3d::ScriptCommand& command);
    void AddThing(soft3d::ScriptCommand& command);
    void SetThingDirection(soft3d::ScriptCommand& command);
    void SetPickableItem(soft3d::ScriptCommand& command);
    void SetThingCollision(soft3d::ScriptCommand& command);
    void SetThingIntensityBounce(soft3d::ScriptCommand& command);
    void SetThingVisible(soft3d::ScriptCommand& command);
    void IsThingVisible(soft3d::ScriptCommand& command);
    void SetThingAnimation(soft3d::ScriptCommand& command);
    void SetThingAnimationPaused(soft3d::ScriptCommand& command);
    void IsThingAnimationPaused(soft3d::ScriptCommand& command);
    void SetThingAnimationTime(soft3d::ScriptCommand& command);
    void GetThingAnimationTime(soft3d::ScriptCommand& command);
    void IsThingAnimationFinished(soft3d::ScriptCommand& command);
    void SetThingBreakModel(soft3d::ScriptCommand& command);
    void SetThingBreakAnimation(soft3d::ScriptCommand& command);
    void SetBreakableSound(soft3d::ScriptCommand& command);
    void SetPushableSound(soft3d::ScriptCommand& command);
    void RotateThingX(soft3d::ScriptCommand& command);
    void RotateThingY(soft3d::ScriptCommand& command);
    void RotateThingZ(soft3d::ScriptCommand& command);
    void RotateThing(soft3d::ScriptCommand& command, int axis);
    void SetThingPosition(soft3d::ScriptCommand& command);
    void MoveThing(soft3d::ScriptCommand& command);
    void MoveThingTowards(soft3d::ScriptCommand& command);
    void DestroyThing(soft3d::ScriptCommand& command);
    void SetThingDepthBias(soft3d::ScriptCommand& command);
    void RemoveThing(soft3d::ScriptCommand& command);
    void ThingExists(soft3d::ScriptCommand& command);
    void ConsumeBreakableEvent(soft3d::ScriptCommand& command);
    void ThingBoundsInside(soft3d::ScriptCommand& command);
    void ThingBoundsOutside(soft3d::ScriptCommand& command);
    void TestThingBounds(soft3d::ScriptCommand& command, bool outside);
    void SetThingLighting(soft3d::ScriptCommand& command);
    void SetPushableEnabled(soft3d::ScriptCommand& command);
    void SetThingTexture(soft3d::ScriptCommand& command);
    void GetMagicCubeOrientation(soft3d::ScriptCommand& command);
    soft3d::Things& RequireThing(soft3d::ScriptCommand& command);
    Breakable& RequireBreakable(soft3d::ScriptCommand& command);
    Pushable& RequirePushable(soft3d::ScriptCommand& command);
    void SetPlayerDirection(soft3d::ScriptCommand& command);
    void SetPlayerClipping(soft3d::ScriptCommand& command);
    void SetPlayerAnimation(soft3d::ScriptCommand& command);
    void IsPlayerAnimationFinished(soft3d::ScriptCommand& command);
    void TeleportPlayer(soft3d::ScriptCommand& command);
    void NavigatePlayer(soft3d::ScriptCommand& command);
    void PlayerHearSound(soft3d::ScriptCommand& command);
    void PlayerPointInside(soft3d::ScriptCommand& command);
    void PlayerPointOutside(soft3d::ScriptCommand& command);
    void PlayerHearEnemy(soft3d::ScriptCommand& command);
    void PlayerHearThing(soft3d::ScriptCommand& command);
    void PlayerAttack(soft3d::ScriptCommand& command);
    void IsPlayerDead(soft3d::ScriptCommand& command);
    void GetPlayerHealth(soft3d::ScriptCommand& command);
    void SetPlayerHealth(soft3d::ScriptCommand& command);
    void AddPlayerHealth(soft3d::ScriptCommand& command);
    void DamagePlayer(soft3d::ScriptCommand& command);
    void SetPlayerWeapon(soft3d::ScriptCommand& command);
    void GetPlayerWeapon(soft3d::ScriptCommand& command);
    void IsPlayerAttacking(soft3d::ScriptCommand& command);
    void IsPlayerHidden(soft3d::ScriptCommand& command);
    void StartPlayerQTE(soft3d::ScriptCommand& command);
    void GetPlayerQTEResult(soft3d::ScriptCommand& command);
    void GetPlayerUsedItem(soft3d::ScriptCommand& command);
    void ClearPlayerUsedItem(soft3d::ScriptCommand& command);
    void EnterPlayerFPS(soft3d::ScriptCommand& command);
    void HasAttackingEnemy(soft3d::ScriptCommand& command);
    void SetPlayerSpeed(soft3d::ScriptCommand& command);
    void IsPlayerPushing(soft3d::ScriptCommand& command);
    void SetPlayerYoung(soft3d::ScriptCommand& command);
    void IsPlayerYoung(soft3d::ScriptCommand& command);
    void ExitPlayerHide(soft3d::ScriptCommand& command);
    void AddPlayerItem(soft3d::ScriptCommand& command);
    void HasPlayerItem(soft3d::ScriptCommand& command);
    void RemovePlayerItem(soft3d::ScriptCommand& command);
    void SetSharedWord255(soft3d::ScriptCommand& command);
    void NullCallback(soft3d::ScriptCommand& command);

    GameSIContext context;
    soft3d::s3dLog scriptLog;
    std::unique_ptr<soft3d::CameraPathCollection> cameraCommandPaths;
    std::array<Callback, gameSICallbackCount> callbacks{};
};

} // namespace game

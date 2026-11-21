#pragma once

#include "game/Scene.hpp"
#include "soft3d/SimpleScript.hpp"

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
    soft3d::Creature* player = nullptr;
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
    void Unimplemented(soft3d::ScriptCommand& command);
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
    void SetPlayerDirection(soft3d::ScriptCommand& command);
    void TeleportPlayer(soft3d::ScriptCommand& command);
    void SetSharedWord255(soft3d::ScriptCommand& command);
    void NullCallback(soft3d::ScriptCommand& command);

    GameSIContext context;
    std::unique_ptr<soft3d::CameraPathCollection> cameraCommandPaths;
    std::array<Callback, gameSICallbackCount> callbacks{};
};

} // namespace game

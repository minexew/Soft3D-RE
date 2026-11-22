#pragma once

#include "game/CameraController.hpp"
#include "game/MyGameUnitMgr.hpp"
#include "game/Player.hpp"
#include "soft3d/BezierCurve3.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/PresentationShaders.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace game {

class SceneCameraSpot final : public soft3d::CameraSpot {
public:
    explicit SceneCameraSpot(soft3d::Creature* unit = nullptr) : unit(unit) {}

    void SetUnit(soft3d::Creature* value) { unit = value; }
    const soft3d::Vec3& GetPosition() const override;
    const soft3d::Vec3& GetRight() const override { return right; }
    const soft3d::Vec3& GetUp() const override { return up; }
    const soft3d::Vec3& GetForward() const override { return forward; }
    void SetRight(const soft3d::Vec3& value) override;
    void SetUp(const soft3d::Vec3& value) override;
    void SetForward(const soft3d::Vec3& value) override;

private:
    soft3d::Creature* unit;
    soft3d::Vec3 right{soft3d::fixedOne, 0, 0};
    soft3d::Vec3 up{0, soft3d::fixedOne, 0};
    soft3d::Vec3 forward{0, 0, soft3d::fixedOne};
};

class Scene final : public soft3d::SceneHost {
public:
    struct EnvironmentChannel {
        std::array<soft3d::AudioData*, 4> resources{};
        int32_t resourceCount = 0;
        int32_t baseDelay = 0;
        int32_t randomDelay = 0;
        int32_t elapsed = 0;
        int32_t nextDelay = -1;
        int32_t voice = 0;
    };

    explicit Scene(soft3d::GameEngine& engine);
    ~Scene() override;

    soft3d::result Initialize();
    void StartNewGame(const soft3d::NewGameOptions& options);
    bool LoadSaveSlot(int slot);
    void ReturnToMainMenu();

    soft3d::result LoadDay(int day);
    int LoadMap(int mapId);
    void LoadNextLevel(int mapId, int episodeId);
    void SetLoadMapParams(int doorResourceIndex, const soft3d::Vec3& position,
                          const soft3d::Vec3& direction, uint16_t transitionColor);
    void BeginMapLoad();
    void FinishMapLoad();
    void UnloadLevel();

    void Update() override;
    void PostUpdate() override;
    void PrepareRender() override;
    void SubmitRenderables() override;

    bool IsMapLoaded() const { return mapProgress == 100; }
    void SetEventFlag(int32_t index);
    bool GetEventFlag(int32_t index) const;

    soft3d::GameEngine& GetEngine() { return engine; }
    soft3d::Bsp* GetBsp() { return bsp.get(); }
    MyGameUnitMgr* GetUnitManager() { return unitManager.get(); }
    Player* GetPlayer() { return player.get(); }
    CameraController* GetCameraController() { return &cameraController; }
    soft3d::GameUIDlg* GetGameUi() { return engine.GetGameUi(); }
    soft3d::AudioManager* GetAudio() { return &engine.GetAudioManager(); }
    soft3d::s3dResManager* GetResources() { return &engine.GetResourceManager(); }
    soft3d::ControlMap* GetControlMap() { return &engine.GetControlMap(); }

    int GetDay() const { return day; }
    int GetMapId() const { return mapId; }
    int GetMapProgress() const { return mapProgress; }
    int GetTransitionStage() const { return transitionStage; }
    int GetTransitionTimer() const { return transitionTimer; }
    int GetNextMapId() const { return nextMapId; }
    int GetNextEpisodeId() const { return nextEpisodeId; }
    int GetDoorResourceIndex() const { return doorResourceIndex; }
    const soft3d::Vec3& GetSpawnPosition() const { return spawnPosition; }
    const soft3d::Vec3& GetSpawnDirection() const { return spawnDirection; }
    uint16_t GetTransitionColor() const { return transitionColor; }
    const soft3d::Vec3& GetTransitionCameraPosition() const { return transitionCameraPosition; }
    const soft3d::Vec3& GetTransitionCameraTarget() const { return transitionCameraTarget; }
    soft3d::Fixed GetTransitionCameraSpeed() const { return transitionCameraSpeed; }
    bool IsDayIntroPending() const { return dayIntroPending; }
    bool IsDayTitleVisible() const { return showDayTitle; }
    bool IsNextLevelPending() const { return nextLevelPending; }
    bool IsInitialized() const { return initialized; }
    bool IsUsingDay1Fallback() const { return useDay1Fallback; }
    const std::u16string& GetScriptName() const { return scriptName; }
    const soft3d::CameraPathCollection& GetCameraPaths() const { return *cameraPaths; }
    soft3d::FadeShader* GetFadeShader() { return &fadeShader; }
    soft3d::InkShader* GetInkShader() { return &inkShader; }
    soft3d::GrayShader* GetGrayShader() { return &grayShader; }
    soft3d::BlindShader* GetBlindShader() { return &blindShader; }
    soft3d::TwistShader* GetTwistShader() { return &twistShader; }
    soft3d::MotionBlurShader* GetMotionBlurShader() { return &motionBlurShader; }
    soft3d::VisUnit* GetDoorModel() { return doorModel.get(); }
    const EnvironmentChannel& GetEnvironmentChannel(int index) const;
    int GetGameOverStage() const { return gameOverStage; }
    int GetInkTimer() const { return inkTimer; }
    int GetSpecialDoorIndex() const { return specialDoorIndex; }
    bool UsesCameraListener() const { return useCameraListener != 0; }
    const soft3d::NewGameOptions& GetNewGameOptions() const { return newGameOptions; }
    int32_t GetQteUiKey() const { return qteUiKey; }
    int32_t GetQteUiTime() const { return qteUiTime; }
    int32_t GetQteUiMode() const { return qteUiMode; }
    int32_t GetQteUiFailure() const { return qteUiFailure; }
    int32_t GetHideMask() const { return hideMaskIndex; }
    bool IsHideMaskActive() const { return hideMaskActive != 0; }
    int32_t GetTutorialId() const { return tutorialId; }
    int32_t GetTutorialFrames() const { return tutorialFrames; }

    bool AddEnvironmentSound(int channel, const char16_t* name, int32_t baseDelay,
                             int32_t randomDelay);
    bool ClearEnvironmentChannel(int channel);
    void SelectPresentationShader(int selection);
    bool FadeCommand(bool fadeOut, uint16_t color, int frameCount);
    void BindCameraToPlayer();
    void BindCameraToEnemy();
    void SetPlayerDirection(const soft3d::Vec3& direction);
    void SetGameOverStage(int stage) { gameOverStage = stage; }
    void SetUseCameraListener(bool enabled) { useCameraListener = enabled ? 1 : 0; }
    void ResetCameraWarmup() { cameraWarmupCounter = 0; }
    int32_t GetCameraWarmupCounter() const { return cameraWarmupCounter; }
    void ShowQte(int32_t key, int32_t time, int32_t mode);
    void SetQteMode(int32_t mode) { qteUiMode = mode; }
    void FailQte();
    void SetHideMask(int32_t index, bool active);
    void ShowTutorial(int32_t id, int32_t frames);
    int32_t GetDoorAuthorizationStatus() const { return -1; }
    void BeginAuthorization() { authorizationStage = 1; authorizationTimer = 1; }
    int32_t GetAuthorizationStage() const { return authorizationStage; }
    int32_t GetAuthorizationTimer() const { return authorizationTimer; }

    static soft3d::BspEntity* CreateGameEntity(soft3d::BspEntityRecord* record, void* context,
                                                soft3d::BspEntityRecord* recordBase);

private:
    void ResetEnvironmentAudio();
    void UpdateEnvironmentAudio();
    void UpdateMapLoad();
    void UpdateWorld();
    void UpdateTransition();
    void LoadNextScript();
    void RenderDayTitle();
    void RestartAfterGameOver();

    soft3d::GameEngine& engine;
    std::unique_ptr<soft3d::Bsp> bsp;
    std::unique_ptr<soft3d::CameraPathCollection> cameraPaths;
    CameraController cameraController;
    std::unique_ptr<Player> player;
    SceneCameraSpot playerSpot;
    soft3d::DummySpot enemySpot;
    std::unique_ptr<MyGameUnitMgr> unitManager;
    std::unique_ptr<soft3d::VisUnit> doorModel;
    soft3d::InkShader inkShader;
    soft3d::FadeShader fadeShader;
    soft3d::GrayShader grayShader;
    soft3d::BlindShader blindShader;
    soft3d::TwistShader twistShader;
    soft3d::MotionBlurShader motionBlurShader;
    std::array<EnvironmentChannel, 2> environment{};
    std::array<uint8_t, 0x2800> eventFlags{};
    std::array<uint8_t, 0x2800> dayStartSnapshot{};
    soft3d::NewGameOptions newGameOptions;
    std::u16string scriptName;
    int32_t day = 0;
    int32_t mapId = 0;
    int32_t mapProgress = -1;
    int32_t dayIntroPending = 0;
    int32_t showDayTitle = 0;
    int32_t nextLevelPending = 0;
    int32_t saveSlotPending = 0;
    int32_t nextMapId = 0;
    int32_t nextEpisodeId = 0;
    int32_t doorResourceIndex = 0;
    int32_t useDay1Fallback = 0;
    soft3d::Vec3 spawnPosition;
    soft3d::Vec3 spawnDirection{0, 0, soft3d::fixedOne};
    uint16_t transitionColor = 0;
    soft3d::Vec3 transitionCameraPosition;
    soft3d::Vec3 transitionCameraTarget;
    soft3d::Fixed transitionCameraSpeed = 0;
    int32_t transitionStage = 0;
    int32_t transitionTimer = 0;
    int32_t restoreSavedState = 0;
    int32_t cameraWarmupCounter = 0;
    int32_t useCameraListener = 0;
    int32_t gameOverStage = 0;
    int32_t inkTimer = 0;
    int32_t specialDoorIndex = -1;
    int32_t qteUiKey = 0;
    int32_t qteUiTime = 0;
    int32_t qteUiMode = 0;
    int32_t qteUiFailure = 0;
    int32_t hideMaskIndex = -1;
    int32_t hideMaskActive = 0;
    int32_t tutorialId = 0;
    int32_t tutorialFrames = 0;
    int32_t authorizationStage = 0;
    int32_t authorizationTimer = 0;
    bool initialized = false;
};

} // namespace game

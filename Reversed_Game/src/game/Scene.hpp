#pragma once

#include "game/MyGameUnitMgr.hpp"
#include "soft3d/BezierCurve3.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/PresentationShaders.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace game {

class Scene final : public soft3d::SceneHost {
public:
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
    soft3d::Creature* GetPlayer() { return player.get(); }
    soft3d::CameraPathController* GetCameraController() { return &cameraController; }
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
    bool IsDayIntroPending() const { return dayIntroPending; }
    bool IsDayTitleVisible() const { return showDayTitle; }
    bool IsNextLevelPending() const { return nextLevelPending; }
    bool IsInitialized() const { return initialized; }
    bool IsUsingDay1Fallback() const { return useDay1Fallback; }
    const std::u16string& GetScriptName() const { return scriptName; }
    const soft3d::CameraPathCollection& GetCameraPaths() const { return *cameraPaths; }
    soft3d::FadeShader* GetFadeShader() { return &fadeShader; }

    static soft3d::BspEntity* CreateGameEntity(soft3d::BspEntityRecord* record, void* context,
                                                soft3d::BspEntityRecord* recordBase);

private:
    struct EnvironmentChannel {
        std::array<soft3d::AudioData*, 4> resources{};
        int32_t resourceCount = 0;
        int32_t baseDelay = 0;
        int32_t randomDelay = 0;
        int32_t elapsed = 0;
        int32_t nextDelay = -1;
        int32_t voice = 0;
    };

    void ResetEnvironmentAudio();
    void UpdateMapLoad();
    void UpdateWorld();
    void LoadNextScript();
    void RenderDayTitle();

    soft3d::GameEngine& engine;
    std::unique_ptr<soft3d::Bsp> bsp;
    std::unique_ptr<soft3d::CameraPathCollection> cameraPaths;
    soft3d::CameraPathController cameraController;
    std::unique_ptr<soft3d::Creature> player;
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
    int32_t transitionStage = 0;
    int32_t transitionTimer = 0;
    int32_t restoreSavedState = 0;
    int32_t cameraWarmupCounter = 0;
    int32_t useCameraListener = 0;
    bool initialized = false;
};

} // namespace game

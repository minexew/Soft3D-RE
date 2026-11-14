#pragma once

#include "soft3d/AudioManager.hpp"
#include "soft3d/Camera.hpp"
#include "soft3d/EngineClock.hpp"
#include "soft3d/Input.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/ScreenBlitter.hpp"
#include "soft3d/VID_impl.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dExit.hpp"
#include "soft3d/s3dMemInfo.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cstdint>
#include <memory>

namespace soft3d {

struct EngineConfig {
    int width = 320;
    int height = 240;
    std::unique_ptr<EngineClock> clock;
    std::unique_ptr<VideoBackend> videoBackend;
    std::unique_ptr<AudioBackend> audioBackend;
};

class Engine : public s3dBase {
public:
    explicit Engine(EngineConfig config);
    ~Engine() override;

    virtual int Init() = 0;
    virtual void Shutdown() = 0;
    virtual void LoadGameState() = 0;
    virtual void SaveGameState() = 0;

    void Start();
    virtual void Suspend();
    virtual void Resume();
    bool RunFrame();
    void StartGameEngineTimer(uint32_t targetFps);

    virtual void PreUpdate() = 0;
    virtual void PostUpdate() = 0;
    virtual void Update() = 0;
    virtual void PrepareRender() = 0;
    virtual void SubmitRenderables() = 0;
    virtual void PostRender() = 0;
    virtual void DrawFPS();

    RenderQueues& GetRenderQueues();
    Renderer& GetRenderer();
    Camera& GetCamera();
    s3dResManager& GetResourceManager();
    AudioManager& GetAudioManager();
    ScreenBlitter& GetScreenBlitter();
    VID_impl& GetVideo();
    Input& GetInput();
    uint32_t GetFrameIntervalMicroseconds() const;
    int32_t GetRealFps() const;
    void SetFpsDisplayEnabled(bool enabled);
    bool IsRunning() const;

protected:
    void UpdateFrame();
    void RenderFrame();
    void LimitFrameRate();

    bool running = false;
    bool preUpdateEnabled = true;
    bool postUpdateEnabled = true;
    bool submitEnabled = true;
    bool prepareEnabled = true;
    bool renderEnabled = true;
    uint32_t frameStart = 0;
    uint32_t previousFrameStart = 0;
    uint32_t targetFps = 0;
    int32_t realFps = 0;
    uint32_t frameIntervalMicroseconds = 0;
    bool fpsDisplayEnabled = false;
    std::unique_ptr<EngineClock> clock;
    int width;
    int height;
    RenderQueues renderQueues;
    s3dResManager resourceManager;
    AudioManager audioManager;
    Input input;
    Renderer renderer;
    Camera camera;
    ScreenBlitter screenBlitter;
    s3dMemInfo memInfo;
    s3dExit exitState;
    VID_impl video;
};

} // namespace soft3d

#pragma once

#include "soft3d/Renderable.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/ScreenBlitter.hpp"
#include "soft3d/VID_impl.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dResManager.hpp"

#include <chrono>
#include <cstdint>
#include <memory>

namespace soft3d {

struct EngineConfig {
    int width = 320;
    int height = 240;
    std::unique_ptr<VideoBackend> videoBackend;
};

class AudioManagerStub {
public:
    void Update() {}
    void Pause() {}
    void Resume() {}
    // The software mixer and Symbian streaming device are not reconstructed yet.
};

class InputStub {
public:
    void Update() {}
    void ClearKey() {}
};

class Engine : public s3dBase {
public:
    explicit Engine(EngineConfig config);
    ~Engine() override;

    virtual int Init();
    virtual void Shutdown();
    virtual void LoadGameState();
    virtual void SaveGameState();

    void Start();
    void Suspend();
    void Resume();
    bool RunFrame();
    void StartGameEngineTimer(uint32_t interval);

    virtual void PreUpdate();
    virtual void PostUpdate();
    virtual void Update();
    virtual void SubmitRenderables();
    virtual void PrepareRender();
    virtual void PostRender();

    RenderQueues& GetRenderQueues();
    Renderer& GetRenderer();
    s3dResManager& GetResourceManager();
    ScreenBlitter& GetScreenBlitter();
    VID_impl& GetVideo();
    bool IsRunning() const;

protected:
    virtual void UpdateFrame();
    virtual void RenderFrame();
    void LimitFrameRate();

    bool running = false;
    bool preUpdateEnabled = true;
    bool postUpdateEnabled = true;
    bool submitEnabled = true;
    bool prepareEnabled = true;
    bool renderEnabled = true;
    uint32_t frameIntervalMs = 0;
    std::chrono::steady_clock::time_point frameStart;
    std::chrono::steady_clock::time_point previousFrameStart;
    int width;
    int height;
    RenderQueues renderQueues;
    s3dResManager resourceManager;
    AudioManagerStub audioManager;
    InputStub input;
    Renderer renderer;
    ScreenBlitter screenBlitter;
    VID_impl video;
};

} // namespace soft3d

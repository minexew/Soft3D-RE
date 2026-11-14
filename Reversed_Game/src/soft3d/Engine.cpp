#include "soft3d/Engine.hpp"

#include "platform/linux/SystemClock.hpp"
#include "soft3d/s3dIO.hpp"

namespace soft3d {

Engine::Engine(EngineConfig config)
    : clock(std::move(config.clock))
    , width(config.width)
    , height(config.height)
    , audioManager(&resourceManager, std::move(config.audioBackend))
    , renderer(config.width, config.height)
    , camera(config.width, config.height, &renderer)
    , video(config.width, config.height, std::move(config.videoBackend))
{
    if (clock == nullptr) {
        clock = std::make_unique<platform::SystemClock>();
    }
    screenBlitter.SetTexture(renderer.GetCurrentTexture());
    memInfo.SetEngine(this);
    exitState.SetEngine(this);
    frameStart = clock->NowMicroseconds();
}

Engine::~Engine()
{
    video.Stop();
}

void Engine::Start()
{
    if (!running) {
        video.Start();
        renderer.SetBackBufferPtr(video.GetBackBufferPtr());
        screenBlitter.SetTexture(renderer.GetCurrentTexture());
        running = true;
        input.ClearKey();
    }
}

void Engine::Suspend()
{
    if (running) {
        video.Stop();
        audioManager.Pause();
        running = false;
    }
}

void Engine::Resume()
{
    if (running) {
        video.Start();
        renderer.SetBackBufferPtr(video.GetBackBufferPtr());
        screenBlitter.SetTexture(renderer.GetCurrentTexture());
        audioManager.Resume();
        running = true;
    }
}

bool Engine::RunFrame()
{
    uint32_t* backBuffer = video.GetBackBufferPtr();
    if (backBuffer == nullptr) {
        return false;
    }
    renderer.SetBackBufferPtr(backBuffer);
    if (preUpdateEnabled) {
        PreUpdate();
    }
    UpdateFrame();
    if (submitEnabled) {
        SubmitRenderables();
    }
    if (prepareEnabled) {
        PrepareRender();
    }
    RenderFrame();
    if (postUpdateEnabled) {
        PostUpdate();
    }
    audioManager.Update();
    if (!video.Update()) {
        return false;
    }
    input.Update();
    LimitFrameRate();
    return true;
}

void Engine::UpdateFrame()
{
    renderQueues.Clear();
    Update();
}

void Engine::RenderFrame()
{
    renderQueues.SortSceneQueues(false);
    renderQueues.RenderSceneQueues();
    renderQueues.RenderOverlayQueue();
    PostRender();
    if (renderEnabled) {
        renderer.Present();
    }
    if (fpsDisplayEnabled) {
        DrawFPS();
    }
}

void Engine::StartGameEngineTimer(uint32_t fps)
{
    targetFps = fps;
    frameIntervalMicroseconds = 1000000 / fps;
}

void Engine::LimitFrameRate()
{
    previousFrameStart = clock->NowMicroseconds();
    const uint32_t elapsed = previousFrameStart - frameStart;
    if (static_cast<int32_t>(elapsed) >= 0 && elapsed < frameIntervalMicroseconds) {
        clock->SleepMicroseconds(frameIntervalMicroseconds - elapsed);
    }
    frameStart = clock->NowMicroseconds();
}

void Engine::DrawFPS()
{
    char16_t text[10];
    WideSprintf(text, u"%d", realFps);
    video.DrawText(text, width - 20, 10);
}

RenderQueues& Engine::GetRenderQueues()
{
    return renderQueues;
}

Renderer& Engine::GetRenderer()
{
    return renderer;
}

Camera& Engine::GetCamera()
{
    return camera;
}

s3dResManager& Engine::GetResourceManager()
{
    return resourceManager;
}

AudioManager& Engine::GetAudioManager()
{
    return audioManager;
}

ScreenBlitter& Engine::GetScreenBlitter()
{
    return screenBlitter;
}

VID_impl& Engine::GetVideo()
{
    return video;
}

Input& Engine::GetInput()
{
    return input;
}

uint32_t Engine::GetFrameIntervalMicroseconds() const
{
    return frameIntervalMicroseconds;
}

int32_t Engine::GetRealFps() const
{
    return realFps;
}

void Engine::SetFpsDisplayEnabled(bool enabled)
{
    fpsDisplayEnabled = enabled;
}

bool Engine::IsRunning() const
{
    return running;
}

} // namespace soft3d

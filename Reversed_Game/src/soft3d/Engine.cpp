#include "soft3d/Engine.hpp"

#include <thread>

namespace soft3d {

Engine::Engine(EngineConfig config)
    : width(config.width)
    , height(config.height)
    , audioManager(&resourceManager, std::move(config.audioBackend))
    , renderer(config.width, config.height)
    , camera(config.width, config.height, &renderer)
    , video(config.width, config.height, std::move(config.videoBackend))
{
    screenBlitter.SetTexture(renderer.GetCurrentTexture());
    frameStart = std::chrono::steady_clock::now();
    previousFrameStart = frameStart;
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
}

void Engine::StartGameEngineTimer(uint32_t interval)
{
    frameIntervalMs = interval;
}

void Engine::LimitFrameRate()
{
    const auto now = std::chrono::steady_clock::now();
    previousFrameStart = now;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - frameStart).count();
    if (elapsed >= 0 && static_cast<uint32_t>(elapsed) < frameIntervalMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(frameIntervalMs - elapsed));
    }
    frameStart = std::chrono::steady_clock::now();
}

void Engine::DrawFPS() {}

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

bool Engine::IsRunning() const
{
    return running;
}

} // namespace soft3d

#pragma once

#include "app/EngineStubs.hpp"
#include "soft3d/GameEngine.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace app {

struct ApplicationConfig {
    int scale = 3;
    int maximumFrames = 0;
    std::string renderer = "auto";
    bool enableAudio = true;
};

class Application {
public:
    explicit Application(ApplicationConfig config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int Run();

private:
    class HostEngine;

    bool InitializeVideo();
    void RenderPlaceholder(uint64_t frameNumber, double deltaSeconds);
    void Shutdown();

    ApplicationConfig config;
    std::unique_ptr<HostEngine> engine;
    AudioStub audio;
    ControlsStub controls;
    TimerStub timer;
    float markerX = 148.0F;
    float markerY = 108.0F;
    uint64_t currentFrame = 0;
    double currentDeltaSeconds = 0.0;
};

} // namespace app

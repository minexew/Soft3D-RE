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
    std::string resourceArchive;
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
    bool InitializeVideo();
    void Shutdown();

    ApplicationConfig config;
    std::unique_ptr<soft3d::GameEngine> engine;
    AudioStub audio;
    ControlsStub controls;
    TimerStub timer;
    uint64_t currentFrame = 0;
};

} // namespace app

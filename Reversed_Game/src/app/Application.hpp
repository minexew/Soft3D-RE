#pragma once

#include "app/EngineStubs.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
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
    bool InitializeVideo();
    void RenderPlaceholder(uint64_t frameNumber, double deltaSeconds);
    void Shutdown();

    ApplicationConfig config;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    AudioStub audio;
    ControlsStub controls;
    TimerStub timer;
    SoftwareFrame frame;
    float markerX = 148.0F;
    float markerY = 108.0F;
};

} // namespace app

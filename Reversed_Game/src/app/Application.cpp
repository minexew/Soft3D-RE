#include "app/Application.hpp"

#include "platform/sdl/SdlAudioBackend.hpp"
#include "platform/sdl/SdlClock.hpp"
#include "platform/sdl/SdlInput.hpp"
#include "platform/sdl/SdlVideoBackend.hpp"
#include <filesystem>
#include <iostream>
#include <utility>

namespace app {
namespace {

constexpr int frameWidth = 320;
constexpr int frameHeight = 240;

std::u16string WidePath(const std::string& path)
{
    std::u16string result;
    result.reserve(path.size());
    for (unsigned char character : path) {
        result.push_back(static_cast<char16_t>(character));
    }
    return result;
}

std::string FindResourceArchive(const std::string& configured)
{
    if (!configured.empty()) {
        return configured;
    }
    const std::filesystem::path candidates[] = {
        "res.spk",
        "../src/resources/res.spk",
        "Reversed_Game/src/resources/res.spk",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate)) {
            return candidate.string();
        }
    }
    return candidates[1].string();
}

} // namespace

Application::Application(ApplicationConfig applicationConfig)
    : config(std::move(applicationConfig))
{
}

Application::~Application()
{
    Shutdown();
}

int Application::Run()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }
    if (!InitializeVideo()) {
        Shutdown();
        return 1;
    }
    if (config.enableAudio && !engine->GetAudioManager().IsRunning()) {
        std::cerr << "Audio unavailable; continuing silently: " << SDL_GetError() << '\n';
    }

    std::cout << "Renderer: " << engine->GetVideo().GetBackendName() << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight
              << " (CPU), audio: "
              << (engine->GetAudioManager().IsRunning() ? "16 kHz mono" : "disabled") << '\n';

    bool running = true;
    while (running && (config.maximumFrames == 0 ||
                       currentFrame < static_cast<uint64_t>(config.maximumFrames))) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
            soft3d::platform::ProcessSdlKeyEvent(engine->GetInput(), event);
        }

        if (!engine->RunFrame()) {
            std::cerr << "Frame presentation failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        ++currentFrame;
    }

    Shutdown();
    return 0;
}

bool Application::InitializeVideo()
{
    soft3d::EngineConfig engineConfig;
    engineConfig.width = frameWidth;
    engineConfig.height = frameHeight;
    engineConfig.clock = std::make_unique<soft3d::platform::SdlClock>();
    engineConfig.videoBackend = std::make_unique<soft3d::platform::SdlVideoBackend>(
        config.scale, config.renderer);
    if (config.enableAudio) {
        engineConfig.audioBackend = std::make_unique<soft3d::platform::SdlAudioBackend>();
    }
    engine = std::make_unique<soft3d::GameEngine>(std::move(engineConfig));
    engine->SetFpsDisplayEnabled(config.showFps);
    config.resourceArchive = FindResourceArchive(config.resourceArchive);
    engine->SetResourceArchive(WidePath(config.resourceArchive));
    if (engine->Init() != 0) {
        std::cerr << "Game initialization failed using resources: " << config.resourceArchive << '\n';
        return false;
    }
    engine->Start();
    return engine->IsRunning();
}

void Application::Shutdown()
{
    if (engine != nullptr) {
        engine->GetVideo().Stop();
        engine->Shutdown();
        engine.reset();
    }
    SDL_Quit();
}

} // namespace app

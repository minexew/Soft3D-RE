#include "app/Application.hpp"

#include "platform/sdl/SdlVideoBackend.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <utility>

namespace app {
namespace {

constexpr int frameWidth = 320;
constexpr int frameHeight = 240;
constexpr uint64_t frameIntervalNs = 1000000000ULL / 60ULL;

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
    if (config.enableAudio && !audio.Start()) {
        std::cerr << "Audio unavailable; continuing silently: " << SDL_GetError() << '\n';
        audio.Stop();
    }

    std::cout << "Renderer: " << engine->GetVideo().GetBackendName() << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight
              << " (CPU), audio: " << (audio.IsRunning() ? "ready" : "disabled") << '\n';

    timer.Reset();
    bool running = true;
    while (running && (config.maximumFrames == 0 ||
                       currentFrame < static_cast<uint64_t>(config.maximumFrames))) {
        const uint64_t frameStart = timer.NowNanoseconds();
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
            controls.ProcessEvent(event);
        }

        const ControlState& input = controls.State();
        const int selection = controls.ConsumeMenuSelection();
        engine->SetMenuInput(selection < 0, selection > 0, input.action);
        if (!engine->RunFrame()) {
            std::cerr << "Frame presentation failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        ++currentFrame;

        const uint64_t remaining = frameIntervalNs -
            std::min(frameIntervalNs, timer.NowNanoseconds() - frameStart);
        if (remaining >= 1000000ULL) {
            SDL_Delay(static_cast<uint32_t>(remaining / 1000000ULL));
        }
    }

    Shutdown();
    return 0;
}

bool Application::InitializeVideo()
{
    soft3d::EngineConfig engineConfig;
    engineConfig.width = frameWidth;
    engineConfig.height = frameHeight;
    engineConfig.videoBackend = std::make_unique<soft3d::platform::SdlVideoBackend>(
        config.scale, config.renderer);
    engine = std::make_unique<soft3d::GameEngine>(std::move(engineConfig));
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
    audio.Stop();
    if (engine != nullptr) {
        engine->GetVideo().Stop();
        engine->Shutdown();
        engine.reset();
    }
    SDL_Quit();
}

} // namespace app

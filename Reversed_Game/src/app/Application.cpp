#include "app/Application.hpp"

#include "game/GameSI.hpp"
#include "game/GameUiController.hpp"
#include "game/Scene.hpp"
#include "platform/sdl/SdlAudioBackend.hpp"
#include "platform/sdl/SdlClock.hpp"
#include "platform/sdl/SdlInput.hpp"
#include "platform/sdl/SdlVideoBackend.hpp"
#include "platform/sdl/SdlVibrationBackend.hpp"
#include "soft3d/MenuDialogs.hpp"
#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

namespace app {
namespace {

constexpr int frameWidth = 320;
constexpr int frameHeight = 240;

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

    const char* display = "automatic";
    if (config.displayOrientation == DisplayOrientation::Landscape) {
        display = "landscape";
    } else if (config.displayOrientation == DisplayOrientation::Portrait) {
        display = "portrait";
    }
    std::cout << "Renderer: " << engine->GetVideo().GetBackendName() << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight << " (CPU), display: "
              << display << ", audio: "
              << (engine->GetAudioManager().IsRunning() ? "16 kHz mono" : "disabled") << '\n';

    bool running = true;
    while (running && engine->IsRunning() && (config.maximumFrames == 0 ||
                       currentFrame < static_cast<uint64_t>(config.maximumFrames))) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
            soft3d::platform::ProcessSdlKeyEvent(
                engine->GetInput(), event,
                IsLandscapeDisplay() ? engine->GetControlMap().GetOrientation() : 0);
        }

        if (!engine->RunFrame()) {
            std::cerr << "Frame presentation failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        ++currentFrame;
    }

    if (config.startNewGame && scene != nullptr) {
        std::cout << "New game: day " << scene->GetDay() << ", map " << scene->GetMapId()
                  << ", progress " << scene->GetMapProgress() << '\n';
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
    auto videoBackend = std::make_unique<soft3d::platform::SdlVideoBackend>(
        config.scale, config.renderer,
        config.displayOrientation == DisplayOrientation::Landscape);
    auto* sdlVideo = videoBackend.get();
    engineConfig.videoBackend = std::move(videoBackend);
    engineConfig.vibrationBackend = std::make_unique<soft3d::platform::SdlVibrationBackend>();
    if (config.enableAudio) {
        engineConfig.audioBackend = std::make_unique<soft3d::platform::SdlAudioBackend>();
    }
    engine = std::make_unique<soft3d::GameEngine>(std::move(engineConfig));
    sdlVideo->SetControlOrientationSource([game = engine.get()] {
        return game->GetControlMap().GetOrientation();
    });
    sdlVideo->SetLandscapeSource([this] { return IsLandscapeDisplay(); });
    engine->SetFpsDisplayEnabled(config.showFps);
    config.resourceArchive = FindResourceArchive(config.resourceArchive);
    std::vector<char16_t> resourceArchive(config.resourceArchive.size() + 1);
    soft3d::ConvertAsciiToWide(resourceArchive.data(), config.resourceArchive.c_str());
    engine->SetResourceArchive(resourceArchive.data());
    if (engine->Init() != 0) {
        std::cerr << "Game initialization failed using resources: " << config.resourceArchive << '\n';
        return false;
    }

    auto sceneOwner = std::make_unique<game::Scene>(*engine);
    scene = sceneOwner.get();
    const soft3d::result sceneStatus = sceneOwner->Initialize();
    if (sceneStatus != soft3d::S3D_OK) {
        std::cerr << "Scene initialization failed with Soft3D result " << sceneStatus << '\n';
        scene = nullptr;
        return false;
    }
    engine->SetSceneHost(std::move(sceneOwner));
    engine->SetAuthorizationHost(scene->GetLicense());
    gameInterface = std::make_unique<game::GameSI>();
    gameInterface->Bind({scene, scene->GetUnitManager(), scene->GetCameraController(),
                         scene->GetPlayer(), scene->GetGameUi(), scene->GetAudio(),
                         scene->GetResources(), scene->GetBsp(), scene->GetControlMap()});
    engine->SetScriptInterface(gameInterface.get());

    soft3d::GameplayHooks hooks;
    hooks.startNewGame = [this](const soft3d::NewGameOptions& options) {
        scene->StartNewGame(options);
    };
    hooks.loadGame = [this](int slot) {
        return scene->LoadSaveSlot(slot);
    };
    hooks.returnToMenu = [this] {
        scene->ReturnToMainMenu();
    };
    engine->SetGameplayHooks(std::move(hooks));
    scene->GetUiController()->InstallHooks();
    engine->Start();
    if (config.startNewGame) {
        engine->StartNewGame();
    }
    return engine->IsRunning();
}

bool Application::IsLandscapeDisplay() const
{
    if (config.displayOrientation == DisplayOrientation::Landscape) {
        return true;
    }
    if (config.displayOrientation == DisplayOrientation::Portrait) {
        return false;
    }
    return engine != nullptr && engine->GetMainMenu() != nullptr &&
           engine->GetMainMenu()->GetMode() != soft3d::MainUIDlg::Handedness;
}

void Application::Shutdown()
{
    if (engine != nullptr) {
        engine->SetScriptInterface(nullptr);
        engine->SetAuthorizationHost(nullptr);
        engine->GetVideo().Stop();
        engine->Shutdown();
        engine.reset();
        gameInterface.reset();
        scene = nullptr;
    }
    SDL_Quit();
}

} // namespace app

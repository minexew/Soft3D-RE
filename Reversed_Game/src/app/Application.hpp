#pragma once

#include "soft3d/GameEngine.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace game {
class GameSI;
class Scene;
}

namespace app {

enum class DisplayOrientation {
    Automatic,
    Portrait,
    Landscape,
};

struct ApplicationConfig {
    int scale = 3;
    int maximumFrames = 0;
    std::string renderer = "auto";
    std::string resourceArchive;
    DisplayOrientation displayOrientation = DisplayOrientation::Automatic;
    bool enableAudio = true;
    bool showFps = false;
    bool startNewGame = false;
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
    bool IsLandscapeDisplay() const;
    void Shutdown();

    ApplicationConfig config;
    std::unique_ptr<game::GameSI> gameInterface;
    game::Scene* scene = nullptr;
    std::unique_ptr<soft3d::GameEngine> engine;
    uint64_t currentFrame = 0;
};

} // namespace app

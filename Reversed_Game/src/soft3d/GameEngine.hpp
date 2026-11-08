#pragma once

#include "soft3d/Engine.hpp"

#include <string>

namespace soft3d {

class SceneStub {
public:
    void Update() {}
    void PostUpdate() {}
    void PrepareRender() {}
    void SubmitRenderables() {}
    // Game-specific scene, BSP, creature, and script behavior remains to be reconstructed.
};

class UiManagerStub : public Renderable {
public:
    void Render() override {}
};

class GameMenuStub {
public:
    void Update() {}
};

class ScriptEngineStub {
public:
    void Init() {}
    void Update() {}
};

class StringManagerStub {
};

class GameEngine : public Engine {
public:
    explicit GameEngine(EngineConfig config);

    int Init() override;
    void Shutdown() override;
    void PreUpdate() override;
    void PostUpdate() override;
    void Update() override;
    void SubmitRenderables() override;
    void PrepareRender() override;

    void SetResourceArchive(std::u16string path);
    void SetInGame(bool inGame);

private:
    std::unique_ptr<SceneStub> scene;
    std::unique_ptr<UiManagerStub> uiManager;
    std::unique_ptr<GameMenuStub> gameMenu;
    std::unique_ptr<StringManagerStub> baseFont;
    std::unique_ptr<StringManagerStub> secondaryFont;
    ScriptEngineStub scriptEngine;
    std::u16string resourceArchive;
    bool inGame = false;
    bool menuActive = false;
    int language = 0;
};

} // namespace soft3d

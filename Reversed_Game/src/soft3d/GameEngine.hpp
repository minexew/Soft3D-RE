#pragma once

#include "soft3d/Engine.hpp"
#include "soft3d/Font.hpp"
#include "soft3d/UiManager.hpp"

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

class ScriptEngineStub {
public:
    void Init() {}
    void Update() {}
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
    void SetLanguage(int value);
    void SetMenuInput(bool left, bool right, bool accept);
    StringManager* GetBaseFont();
    StringManager* GetSecondaryFont();
    UiManager* GetUiManager();

private:
    std::unique_ptr<SceneStub> scene;
    std::unique_ptr<UiManager> uiManager;
    std::unique_ptr<GameMenu> gameMenu;
    std::unique_ptr<StringManager> baseFont;
    std::unique_ptr<StringManager> secondaryFont;
    ScriptEngineStub scriptEngine;
    std::u16string resourceArchive;
    bool inGame = false;
    bool menuActive = false;
    int language = 2;
};

} // namespace soft3d

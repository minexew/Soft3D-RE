#include "soft3d/GameEngine.hpp"

#include <utility>

namespace soft3d {

GameEngine::GameEngine(EngineConfig config)
    : Engine(std::move(config))
    , scene(std::make_unique<SceneStub>())
    , uiManager(std::make_unique<UiManagerStub>())
    , gameMenu(std::make_unique<GameMenuStub>())
    , baseFont(std::make_unique<StringManagerStub>())
    , secondaryFont(std::make_unique<StringManagerStub>())
{
}

int GameEngine::Init()
{
    if (!resourceArchive.empty()) {
        OpenGameResourceArchive(&resourceManager, resourceArchive.c_str());
    }
    scriptEngine.Init();
    StartGameEngineTimer(0x11);
    return 0;
}

void GameEngine::Shutdown()
{
    inGame = false;
    menuActive = false;
}

void GameEngine::PreUpdate()
{
    if (inGame) {
        scene->Update();
    } else {
        scriptEngine.Update();
    }
}

void GameEngine::PostUpdate()
{
    if (inGame) {
        scene->PostUpdate();
    }
}

void GameEngine::Update()
{
}

void GameEngine::SubmitRenderables()
{
    if (inGame) {
        scene->SubmitRenderables();
    }
    renderQueues.Submit(uiManager.get(), RenderQueues::sceneQueueCount);
}

void GameEngine::PrepareRender()
{
    if (inGame || menuActive) {
        scene->PrepareRender();
    }
    gameMenu->Update();
}

void GameEngine::SetResourceArchive(std::u16string path)
{
    resourceArchive = std::move(path);
}

void GameEngine::SetInGame(bool value)
{
    inGame = value;
}

} // namespace soft3d

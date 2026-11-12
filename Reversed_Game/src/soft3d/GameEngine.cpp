#include "soft3d/GameEngine.hpp"

#include <utility>

namespace soft3d {

GameEngine::GameEngine(EngineConfig config)
    : Engine(std::move(config))
    , scene(std::make_unique<SceneStub>())
    , uiManager(std::make_unique<UiManager>())
    , gameMenu(std::make_unique<GameMenu>())
    , baseFont(std::make_unique<StringManager>())
    , secondaryFont(std::make_unique<StringManager>())
{
}

int GameEngine::Init()
{
    if (resourceArchive.empty() ||
        OpenGameResourceArchive(&resourceManager, resourceArchive.c_str()) != S3D_OK) {
        return S3D_FILE;
    }
    scriptEngine.Init();
    StartGameEngineTimer(0x11);

    baseFont->SetEngine(this);
    if (baseFont->LoadBaseFont(language, resourceManager.GetFilePak()) != S3D_OK) {
        return S3D_FILE;
    }
    baseFont->SetRenderProperties(
        reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels()), width, height);

    secondaryFont->SetEngine(this);
    if (secondaryFont->LoadBaseFont(language, resourceManager.GetFilePak()) != S3D_OK ||
        secondaryFont->LoadSbnFont(language == 2 ? u".\\ui\\system.sbn" : u".\\uien\\system.sbn",
                                   resourceManager.GetFilePak()) != S3D_OK) {
        return S3D_FILE;
    }
    secondaryFont->SetRenderProperties(
        reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels()), width, height);

    uiManager->SetResourceManager(&resourceManager);
    uiManager->SetScreenBlitter(&screenBlitter);
    uiManager->SetFont(secondaryFont.get());
    if (uiManager->LoadStrSdt(language == 2 ? u".\\ui\\str.sdt" : u".\\uien\\str.sdt") != S3D_OK ||
        uiManager->LoadStateSdt(language == 2 ? u".\\ui\\state.sdt" : u".\\uien\\state.sdt") != S3D_OK) {
        return S3D_FILE;
    }
    gameMenu->Update(*uiManager);
    return 0;
}

void GameEngine::Shutdown()
{
    inGame = false;
    menuActive = false;
}

void GameEngine::LoadGameState()
{
}

void GameEngine::SaveGameState()
{
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
    gameMenu->Update(*uiManager);
}

void GameEngine::PostRender()
{
}

void GameEngine::DrawFPS()
{
}

void GameEngine::SetResourceArchive(std::u16string path)
{
    resourceArchive = std::move(path);
}

void GameEngine::SetInGame(bool value)
{
    inGame = value;
}

void GameEngine::SetLanguage(int value)
{
    language = value;
}

void GameEngine::SetMenuInput(bool left, bool right, bool accept)
{
    gameMenu->SetInput(left, right, accept);
}

StringManager* GameEngine::GetBaseFont()
{
    return baseFont.get();
}

StringManager* GameEngine::GetSecondaryFont()
{
    return secondaryFont.get();
}

UiManager* GameEngine::GetUiManager()
{
    return uiManager.get();
}

} // namespace soft3d

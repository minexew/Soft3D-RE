#include "soft3d/GameEngine.hpp"

#include "soft3d/GameUIDlg.hpp"
#include "soft3d/MenuBackdrop.hpp"
#include "soft3d/MenuDialogs.hpp"

#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace soft3d {

GameEngine::GameEngine(EngineConfig config)
    : Engine(std::move(config))
    , menuBackdrop(std::make_unique<MenuBackdrop>(this))
    , uiManager(std::make_unique<UiManager>())
    , scene(std::make_unique<SceneHost>())
    , baseFont(std::make_unique<StringManager>())
    , secondaryFont(std::make_unique<StringManager>())
    , controlMap(configuration)
{
}

GameEngine::~GameEngine() = default;

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
    uiManager->SetLanguage(language);
    if (uiManager->LoadStrSdt(language == 2 ? u".\\ui\\str.sdt" : u".\\uien\\str.sdt") != S3D_OK) {
        return S3D_FILE;
    }
    InitializeUiSoundNames();

    auto main = std::make_unique<MainUIDlg>(*this, uiManager.get());
    mainMenu = main.get();
    uiManager->AddDialog(std::move(main));
    uiManager->AddDialog(std::make_unique<OptionDlg>(*this, uiManager.get()));
    auto about = std::make_unique<AboutDlg>(*this, uiManager.get());
    aboutDialog = about.get();
    uiManager->AddDialog(std::move(about));
    uiManager->AddDialog(std::make_unique<HelpDlg>(*this, uiManager.get()));
    uiManager->AddDialog(std::make_unique<PlayDlg>(*this, uiManager.get()));
    uiManager->AddDialog(std::make_unique<LoadDlg>(*this, uiManager.get()));
    auto game = std::make_unique<GameUIDlg>(*this, uiManager.get());
    gameUi = game.get();
    uiManager->AddDialog(std::move(game));

    if (uiManager->LoadStateSdt(language == 2 ? u".\\ui\\state.sdt" : u".\\uien\\state.sdt") !=
        S3D_OK) {
        return S3D_FILE;
    }
    LoadGameState();
    for (size_t index = 0; index < uiManager->GetDialogCount(); ++index) {
        uiManager->GetDialogRuntime(index)->SetActive(index == 0);
    }
    return 0;
}

void GameEngine::Shutdown()
{
    inGame = false;
    menuActive = false;
}

void GameEngine::LoadGameState()
{
    const std::u16string path = PersistencePath(u"config.sdt");
    if (!persistenceIo.Open(path.c_str(), u"rb")) {
        return;
    }
    persistenceIo.Seek(0, SEEK_END);
    const uint32_t bytes = persistenceIo.Tell();
    persistenceIo.Seek(0, SEEK_SET);
    std::vector<uint8_t> data(bytes);
    const uint32_t read = data.empty() ? 0 : persistenceIo.Read(data.data(), bytes);
    persistenceIo.Close();
    if (read == data.size()) {
        DeserializeGameConfiguration(data.data(), data.size(), configuration);
        vibration->SetEnabled(configuration.vibrationEnabled != 0);
    }
}

void GameEngine::SaveGameState()
{
    configuration.masterVolume = static_cast<uint32_t>(audioManager.GetDevice().GetMasterVolume());
    const auto data = SerializeGameConfiguration(configuration);
    const std::u16string path = PersistencePath(u"config.sdt");
    if (persistenceIo.Open(path.c_str(), u"wb")) {
        persistenceIo.Write(data.data(), static_cast<int>(data.size()), 1);
        persistenceIo.Close();
    }
}

void GameEngine::PreUpdate()
{
    if (menuActive) {
        menuBackdrop->Update();
    }
    if (inGame) {
        if (authorizationHost != nullptr) {
            authorizationHost->PollSession();
        }
        scriptEngine.Update();
        scene->Update();
        return;
    }
    static constexpr Key priority[] = {
        Key::Up, Key::Left, Key::Down, Key::Right, Key::SoftLeft, Key::SoftRight,
        Key::Accept, Key::Number1, Key::Number2, Key::Number3, Key::Number4,
        Key::Number5, Key::Number6, Key::Number7, Key::Number8, Key::Number9, Key::Number0,
    };
    for (Key key : priority) {
        if (input.IsKeyClick(key)) {
            uiManager->DispatchInput(1, static_cast<int32_t>(key), 0);
            break;
        }
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
    if (inGame) {
        scene->PrepareRender();
        if (gameUi != nullptr) gameUi->PrepareModels();
    } else if (menuActive) {
        menuBackdrop->PrepareRender();
    }
    if (aboutDialog != nullptr) {
        aboutDialog->PrepareModel();
    }
}

void GameEngine::PostRender()
{
    if (inGame) {
        scene->PostRender();
    }
}

void GameEngine::DrawFPS()
{
    Engine::DrawFPS();
}

void GameEngine::SetResourceArchive(std::u16string path)
{
    resourceArchive = std::move(path);
}

void GameEngine::SetPersistenceDirectory(std::u16string path)
{
    persistenceDirectory = std::move(path);
}

void GameEngine::SetGameplayHooks(GameplayHooks hooks)
{
    gameplayHooks = std::move(hooks);
}

void GameEngine::SetSceneHost(std::unique_ptr<SceneHost> value)
{
    if (value != nullptr) {
        scene = std::move(value);
    }
}

result GameEngine::LoadMenuBackdrop()
{
    const result status = menuBackdrop->Load();
    menuActive = status == S3D_OK;
    return status;
}

void GameEngine::ActivateMenuDialog(int index)
{
    for (size_t current = 0; current < uiManager->GetDialogCount(); ++current) {
        uiManager->GetDialogRuntime(current)->SetActive(static_cast<int>(current) == index);
    }
    if (index == 3) {
        static_cast<HelpDlg*>(uiManager->GetDialogRuntime(3))->Reset();
    }
}

void GameEngine::ShowMainMenu(int selection)
{
    inGame = false;
    ActivateMenuDialog(0);
    mainMenu->ShowMain(selection);
}

void GameEngine::StartNewGame(const NewGameOptions& options)
{
    if (gameplayHooks.startNewGame) {
        gameplayHooks.startNewGame(options);
    }
    inGame = true;
    menuActive = false;
    gameUi->Initialize();
    ActivateMenuDialog(6);
}

bool GameEngine::LoadSaveSlot(int slot)
{
    if (slot < 1 || slot > 3) {
        return false;
    }
    char16_t name[32];
    WideSprintf(name, u"slot%d.sav", slot);
    const std::u16string path = PersistencePath(name);
    if (!persistenceIo.Open(path.c_str(), u"rb")) {
        return false;
    }
    persistenceIo.Close();
    if (gameplayHooks.loadGame) {
        gameUi->Initialize();
        if (!gameplayHooks.loadGame(slot)) {
            return false;
        }
    }
    inGame = true;
    menuActive = false;
    if (!gameplayHooks.loadGame) {
        gameUi->Initialize();
    }
    ActivateMenuDialog(6);
    return true;
}

void GameEngine::ReturnToMainMenu()
{
    if (gameplayHooks.returnToMenu) {
        gameplayHooks.returnToMenu();
    }
    gameUi->Initialize();
    ShowMainMenu(0);
    LoadMenuBackdrop();
}

void GameEngine::RequestExit()
{
    SaveGameState();
    if (gameplayHooks.exitGame) {
        gameplayHooks.exitGame();
    } else {
        Stop();
    }
}

void GameEngine::SetSoundEnabled(bool enabled)
{
    audioManager.SetGlobalVolume(enabled ? 255 : 0);
}

void GameEngine::SetVibrationEnabled(bool enabled)
{
    configuration.vibrationEnabled = enabled ? 1 : 0;
    vibration->SetEnabled(enabled);
}

bool GameEngine::Vibrate(uint32_t durationMilliseconds)
{
    return configuration.vibrationEnabled != 0 && vibration->Vibrate(durationMilliseconds);
}

bool GameEngine::IsVibrationSupported() const
{
    return vibration->IsSupported();
}

void GameEngine::SetAuthorizationHost(AuthorizationHost* host)
{
    authorizationHost = host;
}

void GameEngine::SetControlOrientation(int orientation)
{
    renderer.SetOrientation(orientation);
    controlMap.SetOrientation(orientation);
}

bool GameEngine::HasSaveSlots()
{
    for (int slot = 1; slot <= 3; ++slot) {
        char16_t name[32];
        WideSprintf(name, u"slot%d.sav", slot);
        const std::u16string path = PersistencePath(name);
        if (persistenceIo.Open(path.c_str(), u"rb")) {
            persistenceIo.Close();
            return true;
        }
    }
    return false;
}

result GameEngine::RefreshSaveSlots()
{
    for (int slot = 0; slot < 3; ++slot) {
        char16_t name[32];
        WideSprintf(name, u"slot%d.sav", slot + 1);
        const std::u16string path = PersistencePath(name);
        saveSlots[slot].present = false;
        saveSlots[slot].words.fill(0);
        if (!persistenceIo.Open(path.c_str(), u"rb")) {
            continue;
        }
        saveSlots[slot].present = persistenceIo.Read(
            saveSlots[slot].words.data(), sizeof(uint32_t) * saveSlots[slot].words.size()) ==
            sizeof(uint32_t) * saveSlots[slot].words.size();
        persistenceIo.Close();
    }
    return S3D_OK;
}

void GameEngine::SetInGame(bool value)
{
    inGame = value;
}

void GameEngine::SetLanguage(int value)
{
    language = value;
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

MenuBackdrop* GameEngine::GetMenuBackdrop()
{
    return menuBackdrop.get();
}

MainUIDlg* GameEngine::GetMainMenu()
{
    return mainMenu;
}

GameUIDlg* GameEngine::GetGameUi()
{
    return gameUi;
}

AboutDlg* GameEngine::GetAboutDialog()
{
    return aboutDialog;
}

GameConfiguration& GameEngine::GetConfiguration()
{
    return configuration;
}

const GameConfiguration& GameEngine::GetConfiguration() const
{
    return configuration;
}

ControlMap& GameEngine::GetControlMap()
{
    return controlMap;
}

GameContent& GameEngine::GetContent()
{
    return content;
}

GameplayHooks& GameEngine::GetGameplayHooks()
{
    return gameplayHooks;
}

std::array<SaveSlotInfo, 3>& GameEngine::GetSaveSlots()
{
    return saveSlots;
}

const std::array<SaveSlotInfo, 3>& GameEngine::GetSaveSlots() const
{
    return saveSlots;
}

int GameEngine::GetWidth() const
{
    return width;
}

int GameEngine::GetHeight() const
{
    return height;
}

int GameEngine::GetLanguage() const
{
    return language;
}

std::u16string GameEngine::PersistencePath(const char16_t* name) const
{
    if (persistenceDirectory.empty() || persistenceDirectory == u".") {
        return u".\\" + std::u16string(name);
    }
    return persistenceDirectory + u"\\" + name;
}

void GameEngine::InitializeUiSoundNames()
{
    static constexpr const char16_t* names[] = {
        u"", u"ui_select", u"ui_selectitem", u"ui_confirm", u"ui_cancel",
        u"ui_item", u"ui_false", u"ui_paper", u"t_lock_open",
    };
    uiSoundNames.fill(0);
    for (size_t index = 0; index < 9; ++index) {
        const char16_t* source = names[index];
        char16_t* destination = uiSoundNames.data() + index * 64;
        do {
            *destination++ = *source;
        } while (*source++ != u'\0');
    }
    audioManager.SetSoundNameTable(uiSoundNames.data());
}

} // namespace soft3d

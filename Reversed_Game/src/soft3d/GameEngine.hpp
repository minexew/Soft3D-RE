#pragma once

#include "soft3d/Engine.hpp"
#include "soft3d/Font.hpp"
#include "soft3d/GameConfig.hpp"
#include "soft3d/GameContent.hpp"
#include "soft3d/SimpleScript.hpp"
#include "soft3d/UiManager.hpp"

#include <array>
#include <functional>
#include <string>

namespace soft3d {

class AboutDlg;
class GameUIDlg;
class MainUIDlg;
class MenuBackdrop;

class AuthorizationHost {
public:
    virtual ~AuthorizationHost() = default;
    virtual void ResetSession() = 0;
    virtual bool ConnectSession() = 0;
    virtual bool PollSession() = 0;
    virtual void SetMenuResultHandler(std::function<void(int)> handler) = 0;
};

struct SaveSlotInfo {
    std::array<uint32_t, 11> words{};
    bool present = false;

    uint32_t GetDay() const { return words[0]; }
    uint32_t GetSaveNumber() const { return words[5]; }
    uint32_t GetPlayTime() const { return words[9]; }
};

struct GameplayHooks {
    std::function<void(const NewGameOptions&)> startNewGame;
    std::function<bool(int)> loadGame;
    std::function<void()> returnToMenu;
    std::function<void()> exitGame;
    std::function<void(int, int, int, int)> renderAboutModel;
    std::function<void(int)> renderMap;
    std::function<void()> renderGameHud;
    std::function<int(int, int)> useItem;
    std::function<bool(int, int)> combineItems;
    std::function<void()> synchronizeContent;
    std::function<void(bool)> setGamePaused;
    std::function<bool()> canOpenGameMenu;
    std::function<void(int, int)> prepareGameModels;
    std::function<bool(int)> saveGame;
    std::function<void()> finishGame;
};

class SceneHost {
public:
    virtual ~SceneHost() = default;
    virtual void Update() {}
    virtual void PostUpdate() {}
    virtual void PrepareRender() {}
    virtual void SubmitRenderables() {}
    virtual void PostRender() {}
};

class GameEngine : public Engine {
public:
    explicit GameEngine(EngineConfig config);
    ~GameEngine() override;

    int Init() override;
    void Shutdown() override;
    void LoadGameState() override;
    void SaveGameState() override;
    void PreUpdate() override;
    void PostUpdate() override;
    void Update() override;
    void PrepareRender() override;
    void SubmitRenderables() override;
    void PostRender() override;
    void DrawFPS() override;

    void SetResourceArchive(std::u16string path);
    void SetPersistenceDirectory(std::u16string path);
    void SetGameplayHooks(GameplayHooks hooks);
    void SetSceneHost(std::unique_ptr<SceneHost> scene);
    void SetInGame(bool inGame);
    void SetLanguage(int value);
    result LoadMenuBackdrop();
    void ActivateMenuDialog(int index);
    void ShowMainMenu(int selection);
    void StartNewGame(const NewGameOptions& options = {});
    bool LoadSaveSlot(int slot);
    void ReturnToMainMenu();
    void RequestExit();
    void SetSoundEnabled(bool enabled);
    void SetVibrationEnabled(bool enabled);
    bool Vibrate(uint32_t durationMilliseconds);
    bool IsVibrationSupported() const;
    void SetAuthorizationHost(AuthorizationHost* host);
    AuthorizationHost* GetAuthorizationHost() const { return authorizationHost; }
    void SetControlOrientation(int orientation);
    bool HasSaveSlots();
    result RefreshSaveSlots();

    StringManager* GetBaseFont();
    StringManager* GetSecondaryFont();
    UiManager* GetUiManager();
    MenuBackdrop* GetMenuBackdrop();
    MainUIDlg* GetMainMenu();
    GameUIDlg* GetGameUi();
    AboutDlg* GetAboutDialog();
    GameConfiguration& GetConfiguration();
    const GameConfiguration& GetConfiguration() const;
    ControlMap& GetControlMap();
    GameContent& GetContent();
    SimpleScript& GetScriptEngine() { return scriptEngine; }
    void SetScriptInterface(ScriptInterface* interface) { scriptEngine.SetScriptInterface(interface); }
    GameplayHooks& GetGameplayHooks();
    std::array<SaveSlotInfo, 3>& GetSaveSlots();
    const std::array<SaveSlotInfo, 3>& GetSaveSlots() const;
    std::u16string PersistencePath(const char16_t* name) const;
    int GetWidth() const;
    int GetHeight() const;
    int GetLanguage() const;
    bool IsInGame() const { return inGame; }

private:
    void InitializeUiSoundNames();

    std::unique_ptr<MenuBackdrop> menuBackdrop;
    std::unique_ptr<UiManager> uiManager;
    std::unique_ptr<SceneHost> scene;
    MainUIDlg* mainMenu = nullptr;
    GameUIDlg* gameUi = nullptr;
    AboutDlg* aboutDialog = nullptr;
    std::unique_ptr<StringManager> baseFont;
    std::unique_ptr<StringManager> secondaryFont;
    SimpleScript scriptEngine;
    std::u16string resourceArchive;
    std::u16string persistenceDirectory = u".";
    bool inGame = false;
    bool menuActive = false;
    int language = 2;
    GameConfiguration configuration;
    ControlMap controlMap;
    GameContent content;
    AuthorizationHost* authorizationHost = nullptr;
    GameplayHooks gameplayHooks;
    std::array<SaveSlotInfo, 3> saveSlots;
    std::array<char16_t, 9 * 64> uiSoundNames{};
    s3dIO persistenceIo;
};

} // namespace soft3d

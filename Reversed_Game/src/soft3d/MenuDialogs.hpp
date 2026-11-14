#pragma once

#include "soft3d/Dialog.hpp"

namespace soft3d {

class AudioManager;
class GameEngine;

class MenuDialog : public Dialog {
public:
    MenuDialog(GameEngine& engine, UiManager* manager, uint16_t index, bool active = false);

protected:
    GameEngine& engine;
};

class MainUIDlg : public MenuDialog {
public:
    enum Mode {
        Handedness = 0,
        PublisherLogo = 1,
        Disclaimer = 2,
        SoundPrompt = 3,
        Authorization = 4,
        MainMenu = 5,
        ExitConfirmation = 6,
        ExtraNotice = 7,
        NewGameOptions = 8,
        StudioLogo = 100,
    };

    MainUIDlg(GameEngine& engine, UiManager* manager);
    int Render(uint32_t context) override;
    int32_t GetSelection() const;
    void SetSelection(int32_t value);
    void ShowMain(int32_t value);
    int32_t GetMode() const;
    bool IsSoundEnabled() const;

private:
    int32_t mode = Handedness;
    int32_t selection = 0;
    int32_t soundSelection = 0;
    int32_t startupFrames = 0;
    int32_t weapon = 0;
    int32_t outfit = 0;
    bool soundEnabled = true;
};

class OptionDlg : public MenuDialog {
public:
    OptionDlg(GameEngine& engine, UiManager* manager);
    int Render(uint32_t context) override;
    int GetPage() const;
    int GetSelection() const;

private:
    void RenderMainPage();
    void RenderOrientationPage();

    int32_t selection = 0;
    int32_t page = 0;
    int32_t orientationChoice = 0;
};

int GetVolumeLevel(AudioManager& audio);
bool UpdateControlBindings(Dialog& dialog, GameEngine& engine, int32_t& selection);

class AboutDlg : public MenuDialog {
public:
    AboutDlg(GameEngine& engine, UiManager* manager);
    int Render(uint32_t context) override;
    void PrepareModel();
    int32_t GetPage() const;
    int32_t GetModelIndex() const;
    int32_t GetTextIndex() const;
    bool IsPhotoMode() const;

private:
    int32_t FindModel(int32_t start, int32_t direction) const;
    int32_t FindText(bool photos, int32_t start, int32_t direction) const;
    void SelectModel(int32_t start);
    void SelectText(int32_t start);
    void AdvanceText(int32_t step);
    void RenderText();
    void RenderPhoto();

    int32_t page = 0;
    int32_t modelIndex = -1;
    int32_t direction = 0;
    int32_t transition = 0;
    int32_t zoom = -8 * 0x10000;
    int32_t textIndex = -1;
    int32_t textLines = 0;
    int32_t scroll = 0;
    bool photoMode = false;
};

class HelpDlg : public MenuDialog {
public:
    HelpDlg(GameEngine& engine, UiManager* manager);
    void Reset();
    void ShowStatistics();
    int Render(uint32_t context) override;
    int HandleInput(int32_t type, int32_t key, int32_t context) override;

private:
    bool DrawCredits();
    void DrawStatistics();

    int32_t mode = 0;
    int32_t scroll = 0;
};

class PlayDlg : public MenuDialog {
public:
    PlayDlg(GameEngine& engine, UiManager* manager);
    int Render(uint32_t context) override;
    int HandleInput(int32_t type, int32_t key, int32_t context) override;
};

class LoadDlg : public MenuDialog {
public:
    LoadDlg(GameEngine& engine, UiManager* manager);
    int Render(uint32_t context) override;

private:
    int32_t DrawPlayTime(uint32_t value, int32_t x, int32_t y);

    int32_t selection = 0;
};

} // namespace soft3d

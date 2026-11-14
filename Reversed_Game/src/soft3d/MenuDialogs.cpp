#include "soft3d/MenuDialogs.hpp"

#include "soft3d/GameEngine.hpp"
#include "soft3d/MenuBackdrop.hpp"
#include "soft3d/TextureRes.hpp"

#include <algorithm>

namespace soft3d {
namespace {

bool Accept(const Input& input)
{
    return input.IsKeyClick(Key::Accept) || input.IsKeyClick(Key::Number6);
}

bool Back(const Input& input)
{
    return input.IsKeyClick(Key::SoftRight) || input.IsKeyClick(Key::Number0);
}

void Sound(GameEngine& engine, int index)
{
    engine.GetAudioManager().PlaySound(index, false);
}

void Wrap(int32_t& value, int32_t count)
{
    if (value < 0) {
        value = count - 1;
    } else if (value >= count) {
        value = 0;
    }
}

} // namespace

MenuDialog::MenuDialog(GameEngine& owner, UiManager* manager, uint16_t index, bool active)
    : Dialog(manager, index, active)
    , engine(owner)
{
}

MainUIDlg::MainUIDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 0, true)
{
}

int MainUIDlg::Render(uint32_t context)
{
    (void)context;
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    switch (mode) {
    case Handedness:
        SetStateTime(7, selection);
        if (controls.ActionClick(input, 3) || controls.ActionClick(input, 1)) {
            selection = 0;
        } else if (controls.ActionClick(input, 4) || controls.ActionClick(input, 2)) {
            selection = 1;
        }
        if (Accept(input)) {
            controls.SetOrientation(selection == 0 ? 1 : 2);
            SetStateTime(0, 0);
            RenderElementAtTime(0, 8, 0);
            RenderElementAtTime(0, 9, 0);
            startupFrames = 0;
            mode = PublisherLogo;
        }
        break;
    case PublisherLogo:
        ++startupFrames;
        if (AdvanceStateTime(9) || startupFrames > 12) {
            mode = StudioLogo;
        }
        break;
    case StudioLogo: {
        const bool completed = AdvanceStateTime(0);
        AdvanceElementTime(0, 8);
        AdvanceElementTime(0, 9);
        if (completed) {
            mode = Disclaimer;
        }
        break;
    }
    case Disclaimer: {
        const bool completed = AdvanceStateTime(1);
        AdvanceElementTime(1, 2);
        if (completed) {
            engine.RefreshSaveSlots();
            mode = SoundPrompt;
        }
        break;
    }
    case SoundPrompt:
        engine.GetScreenBlitter().Clear(255, 255, 255);
        if (controls.ActionClick(input, 3) || controls.ActionClick(input, 1)) {
            selection = 0;
            soundSelection = 1;
            soundEnabled = false;
            Sound(engine, 1);
        } else if (controls.ActionClick(input, 4) || controls.ActionClick(input, 2)) {
            selection = 1;
            soundSelection = 0;
            soundEnabled = true;
            Sound(engine, 1);
        }
        SetStateTime(3, soundSelection);
        if (Accept(input) || input.IsKeyClick(Key::SoftLeft) ||
            input.IsKeyClick(Key::SoftRight)) {
            engine.GetAudioManager().StopMusic();
            engine.GetAudioManager().StopAllSounds();
            engine.SetSoundEnabled(soundEnabled);
            selection = 0;
            mode = MainMenu;
            if (!engine.GetMenuBackdrop()->IsLoaded()) {
                engine.LoadMenuBackdrop();
            }
            engine.GetAudioManager().PlaySound(u"e_wind01", true);
        }
        break;
    case Authorization:
        mode = MainMenu;
        break;
    case MainMenu:
        SetStateTime(5, selection);
        if (controls.ActionClick(input, 4) || controls.ActionClick(input, 2)) {
            ++selection;
            Sound(engine, 1);
            Wrap(selection, 7);
        }
        if (controls.ActionClick(input, 3) || controls.ActionClick(input, 1)) {
            --selection;
            Sound(engine, 1);
            Wrap(selection, 7);
        }
        if (Accept(input)) {
            switch (selection) {
            case 0:
                engine.ActivateMenuDialog(5);
                engine.GetInput().ClearKey();
                Sound(engine, 3);
                break;
            case 1:
                engine.ActivateMenuDialog(1);
                engine.GetInput().ClearKey();
                Sound(engine, 3);
                break;
            case 2:
                engine.ActivateMenuDialog(2);
                engine.GetInput().ClearKey();
                Sound(engine, 3);
                break;
            case 3:
                engine.ActivateMenuDialog(3);
                engine.GetInput().ClearKey();
                engine.GetAudioManager().StopAllSounds();
                Sound(engine, 3);
                engine.GetAudioManager().PlaySound(u"7dayspiano03", true);
                break;
            case 4:
                mode = ExtraNotice;
                engine.GetInput().ClearKey();
                Sound(engine, 3);
                break;
            case 5:
                mode = ExitConfirmation;
                engine.GetInput().ClearKey();
                Sound(engine, 3);
                break;
            case 6:
                if (engine.GetConfiguration().unlocks.newGameOptions == 0) {
                    SetActive(false);
                    engine.StartNewGame();
                } else {
                    mode = NewGameOptions;
                    selection = 0;
                }
                Sound(engine, 3);
                break;
            }
        }
        break;
    case ExitConfirmation:
        SetStateTime(6, 0);
        if (Accept(input)) {
            Sound(engine, 3);
            engine.RequestExit();
        }
        if (Back(input)) {
            mode = MainMenu;
            Sound(engine, 4);
        }
        break;
    case ExtraNotice:
        SetStateTime(1, 0);
        RenderElementAtTime(1, 3, 0);
        if (Accept(input) || Back(input)) {
            mode = MainMenu;
            Sound(engine, 4);
        }
        break;
    case NewGameOptions: {
        const GameUnlocks& unlocks = engine.GetConfiguration().unlocks;
        SetStateTime(8, 1);
        if (unlocks.extraWeapon0 != 0) {
            RenderElementAtTime(8, 15, 1);
        }
        if (unlocks.extraWeapon1 != 0) {
            RenderElementAtTime(8, 17, 1);
        }
        RenderElementAtTime(8, selection * 2 + 2, 0);
        if (weapon == 0) {
            RenderElementAtTime(8, 14, 0);
        } else if (weapon == 1) {
            if (unlocks.extraWeapon0 != 0) {
                RenderElementAtTime(8, 16, 0);
            }
        } else {
            weapon = 2;
            if (unlocks.extraWeapon1 != 0) {
                RenderElementAtTime(8, 18, 0);
            }
        }
        if (unlocks.extraWeapon0 == 0) {
            RenderElementAtTime(8, 19, 0);
        }
        if (unlocks.extraWeapon1 == 0) {
            RenderElementAtTime(8, 19, 1);
        }
        RenderElementAtTime(8, outfit * 2 + 10, 0);
        if (controls.ActionClick(input, 2)) {
            ++selection;
            Sound(engine, 1);
            Wrap(selection, 4);
        }
        if (controls.ActionClick(input, 1)) {
            --selection;
            Sound(engine, 1);
            Wrap(selection, 4);
        }
        if (selection == 0) {
            if (controls.ActionClick(input, 4)) {
                ++weapon;
                Sound(engine, 1);
                if (weapon == 1 && unlocks.extraWeapon0 == 0) ++weapon;
                if (weapon == 2 && unlocks.extraWeapon1 == 0) ++weapon;
                if (weapon > 2) weapon = 0;
            }
            if (controls.ActionClick(input, 3)) {
                --weapon;
                Sound(engine, 1);
                if (weapon == 2 && unlocks.extraWeapon1 == 0) --weapon;
                if (weapon == 1 && unlocks.extraWeapon0 == 0) --weapon;
                if (weapon < 0) weapon = 2;
                if (unlocks.extraWeapon0 == 0 && unlocks.extraWeapon1 == 0) weapon = 0;
            }
        } else if (selection == 1) {
            if (controls.ActionClick(input, 4)) {
                outfit = (outfit + 1) % 2;
                Sound(engine, 1);
            }
            if (controls.ActionClick(input, 3)) {
                outfit = (outfit + 1) % 2;
                Sound(engine, 1);
            }
        }
        if (Accept(input)) {
            if (selection == 2) {
                SetActive(false);
                engine.StartNewGame({weapon, outfit});
                engine.GetAudioManager().StopAllSounds();
                Sound(engine, 3);
            } else if (selection == 3) {
                mode = MainMenu;
                selection = 6;
                Sound(engine, 4);
            }
        }
        if (Back(input)) {
            mode = MainMenu;
            selection = 6;
            Sound(engine, 4);
        }
        break;
    }
    }
    return 1;
}

int32_t MainUIDlg::GetSelection() const
{
    return selection;
}

void MainUIDlg::SetSelection(int32_t value)
{
    selection = value;
}

void MainUIDlg::ShowMain(int32_t value)
{
    selection = value;
    mode = MainMenu;
}

int32_t MainUIDlg::GetMode() const
{
    return mode;
}

bool MainUIDlg::IsSoundEnabled() const
{
    return soundEnabled;
}

OptionDlg::OptionDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 1)
{
}

int OptionDlg::Render(uint32_t context)
{
    (void)context;
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    switch (page) {
    case 0:
        RenderMainPage();
        break;
    case 1:
        if (UpdateControlBindings(*this, engine, selection)) {
            page = 0;
            selection = 0;
            Sound(engine, 4);
        }
        break;
    case 2:
        RenderOrientationPage();
        break;
    case 3:
        SetStateTime(2, 2);
        if (Back(input)) {
            page = 2;
            Sound(engine, 4);
        }
        if (Accept(input)) {
            controls.SetOrientation(orientationChoice == 0 ? 2 : 1);
            page = 0;
            Sound(engine, 3);
        }
        break;
    }
    return 1;
}

void OptionDlg::RenderMainPage()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    int32_t volume = GetVolumeLevel(engine.GetAudioManager());
    SetStateTime(page, 0);
    RenderElementAtTime(page, 6, selection);
    RenderElementAtTime(page, 7, volume);
    RenderElementAtTime(page, 10, selection);
    RenderElementAtTime(page, 17, selection);
    RenderElementAtTime(page, engine.GetConfiguration().vibrationEnabled != 0 ? 11 : 12,
                        engine.GetConfiguration().vibrationEnabled != 0 ? 0 : 1);
    if (controls.ActionClick(input, 1)) {
        --selection;
        if (selection < 0) selection = 3;
        Sound(engine, 1);
    }
    if (controls.ActionClick(input, 2)) {
        ++selection;
        if (selection > 3) selection = 0;
        Sound(engine, 1);
    }
    if (selection == 0) {
        if (controls.ActionClick(input, 3)) {
            volume = std::max(0, volume - 1);
            engine.GetAudioManager().SetGlobalVolume(1 << volume);
            Sound(engine, 1);
        }
        if (controls.ActionClick(input, 4)) {
            volume = std::min(8, volume + 1);
            engine.GetAudioManager().SetGlobalVolume(1 << volume);
            Sound(engine, 1);
        }
    } else if (selection == 2 &&
               (controls.ActionClick(input, 3) || controls.ActionClick(input, 4))) {
        engine.SetVibrationEnabled(engine.GetConfiguration().vibrationEnabled == 0);
        Sound(engine, 1);
    }
    if (Back(input)) {
        engine.ShowMainMenu(1);
        Sound(engine, 4);
        page = 0;
    }
    if (Accept(input) && selection != 0) {
        if (selection == 1) {
            page = 1;
            selection = 0;
            Sound(engine, 3);
        } else if (selection == 3) {
            page = 2;
            selection = 0;
            Sound(engine, 3);
        }
    }
}

void OptionDlg::RenderOrientationPage()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(2, orientationChoice == 0 ? 1 : 0);
    if (controls.ActionClick(input, 4) && orientationChoice != 0) {
        orientationChoice = 0;
        Sound(engine, 1);
    }
    if (controls.ActionClick(input, 3) && orientationChoice == 0) {
        orientationChoice = 1;
        Sound(engine, 1);
    }
    if (Accept(input)) {
        page = 3;
        Sound(engine, 3);
    }
    if (Back(input)) {
        page = 0;
        Sound(engine, 4);
    }
}

int OptionDlg::GetPage() const
{
    return page;
}

int OptionDlg::GetSelection() const
{
    return selection;
}

int GetVolumeLevel(AudioManager& audio)
{
    const int volume = audio.GetDevice().GetMasterVolume();
    int level = 8;
    while (level >= 0 && (((volume + 1) >> level) & 1) == 0) {
        --level;
    }
    return level;
}

bool UpdateControlBindings(Dialog& dialog, GameEngine& engine, int32_t& selection)
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    dialog.SetStateTime(1, 0);
    dialog.RenderElementAtTime(1, 7, selection);
    dialog.SetStateTime(1, 1);
    dialog.RenderElementAtTime(1, 16, selection);
    dialog.RenderElementAtTime(1, 17, selection);
    for (int32_t action = 1; action <= 7; ++action) {
        uint32_t binding = controls.GetBinding(action);
        dialog.RenderElementAtTime(1, action + 17, binding == 0 ? 12 : binding - 1);
    }
    if (controls.ActionClick(input, 3)) {
        int candidate = static_cast<int>(controls.GetBinding(selection + 1));
        for (int count = 0; count < 13; ++count) {
            candidate = candidate == 0 ? 12 : candidate - 1;
            bool used = false;
            for (int action = 1; action <= 7; ++action) {
                if (controls.GetBinding(action) == static_cast<uint32_t>(candidate) && candidate != 0) {
                    used = true;
                }
            }
            if (!used) break;
        }
        controls.SetBinding(selection + 1, candidate);
        Sound(engine, 1);
    }
    if (controls.ActionClick(input, 4)) {
        int candidate = static_cast<int>(controls.GetBinding(selection + 1));
        for (int count = 0; count < 13; ++count) {
            candidate = (candidate + 1) % 13;
            bool used = false;
            for (int action = 1; action <= 7; ++action) {
                if (controls.GetBinding(action) == static_cast<uint32_t>(candidate) && candidate != 0) {
                    used = true;
                }
            }
            if (!used) break;
        }
        controls.SetBinding(selection + 1, candidate);
        Sound(engine, 1);
    }
    if (controls.ActionClick(input, 1)) {
        --selection;
        if (selection < 0) selection = 6;
        Sound(engine, 1);
    }
    if (controls.ActionClick(input, 2)) {
        ++selection;
        if (selection > 6) selection = 0;
        Sound(engine, 1);
    }
    return controls.ActionClick(input, 7, false) || Back(input);
}

AboutDlg::AboutDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 2)
{
}

int32_t AboutDlg::FindModel(int32_t start, int32_t step) const
{
    int32_t candidate = start;
    for (int count = 0; count < 99; ++count) {
        if (candidate < 1) candidate = 99;
        if (candidate > 99) candidate = 1;
        if (engine.GetConfiguration().unlocks.models[candidate] != 0) {
            return candidate;
        }
        candidate += step;
    }
    return -1;
}

int32_t AboutDlg::FindText(bool photos, int32_t start, int32_t step) const
{
    const auto& unlocked = photos ? engine.GetConfiguration().unlocks.photos
                                  : engine.GetConfiguration().unlocks.documents;
    int32_t candidate = start;
    for (int count = 0; count < 99; ++count) {
        if (candidate < 1) candidate = 99;
        if (candidate > 99) candidate = 1;
        if (unlocked[candidate] != 0) return candidate;
        candidate += step;
    }
    return -1;
}

void AboutDlg::SelectModel(int32_t start)
{
    modelIndex = FindModel(start, 1);
    transition = 0;
    zoom = -8 * 0x10000;
}

void AboutDlg::SelectText(int32_t start)
{
    photoMode = false;
    textIndex = FindText(false, start, 1);
    if (textIndex < 0) {
        photoMode = true;
        textIndex = FindText(true, start, 1);
    }
    scroll = 0;
    if (!photoMode && textIndex >= 0) {
        const GameTextRecord& record = engine.GetContent().documents[textIndex];
        textLines = static_cast<int32_t>(engine.GetSecondaryFont()->CountTextLines(
            engine.GetSecondaryFont()->GetString(record.bodyString), 0, engine.GetWidth() - 30));
    }
}

void AboutDlg::AdvanceText(int32_t step)
{
    const int32_t originalIndex = textIndex;
    const bool originalMode = photoMode;
    for (int count = 0; count < 198; ++count) {
        textIndex += step;
        if (textIndex < 1 || textIndex > 99) {
            photoMode = !photoMode;
            textIndex = step > 0 ? 1 : 99;
        }
        if (FindText(photoMode, textIndex, 0) == textIndex) {
            scroll = 0;
            if (!photoMode) {
                const GameTextRecord& record = engine.GetContent().documents[textIndex];
                textLines = static_cast<int32_t>(engine.GetSecondaryFont()->CountTextLines(
                    engine.GetSecondaryFont()->GetString(record.bodyString), 0,
                    engine.GetWidth() - 30));
            }
            Sound(engine, 7);
            return;
        }
    }
    textIndex = originalIndex;
    photoMode = originalMode;
}

int AboutDlg::Render(uint32_t context)
{
    (void)context;
    SetStateTime(0, page);
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    switch (page) {
    case 0:
        if (Accept(input)) {
            page = 1;
            SelectModel(1);
            Sound(engine, 3);
        } else if (Back(input)) {
            engine.ShowMainMenu(2);
            page = 0;
            Sound(engine, 4);
        }
        if (controls.ActionClick(input, 4)) {
            page = 2;
            Sound(engine, 1);
        }
        break;
    case 1:
        if (transition == 0) {
            if (Back(input)) {
                page = 0;
                Sound(engine, 4);
            }
            if (modelIndex >= 0) {
                if (controls.ActionClick(input, 3)) {
                    direction = -1;
                    transition = 16;
                    Sound(engine, 1);
                } else if (controls.ActionClick(input, 4)) {
                    direction = 1;
                    transition = 16;
                    Sound(engine, 1);
                }
                if (controls.ActionDown(input, 1)) {
                    zoom = std::min(zoom + 0x6666, -4 * 0x10000);
                }
                if (controls.ActionDown(input, 2)) {
                    zoom = std::max(zoom - 0x6666, -12 * 0x10000);
                }
                if (modelIndex >= 0) {
                    engine.GetSecondaryFont()->DrawString(
                        engine.GetContent().models[modelIndex].descriptionString, 0xffff,
                        {10, engine.GetHeight() - 42, engine.GetWidth(), engine.GetHeight()});
                }
            }
        } else {
            --transition;
            if (transition == 8) {
                modelIndex = FindModel(modelIndex + direction, direction);
            }
        }
        break;
    case 2:
        if (Accept(input)) {
            page = 3;
            SelectText(1);
            Sound(engine, 7);
        } else if (Back(input)) {
            engine.ShowMainMenu(2);
            page = 0;
            Sound(engine, 4);
        }
        if (controls.ActionClick(input, 3)) {
            page = 0;
            Sound(engine, 1);
        }
        break;
    case 3:
        if (Back(input)) {
            page = 2;
            Sound(engine, 4);
        }
        if (textIndex >= 0) {
            if (controls.ActionDown(input, 1)) {
                scroll = std::max(0, scroll - 1);
            }
            if (controls.ActionDown(input, 2) && !photoMode) {
                const int32_t linesPerPage =
                    (engine.GetHeight() - 75) / engine.GetSecondaryFont()->GetTextHeight() + 1;
                scroll = std::min(scroll + 1, std::max(0, textLines - linesPerPage));
            }
            if (controls.ActionClick(input, 4)) {
                AdvanceText(1);
            }
            if (controls.ActionClick(input, 3)) {
                AdvanceText(-1);
            }
            if (photoMode) {
                RenderPhoto();
            } else {
                RenderText();
            }
        }
        break;
    }
    return 1;
}

void AboutDlg::RenderText()
{
    RenderElementAtTime(0, 13, 3);
    RenderElementAtTime(0, 14, 3);
    StringManager* font = engine.GetSecondaryFont();
    const GameTextRecord& record = engine.GetContent().documents[textIndex];
    font->DrawString(record.titleString, 0xffff, 20, Point{engine.GetWidth() / 2, 32});
    font->DrawStringRange(record.bodyString, 0xffff, scroll,
                          scroll + (engine.GetHeight() - 75) / font->GetTextHeight() + 1,
                          {18, 50, engine.GetWidth() - 12, engine.GetHeight() - 25});
}

void AboutDlg::RenderPhoto()
{
    char16_t name[64];
    if (textIndex < 10) {
        WideSprintf(name, u"photo_200%d", textIndex);
    } else {
        WideSprintf(name, u"photo_20%d", textIndex);
    }
    auto* resource = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
        ResourceTexture, 3, name, engine.GetLanguage() == 2 ? u".\\ui\\" : u".\\uien\\"));
    if (resource != nullptr && resource->GetTexture() != nullptr) {
        const Texture* texture = resource->GetTexture();
        engine.GetScreenBlitter().BlitUnscaled(
            texture, (engine.GetWidth() - static_cast<int32_t>(texture->GetWidth())) / 2,
            (engine.GetHeight() - static_cast<int32_t>(texture->GetHeight())) / 2,
            {0, 0, static_cast<int32_t>(texture->GetWidth()),
             static_cast<int32_t>(texture->GetHeight())});
    }
    const GameTextRecord& record = engine.GetContent().photos[textIndex];
    engine.GetSecondaryFont()->DrawString(record.titleString, 0xffff, 20,
                                          Point{engine.GetWidth() / 2, 32});
}

void AboutDlg::PrepareModel()
{
    if (IsActive() && page == 1 && modelIndex >= 0 &&
        engine.GetGameplayHooks().renderAboutModel) {
        engine.GetGameplayHooks().renderAboutModel(modelIndex, direction, transition, zoom);
    }
}

int32_t AboutDlg::GetPage() const
{
    return page;
}

int32_t AboutDlg::GetModelIndex() const
{
    return modelIndex;
}

int32_t AboutDlg::GetTextIndex() const
{
    return textIndex;
}

bool AboutDlg::IsPhotoMode() const
{
    return photoMode;
}

HelpDlg::HelpDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 3)
{
    Reset();
}

void HelpDlg::Reset()
{
    mode = 0;
    scroll = 0;
    SetTime(0);
}

void HelpDlg::ShowStatistics()
{
    mode = 2;
    scroll = 0;
}

int HelpDlg::Render(uint32_t context)
{
    (void)context;
    if (mode == 0) {
        DrawCredits();
    } else if (mode == 1) {
        if (DrawCredits()) {
            mode = 2;
        }
    } else {
        DrawStatistics();
    }
    return 1;
}

bool HelpDlg::DrawCredits()
{
    RenderElementAtTime(0, 0, 0);
    const ScreenRect key = GetKeyDestinationRect(0, 1, 0);
    StringManager* font = engine.GetSecondaryFont();
    for (int32_t line = 0; line < 140; ++line) {
        font->DrawString(450 + line, 0xffff, 4,
                         {key.left, line * 15 + key.top + scroll + 15,
                          key.left + 200, key.bottom + scroll + 15});
    }
    if (scroll < -2300 && AdvanceStateTime(0)) {
        scroll = 0;
        return true;
    }
    scroll -= 2;
    return false;
}

void HelpDlg::DrawStatistics()
{
    SetStateTime(2, 0);
    AdvanceElementTime(2, 2);
    const HelpStatistics& stats = engine.GetContent().statistics;
    const int32_t x = GetKeyX(2, 4, 0);
    RenderNumber(stats.first, x, GetKeyY(2, 4, 0), 2, 4);
    RenderNumber(stats.second, x, GetKeyY(2, 4, 1), 2, 4);
    RenderNumber(stats.fourth, x, GetKeyY(2, 4, 2), 2, 4);
    RenderNumber(stats.filesFound, x, GetKeyY(2, 4, 3), 2, 4);
    RenderNumber(stats.secretsFound, x, GetKeyY(2, 4, 4), 2, 4);
    const int32_t hours = stats.elapsedSeconds / 3600;
    const int32_t minutes = (stats.elapsedSeconds - hours * 3600) / 60;
    const int32_t seconds = stats.elapsedSeconds - hours * 3600 - minutes * 60;
    if (hours < 1) {
        RenderElementAtTime(2, 5, 0);
        RenderElementAtTime(2, 6, 0);
    } else {
        RenderNumber(hours, GetKeyX(2, 4, 7), GetKeyY(2, 4, 7), 2, 4);
    }
    if (minutes < 1) {
        RenderElementAtTime(2, 5, 1);
        RenderElementAtTime(2, 6, 1);
    } else {
        RenderNumber(minutes, GetKeyX(2, 4, 6), GetKeyY(2, 4, 6), 2, 4);
    }
    RenderNumber(seconds, GetKeyX(2, 4, 5), GetKeyY(2, 4, 5), 2, 4);
    RenderElementAtTime(2, 5, 5);
    RenderElementAtTime(2, 5, 6);
    const int32_t overtime = stats.elapsedSeconds > 18000
        ? (stats.elapsedSeconds - 18000) / 3600 : 0;
    const int32_t score = static_cast<int32_t>(stats.filesFound + stats.secretsFound) -
        static_cast<int32_t>(stats.first / 2 + stats.second / 2 + stats.fourth) - overtime + 60;
    RenderElementAtTime(2, 3, score >= 90 ? 0 : score >= 80 ? 1 : score >= 70 ? 2 : 3);
}

int HelpDlg::HandleInput(int32_t type, int32_t key, int32_t context)
{
    (void)context;
    if ((type != 1 && type != 8) ||
        (key != 18 && key != 11 && key != 19 && key != 8 &&
         key != 15 && key != 16 && key != 13 && key != 14)) {
        return Dialog::HandleInput(type, key, context);
    }
    if (mode == 0 || mode == 2) {
        engine.GetAudioManager().StopMusic();
        engine.GetAudioManager().StopAllSounds();
        engine.GetAudioManager().PlaySound(u"e_wind01", true);
        engine.ShowMainMenu(mode == 0 ? 3 : 6);
        Sound(engine, 4);
        engine.GetInput().ClearKey();
    } else {
        mode = 2;
    }
    return 1;
}

PlayDlg::PlayDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 4)
{
}

int PlayDlg::Render(uint32_t context)
{
    (void)context;
    if (AdvanceStateTime(0)) {
        SetActive(false);
    }
    return 1;
}

int PlayDlg::HandleInput(int32_t type, int32_t key, int32_t context)
{
    if ((type == 1 || type == 8) && (key == 8 || key == 19)) {
        SetActive(false);
        engine.StartNewGame();
        return 1;
    }
    return Dialog::HandleInput(type, key, context);
}

LoadDlg::LoadDlg(GameEngine& owner, UiManager* manager)
    : MenuDialog(owner, manager, 5)
{
}

int LoadDlg::Render(uint32_t context)
{
    (void)context;
    SetStateTime(0, selection);
    const auto& slots = engine.GetSaveSlots();
    for (int32_t slot = 0; slot < 3; ++slot) {
        if (!slots[slot].present) {
            RenderElementAtTime(0, 12, slot);
        } else {
            RenderElementAtTime(0, slots[slot].GetDay() + 42, slot);
            RenderElementAtTime(0, 50, slot);
            DrawPlayTime(slots[slot].GetPlayTime(), GetKeyX(0, 12, slot), GetKeyY(0, 12, slot));
        }
    }
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    if (Back(input)) {
        engine.ShowMainMenu(0);
        Sound(engine, 4);
    } else {
        if (controls.ActionClick(input, 1)) {
            --selection;
            Wrap(selection, 3);
            Sound(engine, 1);
        }
        if (controls.ActionClick(input, 2)) {
            ++selection;
            Wrap(selection, 3);
            Sound(engine, 1);
        }
        if (Accept(input)) {
            if (!engine.LoadSaveSlot(selection + 1)) {
                Sound(engine, 6);
            } else {
                SetActive(false);
            }
        }
    }
    return 1;
}

int32_t LoadDlg::DrawPlayTime(uint32_t value, int32_t x, int32_t y)
{
    if (value > 10000) {
        return 0;
    }
    char16_t text[10];
    WideSprintf(text, u"%d", static_cast<int32_t>(value));
    auto* resource = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
        ResourceTexture, 1, u"shuzi_1", engine.GetLanguage() == 2 ? u".\\ui\\" : u".\\uien\\"));
    if (resource == nullptr || resource->GetTexture() == nullptr) {
        return 0;
    }
    int32_t length = 0;
    while (text[length] != u'\0') ++length;
    for (int32_t index = 0; index < length; ++index) {
        const int32_t digit = text[index] - u'0';
        engine.GetScreenBlitter().BlitUnscaled(resource->GetTexture(), x, y,
                                               {digit * 9, 0, digit * 9 + 9, 8});
        x += 9;
    }
    return length * 9;
}

} // namespace soft3d

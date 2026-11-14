#include "soft3d/GameUIDlg.hpp"

#include "soft3d/GameEngine.hpp"
#include "soft3d/MenuDialogs.hpp"
#include "soft3d/TextureRes.hpp"

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
    if (value < 0) value = count - 1;
    if (value >= count) value = 0;
}

} // namespace

GameUIDlg::GameUIDlg(GameEngine& owner, UiManager* manager)
    : Dialog(manager, 6)
    , engine(owner)
{
}

void GameUIDlg::Initialize()
{
    LoadResources();
    mode = -1;
    selection = 0;
    returnSelection = 0;
    optionSelection = 0;
    itemAction = 0;
    combineSource = -1;
    documentIndex = 0;
    scroll = 0;
    errorTimer = 0;
    menuOpen = false;
    viewingDocument = false;
}

void GameUIDlg::LoadResources()
{
    static constexpr const char16_t* names[] = {
        u"tishi_huoqu", u"tishi_kaimen", u"tishi_jilu", u"tishi_hudong",
        u"tishi_duobi", u"tishi_duihua", u"zhishitiao", u"zhishi_04", u"zhishi_05",
        u"qte_over_1", u"qte_over_2", u"qte_over_3", u"qte_over_4",
        u"qte_zhishi01", u"qte_zhishi02", u"_1f", u"_2f", u"_3f",
        u"juqing_01", u"juqing_02", u"juqing_03", u"here", u"here_alp",
        u"shuzi_1", u"touxiang_02", u"qte_tishi_01", u"qte_tishi_02",
        u"door01", u"door02", u"door_id",
    };
    textures.clear();
    textures.reserve(47);
    const char16_t* directory = engine.GetLanguage() == 2 ? u".\\ui\\" : u".\\uien\\";
    for (const char16_t* name : names) {
        textures.push_back(static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 1, name, directory)));
    }
    for (int index = 1; index <= 4; ++index) {
        char16_t name[32];
        WideSprintf(name, u"qte_a_%d", index);
        textures.push_back(static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 1, name, directory)));
        WideSprintf(name, u"qte_b_%d", index);
        textures.push_back(static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 1, name, directory)));
        WideSprintf(name, u"qte_b_%d_alp", index);
        textures.push_back(static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 1, name, directory)));
    }
}

void GameUIDlg::OpenMenu()
{
    menuOpen = true;
    mode = 0;
    selection = 0;
    SetActive(true);
}

void GameUIDlg::CloseMenu()
{
    mode = 11;
}

int GameUIDlg::Render(uint32_t context)
{
    (void)context;
    if (!menuOpen) {
        if (engine.GetGameplayHooks().renderGameHud) {
            engine.GetGameplayHooks().renderGameHud();
        }
        return 1;
    }
    switch (mode) {
    case 0:
        if (AdvanceStateTimeToward(0, 0, 10)) {
            mode = 1;
        }
        break;
    case 1:
        RenderMainMenu();
        break;
    case 2:
        RenderInventory(false);
        break;
    case 3:
        if (engine.GetGameplayHooks().renderMap) {
            engine.GetGameplayHooks().renderMap(engine.GetContent().mapId);
        }
        if (Back(engine.GetInput())) {
            CloseMenu();
            Sound(engine, 4);
        }
        break;
    case 4:
        RenderFiles();
        break;
    case 5:
        RenderOptions();
        break;
    case 6:
        RenderItemDescription();
        break;
    case 7:
        if (UpdateControlBindings(*this, engine, selection)) {
            mode = 5;
            selection = optionSelection;
            Sound(engine, 4);
        }
        break;
    case 8:
        SetStateTime(4, 0);
        RenderElementAtTime(4, 5, 0);
        if (Back(engine.GetInput())) {
            mode = 5;
            selection = optionSelection;
            Sound(engine, 4);
        } else if (Accept(engine.GetInput())) {
            engine.ReturnToMainMenu();
            menuOpen = false;
            mode = -1;
            engine.GetAudioManager().PlaySound(u"e_wind01", true);
        }
        break;
    case 9:
        SetStateTime(4, 0);
        RenderElementAtTime(4, 6, 0);
        if (Back(engine.GetInput())) {
            mode = 5;
            selection = optionSelection;
            Sound(engine, 4);
        } else if (Accept(engine.GetInput())) {
            engine.RequestExit();
        }
        break;
    case 10:
        RenderItemActions();
        break;
    case 11:
        if (AdvanceStateTimeToward(0, 10, 0)) {
            menuOpen = false;
            mode = -1;
        }
        break;
    case 12:
        engine.SaveGameState();
        engine.ReturnToMainMenu();
        menuOpen = false;
        mode = -1;
        break;
    case 14:
        RenderInventory(true);
        break;
    }
    if (errorTimer > 0) {
        --errorTimer;
    }
    return 1;
}

void GameUIDlg::RenderMainMenu()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(0, 11);
    RenderElementAtTime(0, 11, selection);
    if (controls.ActionClick(input, 1, false)) {
        --selection;
        Wrap(selection, 4);
        Sound(engine, 1);
    } else if (controls.ActionClick(input, 2, false)) {
        ++selection;
        Wrap(selection, 4);
        Sound(engine, 1);
    } else if (Accept(input)) {
        returnSelection = selection;
        switch (selection) {
        case 0:
            mode = 2;
            selection = 0;
            Sound(engine, 3);
            break;
        case 1:
            mode = 3;
            Sound(engine, 7);
            break;
        case 2:
            mode = 4;
            selection = 0;
            viewingDocument = false;
            Sound(engine, 7);
            break;
        case 3:
            mode = 5;
            selection = 0;
            Sound(engine, 3);
            break;
        }
        selection = 0;
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        CloseMenu();
        selection = 0;
        Sound(engine, 4);
    }
}

void GameUIDlg::RenderInventory(bool combining)
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(0, combining ? 12 : 11);
    RenderElementAtTime(0, combining ? 11 : 13, combining ? itemAction : 12);
    RenderElementAtTime(0, 14, selection);
    DrawItemDescription(selection);
    if (controls.ActionClick(input, 1, false)) {
        selection -= 3;
        if (selection < 0) selection += 9;
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 2, false)) {
        selection += 3;
        if (selection > 8) selection -= 9;
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 3, false)) {
        if (selection % 3 == 0) {
            selection += 2;
        } else {
            --selection;
        }
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 4, false)) {
        if (selection % 3 == 2) {
            selection -= 2;
        } else {
            ++selection;
        }
        Sound(engine, 2);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = combining ? 10 : 1;
        if (!combining) selection = returnSelection;
        Sound(engine, 4);
    } else if (Accept(input)) {
        const int item = engine.GetContent().inventory[selection];
        if (item <= 0) {
            errorTimer = combining ? 48 : 10;
            Sound(engine, 6);
            return;
        }
        if (!combining) {
            mode = 10;
            itemAction = 0;
            Sound(engine, 3);
            return;
        }
        bool combined = false;
        if (engine.GetGameplayHooks().combineItems) {
            combined = engine.GetGameplayHooks().combineItems(
                engine.GetContent().inventory[combineSource], item);
        }
        if (combined) {
            mode = 6;
            Sound(engine, 3);
        } else {
            errorTimer = 48;
            Sound(engine, 6);
        }
    }
}

void GameUIDlg::RenderItemActions()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(0, 12);
    RenderElementAtTime(0, 11, itemAction);
    RenderElementAtTime(0, 13, 12);
    DrawItemDescription(selection);
    if (controls.ActionClick(input, 1, false)) {
        --itemAction;
        Wrap(itemAction, 3);
        Sound(engine, 1);
    } else if (controls.ActionClick(input, 2, false)) {
        ++itemAction;
        Wrap(itemAction, 3);
        Sound(engine, 1);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = 2;
        Sound(engine, 4);
    } else if (Accept(input)) {
        if (itemAction == 0) {
            int result = 1;
            if (engine.GetGameplayHooks().useItem) {
                result = engine.GetGameplayHooks().useItem(
                    engine.GetContent().inventory[selection], 0);
            }
            if (result == 0) {
                errorTimer = 10;
                mode = 2;
                Sound(engine, 6);
            } else if (result == 1) {
                mode = 2;
                Sound(engine, 3);
            } else {
                CloseMenu();
                Sound(engine, 3);
            }
        } else if (itemAction == 1) {
            combineSource = selection;
            itemAction = 1;
            mode = 14;
            Sound(engine, 3);
        } else {
            mode = 6;
            Sound(engine, 3);
        }
    }
}

void GameUIDlg::RenderItemDescription()
{
    SetStateTime(0, 12);
    RenderElementAtTime(0, 13, 12);
    DrawItemDescription(selection);
    if (Back(engine.GetInput()) || engine.GetControlMap().ActionClick(engine.GetInput(), 7, false)) {
        mode = 2;
        Sound(engine, 4);
    }
}

void GameUIDlg::DrawItemDescription(int slot)
{
    if (slot < 0 || slot >= static_cast<int>(engine.GetContent().inventory.size()) ||
        errorTimer != 0) {
        return;
    }
    const int item = engine.GetContent().inventory[slot];
    if (item <= 0 || item >= static_cast<int>(engine.GetContent().items.size())) {
        return;
    }
    const GameItemRecord& record = engine.GetContent().items[item];
    engine.GetSecondaryFont()->DrawString(
        record.descriptionString, 0xffff,
        {GetKeyX(0, 16, 0), GetKeyY(0, 16, 0), engine.GetWidth() - 4, engine.GetHeight()});
}

void GameUIDlg::RenderFiles()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(3, 0);
    const auto& notes = engine.GetContent().noteIds;
    if (viewingDocument) {
        const int id = notes[documentIndex] - 1000;
        if (id >= 0 && id < static_cast<int>(engine.GetContent().documents.size())) {
            const GameTextRecord& record = engine.GetContent().documents[id];
            engine.GetSecondaryFont()->DrawString(record.titleString, 0xffff, 20,
                                                  Point{engine.GetWidth() / 2, 20});
            engine.GetSecondaryFont()->DrawStringRange(
                record.bodyString, 0xffff, scroll,
                scroll + (engine.GetHeight() - 92) / engine.GetSecondaryFont()->GetTextHeight(),
                {12, 48, engine.GetWidth() - 12, engine.GetHeight() - 20});
        }
        if (controls.ActionDown(input, 1, false)) {
            if (scroll > 0) --scroll;
        }
        if (controls.ActionDown(input, 2, false)) {
            ++scroll;
        }
        if (Back(input) || controls.ActionClick(input, 7, false) ||
            controls.ActionClick(input, 5, false)) {
            viewingDocument = false;
            Sound(engine, 4);
        }
        return;
    }
    RenderElementAtTime(0, 12, documentIndex);
    if (controls.ActionClick(input, 1, false) || controls.ActionClick(input, 3, false)) {
        const int original = documentIndex;
        do {
            --documentIndex;
            if (documentIndex < 0) documentIndex = 127;
        } while (notes[documentIndex] <= 0 && documentIndex != original);
        scroll = 0;
        Sound(engine, 1);
    } else if (controls.ActionClick(input, 2, false) || controls.ActionClick(input, 4, false)) {
        const int original = documentIndex;
        do {
            ++documentIndex;
            if (documentIndex > 127) documentIndex = 0;
        } while (notes[documentIndex] <= 0 && documentIndex != original);
        scroll = 0;
        Sound(engine, 1);
    } else if (Accept(input) && notes[documentIndex] > 0) {
        viewingDocument = true;
        scroll = 0;
        Sound(engine, 3);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = 1;
        selection = returnSelection;
        Sound(engine, 4);
    }
}

void GameUIDlg::RenderOptions()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    int volume = GetVolumeLevel(engine.GetAudioManager());
    SetStateTime(6, 0);
    RenderElementAtTime(6, 4, selection);
    RenderElementAtTime(6, 7, volume);
    RenderElementAtTime(6, 8, selection);
    RenderElementAtTime(6, engine.GetConfiguration().vibrationEnabled != 0 ? 9 : 10,
                        engine.GetConfiguration().vibrationEnabled != 0 ? 0 : 1);
    if (controls.ActionClick(input, 1, false)) {
        --selection;
        if (selection == 1) selection = 0;
        Wrap(selection, 5);
        Sound(engine, 1);
    } else if (controls.ActionClick(input, 2, false)) {
        ++selection;
        if (selection == 1) selection = 2;
        Wrap(selection, 5);
        Sound(engine, 1);
    } else if (selection == 0 && controls.ActionClick(input, 3, false)) {
        volume = volume > 0 ? volume - 1 : 0;
        engine.GetAudioManager().SetGlobalVolume(1 << volume);
        Sound(engine, 1);
    } else if (selection == 0 && controls.ActionClick(input, 4, false)) {
        volume = volume < 8 ? volume + 1 : 8;
        engine.GetAudioManager().SetGlobalVolume(1 << volume);
        Sound(engine, 1);
    } else if (selection == 4 &&
               (controls.ActionClick(input, 3, false) || controls.ActionClick(input, 4, false))) {
        engine.SetVibrationEnabled(engine.GetConfiguration().vibrationEnabled == 0);
        Sound(engine, 3);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = 1;
        selection = returnSelection;
        Sound(engine, 4);
    } else if (Accept(input)) {
        optionSelection = selection;
        if (selection == 1) {
            mode = 7;
            selection = 0;
        } else if (selection == 2) {
            mode = 8;
        } else if (selection == 3) {
            mode = 9;
        }
        Sound(engine, 3);
    }
}

int GameUIDlg::GetMode() const
{
    return mode;
}

int GameUIDlg::GetSelection() const
{
    return selection;
}

bool GameUIDlg::IsMenuOpen() const
{
    return menuOpen;
}

} // namespace soft3d

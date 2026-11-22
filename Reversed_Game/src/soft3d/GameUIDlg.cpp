#include "soft3d/GameUIDlg.hpp"

#include "soft3d/GameEngine.hpp"
#include "soft3d/MenuDialogs.hpp"
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
    if (value < 0) value = count - 1;
    if (value >= count) value = 0;
}

uint16_t Color565(int32_t red, int32_t green, int32_t blue)
{
    return static_cast<uint16_t>(((red & 0xff) >> 3) << 11 |
                                 ((green & 0xff) >> 2) << 5 | ((blue & 0xff) >> 3));
}

} // namespace

GameUIDlg::GameUIDlg(GameEngine& owner, UiManager* manager)
    : Dialog(manager, 6)
    , engine(owner)
{
    dialogOptions.fill(-1);
}

void GameUIDlg::Initialize()
{
    LoadResources();
    ResetLevelState();
}

void GameUIDlg::ResetLevelState()
{
    mode = -1;
    selection = 0;
    returnSelection = 0;
    optionSelection = 0;
    itemAction = 0;
    combineSource = -1;
    documentIndex = 0;
    documentLineCount = 0;
    scroll = 0;
    errorTimer = 0;
    saveStatus = 0;
    menuOpen = false;
    viewingDocument = false;
    interactionPrompt = -1;
    interactionPromptFrame = 0;
    dialogActive = 0;
    dialogKind = 0;
    dialogTextId = 0;
    dialogSecondaryTextId = -1;
    dialogResult = -1;
    dialogOptionCount = 0;
    dialogPortrait = 0;
    dialogSelection = 0;
    dialogTextLine = 0;
    dialogLineCount = 0;
    dialogTimer = 0;
    dialogOptions.fill(-1);
    subtitleActive = 0;
    subtitleTextId = -1;
    hideMaskIndex = -1;
    hideMaskActive = 0;
    gunHudEnabled = 0;
    mapOffsetX = 0;
    mapOffsetY = 0;
    mapFloor = 0;
    mapBlinkDirection = 0;
    mapBlinkFrame = 0;
    directMap = 0;
    mapNeedsCentering = false;
}

void GameUIDlg::LoadResources()
{
    static constexpr const char16_t* names[] = {
        u"tishi_huoqu", u"tishi_kaimen", u"tishi_jilu", u"tishi_hudong",
        u"tishi_duobi", u"tishi_duihua",
        u"qte_a_1", u"qte_b_1", u"qte_a_2", u"qte_b_2",
        u"qte_a_3", u"qte_b_3", u"qte_a_4", u"qte_b_4",
        u"qte_b_1_alp", u"qte_b_2_alp", u"qte_b_3_alp", u"qte_b_4_alp",
        u"qte_over_1", u"qte_over_2", u"qte_over_3", u"qte_over_4",
        u"qte_zhishi01", u"qte_zhishi02", u"qte_tishi_01", u"qte_tishi_02",
        u"zhishitiao", u"zhishi_04", u"zhishi_05",
        u"_1f", u"1f", u"2f", u"3f", u"door_id",
        u"juqing_01", u"juqing_02", u"juqing_03", u"touxiang_02",
        u"here", u"here_alp", u"shuzi_1", u"door01", u"door02",
        nullptr, nullptr, nullptr, nullptr,
    };
    static_assert(std::size(names) == UiTextureCount);
    textures.clear();
    textures.reserve(UiTextureCount);
    const char16_t* directory = engine.GetLanguage() == 2 ? u".\\ui\\" : u".\\uien\\";
    for (const char16_t* name : names) {
        textures.push_back(name == nullptr ? nullptr : static_cast<TextureRes*>(
            engine.GetResourceManager().GetResource(ResourceTexture, 1, name, directory)));
    }
    TextureRes* portrait = textures[UiPortrait];
    portraitTextWidth = portrait != nullptr && portrait->GetTexture() != nullptr
        ? engine.GetWidth() - static_cast<int32_t>(portrait->GetTexture()->GetWidth()) - 16
        : engine.GetWidth() - 16;
    simpleTextWidth = engine.GetWidth() - 16;
}

void GameUIDlg::LoadDayResources(int day)
{
    if (textures.empty()) LoadResources();
    const char16_t* directory = u".\\ui\\";
    if (day == 5) {
        textures[UiGunFill] = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 2, u"qiang_01", directory));
        textures[UiGunBackground0] = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 2, u"qiang_02", directory));
        textures[UiGunBackground1] = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 2, u"qiang_03", directory));
        textures[UiGunLevel] = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
            ResourceTexture, 2, u"qiang_04", directory));
    } else {
        for (int32_t index = UiGunFill; index <= UiGunLevel; ++index) {
            textures[static_cast<std::size_t>(index)] = nullptr;
        }
    }
    gunHudEnabled = 0;
}

void GameUIDlg::SynchronizeContent()
{
    if (engine.GetGameplayHooks().synchronizeContent) {
        engine.GetGameplayHooks().synchronizeContent();
    }
}

void GameUIDlg::SetPaused(bool paused)
{
    if (engine.GetGameplayHooks().setGamePaused) {
        engine.GetGameplayHooks().setGamePaused(paused);
    }
}

void GameUIDlg::OpenMenu()
{
    SynchronizeContent();
    menuOpen = true;
    mode = 0;
    selection = 0;
    directMap = 0;
    mapOffsetX = 0;
    mapOffsetY = 0;
    SetPaused(true);
    engine.GetAudioManager().SetMusicVolume(0);
    SetActive(true);
}

void GameUIDlg::OpenMap()
{
    OpenMenu();
    mode = 3;
    directMap = 1;
    mapBlinkDirection = 1;
    mapNeedsCentering = true;
    Sound(engine, 7);
}

void GameUIDlg::OpenSaveMenu()
{
    OpenMenu();
    mode = 13;
    saveStatus = 0;
}

void GameUIDlg::CompleteGame()
{
    SetPaused(true);
    mode = 12;
    menuOpen = true;
    SetSubtitle(false, -1);
}

void GameUIDlg::OpenDocument(int32_t noteSlot)
{
    if (noteSlot < 0 || noteSlot >= static_cast<int32_t>(engine.GetContent().noteIds.size())) return;
    OpenMenu();
    mode = 4;
    documentIndex = noteSlot;
    viewingDocument = true;
    scroll = 0;
    const int32_t id = engine.GetContent().noteIds[noteSlot];
    if (id >= 1000 && id < 2000) {
        documentLineCount = static_cast<int32_t>(engine.GetSecondaryFont()->CountTextLines(
            engine.GetSecondaryFont()->GetString(engine.GetContent().documents[id - 1000].bodyString),
            0, engine.GetWidth() - 50));
    }
    Sound(engine, 7);
}

void GameUIDlg::OpenItemDescription(int32_t itemSlot)
{
    if (itemSlot < 0 || itemSlot >= static_cast<int32_t>(engine.GetContent().inventory.size())) return;
    OpenMenu();
    mode = 6;
    selection = itemSlot;
    errorTimer = 0;
}

void GameUIDlg::CloseMenu()
{
    if (!menuOpen) return;
    mode = 11;
    SetPaused(false);
    engine.GetAudioManager().SetMusicVolume(255);
}

void GameUIDlg::PrepareModels()
{
    if (menuOpen && engine.GetGameplayHooks().prepareGameModels) {
        engine.GetGameplayHooks().prepareGameModels(mode, selection);
    }
}

int GameUIDlg::Render(uint32_t context)
{
    (void)context;
    if (!menuOpen) {
        if (engine.GetGameplayHooks().renderGameHud) {
            engine.GetGameplayHooks().renderGameHud();
        }
        if (interactionPrompt != -1) ++interactionPromptFrame;
        const bool canOpen = !engine.GetGameplayHooks().canOpenGameMenu ||
                             engine.GetGameplayHooks().canOpenGameMenu();
        if (canOpen && !dialogActive && !subtitleActive && !hideMaskActive) {
            ControlMap& controls = engine.GetControlMap();
            if (controls.ActionClick(engine.GetInput(), 7, false)) {
                OpenMenu();
                Sound(engine, 4);
            } else if (controls.ActionClick(engine.GetInput(), 6, false)) {
                OpenMap();
            }
        }
        return 1;
    }
    switch (mode) {
    case 0:
        if (AdvanceStateTimeToward(0, 0, 10)) mode = 1;
        break;
    case 1:
        RenderMainMenu();
        break;
    case 2:
        RenderInventory(false);
        break;
    case 3:
        SynchronizeContent();
        if (engine.GetGameplayHooks().renderMap) {
            engine.GetGameplayHooks().renderMap(engine.GetContent().mapId);
        }
        if (Back(engine.GetInput()) || (directMap &&
            (Accept(engine.GetInput()) || engine.GetControlMap().ActionClick(
                engine.GetInput(), 6, false)))) {
            if (directMap) {
                menuOpen = false;
                mode = -1;
                directMap = 0;
                SetPaused(false);
                engine.GetAudioManager().SetMusicVolume(255);
            } else {
                mode = 1;
                selection = returnSelection;
            }
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
        if (engine.GetGameplayHooks().finishGame) engine.GetGameplayHooks().finishGame();
        engine.SaveGameState();
        engine.ReturnToMainMenu();
        menuOpen = false;
        mode = -1;
        break;
    case 13:
        RenderSaveMenu();
        break;
    case 14:
        RenderInventory(true);
        break;
    }
    if (errorTimer > 0) --errorTimer;
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
            directMap = 0;
            mapNeedsCentering = true;
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
    SynchronizeContent();
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(0, combining ? 12 : 11);
    RenderElementAtTime(0, combining ? 11 : 13, combining ? itemAction : 12);
    RenderElementAtTime(0, 14, selection);
    DrawItemText(selection, false);
    if (engine.GetContent().inventory[selection] == 1 && mode != 6) {
        const int32_t width = GetKeyWidth(0, 12, 0);
        const int32_t height = GetKeyHeight(0, 12, 0);
        DrawSmallNumber(engine.GetContent().healthPackCount,
            GetKeyX(0, 12, 0) + width * 2 / 3,
            GetKeyY(0, 12, 0) + height * 2 / 3);
    }
    if (errorTimer > 0) {
        engine.GetSecondaryFont()->DrawString(14, Color565(255, 0, 0),
            {GetKeyX(0, 16, 0), GetKeyY(0, 16, 0), engine.GetWidth() - 4, engine.GetHeight()});
    }
    if (controls.ActionClick(input, 1, false)) {
        selection -= 3;
        if (selection < 0) selection += 9;
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 2, false)) {
        selection += 3;
        if (selection > 8) selection -= 9;
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 3, false)) {
        if (selection % 3 == 0) selection += 2;
        else --selection;
        Sound(engine, 2);
    } else if (controls.ActionClick(input, 4, false)) {
        if (selection % 3 == 2) selection -= 2;
        else ++selection;
        Sound(engine, 2);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = 1;
        selection = combining ? 0 : returnSelection;
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
            combined = engine.GetGameplayHooks().combineItems(selection, combineSource);
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
    SynchronizeContent();
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(0, 12);
    RenderElementAtTime(0, 11, itemAction);
    RenderElementAtTime(0, 13, 12);
    DrawItemText(selection, false);
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
            int result = 0;
            if (engine.GetGameplayHooks().useItem) {
                result = engine.GetGameplayHooks().useItem(selection, 0);
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
    SynchronizeContent();
    SetStateTime(0, 12);
    RenderElementAtTime(0, 13, 12);
    DrawItemText(selection, true);
    if (Back(engine.GetInput()) || Accept(engine.GetInput()) ||
        engine.GetControlMap().ActionClick(engine.GetInput(), 7, false)) {
        mode = 2;
        Sound(engine, 4);
    }
}

void GameUIDlg::DrawItemText(int slot, bool description)
{
    if (slot < 0 || slot >= static_cast<int>(engine.GetContent().inventory.size()) ||
        errorTimer != 0) {
        return;
    }
    const int item = engine.GetContent().inventory[slot];
    if (item <= 0 || item >= static_cast<int>(engine.GetContent().items.size())) return;
    const GameItemRecord& record = engine.GetContent().items[item];
    engine.GetSecondaryFont()->DrawString(description ? record.descriptionString : record.nameString,
        0xffff, {GetKeyX(0, 16, 0), GetKeyY(0, 16, 0), engine.GetWidth() - 4, engine.GetHeight()});
}

int32_t GameUIDlg::DrawSmallNumber(int32_t value, int32_t x, int32_t y)
{
    if (value < -10000 || value > 10000) return 0;
    char16_t text[10];
    WideSprintf(text, u"%d", value);
    TextureRes* resource = textures[UiDigits];
    if (resource == nullptr || resource->GetTexture() == nullptr) return 0;
    int32_t length = 0;
    while (text[length] != u'\0') ++length;
    for (int32_t index = 0; index < length; ++index) {
        const int32_t digit = text[index] - u'0';
        if (digit < 0 || digit > 9) continue;
        engine.GetScreenBlitter().BlitUnscaled(resource->GetTexture(), x, y,
                                               {digit * 9, 0, digit * 9 + 9, 8});
        x += 9;
    }
    return length * 9;
}

void GameUIDlg::RenderFiles()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(3, 0);
    const auto& notes = engine.GetContent().noteIds;
    const int32_t noteId = notes[documentIndex];
    if (viewingDocument) {
        RenderElementAtTime(3, 4, 0);
        RenderElementAtTime(3, 6, 0);
        RenderElementAtTime(3, 7, 0);
        if (noteId < 2001) {
            const int32_t index = noteId - 1000;
            if (index >= 0 && index < static_cast<int32_t>(engine.GetContent().documents.size())) {
                const GameTextRecord& record = engine.GetContent().documents[index];
                engine.GetSecondaryFont()->DrawString(record.titleString, 0xffff, 20,
                                                      Point{engine.GetWidth() / 2, 28});
                const int32_t lines = (engine.GetHeight() - 92) /
                    engine.GetSecondaryFont()->GetTextHeight();
                engine.GetSecondaryFont()->DrawStringRange(record.bodyString, 0xffff, scroll,
                    scroll + lines, {25, 46, engine.GetWidth() - 25, engine.GetHeight() - 46});
            }
        } else {
            const int32_t index = noteId - 2000;
            if (index >= 0 && index < static_cast<int32_t>(engine.GetContent().photos.size())) {
                char16_t name[64];
                WideSprintf(name, index < 10 ? u"photo_200%d" : u"photo_20%d", index);
                auto* resource = static_cast<TextureRes*>(engine.GetResourceManager().GetResource(
                    ResourceTexture, 3, name,
                    engine.GetLanguage() == 2 ? u".\\ui\\" : u".\\uien\\"));
                if (resource != nullptr && resource->GetTexture() != nullptr) {
                    const Texture* texture = resource->GetTexture();
                    engine.GetScreenBlitter().BlitUnscaled(texture,
                        (engine.GetWidth() - static_cast<int32_t>(texture->GetWidth())) / 2,
                        (engine.GetHeight() - static_cast<int32_t>(texture->GetHeight())) / 2,
                        {0, 0, static_cast<int32_t>(texture->GetWidth()),
                         static_cast<int32_t>(texture->GetHeight())});
                }
                engine.GetSecondaryFont()->DrawString(
                    engine.GetContent().photos[index].titleString, 0xffff, 20,
                    Point{engine.GetWidth() / 2, 28});
            }
        }
        if (noteId < 2001) {
            if (controls.ActionDown(input, 1, false) && scroll > 0) --scroll;
            if (controls.ActionDown(input, 2, false)) {
                const int32_t lines = (engine.GetHeight() - 92) /
                    engine.GetSecondaryFont()->GetTextHeight();
                if (scroll + lines < documentLineCount) ++scroll;
            }
        }
        if (controls.ActionClick(input, 3, false) || controls.ActionClick(input, 4, false)) {
            const int32_t step = controls.ActionClick(input, 3, false) ? -1 : 1;
            const int32_t original = documentIndex;
            do {
                documentIndex = (documentIndex + step + 128) % 128;
            } while (notes[documentIndex] == 0 && documentIndex != original);
            const int32_t id = notes[documentIndex];
            if (id >= 1000 && id < 2000) {
                documentLineCount = static_cast<int32_t>(engine.GetSecondaryFont()->CountTextLines(
                    engine.GetSecondaryFont()->GetString(engine.GetContent().documents[id - 1000].bodyString),
                    0, engine.GetWidth() - 50));
            }
            scroll = 0;
            Sound(engine, 7);
        }
        if (Back(input) || controls.ActionClick(input, 7, false) ||
            controls.ActionClick(input, 5, false)) {
            viewingDocument = false;
            Sound(engine, 4);
        }
        return;
    }

    bool empty = true;
    for (int32_t note : notes) if (note != 0) empty = false;
    if (empty) {
        if (Back(input) || controls.ActionClick(input, 7, false)) {
            mode = 1;
            selection = returnSelection;
            Sound(engine, 4);
        }
        return;
    }
    RenderElementAtTime(3, 2, 0);
    RenderElementAtTime(3, 4, 0);
    const int32_t linesPerPage = (engine.GetHeight() - 69) /
        engine.GetSecondaryFont()->GetTextHeight();
    const int32_t x = GetKeyX(3, 11, 0);
    int32_t y = GetKeyY(3, 11, 0);
    SetKeyY(3, 12, 0, GetKeyY(3, 11, 0) + scroll * 15 - 2);
    if (scroll >= linesPerPage) {
        SetKeyY(3, 12, 0, GetKeyY(3, 11, 0) + linesPerPage * 15 - 17);
        y += (scroll - linesPerPage) * -15 - 15;
    }
    RenderElementAtTime(3, 12, 0);
    for (int32_t note : notes) {
        if (note == 0) continue;
        if (y > 24 && y <= linesPerPage * 15 + 20) {
            const int32_t index = note - 1000;
            if (note < 2001 && index >= 0 &&
                index < static_cast<int32_t>(engine.GetContent().documents.size())) {
                engine.GetSecondaryFont()->DrawString(
                    engine.GetContent().documents[index].titleString, 0, 17, Point{x, y});
            } else if (note >= 2001 && note - 2000 >= 0 &&
                       note - 2000 < static_cast<int32_t>(engine.GetContent().photos.size())) {
                engine.GetSecondaryFont()->DrawString(
                    engine.GetContent().photos[note - 2000].titleString, 0, 17, Point{x, y});
            }
        }
        y += 15;
    }
    if (controls.ActionClick(input, 1, false) || controls.ActionClick(input, 2, false)) {
        const int32_t step = controls.ActionClick(input, 1, false) ? -1 : 1;
        const int32_t original = documentIndex;
        do {
            documentIndex = (documentIndex + step + 128) % 128;
        } while (notes[documentIndex] == 0 && documentIndex != original);
        scroll = 0;
        Sound(engine, 1);
    } else if ((Accept(input)) && notes[documentIndex] > 0) {
        viewingDocument = true;
        scroll = 0;
        const int32_t id = notes[documentIndex];
        if (id < 2001) {
            documentLineCount = static_cast<int32_t>(engine.GetSecondaryFont()->CountTextLines(
                engine.GetSecondaryFont()->GetString(engine.GetContent().documents[id - 1000].bodyString),
                0, engine.GetWidth() - 50));
        }
        Sound(engine, 3);
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        mode = 1;
        selection = returnSelection;
        Sound(engine, 4);
    }
}

void GameUIDlg::RenderSaveMenu()
{
    Input& input = engine.GetInput();
    ControlMap& controls = engine.GetControlMap();
    SetStateTime(5, selection);
    const auto& slots = engine.GetSaveSlots();
    for (int32_t slot = 0; slot < 3; ++slot) {
        if (!slots[slot].present) {
            RenderElementAtTime(5, 12, slot);
        } else {
            RenderElementAtTime(5, slots[slot].GetDay() + 42, slot);
            RenderElementAtTime(5, 50, slot);
            DrawSmallNumber(slots[slot].GetPlayTime(), GetKeyX(5, 12, slot),
                            GetKeyY(5, 12, slot));
        }
    }
    if (saveStatus == 1) {
        RenderElementAtTime(5, 13, 0);
        RenderElementAtTime(5, 14, 0);
    } else if (saveStatus == 2) {
        RenderElementAtTime(5, 13, 0);
        RenderElementAtTime(5, 15, 0);
        saveStatus = 3;
    } else if (saveStatus == 3) {
        if (engine.GetGameplayHooks().saveGame) engine.GetGameplayHooks().saveGame(selection + 1);
        engine.RefreshSaveSlots();
        saveStatus = 4;
    } else if (saveStatus == 4) {
        RenderElementAtTime(5, 13, 0);
        RenderElementAtTime(5, 16, 0);
    }
    if (controls.ActionClick(input, 5, false)) {
        if (saveStatus == 0) {
            saveStatus = slots[selection].present ? 1 : 2;
        } else if (saveStatus == 1) {
            saveStatus = 2;
        } else if (saveStatus == 4) {
            saveStatus = 0;
        }
    } else if (Back(input) || controls.ActionClick(input, 7, false)) {
        menuOpen = false;
        mode = -1;
        selection = 0;
        SetPaused(false);
        engine.GetAudioManager().SetMusicVolume(255);
    } else if (saveStatus == 0 && controls.ActionClick(input, 1, false)) {
        if (selection > 0) --selection;
        Sound(engine, 1);
    } else if (saveStatus == 0 && controls.ActionClick(input, 2, false)) {
        ++selection;
        if (selection > 2) selection = 0;
        Sound(engine, 1);
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

void GameUIDlg::BeginDialog(int32_t kind, int32_t textId, int32_t width)
{
    dialogActive = 0;
    dialogSecondaryTextId = -1;
    dialogActive = 1;
    dialogKind = kind;
    dialogTextId = textId;
    dialogTextLine = 0;
    VisString* text = engine.GetBaseFont()->GetString(static_cast<uint32_t>(textId));
    dialogLineCount = text == nullptr ? 0 : static_cast<int32_t>(
        engine.GetBaseFont()->CountTextLines(text, 0, static_cast<uint32_t>(std::max(0, width))));
    SetPaused(true);
}

void GameUIDlg::CloseDialog()
{
    dialogActive = 0;
    dialogSecondaryTextId = -1;
    SetPaused(false);
}

int32_t GameUIDlg::GetDialogOption(int32_t index) const
{
    if (index == 4) return dialogSecondaryTextId;
    return index >= 0 && index < static_cast<int32_t>(dialogOptions.size())
        ? dialogOptions[static_cast<std::size_t>(index)] : -1;
}

void GameUIDlg::ShowEventMessage(int32_t textId)
{
    if (menuOpen) return;
    BeginDialog(0, textId, portraitTextWidth);
}

void GameUIDlg::ShowSimpleMessage(int32_t textId)
{
    if (menuOpen) return;
    BeginDialog(3, textId, simpleTextWidth);
    dialogResult = -1;
}

void GameUIDlg::ShowTimedMessage(int32_t textId, int32_t frames)
{
    if (menuOpen || dialogActive) return;
    BeginDialog(2, textId, simpleTextWidth);
    dialogTimer = frames;
}

void GameUIDlg::ShowTimedResultMessage(int32_t textId, int32_t frames)
{
    if (menuOpen || dialogActive) return;
    BeginDialog(4, textId, simpleTextWidth);
    dialogTimer = frames;
    dialogResult = -1;
}

void GameUIDlg::SetDialogString(int32_t index, int32_t textId)
{
    if (index == 4) {
        dialogSecondaryTextId = textId;
    } else if (index >= 0 && index < static_cast<int32_t>(dialogOptions.size())) {
        dialogOptions[static_cast<std::size_t>(index)] = textId;
    }
}

void GameUIDlg::ShowChoiceDialog(int32_t titleId, int32_t optionCount, bool portrait)
{
    if (menuOpen) return;
    dialogActive = 0;
    dialogSecondaryTextId = -1;
    dialogActive = 1;
    dialogResult = -1;
    dialogKind = 1;
    dialogTextId = titleId;
    dialogOptionCount = optionCount;
    dialogPortrait = portrait ? 1 : 0;
    dialogSelection = 0;
    SetPaused(true);
}

void GameUIDlg::SetSubtitle(bool active, int32_t textId)
{
    subtitleActive = active ? 1 : 0;
    subtitleTextId = textId;
}

void GameUIDlg::SetHideMask(int32_t index, bool active)
{
    hideMaskIndex = index;
    hideMaskActive = active ? 1 : 0;
}

void GameUIDlg::SetMapMarker(int32_t index, int32_t type, int32_t x, int32_t y)
{
    if (index < 0 || index >= static_cast<int32_t>(mapMarkers.size())) return;
    mapMarkers[static_cast<std::size_t>(index)] = {x, y, type};
}

void GameUIDlg::ClearMapMarker(int32_t index)
{
    if (index < 0 || index >= static_cast<int32_t>(mapMarkers.size())) return;
    GameUiMapMarker& marker = mapMarkers[static_cast<std::size_t>(index)];
    marker.x = -1;
    marker.y = -1;
}

const GameUiMapMarker& GameUIDlg::GetMapMarker(int32_t index) const
{
    return mapMarkers.at(static_cast<std::size_t>(index));
}

void GameUIDlg::SetMapView(int32_t floor, int32_t x, int32_t y)
{
    mapFloor = floor;
    mapOffsetX = x;
    mapOffsetY = y;
}

bool GameUIDlg::ConsumeMapCentering()
{
    const bool result = mapNeedsCentering;
    mapNeedsCentering = false;
    return result;
}

TextureRes* GameUIDlg::GetGameplayTexture(int32_t index) const
{
    return index >= 0 && index < static_cast<int32_t>(textures.size())
        ? textures[static_cast<std::size_t>(index)] : nullptr;
}

int GameUIDlg::GetMode() const { return mode; }
int GameUIDlg::GetSelection() const { return selection; }
bool GameUIDlg::IsMenuOpen() const { return menuOpen; }

} // namespace soft3d

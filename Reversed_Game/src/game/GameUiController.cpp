#include "game/GameUiController.hpp"

#include "game/Scene.hpp"
#include "soft3d/Color.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/TextureRes.hpp"

#include <algorithm>
#include <stdexcept>

namespace game {

using namespace soft3d;

namespace {

struct MapCalibrationRecord {
    int16_t width;
    int16_t height;
    int16_t offsetX;
    int16_t offsetY;
};

struct MapAxisRecord {
    int16_t xAxis;
    int16_t reserved0;
    int16_t zAxis;
    int16_t invertX;
    int16_t reserved1;
    int16_t invertZ;
};

struct MapMarkerRecord {
    int16_t x;
    int16_t y;
    int16_t floor;
    int16_t day;
    int16_t icon;
};

#include "game/GameMapData.inc"

soft3d::ScreenRect FullImage(const soft3d::Texture* texture)
{
    return texture == nullptr ? soft3d::ScreenRect{} : soft3d::ScreenRect{
        0, 0, static_cast<int32_t>(texture->GetWidth()), static_cast<int32_t>(texture->GetHeight())};
}

bool IsUnavailableMap(int32_t, int32_t mapId)
{
    static constexpr int32_t ids[] = {
        101, 1050, 1051, 201, 3052, 2062, 2066, 2020, 2019, 2006, 1023, 1024,
    };
    return std::find(std::begin(ids), std::end(ids), mapId) != std::end(ids) ||
           mapId / 1000 == 5;
}

bool IsDay6FixedPosition(int32_t mapId, soft3d::Point& point)
{
    switch (mapId) {
    case 6001: case 6002: case 6003: point = {60, 240}; return true;
    case 6004: case 6005: point = {50, 180}; return true;
    case 6007: point = {580, 270}; return true;
    case 6008: point = {120, 242}; return true;
    case 6101: point = {92, 212}; return true;
    case 6102: point = {185, 373}; return true;
    case 6103: point = {318, 210}; return true;
    case 6104: point = {500, 266}; return true;
    case 6107: point = {500, 75}; return true;
    default: return false;
    }
}

} // namespace

GameUiController::GameUiController(Scene& owner)
    : scene(owner)
    , engine(owner.GetEngine())
    , ui(owner.GetGameUi())
{
}

void GameUiController::InstallHooks()
{
    soft3d::GameplayHooks& hooks = engine.GetGameplayHooks();
    hooks.renderGameHud = [this] { RenderHud(); };
    hooks.renderMap = [this](int32_t mapId) { RenderMap(mapId); };
    hooks.renderAboutModel = [this](int32_t model, int32_t direction, int32_t transition,
                                    int32_t zoom) {
        RenderAboutModel(model, direction, transition, zoom);
    };
    hooks.useItem = [this](int32_t slot, int32_t) {
        const int32_t result = scene.GetPlayer()->UseItem(slot);
        SynchronizeContent();
        return result;
    };
    hooks.combineItems = [this](int32_t firstSlot, int32_t secondSlot) {
        const bool result = scene.GetPlayer()->CombineItems(firstSlot, secondSlot);
        SynchronizeContent();
        if (result) {
            const int32_t selected = scene.GetPlayer()->GetSelectedItemSlot();
            if (selected >= 0 && selected < 9) ui->selection = selected;
            scene.GetPlayer()->ClearSelectedItem();
        }
        return result;
    };
    hooks.synchronizeContent = [this] { SynchronizeContent(); };
    hooks.setGamePaused = [this](bool paused) { scene.GetUnitManager()->SetPaused(paused); };
    hooks.canOpenGameMenu = [this] { return CanOpenMenu(); };
    hooks.prepareGameModels = [this](int32_t mode, int32_t selection) {
        PrepareModels(mode, selection);
    };
    hooks.saveGame = [this](int32_t slot) { return SaveGame(slot); };
    hooks.finishGame = [this] { FinishGame(); };
}

void GameUiController::SynchronizeContent()
{
    soft3d::GameContent& content = engine.GetContent();
    Player* player = scene.GetPlayer();
    for (std::size_t index = 0; index < content.inventory.size(); ++index) {
        content.inventory[index] = player->GetInventoryItem(index);
    }
    for (std::size_t index = 0; index < content.noteIds.size(); ++index) {
        content.noteIds[index] = player->GetNote(index);
    }
    content.mapId = scene.GetMapId();
    content.healthPackCount = scene.GetUnitManager()->GetHealthPackCount();
    content.statistics.first = static_cast<uint32_t>(player->GetItemUseCount());
    content.statistics.second = static_cast<uint32_t>(player->GetKillCount());
    content.statistics.elapsedSeconds = static_cast<uint32_t>(player->GetPlayTime());
    content.statistics.fourth = static_cast<uint32_t>(player->GetDeathCount());
    content.statistics.filesFound = static_cast<uint32_t>(CountNotes());
    content.statistics.secretsFound = static_cast<uint32_t>(CountSecrets());
}

int32_t GameUiController::CountNotes() const
{
    int32_t result = 0;
    for (std::size_t index = 0; index < 128; ++index) {
        if (scene.GetPlayer()->GetNote(index) != 0) ++result;
    }
    return result;
}

int32_t GameUiController::CountSecrets() const
{
    int32_t result = 0;
    for (int16_t id : secretEventIds) if (scene.GetEventFlag(id)) ++result;
    return result;
}

void GameUiController::FinishGame()
{
    auto& unlocks = engine.GetConfiguration().unlocks;
    if (scene.GetEventFlag(998)) unlocks.extraWeapon0 = 1;
    if (scene.GetEventFlag(999)) unlocks.extraWeapon1 = 1;
    unlocks.newGameOptions = 1;
}

bool GameUiController::SaveGame(int32_t slot)
{
    return scene.SaveSaveSlot(slot);
}

bool GameUiController::CanOpenMenu() const
{
    return !scene.IsDayIntroPending() && !scene.IsNextLevelPending() &&
           scene.GetTransitionStage() == 0 && scene.GetMapProgress() >= 0 &&
           scene.GetGameOverStage() == 0 && !scene.IsHideMaskActive();
}

void GameUiController::SetPrompt(int32_t prompt)
{
    ui->SetInteractionPrompt(prompt);
}

void GameUiController::ShowMessage(int32_t textId, int32_t kind)
{
    if (kind == 0) ui->ShowEventMessage(textId);
    else ui->ShowSimpleMessage(textId);
}

void GameUiController::SetChoiceString(int32_t index, int32_t textId)
{
    ui->SetDialogString(index, textId);
}

void GameUiController::ShowChoice(int32_t titleId, int32_t optionCount, bool portrait)
{
    ui->ShowChoiceDialog(titleId, optionCount, portrait);
    ui->ClearDialogResult();
}

int32_t GameUiController::GetDialogResult() const
{
    return ui->GetDialogResult();
}

bool GameUiController::IsDialogActive() const
{
    return ui->IsDialogActive();
}

void GameUiController::CloseDialog()
{
    ui->CloseDialog();
}

soft3d::TextureRes* GameUiController::LoadTexture(const char16_t* name, int32_t cacheLevel,
                                                   bool localized)
{
    const char16_t* directory = !localized || engine.GetLanguage() != 2 ? u".\\ui\\" : u".\\uien\\";
    return static_cast<soft3d::TextureRes*>(engine.GetResourceManager().GetResource(
        soft3d::ResourceTexture, cacheLevel, name, directory));
}

void GameUiController::RenderHud()
{
    SynchronizeContent();
    Player* player = scene.GetPlayer();
    if (player->GetSelectedNoteSlot() >= 0) {
        ui->OpenDocument(player->GetSelectedNoteSlot());
        player->ClearSelectedNote();
    } else if (player->GetSelectedItemSlot() >= 0 &&
               player->GetInventoryItem(static_cast<std::size_t>(player->GetSelectedItemSlot())) > 0) {
        ui->OpenItemDescription(player->GetSelectedItemSlot());
        player->ClearSelectedItem();
    }
    if (player->IsDeathComplete()) {
        ui->RenderElementAtTime(7, 1, 0);
        ui->RenderElementAtTime(7, 2, 0);
        return;
    }
    RenderInteractionPrompt();
    RenderQte();
    RenderHideMask();
    RenderDialog();
    RenderHealth();
    RenderGunHud();
    RenderSubtitle();
}

void GameUiController::RenderInteractionPrompt()
{
    const int32_t prompt = ui->GetInteractionPrompt();
    soft3d::TextureRes* resource = ui->GetGameplayTexture(prompt);
    if (resource == nullptr || resource->GetTexture() == nullptr) return;
    const soft3d::Texture* texture = resource->GetTexture();
    const int32_t width = static_cast<int32_t>(texture->GetWidth()) / 4;
    const int32_t frame = ((ui->GetInteractionPromptFrame() & 15) >> 2) * width;
    engine.GetScreenBlitter().BlitUnscaled(texture, engine.GetWidth() - width - 1, 1,
        {frame, 0, frame + width, static_cast<int32_t>(texture->GetHeight())});
}

void GameUiController::RenderQte()
{
    if (scene.GetQteUiFailure() != 0) {
        const int32_t frame = qteFailureFrame++;
        if (frame < 8) {
            soft3d::TextureRes* resource = ui->GetGameplayTexture(UiQteOver1 + (frame >> 1));
            if (resource != nullptr && resource->GetTexture() != nullptr) {
                const soft3d::Texture* texture = resource->GetTexture();
                engine.GetScreenBlitter().BlitUnscaled(texture,
                    (engine.GetWidth() - static_cast<int32_t>(texture->GetWidth())) / 2,
                    (engine.GetHeight() - static_cast<int32_t>(texture->GetHeight())) / 2 + 40,
                    FullImage(texture));
            }
        } else {
            scene.ClearQteFailure();
        }
        return;
    }
    qteFailureFrame = 0;
    if (scene.GetQteUiTime() > 0 && scene.GetQteUiKey() >= 0 && scene.GetQteUiKey() < 4) {
        scene.AdvanceQteUi();
        const int32_t key = scene.GetQteUiKey();
        const int32_t mode = scene.GetQteUiMode();
        bool alternate = false;
        if (mode == 0) {
            alternate = engine.GetInput().IsKeyDown(static_cast<soft3d::Key>(key + 13));
        } else if (mode != 0) {
            alternate = scene.GetQteUiTime() % (mode * 2) <= mode;
        }
        soft3d::TextureRes* resource = ui->GetGameplayTexture(
            alternate ? UiQteB1 + key * 2 : UiQteA1 + key * 2);
        soft3d::TextureRes* alpha = ui->GetGameplayTexture(UiQteAlpha1 + key);
        if (resource != nullptr && resource->GetTexture() != nullptr) {
            const soft3d::Texture* texture = resource->GetTexture();
            const int32_t x = (engine.GetWidth() - static_cast<int32_t>(texture->GetWidth())) / 2;
            const int32_t y = (engine.GetHeight() - static_cast<int32_t>(texture->GetHeight())) / 2 + 40;
            engine.GetScreenBlitter().BlitUnscaled(texture, x, y, FullImage(texture), 0,
                alternate && alpha != nullptr ? alpha->GetTexture() : nullptr);
            if (mode != 0) {
                soft3d::TextureRes* guide = ui->GetGameplayTexture(UiQteGuide1 + alternate);
                if (guide != nullptr && guide->GetTexture() != nullptr) {
                    engine.GetScreenBlitter().BlitUnscaled(guide->GetTexture(), x + 22, y + 28,
                                                           FullImage(guide->GetTexture()));
                }
            }
        }
        return;
    }
    const int32_t mode = scene.GetQteUiMode();
    if (mode == 0) return;
    soft3d::TextureRes* resource = ui->GetGameplayTexture(UiQteHint1 + mode - 1);
    if (resource == nullptr || resource->GetTexture() == nullptr) return;
    uint32_t blend = 0;
    const int32_t counter = scene.GetPlayer()->GetStateCounter();
    const int32_t duration = scene.GetPlayer()->GetQteDuration();
    if (counter < 1 || duration - 1 < counter) blend = 3;
    else if (counter < 2 || duration - 2 < counter) blend = 4;
    else if (counter < 3 || duration - 3 < counter) blend = 5;
    const soft3d::Texture* texture = resource->GetTexture();
    engine.GetScreenBlitter().BlitUnscaled(texture,
        (engine.GetWidth() - static_cast<int32_t>(texture->GetWidth())) / 2,
        (engine.GetHeight() - static_cast<int32_t>(texture->GetHeight())) / 2 + 40,
        FullImage(texture), blend);
}

void GameUiController::RenderHideMask()
{
    if (!scene.IsHideMaskActive()) return;
    soft3d::TextureRes* resource = LoadTexture(scene.GetHideMask() == 0 ?
        u"fengmian_bj" : u"fengmian_bj2", 3, false);
    if (resource != nullptr && resource->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(resource->GetTexture(), 0, 0,
                                               FullImage(resource->GetTexture()));
    }
}

void GameUiController::RenderDialog()
{
    if (!ui->IsDialogActive()) return;
    soft3d::StringManager* font = engine.GetBaseFont();
    const int32_t kind = ui->GetDialogKind();
    if (kind == 0 || kind == 3 || kind == 4) {
        soft3d::TextureRes* background = ui->GetGameplayTexture(UiStoryEvent);
        if (background != nullptr && background->GetTexture() != nullptr) {
            const soft3d::Texture* texture = background->GetTexture();
            const int32_t top = engine.GetHeight() - static_cast<int32_t>(texture->GetHeight());
            engine.GetScreenBlitter().BlitUnscaled(texture, 0, top, FullImage(texture), 4);
            int32_t left = 8;
            if (kind == 0) {
                soft3d::TextureRes* portrait = ui->GetGameplayTexture(UiPortrait);
                if (portrait != nullptr && portrait->GetTexture() != nullptr) {
                    engine.GetScreenBlitter().BlitUnscaled(portrait->GetTexture(), 3,
                        engine.GetHeight() - static_cast<int32_t>(portrait->GetTexture()->GetHeight()),
                        FullImage(portrait->GetTexture()), 4);
                    left += static_cast<int32_t>(portrait->GetTexture()->GetWidth()) + 8;
                }
            }
            const uint16_t color = kind == 2 ? PackRgb565(249, 231, 0) : 0xffff;
            font->DrawStringRange(ui->GetDialogTextId(), color, ui->GetDialogTextLine(),
                ui->GetDialogTextLine() + 1,
                {left, top + 8, left + (kind == 0 ? ui->GetPortraitTextWidth() :
                 ui->GetSimpleTextWidth()), engine.GetHeight() - 8});
        }
        if (kind == 0) {
            if (engine.GetControlMap().ActionClick(engine.GetInput(), 5, false) &&
                ++ui->dialogTextLine >= ui->GetDialogLineCount() - 1) {
                ui->CloseDialog();
            }
        } else if (kind == 3) {
            if (engine.GetControlMap().ActionClick(engine.GetInput(), 5, false) &&
                ++ui->dialogTextLine >= ui->GetDialogLineCount() - 1) {
                ui->dialogResult = 0;
                ui->CloseDialog();
            }
        } else if (kind == 4 && --ui->dialogTimer == 0) {
            ui->dialogResult = 0;
            ui->dialogActive = 0;
        }
        return;
    }
    if (kind == 2) {
        if (--ui->dialogTimer == 0) {
            if (scene.GetPlayer()->IsHiddenOrFPS()) scene.GetUnitManager()->SetPaused(false);
            ui->dialogActive = 0;
        }
        soft3d::TextureRes* background = ui->GetGameplayTexture(UiStoryTimed);
        if (background != nullptr && background->GetTexture() != nullptr) {
            const soft3d::Texture* texture = background->GetTexture();
            const int32_t top = engine.GetHeight() - static_cast<int32_t>(texture->GetHeight());
            engine.GetScreenBlitter().BlitUnscaled(texture, 0, top, FullImage(texture), 4);
            font->DrawStringRange(ui->GetDialogTextId(), PackRgb565(249, 231, 0),
                ui->GetDialogTextLine(), ui->GetDialogTextLine() + 1,
                {8, top + 8, 8 + ui->GetSimpleTextWidth(), engine.GetHeight() - 8});
        }
        return;
    }

    soft3d::TextureRes* background = ui->GetGameplayTexture(UiStoryChoice);
    if (background != nullptr && background->GetTexture() != nullptr) {
        const soft3d::Texture* texture = background->GetTexture();
        engine.GetScreenBlitter().BlitUnscaled(texture,
            0, engine.GetHeight() - static_cast<int32_t>(texture->GetHeight()), FullImage(texture), 4);
    }
    const int32_t titleY = engine.GetHeight() -
        (background != nullptr && background->GetTexture() != nullptr
            ? static_cast<int32_t>(background->GetTexture()->GetHeight()) : 0) + 5;
    if (ui->GetDialogSecondaryTextId() == -1) {
        font->DrawString(ui->GetDialogTextId(), 0xffff, 4,
            {16, titleY - 2, engine.GetWidth() - 16, titleY + 16});
    } else {
        const uint32_t secondaryWidth = font->CountTextWidth(
            font->GetString(ui->GetDialogSecondaryTextId()));
        const uint32_t titleWidth = font->CountTextWidth(font->GetString(ui->GetDialogTextId()));
        soft3d::Point point{static_cast<int32_t>(engine.GetWidth() - secondaryWidth - titleWidth - 8) / 2,
                            titleY};
        font->DrawString(ui->GetDialogSecondaryTextId(), PackRgb565(0, 255, 0), 17, point);
        point.x += 8;
        font->DrawString(ui->GetDialogTextId(), 0xffff, 17, point);
    }
    const int32_t y = engine.GetHeight() - static_cast<int32_t>(font->GetTextHeight()) - 6;
    const int32_t quarter = (engine.GetWidth() - 20) / 4;
    const int32_t base = (engine.GetWidth() - 20) / 8 + 10;
    std::array<int32_t, 4> positions{};
    const int32_t count = ui->GetDialogOptionCount();
    if (count == 1) positions[0] = base + quarter * 3;
    else if (count == 2) positions = {base + quarter, base + quarter * 2, 0, 0};
    else if (count == 3) positions = {base + quarter, base + quarter * 2, base + quarter * 3, 0};
    else positions = {base, base + quarter, base + quarter * 2, base + quarter * 3};
    for (int32_t index = 0; index < count && index < 4; ++index) {
        const int32_t textId = ui->GetDialogOption(index);
        if (index == ui->GetDialogSelection()) {
            soft3d::VisString* string = font->GetString(static_cast<uint32_t>(textId));
            const int32_t width = string == nullptr ? 50 : static_cast<int32_t>(font->CountTextWidth(string));
            const int32_t left = positions[index] - 1 - width / 2;
            engine.GetScreenBlitter().FillRect(PackRgb565(0, 155, 219),
                {left, y - 1, left + width, y + static_cast<int32_t>(font->GetTextHeight()) - 2});
        }
        font->DrawString(static_cast<uint32_t>(textId), 0xffff, 20, Point{positions[index], y});
    }
    if (engine.GetControlMap().ActionClick(engine.GetInput(), 5, false)) {
        ui->dialogResult = ui->GetDialogSelection();
        if (ui->IsDialogPortrait()) {
            ui->CloseDialog();
            ui->dialogOptions.fill(-1);
        }
    }
    if (engine.GetControlMap().ActionClick(engine.GetInput(), 3, true)) {
        if (ui->dialogSelection > 0) --ui->dialogSelection;
        engine.GetAudioManager().PlaySound(2, false);
    }
    if (engine.GetControlMap().ActionClick(engine.GetInput(), 4, true)) {
        if (ui->dialogSelection + 1 < count) ++ui->dialogSelection;
        engine.GetAudioManager().PlaySound(2, false);
    }
}

void GameUiController::RenderHealth()
{
    Player* player = scene.GetPlayer();
    if (ui->IsSubtitleActive()) return;
    if (player->GetHealth() > 0 && player->GetHealth() <= player->GetDangerHealthThreshold()) {
        ui->AdvanceElementTime(7, 0);
    }
}

void GameUiController::RenderGunHud()
{
    if (ui->IsSubtitleActive() || ui->IsGunHudEnabled() == 0 ||
        scene.GetPlayer()->GetWeapon() != 3 || scene.GetSpecialDoorIndex() != -1 ||
        scene.GetDay() == 7) {
        return;
    }
    Player* player = scene.GetPlayer();
    const int32_t level = player->GetGunLevel();
    const int32_t background = level == 0 ? UiGunBackground0 : level == 1 ? UiGunBackground1 : UiGunFill;
    soft3d::TextureRes* resource = ui->GetGameplayTexture(background);
    if (resource != nullptr && resource->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(resource->GetTexture(), 5, 5,
                                               FullImage(resource->GetTexture()));
    }
    soft3d::TextureRes* fillResource = ui->GetGameplayTexture(UiGunFill);
    if (level < 2 && fillResource != nullptr && fillResource->GetTexture() != nullptr) {
        const int32_t width = static_cast<int32_t>(fillResource->GetTexture()->GetWidth());
        const int32_t experience = level == 0 ? player->GetGunExperience()
                                              : player->GetGunExperience() - 50;
        const int32_t maximum = level == 0 ? 50 : 50;
        const int32_t right = maximum == 0 ? 0 : experience * width / maximum;
        engine.GetScreenBlitter().BlitUnscaled(fillResource->GetTexture(), 5, 5,
            {0, 0, right, static_cast<int32_t>(fillResource->GetTexture()->GetHeight())});
    }
    soft3d::TextureRes* levelResource = ui->GetGameplayTexture(UiGunLevel);
    if (levelResource == nullptr || levelResource->GetTexture() == nullptr) return;
    const int32_t width = static_cast<int32_t>(levelResource->GetTexture()->GetWidth()) / 3;
    int32_t x = 68;
    for (int32_t index = 0; index <= level; ++index) {
        engine.GetScreenBlitter().BlitUnscaled(levelResource->GetTexture(), x, 20,
            {index * width, 0, index * width + width,
             static_cast<int32_t>(levelResource->GetTexture()->GetHeight())});
        x -= width + 2;
    }
}

void GameUiController::RenderSubtitle()
{
    if (!ui->IsSubtitleActive() || scene.GetTransitionStage() != 0) return;
    engine.GetScreenBlitter().FillRect(0, {0, 0, engine.GetWidth(), 32});
    engine.GetScreenBlitter().FillRect(0, {0, engine.GetHeight() - 32,
                                           engine.GetWidth(), engine.GetHeight()});
    engine.GetBaseFont()->DrawString(ui->GetSubtitleTextId(), 0xffff, 4,
        {10, engine.GetHeight() - 29, engine.GetWidth() - 10, engine.GetHeight() - 3});
}

void GameUiController::RenderUnavailableMap()
{
    engine.SetPrepareEnabled(true);
    engine.GetRenderer().Clear(0);
    engine.GetSecondaryFont()->DrawString(29, 0xffff, 68,
                                          Point{engine.GetWidth() / 2, engine.GetHeight() / 2});
    soft3d::TextureRes* bar = ui->GetGameplayTexture(UiHintBar);
    if (bar != nullptr && bar->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(bar->GetTexture(),
            0, engine.GetHeight() - static_cast<int32_t>(bar->GetTexture()->GetHeight()),
            FullImage(bar->GetTexture()));
    }
    soft3d::TextureRes* right = ui->GetGameplayTexture(UiHintRight);
    if (right != nullptr && right->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(right->GetTexture(),
            engine.GetWidth() - static_cast<int32_t>(right->GetTexture()->GetWidth()) - 2,
            engine.GetHeight() - static_cast<int32_t>(right->GetTexture()->GetHeight()) - 2,
            FullImage(right->GetTexture()));
    }
    mapTexture = nullptr;
}

void GameUiController::LoadMapTexture(int32_t)
{
    if (scene.GetDay() == 6) {
        mapTexture = LoadTexture(u"day6", 3, false);
        return;
    }
    const int32_t floor = ui->GetMapFloor();
    if (scene.GetDay() == 5) {
        static constexpr const char16_t* names[] = {u"_1f_day5", u"1f_day5", u"2f_day5", u"3f_day5"};
        mapTexture = floor >= 0 && floor < 4 ? LoadTexture(names[floor]) : nullptr;
    } else {
        static constexpr const char16_t* names[] = {nullptr, u"1f_00", u"2f_00", u"3f_00"};
        mapTexture = floor > 0 && floor < 4 ? LoadTexture(names[floor]) : nullptr;
    }
}

void GameUiController::InitializeMapView(int32_t mapId)
{
    if (!ui->ConsumeMapCentering()) return;
    int32_t floor = mapId / 1000;
    int32_t effectiveMap = mapId;
    if ((mapId == 1030 || mapId == 1052) && scene.GetPlayer()->GetPosition().y > -0xa0000) {
        floor = 2;
    }
    if (mapId == 1030) effectiveMap = 2030;
    if (mapId == 1052) effectiveMap = 2048;
    soft3d::Point point{0, 0};
    if (scene.GetDay() != 6) point = ProjectMapPosition(scene.GetPlayer()->GetPosition(), effectiveMap);
    else IsDay6FixedPosition(mapId, point);
    ui->SetMapView(floor, point.x - engine.GetWidth() / 2, point.y - engine.GetHeight() / 2);
}

soft3d::Point GameUiController::ProjectMapPosition(const soft3d::Vec3& position,
                                                    int32_t mapId) const
{
    if (scene.GetDay() == 6) {
        soft3d::Point result{};
        IsDay6FixedPosition(mapId, result);
        return result;
    }
    const int32_t group = mapId / 1000;
    const int32_t index = mapId % 100 - 1;
    if (group < 0 || group >= 7 || index < 0 || index >= 100) return {};
    const MapCalibrationRecord& calibration = mapCalibrationRecords[group * 100 + index];
    const auto& bounds = scene.GetUnitManager()->GetMapHeaderValues();
    int32_t axis = 0;
    while (axis < 7 && axisMapIds[axis] != mapId) ++axis;
    int32_t xAxis = 0;
    int32_t zAxis = 2;
    bool invertX = false;
    bool invertZ = false;
    if (axis < 7) {
        xAxis = mapAxisRecords[axis].xAxis;
        zAxis = mapAxisRecords[axis].zAxis;
        invertX = mapAxisRecords[axis].invertX < 0;
        invertZ = mapAxisRecords[axis].invertZ < 0;
    }
    const soft3d::Fixed coordinates[] = {position.x, position.y, position.z};
    soft3d::Fixed xRatio = soft3d::FixedDivide(
        soft3d::SubtractFixed(coordinates[xAxis], bounds[xAxis + 3]),
        soft3d::SubtractFixed(bounds[xAxis], bounds[xAxis + 3]));
    soft3d::Fixed zRatio = soft3d::FixedDivide(
        soft3d::SubtractFixed(coordinates[zAxis], bounds[zAxis + 3]),
        soft3d::SubtractFixed(bounds[zAxis], bounds[zAxis + 3]));
    if (invertX) xRatio = soft3d::SubtractFixed(soft3d::fixedOne, xRatio);
    if (invertZ) zRatio = soft3d::SubtractFixed(soft3d::fixedOne, zRatio);
    return {
        calibration.width - (soft3d::FixedMultiply(zRatio,
            static_cast<soft3d::Fixed>(calibration.width) << 16) >> 16) + calibration.offsetX,
        (soft3d::FixedMultiply(xRatio, static_cast<soft3d::Fixed>(calibration.height) << 16) >> 16) +
            calibration.offsetY
    };
}

void GameUiController::RenderMapMarkers()
{
    const auto& states = scene.GetUnitManager()->GetDoorStates();
    for (std::size_t index = 0; index < mapMarkerRecords.size(); ++index) {
        const MapMarkerRecord& marker = mapMarkerRecords[index];
        if ((marker.x == 0 && marker.y == 0) || marker.floor != ui->GetMapFloor() ||
            (scene.GetDay() == 6 && marker.day != 5) || states[index] <= 0) {
            continue;
        }
        soft3d::TextureRes* resource = ui->GetGameplayTexture(UiDoorMarker1 + marker.icon);
        if (resource == nullptr || resource->GetTexture() == nullptr) continue;
        const soft3d::Texture* texture = resource->GetTexture();
        const int32_t width = static_cast<int32_t>(texture->GetWidth()) / 3;
        const int32_t left = (states[index] - 1) * width;
        engine.GetScreenBlitter().BlitUnscaled(texture, marker.x - ui->GetMapOffsetX(),
            marker.y - ui->GetMapOffsetY(),
            {left, 0, left + width, static_cast<int32_t>(texture->GetHeight())});
    }
}

void GameUiController::RenderMap(int32_t mapId)
{
    if (IsUnavailableMap(scene.GetDay(), mapId)) {
        RenderUnavailableMap();
        return;
    }
    InitializeMapView(mapId);
    LoadMapTexture(mapId);
    if (mapTexture == nullptr || mapTexture->GetTexture() == nullptr) return;
    const soft3d::Texture* texture = mapTexture->GetTexture();
    int32_t maximumX = static_cast<int32_t>(texture->GetWidth()) - engine.GetWidth();
    int32_t maximumY = static_cast<int32_t>(texture->GetHeight()) - engine.GetHeight();
    soft3d::TextureRes* bar = ui->GetGameplayTexture(UiHintBar);
    if (bar != nullptr && bar->GetTexture() != nullptr && scene.GetDay() != 6) {
        maximumY += static_cast<int32_t>(bar->GetTexture()->GetHeight());
    }
    ui->SetMapView(ui->GetMapFloor(), std::clamp(ui->GetMapOffsetX(), 0, std::max(0, maximumX)),
                   std::clamp(ui->GetMapOffsetY(), 0, std::max(0, maximumY)));
    engine.GetScreenBlitter().BlitUnscaled(texture, -ui->GetMapOffsetX(), -ui->GetMapOffsetY(),
                                           FullImage(texture));
    RenderMapMarkers();
    const int32_t currentFloor = mapId / 1000;
    if (ui->GetMapFloor() == currentFloor ||
        ((mapId == 1030 || mapId == 1052) && ui->GetMapFloor() == 2)) {
        int32_t effectiveMap = mapId == 1030 ? 2030 : mapId == 1052 ? 2048 : mapId;
        const soft3d::Point point = ProjectMapPosition(scene.GetPlayer()->GetPosition(), effectiveMap);
        const soft3d::Vec3 forward = scene.GetPlayer()->GetForward();
        const soft3d::Vec2 flat{forward.x, forward.z};
        mapDirection = 0;
        for (int32_t index = 0; index < 8; ++index) {
            const soft3d::Vec2 direction{soft3d::FixedSin(static_cast<uint16_t>(index * 0x400)),
                                         soft3d::FixedCos(static_cast<int16_t>(index * 0x400))};
            const soft3d::Fixed dot = soft3d::AddFixed(soft3d::FixedMultiply(direction.x, flat.x),
                                                       soft3d::FixedMultiply(direction.y, flat.y));
            const soft3d::Fixed lengths = soft3d::FixedMultiply(soft3d::FixedSqrt(
                soft3d::AddFixed(soft3d::FixedMultiply(flat.x, flat.x),
                                 soft3d::FixedMultiply(flat.y, flat.y))), soft3d::fixedOne);
            if (soft3d::FixedArcCosine(soft3d::FixedDivide(dot, lengths)) < 0x200) {
                mapDirection = index;
                break;
            }
        }
        if (ui->mapBlinkDirection == 0) {
            if (--ui->mapBlinkFrame < 1) ui->mapBlinkDirection = 1;
        } else if (++ui->mapBlinkFrame > 10) {
            ui->mapBlinkDirection = 0;
        }
        soft3d::TextureRes* here = ui->GetGameplayTexture(UiHere);
        soft3d::TextureRes* alpha = ui->GetGameplayTexture(UiHereAlpha);
        if (here != nullptr && here->GetTexture() != nullptr) {
            const int32_t cell = static_cast<int32_t>(here->GetTexture()->GetWidth()) / 3;
            const int32_t frame = ((ui->mapBlinkFrame >> 2) % 3) * cell;
            engine.GetScreenBlitter().BlitUnscaled(here->GetTexture(),
                point.x - ui->GetMapOffsetX() - cell / 2,
                point.y - ui->GetMapOffsetY() - cell / 2,
                {frame, mapDirection * cell, frame + cell, mapDirection * cell + cell}, 0,
                alpha != nullptr ? alpha->GetTexture() : nullptr);
        }
    }
    soft3d::TextureRes* door = ui->GetGameplayTexture(UiDoorId);
    if (door != nullptr && door->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(door->GetTexture(), 0, 209, FullImage(door->GetTexture()), 4);
    }
    soft3d::TextureRes* floor = ui->GetGameplayTexture(UiFloorBasement + ui->GetMapFloor());
    if (floor != nullptr && floor->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(floor->GetTexture(), 10, 14, FullImage(floor->GetTexture()));
    }
    if (bar != nullptr && bar->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(bar->GetTexture(),
            0, engine.GetHeight() - static_cast<int32_t>(bar->GetTexture()->GetHeight()),
            FullImage(bar->GetTexture()));
    }
    soft3d::TextureRes* right = ui->GetGameplayTexture(UiHintLeft);
    if (right != nullptr && right->GetTexture() != nullptr) {
        engine.GetScreenBlitter().BlitUnscaled(right->GetTexture(),
            engine.GetWidth() - static_cast<int32_t>(right->GetTexture()->GetWidth()) - 2,
            engine.GetHeight() - static_cast<int32_t>(right->GetTexture()->GetHeight()) - 2,
            FullImage(right->GetTexture()));
    }
    soft3d::ControlMap& controls = engine.GetControlMap();
    int32_t x = ui->GetMapOffsetX();
    int32_t y = ui->GetMapOffsetY();
    if (controls.ActionDown(engine.GetInput(), 3, false)) x -= 10;
    if (controls.ActionDown(engine.GetInput(), 4, false)) x += 10;
    if (controls.ActionDown(engine.GetInput(), 1, false)) y -= 10;
    if (controls.ActionDown(engine.GetInput(), 2, false)) y += 10;
    ui->SetMapView(ui->GetMapFloor(), std::clamp(x, 0, std::max(0, maximumX)),
                   std::clamp(y, 0, std::max(0, maximumY)));
    if (engine.GetInput().IsKeyClick(soft3d::Key::Accept) ||
        engine.GetInput().IsKeyClick(soft3d::Key::Number6)) {
        int32_t newFloor = ui->GetMapFloor() + 1;
        if (newFloor > 3) newFloor = scene.GetDay() == 5 ? 0 : 1;
        ui->SetMapView(newFloor, ui->GetMapOffsetX(), ui->GetMapOffsetY());
        if (newFloor == currentFloor) {
            const soft3d::Point point = ProjectMapPosition(scene.GetPlayer()->GetPosition(), mapId);
            ui->SetMapView(newFloor, point.x - engine.GetWidth() / 2,
                           point.y - engine.GetHeight() / 2);
        }
    }
}

soft3d::Object& GameUiController::PrepareObject(int32_t itemId, int32_t cacheLevel)
{
    if (itemId < 0 || itemId >= static_cast<int32_t>(itemObjects.size())) {
        throw std::out_of_range("GameUI item model index");
    }
    if (itemObjects[static_cast<std::size_t>(itemId)] == nullptr) {
        itemObjects[static_cast<std::size_t>(itemId)] =
            std::make_unique<soft3d::Object>(&engine.GetRenderer());
    }
    soft3d::Object& object = *itemObjects[static_cast<std::size_t>(itemId)];
    const std::u16string& name = engine.GetContent().items[itemId].modelName;
    object.Load(&engine.GetResourceManager(), name.c_str(), cacheLevel);
    object.LoadMaterials(&engine.GetResourceManager(), cacheLevel);
    object.RotateY(0xcc);
    object.UpdateTransform();
    soft3d::Matrix4 matrix = soft3d::Multiply(object.GetModelMatrix(), RotateXMatrix(0xfc00));
    object.SetModelMatrix(matrix);
    return object;
}

soft3d::Object* GameUiController::GetItemObject(std::size_t index) const
{
    return index < itemObjects.size() ? itemObjects[index].get() : nullptr;
}

void GameUiController::RenderAboutModel(int32_t modelIndex, int32_t direction,
                                        int32_t transition, int32_t zoom)
{
    engine.GetRenderer().Clear(0);
    if (modelIndex < 0 || modelIndex >= static_cast<int32_t>(itemObjects.size())) return;
    engine.GetRenderer().SetViewMatrix(LookAt({0, 0, zoom}, {}, {0, soft3d::fixedOne, 0}));
    soft3d::Object& object = PrepareObject(modelIndex, 1);
    soft3d::Matrix4 matrix = object.GetModelMatrix();
    if (transition != 0) {
        soft3d::Fixed x = 0;
        if (transition < 9) {
            x = soft3d::FixedMultiply(soft3d::FixedMultiply(transition << 16,
                soft3d::FixedDivide(zoom, -0xa0000)), direction << 16);
        } else {
            x = soft3d::FixedMultiply(soft3d::FixedMultiply((16 - transition) << 16,
                soft3d::FixedDivide(zoom, -0xa0000)), soft3d::NegateFixed(direction << 16));
        }
        TranslateMatrix(matrix, x, 0, 0);
        object.SetModelMatrix(matrix);
    }
    engine.GetRenderQueues().Submit(&object, 3);
}

void GameUiController::PrepareModels(int32_t mode, int32_t selection)
{
    if (mode != 11) engine.GetRenderer().Clear(0);
    if (mode == 6) {
        const int32_t itemId = engine.GetContent().inventory[selection];
        if (itemId <= 0) return;
        engine.GetRenderer().SetViewMatrix(LookAt({0, 0, -8 * soft3d::fixedOne}, {},
                                                  {0, soft3d::fixedOne, 0}));
        soft3d::Object& object = PrepareObject(itemId, 3);
        soft3d::Matrix4 matrix = object.GetModelMatrix();
        TranslateMatrix(matrix, 0x18000, 0x10000, 0);
        object.SetModelMatrix(matrix);
        engine.GetRenderQueues().Submit(&object, 3);
        return;
    }
    if (mode != 1 && mode != 2 && mode != 10 && mode != 14) return;
    engine.GetRenderer().SetViewMatrix(LookAt({0, 0, -18 * soft3d::fixedOne}, {},
                                              {0, soft3d::fixedOne, 0}));
    for (int32_t slot = 0; slot < 9; ++slot) {
        const int32_t itemId = engine.GetContent().inventory[slot];
        if (itemId <= 0) continue;
        soft3d::Object& object = PrepareObject(itemId, 2);
        if (selection == slot && mode != 1) object.RotateY(0xcc);
        object.UpdateTransform();
        soft3d::Matrix4 matrix = soft3d::Multiply(object.GetModelMatrix(), RotateXMatrix(0xfc00));
        const soft3d::Fixed x = soft3d::SubtractFixed(
            soft3d::FixedMultiply(0x44ccc, (slot % 3) << 16), 0xcccc);
        const soft3d::Fixed y = soft3d::AddFixed(
            soft3d::FixedMultiply(-0x38000, (slot / 3) << 16), 0x58000);
        TranslateMatrix(matrix, x, y, 0);
        object.SetModelMatrix(matrix);
        engine.GetRenderQueues().Submit(&object, 3);
    }
}

} // namespace game

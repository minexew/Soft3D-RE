#pragma once

#include "soft3d/Dialog.hpp"

#include <array>

namespace game { class GameUiController; }

namespace soft3d {

class GameEngine;
class TextureRes;

enum GameUiTexture {
    UiPromptPick,
    UiPromptDoor,
    UiPromptRecord,
    UiPromptAct,
    UiPromptAvoid,
    UiPromptTalk,
    UiQteA1,
    UiQteB1,
    UiQteA2,
    UiQteB2,
    UiQteA3,
    UiQteB3,
    UiQteA4,
    UiQteB4,
    UiQteAlpha1,
    UiQteAlpha2,
    UiQteAlpha3,
    UiQteAlpha4,
    UiQteOver1,
    UiQteOver2,
    UiQteOver3,
    UiQteOver4,
    UiQteGuide1,
    UiQteGuide2,
    UiQteHint1,
    UiQteHint2,
    UiHintBar,
    UiHintLeft,
    UiHintRight,
    UiFloorBasement,
    UiFloor1,
    UiFloor2,
    UiFloor3,
    UiDoorId,
    UiStoryEvent,
    UiStoryChoice,
    UiStoryTimed,
    UiPortrait,
    UiHere,
    UiHereAlpha,
    UiDigits,
    UiDoorMarker1,
    UiDoorMarker2,
    UiGunFill,
    UiGunBackground0,
    UiGunBackground1,
    UiGunLevel,
    UiTextureCount
};

struct GameUiMapMarker {
    int32_t x = -1;
    int32_t y = -1;
    int32_t type = 0;
};

class GameUIDlg : public Dialog {
public:
    GameUIDlg(GameEngine& engine, UiManager* manager);

    void Initialize();
    void ResetLevelState();
    void LoadDayResources(int day);
    void OpenMenu();
    void OpenMap();
    void OpenSaveMenu();
    void CompleteGame();
    void OpenDocument(int32_t noteSlot);
    void OpenItemDescription(int32_t itemSlot);
    void CloseMenu();
    void PrepareModels();
    int Render(uint32_t context) override;
    int GetMode() const;
    int GetSelection() const;
    bool IsMenuOpen() const;

    void SetInteractionPrompt(int32_t value) { interactionPrompt = value; }
    int32_t GetInteractionPrompt() const { return interactionPrompt; }
    int32_t GetInteractionPromptFrame() const { return interactionPromptFrame; }
    bool IsDialogActive() const { return dialogActive != 0; }
    int32_t GetDialogKind() const { return dialogKind; }
    int32_t GetDialogTextId() const { return dialogTextId; }
    int32_t GetDialogSecondaryTextId() const { return dialogSecondaryTextId; }
    int32_t GetDialogOption(int32_t index) const;
    int32_t GetDialogOptionCount() const { return dialogOptionCount; }
    int32_t GetDialogSelection() const { return dialogSelection; }
    int32_t GetDialogTextLine() const { return dialogTextLine; }
    int32_t GetDialogLineCount() const { return dialogLineCount; }
    int32_t GetDialogTimer() const { return dialogTimer; }
    bool IsDialogPortrait() const { return dialogPortrait != 0; }
    void CloseDialog();
    void ClearDialogResult() { dialogResult = -1; }
    int32_t GetDialogResult() const { return dialogResult; }
    void SetDialogResult(int32_t result) { dialogResult = result; }
    void ShowEventMessage(int32_t textId);
    void ShowSimpleMessage(int32_t textId);
    void ShowTimedMessage(int32_t textId, int32_t frames);
    void ShowTimedResultMessage(int32_t textId, int32_t frames);
    void SetDialogString(int32_t index, int32_t textId);
    void ShowChoiceDialog(int32_t titleId, int32_t optionCount, bool portrait);

    void SetSubtitle(bool active, int32_t textId);
    bool IsSubtitleActive() const { return subtitleActive != 0; }
    int32_t GetSubtitleTextId() const { return subtitleTextId; }
    void SetHideMask(int32_t index, bool active);
    int32_t GetHideMask() const { return hideMaskIndex; }
    bool IsHideMaskActive() const { return hideMaskActive != 0; }
    void SetGunHudEnabled(int32_t enabled) { gunHudEnabled = enabled; }
    int32_t IsGunHudEnabled() const { return gunHudEnabled; }

    void SetMapMarker(int32_t index, int32_t type, int32_t x, int32_t y);
    void ClearMapMarker(int32_t index);
    const GameUiMapMarker& GetMapMarker(int32_t index) const;
    int32_t GetMapFloor() const { return mapFloor; }
    int32_t GetMapOffsetX() const { return mapOffsetX; }
    int32_t GetMapOffsetY() const { return mapOffsetY; }
    void SetMapView(int32_t floor, int32_t x, int32_t y);
    bool ConsumeMapCentering();
    bool IsDirectMap() const { return directMap != 0; }

    TextureRes* GetGameplayTexture(int32_t index) const;
    int32_t GetPortraitTextWidth() const { return portraitTextWidth; }
    int32_t GetSimpleTextWidth() const { return simpleTextWidth; }

private:
    void LoadResources();
    void RenderMainMenu();
    void RenderInventory(bool combining);
    void RenderFiles();
    void RenderOptions();
    void RenderItemActions();
    void RenderItemDescription();
    void RenderSaveMenu();
    void DrawItemText(int slot, bool description);
    int32_t DrawSmallNumber(int32_t value, int32_t x, int32_t y);
    void BeginDialog(int32_t kind, int32_t textId, int32_t width);
    void SynchronizeContent();
    void SetPaused(bool paused);

    GameEngine& engine;
    std::vector<TextureRes*> textures;
    std::array<int32_t, 4> dialogOptions{};
    std::array<GameUiMapMarker, 8> mapMarkers{};
    int32_t mode = -1;
    int32_t selection = 0;
    int32_t returnSelection = 0;
    int32_t optionSelection = 0;
    int32_t itemAction = 0;
    int32_t combineSource = -1;
    int32_t documentIndex = 0;
    int32_t documentLineCount = 0;
    int32_t scroll = 0;
    int32_t errorTimer = 0;
    int32_t saveStatus = 0;
    int32_t interactionPrompt = -1;
    int32_t interactionPromptFrame = 0;
    int32_t dialogKind = 0;
    int32_t dialogActive = 0;
    int32_t dialogTextId = 0;
    int32_t dialogSecondaryTextId = -1;
    int32_t dialogResult = -1;
    int32_t dialogOptionCount = 0;
    int32_t dialogPortrait = 0;
    int32_t dialogSelection = 0;
    int32_t dialogTextLine = 0;
    int32_t dialogLineCount = 0;
    int32_t dialogTimer = 0;
    int32_t subtitleActive = 0;
    int32_t subtitleTextId = -1;
    int32_t hideMaskIndex = -1;
    int32_t hideMaskActive = 0;
    int32_t gunHudEnabled = 0;
    int32_t portraitTextWidth = 0;
    int32_t simpleTextWidth = 0;
    int32_t mapOffsetX = 0;
    int32_t mapOffsetY = 0;
    int32_t mapFloor = 0;
    int32_t mapBlinkDirection = 0;
    int32_t mapBlinkFrame = 0;
    int32_t directMap = 0;
    bool mapNeedsCentering = false;
    bool menuOpen = false;
    bool viewingDocument = false;

    friend class game::GameUiController;
};

} // namespace soft3d

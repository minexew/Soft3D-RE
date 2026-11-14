#pragma once

#include "soft3d/Dialog.hpp"

namespace soft3d {

class GameEngine;
class TextureRes;

class GameUIDlg : public Dialog {
public:
    GameUIDlg(GameEngine& engine, UiManager* manager);

    void Initialize();
    void OpenMenu();
    void CloseMenu();
    int Render(uint32_t context) override;
    int GetMode() const;
    int GetSelection() const;
    bool IsMenuOpen() const;

private:
    void LoadResources();
    void RenderMainMenu();
    void RenderInventory(bool combining);
    void RenderFiles();
    void RenderOptions();
    void RenderItemActions();
    void RenderItemDescription();
    void DrawItemDescription(int slot);

    GameEngine& engine;
    std::vector<TextureRes*> textures;
    int32_t mode = -1;
    int32_t selection = 0;
    int32_t returnSelection = 0;
    int32_t optionSelection = 0;
    int32_t itemAction = 0;
    int32_t combineSource = -1;
    int32_t documentIndex = 0;
    int32_t scroll = 0;
    int32_t errorTimer = 0;
    bool menuOpen = false;
    bool viewingDocument = false;
};

} // namespace soft3d

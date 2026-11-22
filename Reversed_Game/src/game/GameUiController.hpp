#pragma once

#include "game/Triggers.hpp"
#include "soft3d/Font.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/Object.hpp"

#include <array>
#include <memory>

namespace game {

class Scene;

class GameUiController final : public TriggerUiHost {
public:
    explicit GameUiController(Scene& scene);

    void InstallHooks();
    void SynchronizeContent();
    void RenderHud();
    void RenderMap(int32_t mapId);
    void RenderAboutModel(int32_t modelIndex, int32_t direction, int32_t transition,
                          int32_t zoom);
    void PrepareModels(int32_t mode, int32_t selection);
    void FinishGame();
    bool SaveGame(int32_t slot);
    bool CanOpenMenu() const;
    int32_t CountNotes() const;
    int32_t CountSecrets() const;

    void SetPrompt(int32_t prompt) override;
    void ShowMessage(int32_t textId, int32_t kind) override;
    void SetChoiceString(int32_t index, int32_t textId) override;
    void ShowChoice(int32_t titleId, int32_t optionCount, bool portrait) override;
    int32_t GetDialogResult() const override;
    bool IsDialogActive() const override;
    void CloseDialog() override;

    soft3d::Point ProjectMapPosition(const soft3d::Vec3& position, int32_t mapId) const;
    int32_t GetMapDirection() const { return mapDirection; }
    soft3d::Object* GetItemObject(std::size_t index) const;

private:
    void RenderInteractionPrompt();
    void RenderQte();
    void RenderHideMask();
    void RenderDialog();
    void RenderHealth();
    void RenderGunHud();
    void RenderSubtitle();
    void RenderUnavailableMap();
    void RenderMapMarkers();
    void InitializeMapView(int32_t mapId);
    void LoadMapTexture(int32_t mapId);
    soft3d::TextureRes* LoadTexture(const char16_t* name, int32_t cacheLevel = 3,
                                    bool english = true);
    soft3d::Object& PrepareObject(int32_t itemId, int32_t cacheLevel);

    Scene& scene;
    soft3d::GameEngine& engine;
    soft3d::GameUIDlg* ui;
    std::array<std::unique_ptr<soft3d::Object>, 100> itemObjects{};
    soft3d::TextureRes* mapTexture = nullptr;
    soft3d::TextureRes* photoTexture = nullptr;
    int32_t mapDirection = 0;
    int32_t qteFailureFrame = 0;
};

} // namespace game

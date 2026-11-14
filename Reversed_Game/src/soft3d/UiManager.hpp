#pragma once

#include "soft3d/Renderable.hpp"
#include "soft3d/ScreenBlitter.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace soft3d {

class StringManager;
class TextureRes;

struct SdtAnimationKey {
    int32_t time = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t mode = 0;
    int32_t sourceX = 0;
    int32_t sourceY = 0;
    int32_t sourceWidth = 0;
    int32_t sourceHeight = 0;
    int32_t widthDivisor = 0;
    int32_t heightDivisor = 0;
    int32_t unknown = -1;
};

struct SdtElement {
    uint32_t visualIndex = 0;
    uint32_t text = 0;
    uint32_t flags = 0;
    std::vector<SdtAnimationKey> keys;
};

struct SdtState {
    uint32_t id = 0;
    std::vector<SdtElement> elements;
};

struct SdtVisual {
    uint32_t resourceIndex = 0;
    int32_t x = 0;
    int32_t y = 0;
    TextureRes* resource = nullptr;
};

struct SdtDialog {
    std::vector<SdtVisual> visuals;
    std::vector<SdtState> states;
};

class UiManager : public Renderable, public s3dBase {
public:
    UiManager() = default;

    void SetResourceManager(s3dResManager* manager);
    void SetScreenBlitter(ScreenBlitter* blitter);
    void SetFont(StringManager* manager);
    result LoadStrSdt(const char16_t* entryName);
    result LoadStateSdt(const char16_t* entryName);
    result ParseStrSdt(const void* data, size_t size);
    result ParseStateSdt(const void* data, size_t size);

    void SelectState(uint32_t stateIndex, int32_t time);
    void Render() override;
    virtual void Update(uint32_t context);
    void SetEngine(Engine* engine) override;

    size_t GetDialogCount() const;
    const SdtDialog* GetDialog(size_t index) const;
    const std::vector<std::u16string>& GetResourceNames() const;

private:
    s3dResManager* resourceManager = nullptr;
    ScreenBlitter* screenBlitter = nullptr;
    StringManager* font = nullptr;
    std::vector<std::u16string> resourceNames;
    std::vector<std::u16string> soundNames;
    std::vector<TextureRes*> resources;
    std::vector<SdtDialog> dialogs;
    uint32_t selectedState = 7;
    int32_t stateTime = 0;
};

class Input;

class GameMenu {
public:
    void Update(UiManager& uiManager, const Input& input);
    int32_t GetSelection() const;

private:
    // Acceptance and the later logo/loading states remain to be reconstructed.
    int32_t selection = 0;
};

} // namespace soft3d

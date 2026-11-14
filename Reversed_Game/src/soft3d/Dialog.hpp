#pragma once

#include "soft3d/ScreenBlitter.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace soft3d {

class TextureRes;
class UiManager;

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
    int32_t extra = -1;
};

struct SdtElement {
    uint32_t visualIndex = 0;
    uint32_t behavior = 0;
    uint32_t secondaryTextureIndex = 0;
    std::vector<SdtAnimationKey> keys;
};

struct SdtState {
    uint32_t id = 0;
    std::vector<SdtElement> elements;
};

struct SdtVisual {
    uint32_t resourceIndex = 0;
    TextureRes* resource = nullptr;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t blitMode = 0;
    int32_t sourceX = 0;
    int32_t sourceY = 0;
    int32_t sourceWidth = 0;
    int32_t sourceHeight = 0;
    int32_t widthDivisor = 1;
    int32_t heightDivisor = 1;
    int32_t extra = -1;
    bool hidden = false;
};

struct SdtDialog {
    std::vector<SdtVisual> visuals;
    std::vector<SdtState> states;
};

class Dialog {
public:
    Dialog(UiManager* manager, uint16_t index, bool active = false);

    int SetStateTime(int32_t stateIndex, int32_t time);
    int SelectState(int32_t stateIndex);
    int ApplyState(int32_t stateIndex);
    int SetElementTime(int32_t stateIndex, int32_t elementIndex, int32_t time);
    void RenderElement(int32_t stateIndex, int32_t elementIndex);
    int RenderElementAtTime(int32_t stateIndex, int32_t elementIndex, int32_t time);
    int UpdateAndRender(uint32_t context);

    void SetActive(bool active);
    bool IsActive() const;
    uint16_t GetIndex() const;
    int32_t GetStateTime() const;
    SdtVisual* GetVisual(uint32_t index);
    const SdtVisual* GetVisual(uint32_t index) const;
    SdtDialog& GetData();
    const SdtDialog& GetData() const;

private:
    UiManager* manager;
    SdtDialog data;
    uint16_t index;
    int32_t stateTime = 0;
    bool active;
};

} // namespace soft3d

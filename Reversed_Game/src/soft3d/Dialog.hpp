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
    virtual ~Dialog() = default;

    int SetStateTime(int32_t stateIndex, int32_t time);
    void SetTime(int32_t time);
    int SelectState(int32_t stateIndex);
    int ApplyState(int32_t stateIndex);
    int SetElementTime(int32_t stateIndex, int32_t elementIndex, int32_t time);
    void RenderElement(int32_t stateIndex, int32_t elementIndex);
    int RenderElementAtTime(int32_t stateIndex, int32_t elementIndex, int32_t time);
    bool AdvanceStateTime(int32_t stateIndex);
    bool AdvanceStateTimeToward(int32_t stateIndex, int32_t start, int32_t target);
    bool AdvanceElementTime(int32_t stateIndex, int32_t elementIndex);
    int32_t GetStateEndTime(int32_t stateIndex) const;
    const SdtAnimationKey* GetKey(int32_t stateIndex, int32_t elementIndex,
                                  int32_t keyIndex) const;
    int32_t GetKeyX(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const;
    int32_t GetKeyY(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const;
    int32_t GetKeyWidth(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const;
    int32_t GetKeyHeight(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const;
    int32_t GetKeySourceHeight(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const;
    void SetKeyX(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value);
    void SetKeyY(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value);
    void SetKeyWidth(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value);
    void SetKeyHeight(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value);
    int32_t GetElementWidth(int32_t stateIndex, int32_t elementIndex, int32_t time);
    int32_t GetElementHeight(int32_t stateIndex, int32_t elementIndex, int32_t time);
    int32_t GetElementBlitMode(int32_t stateIndex, int32_t elementIndex, int32_t time);
    void SetElementCurrentTime(int32_t stateIndex, int32_t elementIndex, int32_t time);
    int32_t GetElementCurrentTime(int32_t stateIndex, int32_t elementIndex) const;
    ScreenRect GetKeyDestinationRect(int32_t stateIndex, int32_t elementIndex,
                                     int32_t keyIndex) const;
    bool BlitVisualInRect(uint32_t visualIndex, ScreenRect destination);
    bool BlitVisualAtPoint(uint32_t visualIndex, int32_t x, int32_t y);
    bool SetVisualResource(uint32_t visualIndex, const char16_t* name);
    void SetOrigin(int32_t x, int32_t y);
    int RenderNumber(uint32_t value, int32_t x, int32_t y,
                     int32_t stateIndex, int32_t elementIndex);
    virtual int HandleInput(int32_t type, int32_t key, int32_t context);
    virtual int Render(uint32_t context);
    virtual int Update(uint32_t context);
    int UpdateAndRender(uint32_t context);
    void InitializeRuntimeTimes();

    void SetActive(bool active);
    bool IsActive() const;
    void SetIndex(uint16_t index);
    uint16_t GetIndex() const;
    int32_t GetStateTime() const;
    SdtVisual* GetVisual(uint32_t index);
    const SdtVisual* GetVisual(uint32_t index) const;
    SdtDialog& GetData();
    const SdtDialog& GetData() const;
    UiManager* GetManager() const;

private:
    int32_t GetElementFieldAtTime(int32_t stateIndex, int32_t elementIndex, int32_t time,
                                  uint32_t field);

    UiManager* manager;
    SdtDialog data;
    std::vector<std::vector<int32_t>> elementTimes;
    uint16_t index;
    int32_t stateTime = 0;
    bool advanceInitialized = true;
    bool active;
    int32_t originX = 0;
    int32_t originY = 0;
};

} // namespace soft3d

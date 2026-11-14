#include "soft3d/Dialog.hpp"

#include "soft3d/TextureRes.hpp"
#include "soft3d/UiManager.hpp"
#include "soft3d/s3dIO.hpp"

#include <algorithm>

namespace soft3d {
namespace {

int16_t KeyValue(int32_t value)
{
    return static_cast<int16_t>(value);
}

int32_t Interpolate(int32_t previous, int32_t next, int32_t elapsed, int32_t duration)
{
    const int16_t delta = static_cast<int16_t>(elapsed * (KeyValue(next) - KeyValue(previous)) /
                                               duration);
    return static_cast<int16_t>(KeyValue(previous) + delta);
}

} // namespace

Dialog::Dialog(UiManager* owner, uint16_t dialogIndex, bool initialActive)
    : manager(owner)
    , index(dialogIndex)
    , active(initialActive)
{
}

int Dialog::SetStateTime(int32_t stateIndex, int32_t time)
{
    stateTime = time;
    SelectState(stateIndex);
    return 1;
}

void Dialog::SetTime(int32_t time)
{
    stateTime = time;
}

int Dialog::SelectState(int32_t stateIndex)
{
    if (ApplyState(stateIndex) == 0) {
        return 0;
    }
    const SdtState& state = data.states[static_cast<size_t>(stateIndex)];
    for (size_t elementIndex = 0; elementIndex < state.elements.size(); ++elementIndex) {
        if (KeyValue(state.elements[elementIndex].behavior) == 0) {
            RenderElement(stateIndex, static_cast<int32_t>(elementIndex));
        }
    }
    return 1;
}

int Dialog::ApplyState(int32_t stateIndex)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size()) {
        return 0;
    }
    const SdtState& state = data.states[static_cast<size_t>(stateIndex)];
    for (size_t elementIndex = 0; elementIndex < state.elements.size(); ++elementIndex) {
        if (KeyValue(state.elements[elementIndex].behavior) == 0) {
            SetElementTime(stateIndex, static_cast<int32_t>(elementIndex), stateTime);
        }
    }
    return 1;
}

int Dialog::SetElementTime(int32_t stateIndex, int32_t elementIndex, int32_t time)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        elementIndex < 0) {
        return 0;
    }
    SdtState& state = data.states[static_cast<size_t>(stateIndex)];
    if (static_cast<size_t>(elementIndex) >= state.elements.size()) {
        return 0;
    }
    SdtElement& element = state.elements[static_cast<size_t>(elementIndex)];
    SdtVisual* visual = GetVisual(element.visualIndex);
    if (visual == nullptr) {
        return 0;
    }

    for (size_t keyIndex = 0; keyIndex < element.keys.size(); ++keyIndex) {
        const SdtAnimationKey& next = element.keys[keyIndex];
        const int16_t nextTime = KeyValue(next.time);
        if (nextTime == time) {
            visual->x = KeyValue(next.x);
            visual->y = KeyValue(next.y);
            visual->width = KeyValue(next.width);
            visual->height = KeyValue(next.height);
            visual->blitMode = KeyValue(next.mode);
            visual->hidden = false;
            visual->sourceX = KeyValue(next.sourceX);
            visual->sourceY = KeyValue(next.sourceY);
            visual->sourceWidth = KeyValue(next.sourceWidth);
            visual->sourceHeight = KeyValue(next.sourceHeight);
            visual->widthDivisor = KeyValue(next.widthDivisor);
            visual->heightDivisor = KeyValue(next.heightDivisor);
            visual->extra = KeyValue(next.extra);
            return 1;
        }
        if (time >= nextTime) {
            continue;
        }
        if (keyIndex == 0) {
            visual->hidden = true;
            return 0;
        }

        const SdtAnimationKey& previous = element.keys[keyIndex - 1];
        const int16_t previousTime = KeyValue(previous.time);
        const int16_t elapsed = static_cast<int16_t>(time) - previousTime;
        const int16_t duration = nextTime - previousTime;
        visual->widthDivisor = KeyValue(previous.widthDivisor);
        visual->heightDivisor = KeyValue(previous.heightDivisor);
        visual->x = Interpolate(previous.x, next.x, elapsed, duration);
        visual->y = Interpolate(previous.y, next.y, elapsed, duration);
        visual->width = Interpolate(previous.width, next.width, elapsed, duration);
        visual->height = Interpolate(previous.height, next.height, elapsed, duration);
        visual->blitMode = KeyValue(previous.mode);
        visual->sourceX = Interpolate(previous.sourceX, next.sourceX, elapsed, duration);
        visual->sourceY = Interpolate(previous.sourceY, next.sourceY, elapsed, duration);
        visual->sourceWidth = Interpolate(previous.sourceWidth, next.sourceWidth, elapsed, duration);
        visual->sourceHeight = Interpolate(previous.sourceHeight, next.sourceHeight, elapsed, duration);
        visual->extra = -1;
        visual->hidden = false;
        return 1;
    }
    visual->hidden = true;
    return 1;
}

void Dialog::RenderElement(int32_t stateIndex, int32_t elementIndex)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        elementIndex < 0) {
        return;
    }
    const SdtState& state = data.states[static_cast<size_t>(stateIndex)];
    if (static_cast<size_t>(elementIndex) >= state.elements.size()) {
        return;
    }
    const SdtElement& element = state.elements[static_cast<size_t>(elementIndex)];
    const SdtVisual* visual = GetVisual(element.visualIndex);
    if (visual == nullptr || visual->hidden || visual->resource == nullptr ||
        visual->widthDivisor == 0 || visual->heightDivisor == 0) {
        return;
    }
    const TextureRes* secondaryResource = element.secondaryTextureIndex != 0
        ? manager->GetResourceByIndex(element.secondaryTextureIndex)
        : nullptr;
    const Texture* secondary = secondaryResource != nullptr ? secondaryResource->GetTexture() : nullptr;
    ScreenBlitter* blitter = manager->GetScreenBlitter();
    if (blitter == nullptr) {
        return;
    }
    const ScreenRect source{visual->sourceX, visual->sourceY,
        visual->sourceX + visual->sourceWidth, visual->sourceY + visual->sourceHeight};
    const ScreenRect destination{visual->x, visual->y,
        visual->x + visual->sourceWidth * visual->width / visual->widthDivisor,
        visual->y + visual->sourceHeight * visual->height / visual->heightDivisor};
    blitter->Blit(visual->resource->GetTexture(), destination, source,
                  static_cast<uint32_t>(visual->blitMode), secondary);
}

int Dialog::RenderElementAtTime(int32_t stateIndex, int32_t elementIndex, int32_t time)
{
    SetElementTime(stateIndex, elementIndex, time);
    RenderElement(stateIndex, elementIndex);
    return 1;
}

bool Dialog::AdvanceStateTime(int32_t stateIndex)
{
    SelectState(stateIndex);
    ++stateTime;
    if (stateTime < GetStateEndTime(stateIndex)) {
        return false;
    }
    stateTime = 0;
    return true;
}

bool Dialog::AdvanceStateTimeToward(int32_t stateIndex, int32_t start, int32_t target)
{
    if (advanceInitialized) {
        stateTime = start;
        advanceInitialized = false;
    }
    SelectState(stateIndex);
    if (start < target) {
        ++stateTime;
        if (stateTime <= target) {
            return false;
        }
    } else {
        --stateTime;
        if (stateTime >= target) {
            return false;
        }
    }
    stateTime = start;
    advanceInitialized = true;
    return true;
}

bool Dialog::AdvanceElementTime(int32_t stateIndex, int32_t elementIndex)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        static_cast<size_t>(stateIndex) >= elementTimes.size() || elementIndex < 0 ||
        static_cast<size_t>(elementIndex) >= elementTimes[stateIndex].size()) {
        return true;
    }
    int32_t& time = elementTimes[stateIndex][elementIndex];
    RenderElementAtTime(stateIndex, elementIndex, time);
    const SdtElement& element = data.states[stateIndex].elements[elementIndex];
    const int32_t endTime = element.keys.empty() ? 0 : KeyValue(element.keys.back().time);
    ++time;
    if (time <= endTime) {
        return false;
    }
    time = 0;
    return true;
}

int32_t Dialog::GetStateEndTime(int32_t stateIndex) const
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size()) {
        return 0;
    }
    int32_t result = 0;
    for (const SdtElement& element : data.states[stateIndex].elements) {
        if (!element.keys.empty()) {
            result = std::max(result, static_cast<int32_t>(KeyValue(element.keys.back().time)));
        }
    }
    return result;
}

const SdtAnimationKey* Dialog::GetKey(int32_t stateIndex, int32_t elementIndex,
                                      int32_t keyIndex) const
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        elementIndex < 0 || keyIndex < 0) {
        return nullptr;
    }
    const SdtState& state = data.states[stateIndex];
    if (static_cast<size_t>(elementIndex) >= state.elements.size()) {
        return nullptr;
    }
    const SdtElement& element = state.elements[elementIndex];
    return static_cast<size_t>(keyIndex) < element.keys.size() ? &element.keys[keyIndex] : nullptr;
}

int32_t Dialog::GetKeyX(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const
{
    const SdtAnimationKey* key = GetKey(stateIndex, elementIndex, keyIndex);
    return key != nullptr ? KeyValue(key->x) : 0;
}

int32_t Dialog::GetKeyY(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const
{
    const SdtAnimationKey* key = GetKey(stateIndex, elementIndex, keyIndex);
    return key != nullptr ? KeyValue(key->y) : 0;
}

int32_t Dialog::GetKeyWidth(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const
{
    const SdtAnimationKey* key = GetKey(stateIndex, elementIndex, keyIndex);
    return key != nullptr ? KeyValue(key->width) : 0;
}

int32_t Dialog::GetKeyHeight(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex) const
{
    const SdtAnimationKey* key = GetKey(stateIndex, elementIndex, keyIndex);
    return key != nullptr ? KeyValue(key->height) : 0;
}

void Dialog::SetKeyX(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        elementIndex < 0 || keyIndex < 0) {
        return;
    }
    SdtState& state = data.states[stateIndex];
    if (static_cast<size_t>(elementIndex) >= state.elements.size()) {
        return;
    }
    SdtElement& element = state.elements[elementIndex];
    if (static_cast<size_t>(keyIndex) < element.keys.size()) {
        element.keys[keyIndex].x = value;
    }
}

void Dialog::SetKeyY(int32_t stateIndex, int32_t elementIndex, int32_t keyIndex, int32_t value)
{
    if (stateIndex < 0 || static_cast<size_t>(stateIndex) >= data.states.size() ||
        elementIndex < 0 || keyIndex < 0) {
        return;
    }
    SdtState& state = data.states[stateIndex];
    if (static_cast<size_t>(elementIndex) >= state.elements.size()) {
        return;
    }
    SdtElement& element = state.elements[elementIndex];
    if (static_cast<size_t>(keyIndex) < element.keys.size()) {
        element.keys[keyIndex].y = value;
    }
}

ScreenRect Dialog::GetKeyDestinationRect(int32_t stateIndex, int32_t elementIndex,
                                         int32_t keyIndex) const
{
    const int32_t x = GetKeyX(stateIndex, elementIndex, keyIndex);
    const int32_t y = GetKeyY(stateIndex, elementIndex, keyIndex);
    return {x, y, x + GetKeyWidth(stateIndex, elementIndex, keyIndex),
            y + GetKeyHeight(stateIndex, elementIndex, keyIndex)};
}

int Dialog::RenderNumber(uint32_t value, int32_t x, int32_t y,
                         int32_t stateIndex, int32_t elementIndex)
{
    const int32_t signedValue = static_cast<int32_t>(value);
    const uint32_t magnitude = signedValue < 0
        ? 0U - static_cast<uint32_t>(signedValue) : static_cast<uint32_t>(signedValue);
    if (magnitude > 100000) {
        return 0;
    }
    const SdtAnimationKey* widthKey = GetKey(stateIndex, elementIndex, 1);
    if (widthKey == nullptr) {
        return 0;
    }
    char16_t text[10];
    WideSprintf(text, u"%d", signedValue);
    const int32_t digitWidth = KeyValue(widthKey->sourceWidth);
    int32_t length = 0;
    while (text[length] != u'\0') {
        ++length;
    }
    for (int32_t index = length - 1; index >= 0; --index) {
        const int32_t digit = static_cast<int32_t>(text[index]) - 0x30;
        if (digit >= 0) {
            SetKeyX(stateIndex, elementIndex, digit + 1, x);
            SetKeyY(stateIndex, elementIndex, digit + 1, y);
            RenderElementAtTime(stateIndex, elementIndex, digit + 1);
            x -= digitWidth;
        }
    }
    return length * digitWidth;
}

int Dialog::HandleInput(int32_t type, int32_t key, int32_t context)
{
    (void)type;
    (void)key;
    (void)context;
    return 0;
}

int Dialog::Render(uint32_t context)
{
    (void)context;
    return 1;
}

int Dialog::Update(uint32_t context)
{
    (void)context;
    return 1;
}

int Dialog::UpdateAndRender(uint32_t context)
{
    Render(context);
    Update(context);
    return 1;
}

void Dialog::InitializeRuntimeTimes()
{
    elementTimes.clear();
    elementTimes.reserve(data.states.size());
    for (const SdtState& state : data.states) {
        elementTimes.emplace_back(state.elements.size(), 0);
    }
}

void Dialog::SetActive(bool value)
{
    active = value;
}

bool Dialog::IsActive() const
{
    return active;
}

uint16_t Dialog::GetIndex() const
{
    return index;
}

int32_t Dialog::GetStateTime() const
{
    return stateTime;
}

SdtVisual* Dialog::GetVisual(uint32_t visualIndex)
{
    return visualIndex < data.visuals.size() ? &data.visuals[visualIndex] : nullptr;
}

const SdtVisual* Dialog::GetVisual(uint32_t visualIndex) const
{
    return visualIndex < data.visuals.size() ? &data.visuals[visualIndex] : nullptr;
}

SdtDialog& Dialog::GetData()
{
    return data;
}

const SdtDialog& Dialog::GetData() const
{
    return data;
}

UiManager* Dialog::GetManager() const
{
    return manager;
}

} // namespace soft3d

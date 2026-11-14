#include "soft3d/Dialog.hpp"

#include "soft3d/TextureRes.hpp"
#include "soft3d/UiManager.hpp"

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
    manager->GetScreenBlitter()->Blit(visual->resource->GetTexture(), destination, source,
                                      static_cast<uint32_t>(visual->blitMode), secondary);
}

int Dialog::RenderElementAtTime(int32_t stateIndex, int32_t elementIndex, int32_t time)
{
    SetElementTime(stateIndex, elementIndex, time);
    RenderElement(stateIndex, elementIndex);
    return 1;
}

int Dialog::UpdateAndRender(uint32_t context)
{
    (void)context;
    return 1;
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

} // namespace soft3d

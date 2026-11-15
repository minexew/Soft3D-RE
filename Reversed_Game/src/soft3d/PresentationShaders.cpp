#include "soft3d/PresentationShaders.hpp"

#include "soft3d/Engine.hpp"
#include "soft3d/Fixed.hpp"
#include "soft3d/Renderer.hpp"

#include <algorithm>
#include <cstddef>

namespace soft3d {
namespace {

uint16_t PackRgb565(uint32_t red, uint32_t green, uint32_t blue)
{
    red &= 0xff;
    green &= 0xff;
    blue &= 0xff;
    return static_cast<uint16_t>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

uint32_t SubtractInk(uint32_t pixel, uint16_t ink)
{
    const int blue = static_cast<int>(pixel & 0xff) - static_cast<int>(ink & 0x1f) * 8;
    const int green = static_cast<int>((pixel & 0xff00) >> 8) -
                      static_cast<int>((ink & 0x07e0) >> 3);
    const int red = static_cast<int>((pixel & 0xff0000) >> 16) -
                    static_cast<int>((ink & 0xf800) >> 8);
    return static_cast<uint32_t>(std::max(red, 0)) << 16 |
           static_cast<uint32_t>(std::max(green, 0)) << 8 |
           static_cast<uint32_t>(std::max(blue, 0));
}

} // namespace

MotionBlurShader::MotionBlurShader(int shaderWidth, int shaderHeight)
    : BasicShader(shaderWidth, shaderHeight)
    , history(static_cast<std::size_t>(shaderWidth) * shaderHeight)
{
}

void MotionBlurShader::ResetHistory()
{
    resetHistory = 1;
}

void MotionBlurShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    const int direction = enabled == 1 ? -1 : 1;
    const int stride = direction * height;
    uint32_t* output = destination + (enabled == 1 ? height * (width - 1) : height - 1);
    uint32_t* historyOutput = history.data() +
        (enabled == 1 ? height * (width - 1) : height - 1);
    if (resetHistory != 0) {
        uint32_t* resetOutput = output;
        uint32_t* resetHistoryOutput = historyOutput;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                *resetHistoryOutput = *resetOutput;
                resetOutput += stride;
                resetHistoryOutput += stride;
            }
            resetOutput += -stride * width - direction;
            resetHistoryOutput += -stride * width - direction;
        }
        resetHistory = 0;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = input[0];
            const uint32_t previous = *historyOutput;
            const uint32_t red = std::min(255U, ((pixel & 0xf800U) >> 10) +
                (((previous & 0xff0000U) >> 16) * 7 >> 3));
            const uint32_t green = std::min(255U, (((pixel & 0x07e0U) >> 5) << 2) / 4 +
                (((previous & 0x00ff00U) >> 8) * 7 >> 3));
            const uint32_t blue = std::min(255U, ((pixel & 0x001fU) << 3) / 4 +
                ((previous & 0x0000ffU) * 7 >> 3));
            *historyOutput = (red << 16) | (green << 8) | blue;
            *output = *historyOutput;
            input += 2;
            output += stride;
            historyOutput += stride;
        }
        output += -stride * width - direction;
        historyOutput += -stride * width - direction;
    }
}

void GrayShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    const int direction = enabled == 1 ? -1 : 1;
    const int stride = direction * height;
    destination += enabled == 1 ? height * (width - 1) : height - 1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = input[0];
            const uint16_t depth = input[1];
            if (depth == 0x6fff || depth == 0x7fff) {
                *destination = ExpandRgb565(pixel);
            } else {
                const uint32_t gray = pixel & 0x1f;
                *destination = (gray << 19) | (gray << 11) | (gray << 3);
            }
            input += 2;
            destination += stride;
        }
        destination += -stride * width - direction;
    }
}

FadeShader::FadeShader(int shaderWidth, int shaderHeight, Renderer* shaderRenderer)
    : BasicShader(shaderWidth, shaderHeight)
    , renderer(shaderRenderer)
{
}

void FadeShader::StartFade(int newFadeOut, uint16_t newColor, int newFrameCount, int newMode)
{
    frameCount = newFrameCount;
    fadeOut = newFadeOut;
    color = newColor;
    active = 1;
    completed = 0;
    frame = 0;
    mode = newMode;
    if (renderer != nullptr) {
        renderer->SelectShader(this);
    }
}

bool FadeShader::IsCompleted() const
{
    return completed != 0;
}

bool FadeShader::IsActive() const
{
    return active != 0;
}

void FadeShader::Complete()
{
    completed = 1;
    active = 0;
    if (fadeOut == 0 && renderer != nullptr) {
        renderer->SelectShader(nullptr);
    }
}

void FadeShader::Render(const void* source, uint32_t* destination)
{
    if (active == 0) {
        return;
    }
    if (frame == frameCount) {
        if (mode == 0) {
            completed = 1;
        } else {
            Complete();
        }
    } else {
        ++frame;
    }
    int alpha = frame * 31 / frameCount;
    if (fadeOut == 0) {
        alpha = 32 - alpha;
    }
    const uint8_t blend = static_cast<uint8_t>(alpha);
    const uint16_t* input = static_cast<const uint16_t*>(source);
    const int direction = enabled == 1 ? -1 : 1;
    const int stride = direction * height;
    destination += enabled == 1 ? height * (width - 1) : height - 1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = input[0];
            const uint16_t depth = input[1];
            if (depth == 0x6fff) {
                *destination = ExpandRgb565(pixel);
            } else {
                const uint32_t sourceColor = ((pixel & 0xf800U) << 10) |
                    ((pixel & 0x07e0U) << 5) | (pixel & 0x001fU);
                const uint32_t targetColor = ((color & 0xf800U) << 10) |
                    ((color & 0x07e0U) << 5) | (color & 0x001fU);
                const uint32_t mixed = (targetColor - sourceColor) * blend + sourceColor * 32;
                *destination = ((mixed & 0x7c000000U) >> 7) |
                    ((mixed & 0x001f8000U) >> 5) | ((mixed & 0x000003e0U) >> 2);
            }
            input += 2;
            destination += stride;
        }
        destination += -stride * width - direction;
    }
}

void InkShader::SetEngine(Engine* newEngine)
{
    engine = newEngine;
}

void InkShader::Reset()
{
    active = 0;
    completed = 0;
}

void InkShader::StartInk(Engine* newEngine, int, uint32_t* initialHistory, int newMode)
{
    if (newEngine != nullptr && active == 0) {
        completed = 0;
        engine = newEngine;
        mode = newMode;
        previousDestination = initialHistory;
        engine->SetSubmitEnabled(false);
        engine->SetPrepareEnabled(false);
        active = 1;
    }
}

void InkShader::StopInk()
{
    if (engine != nullptr) {
        engine->SetSubmitEnabled(true);
        engine->SetPrepareEnabled(true);
    }
    completed = 1;
    active = 0;
}

bool InkShader::IsCompleted() const
{
    return completed != 0;
}

bool InkShader::IsActive() const
{
    return active != 0;
}

void InkShader::Render(const void*, uint32_t* destination)
{
    if (active == 0) {
        return;
    }
    completed = 1;
    uint32_t redThreshold = 32;
    uint32_t greenThreshold = 32;
    uint32_t blueThreshold = 32;
    uint16_t ink = PackRgb565(16, 16, 16);
    if (mode == 1) {
        ink = PackRgb565(0, 16, 16);
        blueThreshold = 255;
    } else if (mode == 2) {
        ink = PackRgb565(16, 16, 0);
        redThreshold = 255;
    } else if (mode == 3) {
        ink = PackRgb565(16, 0, 16);
        greenThreshold = 255;
    }
    if (previousDestination == nullptr) {
        previousDestination = destination;
    }
    uint32_t* previous = previousDestination + height + 1;
    previousDestination = destination;
    destination += height + 1;
    for (int y = 1; y < width - 1; ++y) {
        for (int x = 1; x < height - 1; ++x) {
            const uint32_t pixel = *previous;
            if (((pixel & 0xff0000U) >> 16) > redThreshold ||
                ((pixel & 0x00ff00U) >> 8) > greenThreshold ||
                (pixel & 0x0000ffU) > blueThreshold) {
                completed = 0;
            } else {
                destination[1] = SubtractInk(previous[1], ink);
                destination[-1] = SubtractInk(previous[-1], ink);
                destination[height] = SubtractInk(previous[height], ink);
                destination[-height] = SubtractInk(previous[-height], ink);
            }
            ++destination;
            ++previous;
        }
        destination += 2;
        previous += 2;
    }
    if (completed != 0) {
        engine->SetPreUpdateEnabled(true);
        engine->SetPostUpdateEnabled(true);
        engine->SetSubmitEnabled(true);
        engine->SetPrepareEnabled(true);
    }
}

void BlindShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    uint16_t* output = reinterpret_cast<uint16_t*>(destination);
    const Fixed scale = FixedDivide(fixedOne, (height - 1) * fixedOne);
    const int direction = enabled == 1 ? -1 : 1;
    const int stride = direction * height;
    output += enabled == 1 ? height * (width - 1) : height - 1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = input[0];
            const int coordinate = enabled == 1 ? y : x;
            const Fixed attenuation = FixedMultiply(coordinate * fixedOne, scale);
            const uint32_t red = FixedMultiply(((pixel & 0xf800U) >> 8) * fixedOne,
                                               attenuation) >> 16;
            const uint32_t green = FixedMultiply((((pixel & 0x07e0U) >> 5) << 2) * fixedOne,
                                                 attenuation) >> 16;
            const uint32_t blue = FixedMultiply(((pixel & 0x001fU) << 3) * fixedOne,
                                                attenuation) >> 16;
            *output = PackRgb565(red, green, blue);
            input += 2;
            output += stride;
        }
        output += -stride * width - direction;
    }
}

void TwistShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    const int direction = enabled == 1 ? -1 : 1;
    const int stride = direction * height;
    destination += enabled == 1 ? height * (width - 1) : height - 1;
    int rowOffset = offset;
    int rowStep = 1;
    for (int y = 0; y < height; ++y) {
        rowOffset += rowStep;
        if (rowOffset >= amplitude || rowOffset <= -amplitude) {
            rowStep = -rowStep;
        }
        for (int x = 0; x < width; ++x) {
            const int sample = std::clamp(x + rowOffset, 0, width - 1);
            *destination = ExpandRgb565(input[sample * 2]);
            destination += stride;
        }
        input += width * 2;
        destination += -stride * width - direction;
    }
    offset += velocity;
    if (offset >= amplitude || offset <= -amplitude) {
        velocity = -velocity;
    }
}

} // namespace soft3d

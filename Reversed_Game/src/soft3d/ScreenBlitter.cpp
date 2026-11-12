#include "soft3d/ScreenBlitter.hpp"

#include <algorithm>
#include <cstring>

namespace soft3d {

void ScreenBlitter::SetTexture(Texture* texture)
{
    target = texture;
}

Texture* ScreenBlitter::GetTexture() const
{
    return target;
}

void ScreenBlitter::Clear(uint8_t red, uint8_t green, uint8_t blue)
{
    if (target == nullptr || target->GetPixels() == nullptr) {
        return;
    }
    const uint16_t color = static_cast<uint16_t>((red >> 3) << 11) |
                           static_cast<uint16_t>((green >> 2) << 5) |
                           static_cast<uint16_t>(blue >> 3);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    const size_t count = static_cast<size_t>(target->GetWidth()) * target->GetHeight();
    std::fill(pixels, pixels + count, static_cast<uint32_t>(color));
}

void ScreenBlitter::Blit(const Texture* source, ScreenRect destination, ScreenRect sourceRect,
                         uint32_t)
{
    if (target == nullptr || source == nullptr || target->GetPixels() == nullptr ||
        source->GetPixels() == nullptr) {
        return;
    }
    const int32_t destinationWidth = destination.right - destination.left;
    const int32_t destinationHeight = destination.bottom - destination.top;
    const int32_t sourceWidth = sourceRect.right - sourceRect.left;
    const int32_t sourceHeight = sourceRect.bottom - sourceRect.top;
    if (destinationWidth <= 0 || destinationHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0) {
        return;
    }

    const int32_t targetWidth = static_cast<int32_t>(target->GetWidth());
    const int32_t targetHeight = static_cast<int32_t>(target->GetHeight());
    const int32_t clippedLeft = std::max(destination.left, 0);
    const int32_t clippedTop = std::max(destination.top, 0);
    const int32_t clippedRight = std::min(destination.right, targetWidth);
    const int32_t clippedBottom = std::min(destination.bottom, targetHeight);
    const uint8_t* sourcePixels = source->GetPixels();
    uint32_t* targetPixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    const uint32_t bytesPerPixel = (source->GetType() & 0x30) >> 4;

    for (int32_t y = clippedTop; y < clippedBottom; ++y) {
        const int32_t sourceY = sourceRect.top + static_cast<int32_t>(
            static_cast<int64_t>(y - destination.top) * sourceHeight / destinationHeight);
        if (sourceY < 0 || sourceY >= static_cast<int32_t>(source->GetHeight())) {
            continue;
        }
        for (int32_t x = clippedLeft; x < clippedRight; ++x) {
            const int32_t sourceX = sourceRect.left + static_cast<int32_t>(
                static_cast<int64_t>(x - destination.left) * sourceWidth / destinationWidth);
            if (sourceX < 0 || sourceX >= static_cast<int32_t>(source->GetWidth())) {
                continue;
            }
            const size_t sourceOffset = static_cast<size_t>(sourceY) * source->GetWidth() + sourceX;
            uint16_t color = 0;
            if (bytesPerPixel == 1) {
                const uint8_t index = sourcePixels[sourceOffset];
                if ((source->HasTransparentColor() && index == source->GetTransparentColor()) ||
                    source->GetPalette() == nullptr) {
                    continue;
                }
                std::memcpy(&color, source->GetPalette() + static_cast<size_t>(index) * 2, 2);
            } else if (bytesPerPixel == 2) {
                std::memcpy(&color, sourcePixels + sourceOffset * 2, 2);
            } else {
                std::memcpy(&color, sourcePixels + sourceOffset * 4, 2);
            }
            targetPixels[static_cast<size_t>(y) * targetWidth + x] = color;
        }
    }
}

} // namespace soft3d

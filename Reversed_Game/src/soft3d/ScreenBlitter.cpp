#include "soft3d/ScreenBlitter.hpp"

#include <algorithm>
#include <cstring>

namespace soft3d {
namespace {

constexpr uint32_t PixelHigh = 0x7fff0000;

struct BlitRegion {
    ScreenRect destination;
    ScreenRect source;
    int32_t scaleX = 0;
    int32_t scaleY = 0;
};

uint16_t PaletteColor(const Texture* source, uint8_t index)
{
    if (source->GetPalette() == nullptr ||
        (source->GetPaletteSize() != 0 && index >= source->GetPaletteSize())) {
        return 0;
    }
    uint16_t color;
    std::memcpy(&color, source->GetPalette() + static_cast<size_t>(index) * 2, 2);
    return color;
}

uint16_t AddColors(uint16_t destination, uint16_t source)
{
    const uint32_t blue = std::min<uint32_t>(0x1f, (destination & 0x1f) + (source & 0x1f));
    const uint32_t green = std::min<uint32_t>(0x7e0, (destination & 0x7e0) + (source & 0x7e0));
    const uint32_t red = std::min<uint32_t>(0xf800, (destination & 0xf800) + (source & 0xf800));
    return static_cast<uint16_t>(red | green | blue);
}

uint16_t SubtractColors(uint16_t destination, uint16_t source)
{
    const uint32_t blue = (destination & 0x1f) >= (source & 0x1f)
        ? (destination & 0x1f) - (source & 0x1f)
        : 0;
    const uint32_t green = (destination & 0x7e0) >= (source & 0x7e0)
        ? (destination & 0x7e0) - (source & 0x7e0)
        : 0;
    const uint32_t red = (destination & 0xf800) >= (source & 0xf800)
        ? (destination & 0xf800) - (source & 0xf800)
        : 0;
    return static_cast<uint16_t>(red | green | blue);
}

void ApplyMode(uint32_t& destination, uint16_t source, uint32_t mode)
{
    if (mode == 0) {
        destination = PixelHigh | source;
        return;
    }
    destination |= PixelHigh;
    uint16_t color = static_cast<uint16_t>(destination);
    switch (mode) {
    case 1:
        color = AddColors(color, source);
        break;
    case 2:
        color = SubtractColors(color, source);
        break;
    case 3:
        color = static_cast<uint16_t>(((color & 0xf7de) >> 1) + ((color & 0xe79c) >> 2) +
                                      ((source & 0xe79c) >> 2));
        break;
    case 4:
        color = static_cast<uint16_t>(((color & 0xf7de) >> 1) + ((source & 0xf7de) >> 1));
        break;
    case 5:
        color = static_cast<uint16_t>(((color & 0xe79c) >> 2) + ((source & 0xf7de) >> 1) +
                                      ((source & 0xe79c) >> 2));
        break;
    default:
        return;
    }
    destination = (destination & 0xffff0000) | color;
}

void ApplySecondary(uint32_t& destination, uint16_t source, uint8_t alpha)
{
    if (alpha <= 1) {
        return;
    }
    if (alpha >= 31) {
        destination = (destination & 0xffff0000) | source;
        return;
    }
    const uint16_t old = static_cast<uint16_t>(destination);
    const uint32_t destinationColor = ((old & 0xf800) << 10) | ((old & 0x7e0) << 5) | (old & 0x1f);
    const uint32_t sourceColor = ((source & 0xf800) << 10) | ((source & 0x7e0) << 5) | (source & 0x1f);
    const uint32_t interpolated = destinationColor * 32 + (sourceColor - destinationColor) * alpha;
    const uint16_t color = static_cast<uint16_t>(((interpolated & 0x7c000000) >> 23) |
        ((interpolated & 0x1f8000) >> 10) | ((interpolated & 0x3e0) >> 5));
    destination = (destination & 0xffff0000) | color;
}

bool PrepareScaled(const Texture* target, ScreenRect destination, ScreenRect source,
                   BlitRegion& region)
{
    const int32_t destinationWidth = destination.right - destination.left;
    const int32_t destinationHeight = destination.bottom - destination.top;
    const int32_t sourceWidth = source.right - source.left;
    const int32_t sourceHeight = source.bottom - source.top;
    if (destinationWidth <= 0 || destinationHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0 ||
        destination.right <= 0 || destination.bottom <= 0) {
        return false;
    }
    region.destination = destination;
    region.source = source;
    region.scaleX = static_cast<int32_t>((static_cast<int64_t>(sourceWidth) << 16) / destinationWidth);
    region.scaleY = static_cast<int32_t>((static_cast<int64_t>(sourceHeight) << 16) / destinationHeight);
    const int32_t targetWidth = static_cast<int32_t>(target->GetWidth());
    const int32_t targetHeight = static_cast<int32_t>(target->GetHeight());
    if (region.destination.right > targetWidth) {
        region.source.right -= static_cast<int32_t>(
            (static_cast<int64_t>(region.destination.right - targetWidth) * region.scaleX) >> 16);
        region.destination.right = targetWidth;
    }
    if (region.destination.left >= targetWidth) {
        return false;
    }
    if (region.destination.left < 0) {
        region.source.left += static_cast<int32_t>(
            (static_cast<int64_t>(-region.destination.left) * region.scaleX) >> 16);
        region.destination.left = 0;
    }
    if (region.destination.bottom > targetHeight) {
        region.source.bottom -= static_cast<int32_t>(
            (static_cast<int64_t>(region.destination.bottom - targetHeight) * region.scaleY) >> 16);
        region.destination.bottom = targetHeight;
    }
    if (region.destination.top >= targetHeight) {
        return false;
    }
    if (region.destination.top < 0) {
        region.source.top += static_cast<int32_t>(
            (static_cast<int64_t>(-region.destination.top) * region.scaleY) >> 16);
        region.destination.top = 0;
    }
    return region.destination.right > region.destination.left &&
           region.destination.bottom > region.destination.top;
}

bool PrepareUnscaled(const Texture* target, int32_t x, int32_t y, ScreenRect source,
                     BlitRegion& region)
{
    const int32_t targetWidth = static_cast<int32_t>(target->GetWidth());
    const int32_t targetHeight = static_cast<int32_t>(target->GetHeight());
    if (x >= targetWidth || y >= targetHeight) {
        return false;
    }
    if (x < 0) {
        source.left -= x;
        x = 0;
    }
    if (source.right - source.left <= 0) {
        return false;
    }
    const int32_t rightClip = x + source.right - source.left - targetWidth;
    if (rightClip > 0) {
        source.right -= rightClip;
    }
    if (y < 0) {
        source.top -= y;
        y = 0;
    }
    if (source.bottom - source.top <= 0) {
        return false;
    }
    const int32_t bottomClip = y + source.bottom - source.top - targetHeight;
    if (bottomClip > 0) {
        source.bottom -= bottomClip;
    }
    region.destination = {x, y, x + source.right - source.left, y + source.bottom - source.top};
    region.source = source;
    return region.destination.right > x && region.destination.bottom > y;
}

const Texture* ValidSecondary(const Texture* source, const Texture* secondary)
{
    if (secondary == nullptr || secondary->GetPixels() == nullptr ||
        (secondary->GetType() & 0xc0) != 0 || secondary->GetWidth() != source->GetWidth() ||
        secondary->GetHeight() != source->GetHeight()) {
        return nullptr;
    }
    return secondary;
}

} // namespace

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
    for (size_t index = count - 1; index > 0; --index) {
        pixels[index] = color;
    }
}

void ScreenBlitter::Blit(const Texture* source, ScreenRect destination, ScreenRect sourceRect,
                         uint32_t mode, const Texture* secondary)
{
    if (target == nullptr || source == nullptr || target->GetPixels() == nullptr ||
        source->GetPixels() == nullptr || source->GetPalette() == nullptr || mode > 5) {
        return;
    }
    BlitRegion region;
    if (!PrepareScaled(target, destination, sourceRect, region)) {
        return;
    }
    secondary = ValidSecondary(source, secondary);
    uint32_t* targetPixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    const int32_t targetWidth = static_cast<int32_t>(target->GetWidth());
    const int32_t sourceWidth = static_cast<int32_t>(source->GetWidth());
    const int32_t sourceHeight = static_cast<int32_t>(source->GetHeight());
    const int32_t width = region.destination.right - region.destination.left;
    const int32_t height = region.destination.bottom - region.destination.top;
    int32_t fixedY = region.source.top << 16;
    for (int32_t y = 0; y < height; ++y) {
        int32_t fixedX = 0;
        const int32_t sourceY = fixedY >> 16;
        for (int32_t x = 0; x < width; ++x) {
            const int32_t sourceXOffset = fixedX >> 16;
            const int32_t sourceX = region.source.left + sourceXOffset;
            uint32_t& pixel = targetPixels[
                static_cast<size_t>(region.destination.top + y) * targetWidth +
                region.destination.left + x];
            if (secondary != nullptr) {
                if (sourceXOffset != source->GetTransparentColor() && sourceX >= 0 &&
                    sourceX < sourceWidth && sourceY >= 0 && sourceY < sourceHeight) {
                    const size_t offset = static_cast<size_t>(sourceY) * sourceWidth + sourceX;
                    pixel |= PixelHigh;
                    ApplySecondary(pixel, PaletteColor(source, source->GetPixels()[offset]),
                                   secondary->GetPixels()[offset]);
                }
            } else if (sourceX >= 0 && sourceX < sourceWidth && sourceY >= 0 && sourceY < sourceHeight) {
                const uint8_t index = source->GetPixels()[static_cast<size_t>(sourceY) * sourceWidth + sourceX];
                if (!source->HasTransparentColor() || index != source->GetTransparentColor()) {
                    ApplyMode(pixel, PaletteColor(source, index), mode);
                }
            }
            fixedX += region.scaleX;
        }
        fixedY += region.scaleY;
    }
}

void ScreenBlitter::BlitUnscaled(const Texture* source, int32_t x, int32_t y,
                                 ScreenRect sourceRect, uint32_t mode, const Texture* secondary)
{
    if (target == nullptr || source == nullptr || target->GetPixels() == nullptr ||
        source->GetPixels() == nullptr || source->GetPalette() == nullptr || mode > 5) {
        return;
    }
    BlitRegion region;
    if (!PrepareUnscaled(target, x, y, sourceRect, region)) {
        return;
    }
    secondary = ValidSecondary(source, secondary);
    uint32_t* targetPixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    const int32_t targetWidth = static_cast<int32_t>(target->GetWidth());
    const int32_t sourceWidth = static_cast<int32_t>(source->GetWidth());
    const int32_t sourceHeight = static_cast<int32_t>(source->GetHeight());
    for (int32_t sourceY = region.source.top; sourceY < region.source.bottom; ++sourceY) {
        for (int32_t sourceX = region.source.left; sourceX < region.source.right; ++sourceX) {
            if (sourceX < 0 || sourceX >= sourceWidth || sourceY < 0 || sourceY >= sourceHeight) {
                continue;
            }
            const size_t offset = static_cast<size_t>(sourceY) * sourceWidth + sourceX;
            const int32_t destinationX = region.destination.left + sourceX - region.source.left;
            const int32_t destinationY = region.destination.top + sourceY - region.source.top;
            uint32_t& pixel = targetPixels[static_cast<size_t>(destinationY) * targetWidth + destinationX];
            if (secondary != nullptr) {
                const uint8_t alpha = secondary->GetPixels()[offset];
                if (alpha > 1) {
                    pixel |= PixelHigh;
                    ApplySecondary(pixel, PaletteColor(source, source->GetPixels()[offset]), alpha);
                }
            } else {
                const uint8_t index = source->GetPixels()[offset];
                if (!source->HasTransparentColor() || index != source->GetTransparentColor()) {
                    ApplyMode(pixel, PaletteColor(source, index), mode);
                }
            }
        }
    }
}

} // namespace soft3d

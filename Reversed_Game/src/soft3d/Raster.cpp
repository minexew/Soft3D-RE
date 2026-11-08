#include "soft3d/Raster.hpp"

#include <algorithm>
#include <cstring>

namespace soft3d {
namespace {

int64_t Edge(const RasterVertex& a, const RasterVertex& b, int64_t x, int64_t y)
{
    return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

uint16_t SampleTexture(const Texture* texture, Fixed u, Fixed v)
{
    if (texture == nullptr || texture->GetPixels() == nullptr ||
        texture->GetWidth() == 0 || texture->GetHeight() == 0) {
        return 0xffff;
    }
    int x = u >> 16;
    int y = v >> 16;
    const int width = static_cast<int>(texture->GetWidth());
    const int height = static_cast<int>(texture->GetHeight());
    x = ((x % width) + width) % width;
    y = ((y % height) + height) % height;
    const std::size_t offset = static_cast<std::size_t>(y) * width + x;
    const uint8_t* pixels = texture->GetPixels();
    const uint8_t bytesPerPixel = static_cast<uint8_t>((texture->GetType() & 0x30) >> 4);
    if (bytesPerPixel == 1 && texture->GetPalette() != nullptr) {
        uint16_t color;
        std::memcpy(&color, texture->GetPalette() + pixels[offset] * 2, sizeof(color));
        return color;
    }
    uint16_t color;
    std::memcpy(&color, pixels + offset * (bytesPerPixel == 0 ? 4 : bytesPerPixel),
                sizeof(color));
    return color;
}

} // namespace

Raster::Raster(int rasterWidth, int rasterHeight)
    : width(rasterWidth)
    , height(rasterHeight)
    , basicShader(rasterWidth, rasterHeight)
    , fogShader(rasterWidth, rasterHeight)
    , activeShader(&basicShader)
{
}

void Raster::SetCurrentTexture(Texture* texture)
{
    previousTexture = currentTexture;
    currentTexture = texture;
}

Texture* Raster::GetCurrentTexture() const
{
    return currentTexture;
}

void Raster::SetBackBufferPtr(uint32_t* address)
{
    backBuffer = address;
}

void Raster::SelectShader(BasicShader* shader)
{
    activeShader = shader != nullptr ? shader : (fogEnabled ? &fogShader : &basicShader);
}

void Raster::SetFogEnabled(bool enabled)
{
    fogEnabled = enabled;
    SelectShader(nullptr);
}

void Raster::SetDepthTestEnabled(bool enabled)
{
    depthTest = enabled;
}

void Raster::Clear(uint16_t color)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr) {
        return;
    }
    uint16_t* pixels = reinterpret_cast<uint16_t*>(currentTexture->GetPixels());
    const std::size_t count = static_cast<std::size_t>(width) * height * 2;
    for (std::size_t index = 0; index < count; index += 2) {
        pixels[index] = color;
        pixels[index + 1] = 0;
    }
}

void Raster::RasterizeTriangle(const RasterVertex& first, const RasterVertex& second,
                               const RasterVertex& third, const Texture* texture)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr) {
        return;
    }
    const int minX = std::max(0, static_cast<int>(std::min({first.x, second.x, third.x}) >> 16));
    const int maxX = std::min(width - 1,
        static_cast<int>(std::max({first.x, second.x, third.x}) >> 16));
    const int minY = std::max(0, static_cast<int>(std::min({first.y, second.y, third.y}) >> 16));
    const int maxY = std::min(height - 1,
        static_cast<int>(std::max({first.y, second.y, third.y}) >> 16));
    const int64_t area = Edge(first, second, third.x, third.y);
    if (area == 0) {
        return;
    }

    uint16_t* output = reinterpret_cast<uint16_t*>(currentTexture->GetPixels());
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const Fixed sampleX = (x << 16) + 0x8000;
            const Fixed sampleY = (y << 16) + 0x8000;
            const int64_t w0 = Edge(second, third, sampleX, sampleY);
            const int64_t w1 = Edge(third, first, sampleX, sampleY);
            const int64_t w2 = Edge(first, second, sampleX, sampleY);
            if (area < 0) {
                if (w0 > 0 || w1 > 0 || w2 > 0) {
                    continue;
                }
            } else if (w0 < 0 || w1 < 0 || w2 < 0) {
                continue;
            }

            const Fixed z = static_cast<Fixed>(
                (first.z * w0 + second.z * w1 + third.z * w2) / area);
            const uint16_t depth = static_cast<uint16_t>(std::clamp(z, 0, 0xffff));
            const std::size_t pixel = (static_cast<std::size_t>(y) * width + x) * 2;
            if (depthTest && output[pixel + 1] != 0 && depth >= output[pixel + 1]) {
                continue;
            }
            const Fixed u = static_cast<Fixed>(
                (first.u * w0 + second.u * w1 + third.u * w2) / area);
            const Fixed v = static_cast<Fixed>(
                (first.v * w0 + second.v * w1 + third.v * w2) / area);
            output[pixel] = texture != nullptr ? SampleTexture(texture, u, v) : first.color;
            output[pixel + 1] = depth;
        }
    }
}

void Raster::Present()
{
    if (activeShader != nullptr && currentTexture != nullptr && backBuffer != nullptr) {
        activeShader->Render(currentTexture->GetPixels(), backBuffer);
    }
}

BasicShader& Raster::GetBasicShader()
{
    return basicShader;
}

FogShader& Raster::GetFogShader()
{
    return fogShader;
}

} // namespace soft3d

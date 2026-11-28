#include "soft3d/Raster.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace soft3d {
namespace {

int32_t WrapAdd(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

int32_t WrapSubtract(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

int32_t WrapMultiply(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) * static_cast<uint32_t>(right));
}

int32_t WrapNegate(int32_t value)
{
    return static_cast<int32_t>(0U - static_cast<uint32_t>(value));
}

uint32_t ReciprocalTable(uint32_t index)
{
    return index == 0 ? 0x7fffffffU : static_cast<uint32_t>(0x40000000ULL / index);
}

int32_t ReciprocalArea(int32_t area)
{
    uint32_t magnitude = static_cast<uint32_t>(area);
    magnitude = (magnitude ^ static_cast<uint32_t>(area >> 31)) -
                static_cast<uint32_t>(area >> 31);
    unsigned normalization = 16;
    if ((magnitude & 0xffff0000U) != 0) {
        normalization = 0;
        if ((magnitude & 0xff000000U) == 0) {
            normalization = 8;
            magnitude <<= 8;
        }
        if ((magnitude & 0xf0000000U) == 0) {
            normalization += 4;
            magnitude <<= 4;
        }
        if ((magnitude & 0xc0000000U) == 0) {
            normalization += 2;
            magnitude <<= 2;
        }
        if ((magnitude & 0x7fffffffU) == 0) {
            normalization += 1;
            magnitude <<= 1;
        }
        magnitude >>= 16;
    }
    const int32_t reciprocal = static_cast<int32_t>(
        ReciprocalTable(magnitude) >> ((18 - normalization) & 0xff));
    return area < 0 ? WrapNegate(reciprocal) : reciprocal;
}

int32_t FloorDivide(int32_t numerator, int32_t denominator)
{
    int32_t quotient = numerator / denominator;
    const int32_t remainder = numerator % denominator;
    if (remainder != 0 && ((remainder < 0) != (denominator < 0))) {
        --quotient;
    }
    return quotient;
}

uint16_t Blend25Destination(uint16_t destination, uint16_t source)
{
    return static_cast<uint16_t>(((destination & 0xf7de) >> 1) +
                                 ((destination & 0xe79c) >> 2) +
                                 ((source & 0xe79c) >> 2));
}

uint16_t Blend50(uint16_t destination, uint16_t source)
{
    return static_cast<uint16_t>(((destination & 0xf7de) >> 1) +
                                 ((source & 0xf7de) >> 1));
}

uint16_t Blend75Destination(uint16_t destination, uint16_t source)
{
    return static_cast<uint16_t>(((destination & 0xe79c) >> 2) +
                                 ((source & 0xf7de) >> 1) +
                                 ((source & 0xe79c) >> 2));
}

uint16_t BlendSecondary(uint16_t destination, uint16_t source, uint8_t alpha)
{
    if (alpha <= 1) {
        return destination;
    }
    if (alpha >= 31) {
        return source;
    }
    const uint32_t oldColor = ((destination & 0xf800) << 10) |
                              ((destination & 0x7e0) << 5) | (destination & 0x1f);
    const uint32_t newColor = ((source & 0xf800) << 10) |
                              ((source & 0x7e0) << 5) | (source & 0x1f);
    const uint32_t interpolated = oldColor * 32 + (newColor - oldColor) * alpha;
    return static_cast<uint16_t>(((interpolated & 0x7c000000) >> 15) |
                                 ((interpolated & 0x1f8000) >> 10) |
                                 ((interpolated & 0x3e0) >> 5));
}

} // namespace

void Raster::Edge::Advance(const AttributeDeltas& deltas)
{
    error = WrapAdd(error, errorStep);
    int32_t stepX = step;
    if (error >= 0) {
        stepX = WrapAdd(step, 1);
        error = WrapSubtract(error, height);
    }
    x = WrapAdd(x, stepX);
    inverseDepth = WrapAdd(inverseDepth,
                           WrapAdd(deltas.depthY, WrapMultiply(stepX, deltas.depthX)));
    u = WrapAdd(u, WrapAdd(deltas.uY, WrapMultiply(stepX, deltas.uX)));
    v = WrapAdd(v, WrapAdd(deltas.vY, WrapMultiply(stepX, deltas.vX)));
    intensity = WrapAdd(intensity,
                        WrapAdd(deltas.intensityY, WrapMultiply(stepX, deltas.intensityX)));
}

Raster::Raster(int rasterWidth, int rasterHeight)
    : width(rasterWidth)
    , height(rasterHeight)
    , basicShader(rasterWidth, rasterHeight)
    , fogShader(rasterWidth, rasterHeight)
    , activeShader(&basicShader)
{
}

void Raster::SetCurrentTexture(Texture* newTexture)
{
    previousTexture = currentTexture;
    currentTexture = newTexture;
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

void Raster::SetOrientation(int orientation)
{
    if (activeShader != nullptr) {
        activeShader->SetEnabled(orientation);
    }
}

void Raster::SetFogEnabled(bool enabled)
{
    fogEnabled = enabled;
    SelectShader(nullptr);
}

void Raster::SetTexture(const Texture* newTexture)
{
    texture = newTexture;
}

void Raster::SetDetailTexture(const Texture* newTexture)
{
    detailTexture = newTexture;
}

void Raster::SetBlendMode(uint32_t mode)
{
    blendMode = mode <= 3 ? mode : 0;
}

void Raster::SetDepthTestEnabled(bool enabled)
{
    depthTest = enabled;
}

void Raster::SetDepthWriteEnabled(bool enabled)
{
    depthWrite = enabled;
}

void Raster::SetDepthBias(Fixed bias)
{
    depthBias = bias >> 8;
}

void Raster::SetFlatColor(uint16_t color)
{
    flatColor = color;
}

void Raster::SetShadingEnabled(bool enabled)
{
    shading = enabled;
}

void Raster::Clear(uint16_t color)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr) {
        return;
    }
    uint32_t* pixels = reinterpret_cast<uint32_t*>(currentTexture->GetPixels());
    const std::size_t count = static_cast<std::size_t>(width) * height;
    for (std::size_t index = 0; index < count; ++index) {
        pixels[index] = color;
    }
}

Raster::AttributeDeltas Raster::ComputeAttributeDeltas(const Vertex& first,
    const Vertex& second, const Vertex& third, int32_t area) const
{
    AttributeDeltas result;
    const int32_t reciprocal = ReciprocalArea(area);
    const int32_t x02 = WrapSubtract(first.x, third.x);
    const int32_t y02 = WrapSubtract(first.y, third.y);
    const int32_t x12 = WrapSubtract(second.x, third.x);
    const int32_t y12 = WrapSubtract(second.y, third.y);

    const int32_t depth20 = WrapSubtract(third.inverseDepth, first.inverseDepth) >> 6;
    const int32_t depth12 = WrapSubtract(second.inverseDepth, third.inverseDepth) >> 6;
    result.depthX = FixedMultiply(WrapAdd(WrapMultiply(depth20, y12),
        WrapMultiply(depth12, y02)), reciprocal) >> 6;
    result.depthY = FixedMultiply(WrapNegate(WrapAdd(WrapMultiply(depth20, x12),
        WrapMultiply(depth12, x02))), reciprocal) >> 6;

    const int32_t u20 = WrapNegate(WrapSubtract(first.u, third.u) >> 6);
    const int32_t u12 = WrapSubtract(second.u, third.u) >> 6;
    result.uX = FixedMultiply(WrapAdd(WrapMultiply(u20, y12),
        WrapMultiply(u12, y02)), reciprocal) >> 6;
    result.uY = FixedMultiply(WrapNegate(WrapAdd(WrapMultiply(u20, x12),
        WrapMultiply(u12, x02))), reciprocal) >> 6;

    const int32_t v20 = WrapSubtract(third.v, first.v) >> 6;
    const int32_t v12 = WrapSubtract(second.v, third.v) >> 6;
    result.vX = FixedMultiply(WrapAdd(WrapMultiply(v20, y12),
        WrapMultiply(v12, y02)), reciprocal) >> 6;
    result.vY = FixedMultiply(WrapNegate(WrapAdd(WrapMultiply(v20, x12),
        WrapMultiply(v12, x02))), reciprocal) >> 6;

    const int32_t intensity20 = WrapSubtract(third.intensity, first.intensity) >> 4;
    const int32_t intensity12 = WrapSubtract(second.intensity, third.intensity) >> 4;
    result.intensityX = FixedMultiply(WrapAdd(WrapMultiply(intensity20, y12),
        WrapMultiply(intensity12, y02)), reciprocal) >> 8;
    result.intensityY = FixedMultiply(WrapNegate(WrapAdd(WrapMultiply(intensity20, x12),
        WrapMultiply(intensity12, x02))), reciprocal) >> 8;
    return result;
}

Raster::Edge Raster::SetupEdge(const Vertex& from, const Vertex& to)
{
    Edge edge;
    edge.x = from.x;
    edge.inverseDepth = from.inverseDepth;
    edge.u = from.u;
    edge.v = from.v;
    edge.intensity = from.intensity;
    const int32_t dx = WrapSubtract(to.x, from.x);
    edge.height = WrapSubtract(to.y, from.y);
    if (edge.height <= 0) {
        return edge;
    }
    edge.step = FloorDivide(dx, edge.height);
    edge.errorStep = WrapSubtract(dx, WrapMultiply(edge.step, edge.height));
    return edge;
}

void Raster::DrawSpan(int32_t y, const Edge& left, const Edge& right,
                      const AttributeDeltas& deltas)
{
    int32_t firstX = left.x;
    int32_t count = WrapSubtract(right.x, left.x);
    if (count <= 0 || y < 0 || y >= height) {
        return;
    }
    int32_t skipped = 0;
    if (firstX < 0) {
        skipped = -firstX;
        count = WrapSubtract(count, skipped);
        firstX = 0;
    }
    if (firstX + count > width) {
        count = width - firstX;
    }
    if (count <= 0) {
        return;
    }
    uint32_t* output = reinterpret_cast<uint32_t*>(currentTexture->GetPixels()) +
                       static_cast<std::size_t>(y) * width + firstX;
    Fixed u = AddFixed(left.u, WrapMultiply(deltas.uX, skipped));
    Fixed v = AddFixed(left.v, WrapMultiply(deltas.vX, skipped));
    Fixed depth = AddFixed(left.inverseDepth, WrapMultiply(deltas.depthX, skipped));
    int32_t intensity = WrapAdd(left.intensity, WrapMultiply(deltas.intensityX, skipped));
    for (int32_t offset = 0; offset < count; ++offset, ++output) {
        const uint32_t depthWord = ((static_cast<uint32_t>(depth) & 0xffffffU) >> 8) << 16;
        const unsigned selector = (depthWrite ? 50U : 0U) + (depthTest ? 25U : 0U) +
            (texture != nullptr ? 5U : 0U) +
            (texture != nullptr && texture->HasTransparentColor() ? 10U : 0U) +
            (texture != nullptr && shading ? 5U : 0U) + (detailTexture != nullptr ? 4U : 0U);
        const bool signedDepthCompare = selector == 80 || selector == 85;
        const bool visible = !depthTest || (signedDepthCompare
            ? static_cast<int32_t>(*output) <= static_cast<int32_t>(depthWord)
            : *output <= depthWord);
        if (visible) {
            uint16_t color = flatColor;
            bool transparent = false;
            if (texture != nullptr) {
                const uint32_t maxU = (texture->GetWidth() - 1) << 16;
                const uint32_t maxV = (texture->GetHeight() - 1) << 16;
                const int32_t widthShift = texture->GetWidthLog2();
                uint32_t sourceOffset;
                if (widthShift >= 0) {
                    sourceOffset = (((static_cast<uint32_t>(v) & maxV) << widthShift) +
                        (static_cast<uint32_t>(u) & maxU)) >> 16;
                } else {
                    const int32_t textureWidth = static_cast<int32_t>(texture->GetWidth());
                    const int32_t textureHeight = static_cast<int32_t>(texture->GetHeight());
                    const int32_t x = ((u >> 16) % textureWidth + textureWidth) % textureWidth;
                    const int32_t row = ((v >> 16) % textureHeight + textureHeight) % textureHeight;
                    sourceOffset = static_cast<uint32_t>(row) * texture->GetWidth() + x;
                }
                const uint8_t index = texture->GetPixels()[sourceOffset];
                transparent = texture->HasTransparentColor() &&
                              index == texture->GetTransparentColor();
                if (!transparent) {
                    const uint32_t paletteIndex = (static_cast<uint32_t>(index) << 5) |
                        (shading && intensity > 0 ? static_cast<uint32_t>(intensity >> 8)
                                                 : (shading ? 0U : 15U));
                    std::memcpy(&color, texture->GetPalette() + paletteIndex * 2, 2);
                    if (detailTexture != nullptr) {
                        const uint8_t alpha = detailTexture->GetPixels()[sourceOffset];
                        color = BlendSecondary(static_cast<uint16_t>(*output), color, alpha);
                    } else if (blendMode == 1) {
                        color = Blend25Destination(static_cast<uint16_t>(*output), color);
                    } else if (blendMode == 2) {
                        color = Blend50(static_cast<uint16_t>(*output), color);
                    } else if (blendMode == 3) {
                        color = Blend75Destination(static_cast<uint16_t>(*output), color);
                    }
                }
            } else if (blendMode == 1) {
                color = Blend25Destination(static_cast<uint16_t>(*output), color);
            } else if (blendMode == 2) {
                color = Blend50(static_cast<uint16_t>(*output), color);
            } else if (blendMode == 3) {
                color = Blend75Destination(static_cast<uint16_t>(*output), color);
            }
            if (!transparent) {
                *output = depthWrite ? depthWord | color
                                     : (*output & 0xffff0000U) | color;
            }
        }
        u = WrapAdd(u, deltas.uX);
        v = WrapAdd(v, deltas.vX);
        depth = WrapAdd(depth, deltas.depthX);
        intensity = WrapAdd(intensity, deltas.intensityX);
    }
}

void Raster::RasterizeTriangle(const RasterVertex& first, const RasterVertex& second,
                               const RasterVertex& third)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr) {
        return;
    }
    std::array<Vertex, 3> vertices;
    const std::array<const RasterVertex*, 3> source = {&first, &second, &third};
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        vertices[index].x = source[index]->x >> 16;
        vertices[index].y = source[index]->y >> 16;
        vertices[index].inverseDepth = std::max(
            WrapAdd(source[index]->inverseDepth, depthBias), 0x200);
        vertices[index].u = source[index]->u;
        vertices[index].v = source[index]->v;
        vertices[index].intensity = source[index]->intensity >> 16;
    }
    const int32_t area = WrapSubtract(
        WrapMultiply(WrapSubtract(vertices[0].y, vertices[1].y),
                     WrapSubtract(vertices[0].x, vertices[2].x)),
        WrapMultiply(WrapSubtract(vertices[0].x, vertices[1].x),
                     WrapSubtract(vertices[0].y, vertices[2].y)));
    if (area == 0) {
        return;
    }
    const AttributeDeltas deltas = ComputeAttributeDeltas(vertices[0], vertices[1],
                                                           vertices[2], area);
    std::stable_sort(vertices.begin(), vertices.end(), [](const Vertex& left, const Vertex& right) {
        return left.y < right.y;
    });
    const int32_t sortedArea = WrapSubtract(
        WrapMultiply(WrapSubtract(vertices[0].y, vertices[1].y),
                     WrapSubtract(vertices[0].x, vertices[2].x)),
        WrapMultiply(WrapSubtract(vertices[0].x, vertices[1].x),
                     WrapSubtract(vertices[0].y, vertices[2].y)));
    Edge longEdge = SetupEdge(vertices[0], vertices[2]);
    Edge shortEdge = SetupEdge(vertices[0], vertices[1]);
    bool shortIsMiddle = false;
    for (int32_t y = vertices[0].y; y < vertices[2].y; ++y) {
        if (!shortIsMiddle && y == vertices[1].y) {
            shortEdge = SetupEdge(vertices[1], vertices[2]);
            shortIsMiddle = true;
        }
        if (sortedArea < 0) {
            DrawSpan(y, longEdge, shortEdge, deltas);
        } else {
            DrawSpan(y, shortEdge, longEdge, deltas);
        }
        longEdge.Advance(deltas);
        shortEdge.Advance(deltas);
    }
}

void Raster::DrawScaled(ScreenRect destination, ScreenRect source, Fixed inverseDepth,
                        uint32_t paletteIntensity)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr || texture == nullptr ||
        texture->GetPixels() == nullptr || texture->GetPalette() == nullptr) {
        return;
    }
    const int32_t destinationWidth = WrapSubtract(destination.right, destination.left);
    const int32_t destinationHeight = WrapSubtract(destination.bottom, destination.top);
    const int32_t sourceWidth = WrapSubtract(source.right, source.left);
    const int32_t sourceHeight = WrapSubtract(source.bottom, source.top);
    if (destinationWidth <= 0 || destinationHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0 ||
        destination.left >= width || destination.right <= 0 ||
        destination.top >= height || destination.bottom <= 0) {
        return;
    }

    const Fixed scaleX = FixedDivide(ShiftLeftFixed(sourceWidth, 16),
                                     ShiftLeftFixed(destinationWidth, 16));
    const Fixed scaleY = FixedDivide(ShiftLeftFixed(sourceHeight, 16),
                                     ShiftLeftFixed(destinationHeight, 16));
    if (destination.left < 0) {
        source.left = WrapSubtract(source.left, FixedToInt(FixedMultiply(
            ShiftLeftFixed(destination.left, 16), scaleX)));
        destination.left = 0;
    }
    if (destination.right > width) {
        source.right = WrapSubtract(source.right, FixedToInt(FixedMultiply(
            ShiftLeftFixed(WrapSubtract(destination.right, width), 16), scaleX)));
        destination.right = width;
    }
    if (destination.top < 0) {
        source.top = WrapSubtract(source.top, FixedToInt(FixedMultiply(
            ShiftLeftFixed(destination.top, 16), scaleY)));
        destination.top = 0;
    }
    if (destination.bottom > height) {
        source.bottom = WrapSubtract(source.bottom, FixedToInt(FixedMultiply(
            ShiftLeftFixed(WrapSubtract(destination.bottom, height), 16), scaleY)));
        destination.bottom = height;
    }
    if (destination.right <= destination.left || destination.bottom <= destination.top) {
        return;
    }

    const uint32_t depthWord = static_cast<uint32_t>(ShiftLeftFixed(inverseDepth, 16));
    Fixed fixedY = ShiftLeftFixed(source.top, 16);
    for (int32_t y = destination.top; y < destination.bottom; ++y) {
        Fixed fixedX = 0;
        const int32_t sourceY = fixedY >> 16;
        for (int32_t x = destination.left; x < destination.right; ++x) {
            const int32_t sourceX = source.left + (fixedX >> 16);
            uint32_t* output = reinterpret_cast<uint32_t*>(currentTexture->GetPixels()) +
                               static_cast<std::size_t>(y) * width + x;
            if (sourceX >= 0 && sourceX < static_cast<int32_t>(texture->GetWidth()) &&
                sourceY >= 0 && sourceY < static_cast<int32_t>(texture->GetHeight()) &&
                (!depthTest || *output <= depthWord)) {
                const uint8_t index = texture->GetPixels()[
                    static_cast<std::size_t>(sourceY) * texture->GetWidth() + sourceX];
                const bool transparent = texture->HasTransparentColor() &&
                                         index == texture->GetTransparentColor();
                if (!transparent) {
                    const uint32_t paletteIndex = (static_cast<uint32_t>(index) << 5) |
                                                  paletteIntensity;
                    uint16_t color;
                    std::memcpy(&color, texture->GetPalette() + paletteIndex * 2, 2);
                    if (detailTexture != nullptr) {
                        const uint8_t alpha = detailTexture->GetPixels()[
                            static_cast<std::size_t>(sourceY) * texture->GetWidth() + sourceX];
                        color = BlendSecondary(static_cast<uint16_t>(*output), color, alpha);
                    } else if (blendMode == 1) {
                        color = Blend25Destination(static_cast<uint16_t>(*output), color);
                    } else if (blendMode == 2) {
                        color = Blend50(static_cast<uint16_t>(*output), color);
                    } else if (blendMode == 3) {
                        color = Blend75Destination(static_cast<uint16_t>(*output), color);
                    }
                    *output = depthWrite ? depthWord | color
                                         : (*output & 0xffff0000U) | color;
                }
            }
            fixedX = WrapAdd(fixedX, scaleX);
        }
        fixedY = WrapAdd(fixedY, scaleY);
    }
}

void Raster::FillColor16(int32_t x, int32_t y, int32_t count, uint16_t color)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr || count <= 0 ||
        y < 0 || y >= height) {
        return;
    }
    const int64_t first = std::max<int64_t>(x, 0);
    const int64_t end = std::min<int64_t>(static_cast<int64_t>(x) + count, width);
    for (int64_t column = first; column < end; ++column) {
        uint32_t* output = reinterpret_cast<uint32_t*>(currentTexture->GetPixels()) +
                           static_cast<std::size_t>(y) * width + column;
        *output = (*output & 0xffff0000U) | color;
    }
}

void Raster::FillColor32(int32_t x, int32_t y, int32_t count, uint32_t color)
{
    if (currentTexture == nullptr || currentTexture->GetPixels() == nullptr || count <= 0 ||
        y < 0 || y >= height) {
        return;
    }
    const int64_t first = std::max<int64_t>(x, 0);
    const int64_t end = std::min<int64_t>(static_cast<int64_t>(x) + count, width);
    uint32_t* output = reinterpret_cast<uint32_t*>(currentTexture->GetPixels());
    for (int64_t column = first; column < end; ++column) {
        output[static_cast<std::size_t>(y) * width + column] = color;
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

BasicShader* Raster::GetActiveShader() const
{
    return activeShader;
}

} // namespace soft3d

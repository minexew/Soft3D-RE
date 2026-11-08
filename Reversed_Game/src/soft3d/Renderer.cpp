#include "soft3d/Renderer.hpp"

#include <cstring>

namespace soft3d {
namespace {

int32_t Read32(const uint8_t* input)
{
    int32_t value;
    std::memcpy(&value, input, sizeof(value));
    return value;
}

uint16_t Read16(const uint8_t* input)
{
    uint16_t value;
    std::memcpy(&value, input, sizeof(value));
    return value;
}

Fixed Divide(Fixed numerator, Fixed denominator)
{
    if (denominator == 0) {
        return numerator < 0 ? INT32_MIN : INT32_MAX;
    }
    return static_cast<Fixed>((static_cast<int64_t>(numerator) << 16) / denominator);
}

struct SourceVertex {
    Vec3 position;
    Fixed u = 0;
    Fixed v = 0;
    uint16_t color = 0xffff;
    uint32_t matrixIndex = 0;
};

SourceVertex ReadVertex(const SojVertices& vertices, uint32_t index)
{
    SourceVertex result;
    const uint8_t* input = vertices.data.data() + index * vertices.strideBytes;
    if ((vertices.format & 0x10) != 0) {
        result.matrixIndex = static_cast<uint32_t>(Read32(input));
        input += 4;
    }
    if ((vertices.format & 1) != 0) {
        result.position = {Read32(input), Read32(input + 4), Read32(input + 8)};
        input += 12;
    }
    if ((vertices.format & 2) != 0) {
        input += 12;
    }
    if ((vertices.format & 4) != 0) {
        result.u = Read32(input);
        result.v = Read32(input + 4);
        input += 8;
    }
    if ((vertices.format & 8) != 0) {
        result.color = static_cast<uint16_t>(Read32(input));
    }
    return result;
}

} // namespace

Renderer::Renderer(int rendererWidth, int rendererHeight)
    : width(rendererWidth)
    , height(rendererHeight)
    , modelMatrix(Matrix4::Identity())
    , viewMatrix(Matrix4::Identity())
    , projectionMatrix(Matrix4::Identity())
    , raster(rendererWidth, rendererHeight)
{
    frameTexture.Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 2);
    raster.SetCurrentTexture(&frameTexture);
}

void Renderer::SetBackBufferPtr(uint32_t* address)
{
    raster.SetBackBufferPtr(address);
}

void Renderer::SetModelMatrix(const Matrix4& matrix)
{
    modelMatrix = matrix;
}

void Renderer::SetViewMatrix(const Matrix4& matrix)
{
    viewMatrix = matrix;
}

void Renderer::SetProjectionMatrix(const Matrix4& matrix)
{
    projectionMatrix = matrix;
}

void Renderer::DrawIndexedSurface(const SojSurface& surface, const SojObject* material,
                                  const Texture* texture, const Texture*,
                                  const std::vector<Matrix4>& matrixPalette)
{
    if (surface.vertices.vertexCount == 0 || surface.indices.data.empty()) {
        return;
    }
    const Matrix4 transform = Multiply(Multiply(modelMatrix, viewMatrix), projectionMatrix);
    std::vector<RasterVertex> transformed(surface.vertices.vertexCount);
    for (uint32_t index = 0; index < surface.vertices.vertexCount; ++index) {
        const SourceVertex source = ReadVertex(surface.vertices, index);
        const Matrix4& vertexTransform = (surface.vertices.format & 0x10) != 0 &&
            source.matrixIndex < matrixPalette.size()
                ? matrixPalette[source.matrixIndex]
                : transform;
        const Vec3 projected = Transform(vertexTransform, source.position);
        RasterVertex& output = transformed[index];
        if (projected.z != 0) {
            output.x = Divide(FixedMultiply(projected.x, width << 15), projected.z) +
                       (width << 15);
            output.y = (height << 15) -
                       Divide(FixedMultiply(projected.y, height << 15), projected.z);
        }
        output.z = projected.z;
        output.u = source.u;
        output.v = source.v;
        output.color = material != nullptr ? material->color : source.color;
    }

    auto draw = [&](uint16_t first, uint16_t second, uint16_t third) {
        if (first >= transformed.size() || second >= transformed.size() ||
            third >= transformed.size()) {
            return;
        }
        if (transformed[first].z < fixedOne || transformed[second].z < fixedOne ||
            transformed[third].z < fixedOne) {
            return;
        }
        raster.RasterizeTriangle(transformed[first], transformed[second], transformed[third],
                                 texture);
    };

    const uint8_t* indices = surface.indices.data.data();
    if (surface.primitiveType == 4) {
        for (uint32_t triangle = 0; triangle < surface.primitiveCount; ++triangle) {
            const std::size_t offset = static_cast<std::size_t>(triangle) * 6;
            draw(Read16(indices + offset), Read16(indices + offset + 2),
                 Read16(indices + offset + 4));
        }
    } else if (surface.primitiveType == 5) {
        for (uint32_t triangle = 0; triangle < surface.primitiveCount; ++triangle) {
            const std::size_t offset = static_cast<std::size_t>(triangle) * 2;
            if ((triangle & 1) == 0) {
                draw(Read16(indices + offset), Read16(indices + offset + 2),
                     Read16(indices + offset + 4));
            } else {
                draw(Read16(indices + offset), Read16(indices + offset + 4),
                     Read16(indices + offset + 2));
            }
        }
    } else if (surface.primitiveType == 6) {
        for (uint32_t triangle = 0; triangle < surface.primitiveCount; ++triangle) {
            draw(Read16(indices), Read16(indices + (triangle + 1) * 2),
                 Read16(indices + (triangle + 2) * 2));
        }
    }
}

void Renderer::Clear(uint16_t color)
{
    raster.Clear(color);
}

void Renderer::Present()
{
    raster.Present();
}

Texture* Renderer::GetCurrentTexture() const
{
    return raster.GetCurrentTexture();
}

Raster& Renderer::GetRaster()
{
    return raster;
}

} // namespace soft3d

#include "soft3d/Renderer.hpp"

#include <algorithm>
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

int32_t RoundFixed(Fixed value)
{
    return WrapAdd(value, 0xffff) >> 16;
}

uint32_t ReciprocalTable(uint32_t index)
{
    return index == 0 ? 0x7fffffffU : static_cast<uint32_t>(0x40000000ULL / index);
}

Fixed PerspectiveReciprocal(Fixed depth)
{
    uint32_t value = static_cast<uint32_t>(depth);
    value = (value ^ static_cast<uint32_t>(depth >> 31)) -
            static_cast<uint32_t>(depth >> 31);
    unsigned normalization = 16;
    if ((value & 0xffff0000U) != 0) {
        normalization = 0;
        if ((value & 0xff000000U) == 0) {
            normalization = 8;
            value <<= 8;
        }
        if ((value & 0xf0000000U) == 0) {
            normalization += 4;
            value <<= 4;
        }
        if ((value & 0xc0000000U) == 0) {
            normalization += 2;
            value <<= 2;
        }
        if ((value & 0x7fffffffU) == 0) {
            normalization += 1;
            value <<= 1;
        }
        value >>= 16;
    }
    Fixed result = static_cast<Fixed>(WrapMultiply(
        static_cast<int32_t>(ReciprocalTable(value) >> ((16 - normalization) & 0xff)), 0x400));
    return depth < 0 ? NegateFixed(result) : result;
}

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

struct SourceVertex {
    Vec3 position;
    Vec3 normal;
    Fixed u = 0;
    Fixed v = 0;
    uint32_t color = 0;
    uint32_t matrixIndex = 0;
};

SourceVertex ReadVertex(const SojVertices& vertices, uint32_t index,
                        bool fixedMatrixIndex = false)
{
    SourceVertex result;
    const uint8_t* input = vertices.data.data() + index * vertices.strideBytes;
    if ((vertices.format & 0x10) != 0) {
        result.matrixIndex = static_cast<uint32_t>(Read32(input));
        if (fixedMatrixIndex) {
            result.matrixIndex >>= 16;
        }
        input += 4;
    }
    if ((vertices.format & 1) != 0) {
        result.position = {Read32(input), Read32(input + 4), Read32(input + 8)};
        input += 12;
    }
    if ((vertices.format & 2) != 0) {
        result.normal = {Read32(input), Read32(input + 4), Read32(input + 8)};
        input += 12;
    }
    if ((vertices.format & 4) != 0) {
        result.u = Read32(input);
        result.v = Read32(input + 4);
        input += 8;
    }
    if ((vertices.format & 8) != 0) {
        result.color = static_cast<uint32_t>(Read32(input));
    }
    return result;
}

SourceVertex ReadSecondaryVertex(const SojVertices& vertices, uint32_t index, SourceVertex result)
{
    const SourceVertex secondary = ReadVertex(vertices, index, true);
    if ((vertices.format & 0x10) != 0) result.matrixIndex = secondary.matrixIndex;
    if ((vertices.format & 1) != 0) result.position = secondary.position;
    if ((vertices.format & 2) != 0) result.normal = secondary.normal;
    if ((vertices.format & 4) != 0) {
        result.u = secondary.u;
        result.v = secondary.v;
    }
    if ((vertices.format & 8) != 0) result.color = secondary.color;
    return result;
}

Matrix4 InverseNormalMatrix(const Matrix4& matrix)
{
    const Fixed a = SubtractFixed(FixedMultiply(matrix.values[5], matrix.values[10]),
                                  FixedMultiply(matrix.values[6], matrix.values[9]));
    const Fixed b = SubtractFixed(FixedMultiply(matrix.values[2], matrix.values[9]),
                                  FixedMultiply(matrix.values[1], matrix.values[10]));
    const Fixed c = SubtractFixed(FixedMultiply(matrix.values[1], matrix.values[6]),
                                  FixedMultiply(matrix.values[2], matrix.values[5]));
    const Fixed determinant = AddFixed(AddFixed(FixedMultiply(matrix.values[0], a),
        FixedMultiply(matrix.values[1], b)), FixedMultiply(matrix.values[2], c));
    if (AbsFixed(determinant) < 0x28f) {
        return matrix;
    }
    const Fixed reciprocal = FixedDivide(fixedOne, determinant);
    Matrix4 result = Matrix4::Identity();
    result.values[0] = FixedMultiply(a, reciprocal);
    result.values[1] = FixedMultiply(b, reciprocal);
    result.values[2] = FixedMultiply(c, reciprocal);
    result.values[4] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[6], matrix.values[8]),
        FixedMultiply(matrix.values[4], matrix.values[10])), reciprocal);
    result.values[5] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[0], matrix.values[10]),
        FixedMultiply(matrix.values[2], matrix.values[8])), reciprocal);
    result.values[6] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[2], matrix.values[4]),
        FixedMultiply(matrix.values[0], matrix.values[6])), reciprocal);
    result.values[8] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[4], matrix.values[9]),
        FixedMultiply(matrix.values[5], matrix.values[8])), reciprocal);
    result.values[9] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[1], matrix.values[8]),
        FixedMultiply(matrix.values[0], matrix.values[9])), reciprocal);
    result.values[10] = FixedMultiply(SubtractFixed(FixedMultiply(matrix.values[0], matrix.values[5]),
        FixedMultiply(matrix.values[1], matrix.values[4])), reciprocal);
    return result;
}

int32_t TriangleArea(const Renderer::Vertex& first, const Renderer::Vertex& second,
                     const Renderer::Vertex& third)
{
    return WrapSubtract(WrapMultiply(WrapSubtract(first.y, second.y),
        WrapSubtract(first.x, third.x)), WrapMultiply(WrapSubtract(first.x, second.x),
        WrapSubtract(first.y, third.y)));
}

Renderer::Vertex InterpolateScreenEdge(const Renderer::Vertex& from,
    const Renderer::Vertex& to, int32_t boundary, bool horizontal)
{
    const int32_t fromCoordinate = horizontal ? from.y : from.x;
    const int32_t toCoordinate = horizontal ? to.y : to.x;
    int32_t numerator = WrapSubtract(boundary, fromCoordinate);
    int32_t denominator = WrapSubtract(toCoordinate, fromCoordinate);
    if (denominator < 0) {
        numerator = NegateFixed(numerator);
        denominator = NegateFixed(denominator);
    }
    const Fixed factor = static_cast<Fixed>((static_cast<uint32_t>(
        ReciprocalTable(static_cast<uint32_t>(denominator)) >> 6) *
        static_cast<uint32_t>(numerator)) >> 8);
    Renderer::Vertex result;
    result.x = WrapAdd(from.x, WrapAdd(WrapMultiply(factor,
        WrapSubtract(to.x, from.x)), 0x8000) >> 16);
    result.y = WrapAdd(from.y, WrapAdd(WrapMultiply(factor,
        WrapSubtract(to.y, from.y)), 0x8000) >> 16);
    result.inverseDepth = AddFixed(from.inverseDepth,
        FixedMultiply(SubtractFixed(to.inverseDepth, from.inverseDepth), factor));
    result.u = AddFixed(from.u, FixedMultiply(SubtractFixed(to.u, from.u), factor));
    result.v = AddFixed(from.v, FixedMultiply(SubtractFixed(to.v, from.v), factor));
    result.intensity = AddFixed(from.intensity,
        FixedMultiply(SubtractFixed(to.intensity, from.intensity), factor));
    return result;
}

std::vector<Renderer::Vertex> ClipScreenEdge(const std::vector<Renderer::Vertex>& input,
    uint32_t flag, int32_t boundary, bool horizontal, bool keepLess)
{
    std::vector<Renderer::Vertex> output;
    if (input.empty()) {
        return output;
    }
    Renderer::Vertex previous = input.back();
    bool previousInside = keepLess ? (horizontal ? previous.y : previous.x) <= boundary
                                   : (horizontal ? previous.y : previous.x) >= boundary;
    for (const Renderer::Vertex& current : input) {
        const bool currentInside = keepLess ? (horizontal ? current.y : current.x) <= boundary
                                            : (horizontal ? current.y : current.x) >= boundary;
        if (currentInside != previousInside) {
            output.push_back(InterpolateScreenEdge(previous, current, boundary, horizontal));
        }
        if (currentInside) {
            output.push_back(current);
        }
        previous = current;
        previousInside = currentInside;
    }
    (void)flag;
    return output;
}

} // namespace

Renderer::Renderer(int rendererWidth, int rendererHeight)
    : width(rendererWidth)
    , height(rendererHeight)
    , modelMatrix(Matrix4::Identity())
    , viewMatrix(Matrix4::Identity())
    , projectionMatrix(Matrix4::Identity())
    , viewProjectionMatrix(Matrix4::Identity())
    , modelViewMatrix(Matrix4::Identity())
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
    viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}

void Renderer::SetProjectionMatrix(const Matrix4& matrix)
{
    projectionMatrix = matrix;
    viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}

void Renderer::SetOrientation(int orientation)
{
    raster.SetOrientation(orientation);
}

void Renderer::SetCullMode(uint32_t mode)
{
    cullMode = mode;
}

void Renderer::SetBlendMode(uint32_t mode)
{
    raster.SetBlendMode(mode);
}

void Renderer::SetTexture(const Texture* newTexture)
{
    texture = newTexture;
    raster.SetTexture(newTexture);
}

void Renderer::SetDetailTexture(const Texture* newTexture)
{
    raster.SetDetailTexture(newTexture);
}

void Renderer::SetTextureOffset(Fixed u, Fixed v)
{
    textureOffsetU = u;
    textureOffsetV = v;
}

void Renderer::SetTextureCoordinateMode(uint32_t mode)
{
    textureCoordinateMode = mode;
}

void Renderer::SetDepthTestEnabled(bool enabled)
{
    raster.SetDepthTestEnabled(enabled);
}

void Renderer::SetDepthWriteEnabled(bool enabled)
{
    raster.SetDepthWriteEnabled(enabled);
}

void Renderer::SetDepthBias(Fixed bias)
{
    raster.SetDepthBias(bias);
}

void Renderer::SetFlatColor(uint16_t color)
{
    raster.SetFlatColor(color);
}

void Renderer::SetLightingEnabled(bool enabled)
{
    lightingEnabled = enabled;
}

void Renderer::SetAmbientLightOnly(bool enabled)
{
    ambientLightOnly = enabled;
}

void Renderer::SetObjectIntensity(Fixed intensity)
{
    objectIntensity = intensity;
}

void Renderer::SetMinimumIntensity(Fixed intensity)
{
    minimumIntensity = intensity;
}

void Renderer::SetMaterialLighting(Fixed directional, Fixed ambient, Fixed object, Fixed animated,
                                   Fixed minimum)
{
    directionalScale = directional;
    ambientIntensity = ambient;
    objectIntensity = object;
    animatedIntensity = animated;
    minimumIntensity = minimum;
}

void Renderer::PrepareDrawState(uint32_t vertexFormat)
{
    modelViewMatrix = Multiply(modelMatrix, viewProjectionMatrix);
    if ((vertexFormat & 2) != 0 && (vertexFormat & 8) == 0 && lightingEnabled) {
        if (ambientLightOnly) {
            lightDirection = {0, 0, -fixedOne};
        } else {
            const Matrix4 normalMatrix = InverseNormalMatrix(modelMatrix);
            lightDirection = Normalize(Transform(normalMatrix, {0, 0, fixedOne}));
        }
        raster.SetShadingEnabled(true);
    } else {
        raster.SetShadingEnabled((vertexFormat & 8) != 0);
    }
}

Renderer::Vertex Renderer::TransformVertex(const SojVertices& vertices, uint32_t index,
                                           const std::vector<Matrix4>& matrixPalette,
                                           const SojVertices* secondaryVertices)
{
    SourceVertex source = ReadVertex(vertices, index);
    uint32_t format = vertices.format;
    if (secondaryVertices != nullptr) {
        source = ReadSecondaryVertex(*secondaryVertices, index, source);
        format |= secondaryVertices->format;
    }
    if ((format & 8) != 0) {
        format &= ~2U;
    }
    Vertex result;
    const bool usePalette = (format & 0x10) != 0 && source.matrixIndex < matrixPalette.size();
    const Matrix4& transform = usePalette ? matrixPalette[source.matrixIndex] : modelViewMatrix;
    result.camera = Transform(transform, source.position);
    if (usePalette) {
        const Vec3 depthRow = {transform.values[3], transform.values[7], transform.values[11]};
        result.camera.z = AddFixed(Dot(depthRow, source.position), transform.values[15]);
    }
    if (result.camera.x < NegateFixed(result.camera.z)) result.clipFlags |= 1;
    else if (result.camera.x > result.camera.z) result.clipFlags |= 2;
    if (result.camera.y < NegateFixed(result.camera.z)) result.clipFlags |= 8;
    else if (result.camera.y > result.camera.z) result.clipFlags |= 4;
    if (result.camera.z < fixedOne) result.clipFlags |= 0x10;
    if (result.camera.z > 0x8c0000) result.clipFlags |= 0x20;

    if ((format & 4) != 0) {
        if (textureCoordinateMode == 3 && texture != nullptr) {
            const Fixed normalX = AddFixed(AddFixed(FixedMultiply(modelMatrix.values[0], source.normal.x),
                FixedMultiply(modelMatrix.values[4], source.normal.y)),
                FixedMultiply(modelMatrix.values[8], source.normal.z));
            const Fixed normalY = AddFixed(AddFixed(FixedMultiply(modelMatrix.values[1], source.normal.x),
                FixedMultiply(modelMatrix.values[5], source.normal.y)),
                FixedMultiply(modelMatrix.values[9], source.normal.z));
            result.u = FixedMultiply(AddFixed(normalX >> 1, 0x8000), texture->GetFixedWidth());
            result.v = FixedMultiply(AddFixed(normalY >> 1, 0x8000), texture->GetFixedHeight());
        } else {
            result.u = AddFixed(source.u, textureOffsetU);
            result.v = AddFixed(source.v, textureOffsetV);
        }
    }
    if ((format & 8) != 0) {
        result.intensity = static_cast<Fixed>(source.color);
    } else if ((format & 2) != 0 && lightingEnabled) {
        Fixed base = AddFixed(AddFixed(objectIntensity, animatedIntensity), ambientIntensity);
        const Fixed directional = FixedMultiply(Dot(lightDirection, source.normal), directionalScale);
        if (directional > 0) {
            base = AddFixed(base, directional);
        }
        base = std::max(base, minimumIntensity);
        result.intensity = base < fixedOne ? WrapMultiply(fixedOne - base, 0x1f00) : 0;
    }
    return result;
}

void Renderer::ProjectVertex(Vertex& vertex) const
{
    const Fixed reciprocal = PerspectiveReciprocal(vertex.camera.z);
    const Fixed scaleX = WrapMultiply(width >> 1, reciprocal >> 8);
    const Fixed scaleY = WrapMultiply(height >> 1, reciprocal >> 8);
    vertex.x = RoundFixed(AddFixed(FixedMultiply(vertex.camera.x, scaleX),
                                   (width >> 1) << 16));
    vertex.y = RoundFixed(AddFixed(FixedMultiply(NegateFixed(vertex.camera.y), scaleY),
                                   (height >> 1) << 16));
    vertex.inverseDepth = reciprocal;
}

void Renderer::RasterizePolygon(const std::vector<Vertex>& polygon)
{
    if (polygon.size() < 3) {
        return;
    }
    for (std::size_t index = 1; index + 1 < polygon.size(); ++index) {
        const Vertex& first = polygon[0];
        const Vertex& second = polygon[index];
        const Vertex& third = polygon[index + 1];
        const int32_t area = TriangleArea(first, second, third);
        if (area == 0) {
            continue;
        }
        auto copy = [](const Vertex& source) {
            RasterVertex result;
            result.x = source.x << 16;
            result.y = source.y << 16;
            result.inverseDepth = source.inverseDepth;
            result.u = source.u;
            result.v = source.v;
            result.intensity = source.intensity;
            return result;
        };
        raster.RasterizeTriangle(copy(first), copy(second), copy(third));
    }
}

void Renderer::ClipAndRasterizeTriangle(const Vertex& first, const Vertex& second,
                                        const Vertex& third)
{
    const uint16_t flags = first.clipFlags | second.clipFlags | third.clipFlags;
    if ((first.clipFlags & second.clipFlags & third.clipFlags) != 0 || (flags & 0x20) != 0) {
        return;
    }
    std::vector<Vertex> polygon = {first, second, third};
    if ((flags & 0x10) != 0) {
        std::vector<Vertex> clipped;
        Vertex previous = polygon.back();
        for (const Vertex& current : polygon) {
            const bool previousInside = (previous.clipFlags & 0x10) == 0;
            const bool currentInside = (current.clipFlags & 0x10) == 0;
            if (previousInside != currentInside) {
                const Fixed factor = FixedDivide(SubtractFixed(fixedOne, previous.camera.z),
                    SubtractFixed(current.camera.z, previous.camera.z));
                Vertex intersection;
                intersection.camera.x = AddFixed(previous.camera.x,
                    FixedMultiply(SubtractFixed(current.camera.x, previous.camera.x), factor));
                intersection.camera.y = AddFixed(previous.camera.y,
                    FixedMultiply(SubtractFixed(current.camera.y, previous.camera.y), factor));
                intersection.camera.z = fixedOne;
                intersection.u = AddFixed(previous.u,
                    FixedMultiply(SubtractFixed(current.u, previous.u), factor));
                intersection.v = AddFixed(previous.v,
                    FixedMultiply(SubtractFixed(current.v, previous.v), factor));
                intersection.intensity = AddFixed(previous.intensity,
                    FixedMultiply(SubtractFixed(current.intensity, previous.intensity), factor));
                intersection.inverseDepth = 0x1000000;
                intersection.x = RoundFixed(AddFixed(FixedMultiply(intersection.camera.x,
                    (width >> 1) << 16), SubtractFixed((width >> 1) << 16, 0x8000)));
                intersection.y = RoundFixed(AddFixed(FixedMultiply(NegateFixed(intersection.camera.y),
                    (height >> 1) << 16), SubtractFixed((height >> 1) << 16, 0x8000)));
                clipped.push_back(intersection);
            }
            if (currentInside) {
                Vertex projected = current;
                ProjectVertex(projected);
                clipped.push_back(projected);
            }
            previous = current;
        }
        polygon = std::move(clipped);
    } else {
        for (Vertex& vertex : polygon) {
            ProjectVertex(vertex);
        }
    }
    if (polygon.size() < 3) {
        return;
    }
    const int32_t area = TriangleArea(polygon[0], polygon[1], polygon[2]);
    if (area == 0 || (cullMode == 1 && area < 0) || (cullMode == 2 && area > 0)) {
        return;
    }
    if ((flags & 1) != 0) polygon = ClipScreenEdge(polygon, 1, 0, false, false);
    if ((flags & 2) != 0) polygon = ClipScreenEdge(polygon, 2, width, false, true);
    if ((flags & 4) != 0) polygon = ClipScreenEdge(polygon, 4, 0, true, false);
    if ((flags & 8) != 0) polygon = ClipScreenEdge(polygon, 8, height, true, true);
    RasterizePolygon(polygon);
}

void Renderer::DrawSequential(const SojVertices& vertices, uint32_t primitiveType,
                              uint32_t firstVertex, uint32_t primitiveCount)
{
    if (primitiveType < 4 || primitiveType > 6 || vertices.data.empty()) {
        return;
    }
    PrepareDrawState(vertices.format);
    const uint32_t vertexCount = primitiveType == 4 ? primitiveCount * 3 : primitiveCount + 2;
    transformed.resize(vertexCount);
    for (uint32_t index = 0; index < vertexCount; ++index) {
        transformed[index] = TransformVertex(vertices, firstVertex + index, {});
    }
    uint32_t first = 0;
    uint32_t second = 1;
    uint32_t third = 2;
    bool alternate = false;
    for (uint32_t triangle = 0; triangle < primitiveCount; ++triangle) {
        ClipAndRasterizeTriangle(transformed[first], transformed[alternate ? third : second],
                                 transformed[alternate ? second : third]);
        if (primitiveType == 4) {
            first += 3;
            second += 3;
            third += 3;
        } else if (primitiveType == 5) {
            alternate = !alternate;
            first = second;
            second = third++;
        } else {
            second = third++;
        }
    }
}

void Renderer::DrawIndexed(const SojVertices& vertices, const SojIndices& indices,
                           uint32_t primitiveType, uint32_t firstIndex, uint32_t primitiveCount,
                           uint32_t firstVertex)
{
    DrawIndexedBuffers(vertices, nullptr, indices, primitiveType, firstIndex, primitiveCount,
                       firstVertex);
}

void Renderer::DrawIndexedBuffers(const SojVertices& vertices,
                                  const SojVertices* secondaryVertices, const SojIndices& indices,
                                  uint32_t primitiveType, uint32_t firstIndex,
                                  uint32_t primitiveCount, uint32_t firstVertex,
                                  const std::vector<Matrix4>& matrixPalette)
{
    if (primitiveType < 4 || primitiveType > 6 || vertices.data.empty() || indices.data.empty()) {
        return;
    }
    uint32_t format = vertices.format;
    if (secondaryVertices != nullptr) {
        format |= secondaryVertices->format;
    }
    if ((format & 8) != 0) {
        format &= ~2U;
    }
    PrepareDrawState(format);
    transformed.resize(vertices.vertexCount);
    for (uint32_t index = 0; index < vertices.vertexCount; ++index) {
        transformed[index] = TransformVertex(vertices, firstVertex + index, matrixPalette,
                                             secondaryVertices);
    }
    uint32_t first = firstIndex;
    uint32_t second = firstIndex + 1;
    uint32_t third = firstIndex + 2;
    bool alternate = false;
    for (uint32_t triangle = 0; triangle < primitiveCount; ++triangle) {
        const uint32_t a = Read16(indices.data.data() + first * 2);
        const uint32_t b = Read16(indices.data.data() + second * 2);
        const uint32_t c = Read16(indices.data.data() + third * 2);
        if (a < transformed.size() && b < transformed.size() && c < transformed.size()) {
            ClipAndRasterizeTriangle(transformed[a], transformed[alternate ? c : b],
                                     transformed[alternate ? b : c]);
        }
        if (primitiveType == 4) {
            first += 3;
            second += 3;
            third += 3;
        } else if (primitiveType == 5) {
            alternate = !alternate;
            first = second;
            second = third++;
        } else {
            second = third++;
        }
    }
}

void Renderer::DrawIndexedTextured(const SojVertices& vertices, const SojIndices& indices,
                                   uint32_t primitiveType, uint32_t firstIndex,
                                   uint32_t primitiveCount, uint32_t firstVertex)
{
    DrawIndexedBuffers(vertices, nullptr, indices, primitiveType, firstIndex, primitiveCount,
                       firstVertex);
}

void Renderer::DrawIndexedTextured(const SojVertices& vertices,
                                   const SojVertices& secondaryVertices, const SojIndices& indices,
                                   uint32_t primitiveType, uint32_t firstIndex,
                                   uint32_t primitiveCount, uint32_t firstVertex,
                                   const std::vector<Matrix4>& matrixPalette)
{
    DrawIndexedBuffers(vertices, &secondaryVertices, indices, primitiveType, firstIndex,
                       primitiveCount, firstVertex, matrixPalette);
}

void Renderer::DrawIndexedSurface(const SojSurface& surface, const SojObject* material,
                                  const Texture* newTexture, const Texture* detailTexture,
                                  const std::vector<Matrix4>& matrixPalette,
                                  Fixed offsetU, Fixed offsetV, Fixed materialIntensity,
                                  bool cullingDisabled)
{
    if (surface.vertices.vertexCount == 0 || surface.indices.data.empty()) {
        return;
    }
    if (material != nullptr) {
        SetCullMode(cullingDisabled ? 0 : material->cullMode);
        SetTexture(newTexture);
        SetDetailTexture(detailTexture);
        SetTextureCoordinateMode(material->textureCoordinateMode);
        SetBlendMode(material->blendMode);
        SetDepthWriteEnabled(material->blendMode == 0);
        SetTextureOffset(offsetU, offsetV);
        SetMaterialLighting(material->directionalScale, material->ambientIntensity,
                            objectIntensity, materialIntensity, minimumIntensity);
        if (newTexture == nullptr) {
            SetFlatColor(material->color);
        }
    }
    if (matrixPalette.empty()) {
        DrawIndexed(surface.vertices, surface.indices, surface.primitiveType, 0,
                    surface.primitiveCount);
        return;
    }
    const SojVertices& vertices = surface.vertices;
    if (surface.primitiveType < 4 || surface.primitiveType > 6) {
        return;
    }
    PrepareDrawState(vertices.format);
    transformed.resize(vertices.vertexCount);
    for (uint32_t index = 0; index < vertices.vertexCount; ++index) {
        transformed[index] = TransformVertex(vertices, index, matrixPalette);
    }
    uint32_t first = 0;
    uint32_t second = 1;
    uint32_t third = 2;
    bool alternate = false;
    for (uint32_t triangle = 0; triangle < surface.primitiveCount; ++triangle) {
        const uint32_t a = Read16(surface.indices.data.data() + first * 2);
        const uint32_t b = Read16(surface.indices.data.data() + second * 2);
        const uint32_t c = Read16(surface.indices.data.data() + third * 2);
        if (a < transformed.size() && b < transformed.size() && c < transformed.size()) {
            ClipAndRasterizeTriangle(transformed[a], transformed[alternate ? c : b],
                                     transformed[alternate ? b : c]);
        }
        if (surface.primitiveType == 4) {
            first += 3;
            second += 3;
            third += 3;
        } else if (surface.primitiveType == 5) {
            alternate = !alternate;
            first = second;
            second = third++;
        } else {
            second = third++;
        }
    }
}

void Renderer::Clear(uint16_t color)
{
    raster.Clear(color);
}

void Renderer::SelectShader(BasicShader* shader)
{
    raster.SelectShader(shader);
}

void Renderer::Present()
{
    raster.Present();
}

bool Renderer::ProjectPoint(const Vec3& point, Vec3& projected) const
{
    const Matrix4 transform = Multiply(modelMatrix, viewProjectionMatrix);
    const Vec3 camera = Transform(transform, point);
    if (camera.z < fixedOne) {
        return false;
    }
    const Fixed reciprocal = FixedDivide(0x1000000, camera.z);
    const Fixed scaleX = (width >> 1) << 16;
    const Fixed scaleY = (height >> 1) << 16;
    projected.x = AddFixed(SubtractFixed(scaleX, 0x8000),
        FixedMultiply(FixedMultiply(camera.x, reciprocal) >> 8, scaleX));
    projected.y = SubtractFixed(SubtractFixed(scaleY, 0x8000),
        FixedMultiply(FixedMultiply(camera.y, reciprocal) >> 8, scaleY));
    projected.z = reciprocal >> 8;
    return true;
}

uint16_t Renderer::GetDepth(int32_t x, int32_t y) const
{
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(frameTexture.GetPixels());
    return static_cast<uint16_t>(pixels[y * width + x] >> 16);
}

int Renderer::GetWidth() const { return width; }
int Renderer::GetHeight() const { return height; }
Fixed Renderer::GetObjectIntensity() const { return objectIntensity; }
Fixed Renderer::GetMinimumIntensity() const { return minimumIntensity; }
const Matrix4& Renderer::GetViewMatrix() const { return viewMatrix; }
const Matrix4& Renderer::GetViewProjectionMatrix() const { return viewProjectionMatrix; }
const Matrix4& Renderer::GetProjectionMatrix() const { return projectionMatrix; }

Texture* Renderer::GetCurrentTexture() const
{
    return raster.GetCurrentTexture();
}

Raster& Renderer::GetRaster()
{
    return raster;
}

} // namespace soft3d

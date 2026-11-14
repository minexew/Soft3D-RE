#include "soft3d/Bsp.hpp"

#include "soft3d/Renderer.hpp"
#include "soft3d/Texture.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <utility>

namespace soft3d {
namespace {

uint32_t Read32(const uint8_t* input)
{
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8) |
           (static_cast<uint32_t>(input[2]) << 16) |
           (static_cast<uint32_t>(input[3]) << 24);
}

Fixed AddFixed(Fixed left, Fixed right)
{
    return static_cast<Fixed>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

Fixed SubtractFixed(Fixed left, Fixed right)
{
    return static_cast<Fixed>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

Fixed Dot(const Vec3& left, const Vec3& right)
{
    const int64_t product = static_cast<int64_t>(left.x) * right.x +
                            static_cast<int64_t>(left.y) * right.y +
                            static_cast<int64_t>(left.z) * right.z;
    return static_cast<Fixed>(static_cast<uint32_t>(static_cast<uint64_t>(product) >> 16));
}

Fixed PlaneDistance(const BspPlane& plane, const Vec3& point)
{
    if (plane.type < 3) {
        return AddFixed((&point.x)[plane.type], plane.distance);
    }
    return AddFixed(Dot(plane.normal, point), plane.distance);
}

Vec3 TransformDirection(const Matrix4& matrix, const Vec3& value)
{
    return {
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[0], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[4], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[8], value.z))),
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[1], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[5], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[9], value.z))),
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[2], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[6], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[10], value.z)))
    };
}

uint32_t ReciprocalTable(uint32_t index)
{
    if (index == 0) {
        return 0x7fffffff;
    }
    return static_cast<uint32_t>(0x40000000ULL / index);
}

uint32_t ReciprocalMagnitude(uint32_t value)
{
    if (value < 0x10000) {
        return ReciprocalTable(value) << 2;
    }
    if (value < 0x40000) {
        return ReciprocalTable(value >> 2);
    }
    if (value < 0x100000) {
        return ReciprocalTable(value >> 4) >> 2;
    }
    if (value < 0x400000) {
        return ReciprocalTable(value >> 6) >> 4;
    }
    if (value < 0x1000000) {
        return ReciprocalTable(value >> 8) >> 6;
    }
    if (value < 0x4000000) {
        return ReciprocalTable(value >> 10) >> 8;
    }
    return ReciprocalTable(value >> 12) >> 10;
}

Fixed ReciprocalSigned(Fixed value)
{
    return value < 1 ? -static_cast<Fixed>(ReciprocalMagnitude(
                           static_cast<uint32_t>(-static_cast<int64_t>(value))))
                     : static_cast<Fixed>(ReciprocalMagnitude(static_cast<uint32_t>(value)));
}

uint32_t PerspectiveReciprocal(Fixed depth)
{
    uint32_t value = static_cast<uint32_t>(depth) >> 5;
    uint32_t normalization = 16;
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
    return (ReciprocalTable(value) >> ((16 - normalization) & 0xff)) << 5;
}

int64_t Edge(Fixed ax, Fixed ay, Fixed bx, Fixed by, int64_t x, int64_t y)
{
    return (x - ax) * static_cast<int64_t>(by - ay) -
           (y - ay) * static_cast<int64_t>(bx - ax);
}

struct ClipVertex {
    Vec3 world;
    Vec3 camera;
};

std::vector<ClipVertex> ClipPolygon(const std::vector<ClipVertex>& input, const BspPlane& plane)
{
    std::vector<ClipVertex> output;
    if (input.empty()) {
        return output;
    }
    ClipVertex previous = input.back();
    Fixed previousDistance = PlaneDistance(plane, previous.camera);
    for (const ClipVertex& current : input) {
        const Fixed currentDistance = PlaneDistance(plane, current.camera);
        const bool currentInside = currentDistance >= 0;
        const bool previousInside = previousDistance >= 0;
        if (currentInside != previousInside) {
            const Fixed denominator = SubtractFixed(previousDistance, currentDistance);
            if (denominator != 0) {
                const Fixed factor = static_cast<Fixed>(
                    (static_cast<int64_t>(previousDistance) * 0x10000) / denominator);
                ClipVertex intersection;
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    const Fixed worldDelta = SubtractFixed((&current.world.x)[axis],
                                                           (&previous.world.x)[axis]);
                    const Fixed cameraDelta = SubtractFixed((&current.camera.x)[axis],
                                                            (&previous.camera.x)[axis]);
                    (&intersection.world.x)[axis] = AddFixed((&previous.world.x)[axis],
                        FixedMultiply(worldDelta, factor));
                    (&intersection.camera.x)[axis] = AddFixed((&previous.camera.x)[axis],
                        FixedMultiply(cameraDelta, factor));
                }
                output.push_back(intersection);
            }
        }
        if (currentInside) {
            output.push_back(current);
        }
        previous = current;
        previousDistance = currentDistance;
    }
    return output;
}

} // namespace

result LoadS3dStyleResource(Bsp* bsp, const char16_t* fileName, s3dFilePak* filePak)
{
    if (bsp == nullptr || fileName == nullptr || filePak == nullptr) {
        return S3D_PARAM;
    }
    bsp->ResetLightStyles();
    result status = filePak->Select(fileName);
    if (status != S3D_OK) {
        return S3D_FILE;
    }
    filePak->Seek(0, SEEK_END);
    const int byteCount = filePak->Tell();
    if (byteCount == S3D_EOF || byteCount < 0) {
        return S3D_UNKNOWN;
    }
    try {
        std::vector<uint8_t> bytes(static_cast<std::size_t>(byteCount));
        filePak->Seek(0, SEEK_SET);
        status = filePak->Read(bytes.data(), byteCount);
        if (status != S3D_OK) {
            return status;
        }
        std::size_t position = 0;
        for (std::size_t index = 0; index < bsp->lightStyles.size(); ++index) {
            if (position >= bytes.size()) {
                bsp->ResetLightStyles();
                return S3D_FORMAT;
            }
            BspLightStyle& style = bsp->lightStyles[index];
            style.frameCount = bytes[position++];
            style.frames.reserve(style.frameCount);
            style.enabled = 1;
            bool terminated = false;
            while (position < bytes.size()) {
                const uint8_t value = bytes[position++];
                if (value == 0xff) {
                    terminated = true;
                    break;
                }
                if (value > 0x10 || style.frames.size() >= style.frameCount) {
                    bsp->ResetLightStyles();
                    return S3D_FORMAT;
                }
                style.frames.push_back(value);
            }
            if (!terminated) {
                bsp->ResetLightStyles();
                return S3D_FORMAT;
            }
        }
        return S3D_OK;
    } catch (const std::bad_alloc&) {
        bsp->ResetLightStyles();
        return S3D_MEMORY;
    }
}

void Bsp::AdvanceLightStyles()
{
    for (std::size_t index = 1; index < lightStyles.size() - 1; ++index) {
        BspLightStyle& style = lightStyles[index];
        if (style.enabled == 0 || style.frameCount == 0 || style.frames.empty()) {
            style.light = 0;
            continue;
        }
        const std::size_t position = std::min<std::size_t>(style.position,
                                                           style.frames.size() - 1);
        style.light = style.frames[position];
        style.position = static_cast<uint8_t>((position + 1) % style.frameCount);
    }
}

void Bsp::SetLightStyleEnabled(uint8_t index, uint8_t enabled)
{
    lightStyles[index].enabled = enabled;
    AdvanceLightStyles();
}

void Bsp::Update()
{
    AdvanceLightStyles();
}

int32_t Bsp::FindLeaf(const Vec3& position) const
{
    if (nodes.empty() || leaves.empty()) {
        return -1;
    }
    int32_t index = 0;
    while (index >= 0) {
        const BspNode& node = nodes[static_cast<std::size_t>(index)];
        const Fixed distance = PlaneDistance(planes[node.planeIndex], position);
        index = node.children[distance < 0 ? 0 : 1];
    }
    return -index - 1;
}

std::vector<uint8_t> Bsp::DecompressPVS(int32_t cluster) const
{
    if (pvs.size() < 4 || cluster < 0) {
        return {};
    }
    const uint32_t clusterCount = Read32(pvs.data());
    const std::size_t byteCount = (clusterCount + 7) >> 3;
    if (static_cast<uint32_t>(cluster) >= clusterCount ||
        static_cast<std::size_t>(cluster + 1) * 4 > pvs.size()) {
        return {};
    }
    const uint32_t offset = Read32(pvs.data() + cluster * 4 + 4);
    if (offset > pvs.size()) {
        return {};
    }
    std::vector<uint8_t> output;
    output.reserve(byteCount);
    std::size_t position = offset;
    while (output.size() < byteCount && position < pvs.size()) {
        const uint8_t value = pvs[position++];
        if (value == 0) {
            if (position >= pvs.size()) {
                return {};
            }
            const uint8_t count = pvs[position++];
            if (count > byteCount - output.size()) {
                return {};
            }
            output.insert(output.end(), count, 0);
        } else {
            output.push_back(value);
        }
    }
    if (output.size() != byteCount) {
        return {};
    }
    return output;
}

void Bsp::MarkVisibleLeaves()
{
    if (currentCluster == previousCluster) {
        return;
    }
    previousCluster = currentCluster;
    ++leafMarkCounter;
    decompressedPVS = DecompressPVS(currentCluster);
    if (currentCluster < 0) {
        return;
    }
    for (BspLeaf& leaf : leaves) {
        if (leaf.contents == 0xffff) {
            continue;
        }
        const uint32_t cluster = leaf.contents;
        if (cluster / 8 < decompressedPVS.size() &&
            ((decompressedPVS[cluster / 8] >> (cluster & 7)) & 1) != 0) {
            leaf.renderMarker = static_cast<int32_t>(leafMarkCounter);
        }
    }
}

void Bsp::SubmitFace(uint32_t index, uint32_t)
{
    if (index >= faces.size() || visibleFaceFlags[index] != 0) {
        return;
    }
    const BspFace& face = faces[index];
    const Fixed distance = PlaneDistance(planes[face.planeIndex], renderState.cameraPosition);
    if ((distance > 0x28f && (face.planeSide & 0x800) != 0) ||
        (distance < -0x28f && (face.planeSide & 0x800) == 0)) {
        return;
    }
    visibleFaceFlags[index] = 1;
    visibleFaces.push_back(index);
}

void Bsp::TraverseVisibleNode(int32_t index, uint32_t planeMask)
{
    if (index < 0) {
        const int32_t leafIndex = -index - 1;
        if (leafIndex < 0 || static_cast<std::size_t>(leafIndex) >= leaves.size()) {
            return;
        }
        BspLeaf& leaf = leaves[static_cast<std::size_t>(leafIndex)];
        if (leaf.renderMarker != static_cast<int32_t>(leafMarkCounter)) {
            return;
        }
        for (uint16_t offset = 0; offset < leaf.visibleFaceCount; ++offset) {
            SubmitFace(visFaces[leaf.firstVisibleFace + offset], planeMask);
        }
        for (uint16_t offset = 0; offset < leaf.visibleEntityCount; ++offset) {
            const uint32_t entityIndex = visibleEntityIndices[leaf.firstVisibleEntity + offset];
            if (entityIndex < entities.size()) {
                entities[entityIndex]->lastVisibleFrame = frameCounter;
            }
        }
        for (uint16_t offset = 0; offset < leaf.visibleBrushCount; ++offset) {
            const uint32_t brushIndex = leafVisibleBrushes[leaf.firstVisibleBrush + offset];
            visBrushes[brushIndex].renderMarker = frameCounter;
            visBrushes[brushIndex].planeMask = static_cast<int16_t>(planeMask);
        }
        return;
    }
    if (static_cast<std::size_t>(index) >= nodes.size()) {
        return;
    }
    const BspNode& node = nodes[static_cast<std::size_t>(index)];
    for (const BspPlane& plane : renderState.clipPlanes) {
        Vec3 positive = {node.maxs[0], node.maxs[1], node.maxs[2]};
        Vec3 negative = {node.mins[0], node.mins[1], node.mins[2]};
        for (std::size_t axis = 0; axis < 3; ++axis) {
            if ((&plane.normal.x)[axis] >= 0) {
                (&positive.x)[axis] = node.maxs[axis];
                (&negative.x)[axis] = node.mins[axis];
            } else {
                (&positive.x)[axis] = node.mins[axis];
                (&negative.x)[axis] = node.maxs[axis];
            }
        }
        const Vec3 positiveFixed = {positive.x << 16, positive.y << 16, positive.z << 16};
        if (PlaneDistance(plane, positiveFixed) < 0) {
            return;
        }
    }
    const Fixed distance = PlaneDistance(planes[node.planeIndex], renderState.cameraPosition);
    const int near = distance < 0 ? 0 : 1;
    TraverseVisibleNode(node.children[near], planeMask);
    for (uint16_t offset = 0; offset < node.faceCount; ++offset) {
        SubmitFace(node.firstFace + offset, planeMask);
    }
    TraverseVisibleNode(node.children[1 - near], planeMask);
}

void Bsp::PrepareRender(const BspRenderState& state)
{
    renderState = state;
    visibleFaces.clear();
    visibleFaceFlags.assign(faces.size(), 0);
    if (nodes.empty()) {
        currentCluster = -1;
        return;
    }
    ++frameCounter;
    const int32_t leafIndex = FindLeaf(renderState.cameraPosition);
    currentCluster = leafIndex < 0 ? -1 : static_cast<int16_t>(leaves[leafIndex].contents);
    MarkVisibleLeaves();
    TraverseVisibleNode(0, 0x1f);
    for (const BspVisBrush& brush : visBrushes) {
        if (brush.renderMarker != frameCounter) {
            continue;
        }
        for (uint16_t offset = 0; offset < brush.faceCount; ++offset) {
            SubmitFace(brush.firstFace + offset, brush.planeMask);
        }
    }
    for (const uint32_t faceIndex : visibleFaces) {
        BuildLightmap(faces[faceIndex]);
    }
}

void Bsp::SubmitVisibleEntities(RenderQueues& queues)
{
    for (const uint32_t index : visibleEntityIndices) {
        if (index >= entities.size()) {
            continue;
        }
        BspEntity& entity = *entities[index];
        if (entity.lastVisibleFrame == frameCounter) {
            entity.Submit(queues);
            --entity.lastVisibleFrame;
        }
    }
}

Bsp::LightmapMapping Bsp::BuildLightmapMapping(const BspFace& face) const
{
    const BspTexInfo& info = texInfo[static_cast<std::size_t>(face.texInfoIndex)];
    const Vec3 uAxis = {info.textureMapping[0], info.textureMapping[1], info.textureMapping[2]};
    const Vec3 vAxis = {info.textureMapping[4], info.textureMapping[5], info.textureMapping[6]};
    const Vec3 rotatedU = TransformDirection(renderState.worldToCamera, uAxis);
    const Vec3 rotatedV = TransformDirection(renderState.worldToCamera, vAxis);
    LightmapMapping mapping;
    mapping.rayUX = FixedMultiply(rotatedU.x, renderState.inverseScreenScaleX);
    mapping.rayUY = FixedMultiply(-rotatedU.y, renderState.inverseScreenScaleY);
    mapping.rayUZ = rotatedU.z;
    mapping.rayVX = FixedMultiply(rotatedV.x, renderState.inverseScreenScaleX);
    mapping.rayVY = FixedMultiply(-rotatedV.y, renderState.inverseScreenScaleY);
    mapping.rayVZ = rotatedV.z;
    mapping.baseU = AddFixed(SubtractFixed(Dot(renderState.cameraPosition, uAxis),
                                           face.lightmapMin[0] * 0x10000), info.textureMapping[3]);
    mapping.baseV = AddFixed(SubtractFixed(Dot(renderState.cameraPosition, vAxis),
                                           face.lightmapMin[1] * 0x10000), info.textureMapping[7]);
    mapping.maxU = (face.lightmapSize[0] << 16) - 1;
    mapping.maxV = (face.lightmapSize[1] << 16) - 1;
    return mapping;
}

result Bsp::BuildLightmap(BspFace& face)
{
    face.lightmapPixels.clear();
    if (face.lightmapOffset < 0 || face.texInfoIndex < 0) {
        return S3D_OK;
    }
    const std::size_t sampleCount = static_cast<std::size_t>(face.lightmapSampleCount);
    std::size_t blockCount = 1;
    for (std::size_t index = 1; index < face.styles.size(); ++index) {
        if (face.styles[index] != 0) {
            ++blockCount;
        }
    }
    const std::size_t lightmapOffset = static_cast<std::size_t>(face.lightmapOffset);
    if (sampleCount == 0 || lightmapOffset > lightmapData.size() ||
        sampleCount * blockCount > lightmapData.size() - lightmapOffset) {
        return S3D_FORMAT;
    }
    std::vector<int32_t> intensity(sampleCount, 0);
    const uint8_t* samples = lightmapData.data() + lightmapOffset;
    for (std::size_t index = 0; index < sampleCount; ++index) {
        intensity[index] = samples[index] * 0x100;
    }
    std::size_t offset = sampleCount;
    for (std::size_t styleIndex = 1; styleIndex < face.styles.size(); ++styleIndex) {
        const uint8_t serializedStyle = face.styles[styleIndex];
        if (serializedStyle == 0) {
            continue;
        }
        const int shift = static_cast<int8_t>(lightStyles[serializedStyle].light);
        for (std::size_t index = 0; index < sampleCount; ++index) {
            const int32_t value = shift >= 0 ? samples[offset + index] << shift
                                             : samples[offset + index] >> -shift;
            intensity[index] = static_cast<int32_t>(static_cast<uint32_t>(intensity[index]) +
                                                    static_cast<uint32_t>(value));
        }
        offset += sampleCount;
    }
    for (int32_t& value : intensity) {
        if (value < 0xff00) {
            value = (0xff00 - value) >> 3;
            if (value < 0x20) {
                value = 0x20;
            }
        } else {
            value = 0x20;
        }
    }
    const BspTexInfo& info = texInfo[static_cast<std::size_t>(face.texInfoIndex)];
    const Texture* texture = info.texture;
    if (texture == nullptr || texture->GetPixels() == nullptr || texture->GetPalette() == nullptr ||
        texture->GetWidth() == 0 || texture->GetHeight() == 0) {
        return S3D_FORMAT;
    }
    const std::size_t paletteBytes = (texture->GetType() & 0xc0) == 0x40
        ? static_cast<std::size_t>(texture->GetPaletteSize()) << 6
        : static_cast<std::size_t>(texture->GetPaletteSize()) << 1;
    const int width = face.lightmapPowerOfTwoWidth;
    const int height = face.lightmapSize[1];
    const int sampleWidth = (face.lightmapSize[0] >> 4) + 1;
    const int generatedWidth = (face.lightmapSize[0] >> 4) << 4;
    const int generatedHeight = (face.lightmapSize[1] >> 4) << 4;
    face.lightmapPixels.resize(static_cast<std::size_t>(width) * height);
    const int scrollU = FixedMultiply(static_cast<Fixed>(frameCounter << 16), face.textureMin[0]) >> 16;
    const int scrollV = FixedMultiply(static_cast<Fixed>(frameCounter << 16), face.textureMin[1]) >> 16;
    for (int y = 0; y < generatedHeight; ++y) {
        const int sampleY = y >> 4;
        const int fractionY = y & 15;
        for (int x = 0; x < generatedWidth; ++x) {
            const int sampleX = x >> 4;
            const int fractionX = x & 15;
            const std::size_t top = sampleY * sampleWidth + sampleX;
            const std::size_t bottom = top + sampleWidth;
            if (bottom + 1 >= intensity.size()) {
                std::fprintf(stderr, "lightmap sample overflow: count=%zu width=%d height=%d sw=%d x=%d y=%d top=%zu bottom=%zu\n",
                             intensity.size(), width, height, sampleWidth, x, y, top, bottom);
                return S3D_FORMAT;
            }
            const int32_t topLeft = intensity[top];
            const int32_t topRight = intensity[top + 1];
            const int32_t bottomLeft = intensity[bottom];
            const int32_t bottomRight = intensity[bottom + 1];
            const int32_t left = topLeft + ((bottomLeft - topLeft) >> 4) * fractionY;
            const int32_t right = topRight + ((bottomRight - topRight) >> 4) * fractionY;
            const int32_t value = left + ((right - left) >> 4) * fractionX;
            const int textureX = static_cast<int>((texture->GetWidth() - 1) &
                static_cast<uint32_t>(face.lightmapMin[0] + scrollU + x));
            const int textureY = static_cast<int>((texture->GetHeight() - 1) &
                static_cast<uint32_t>(face.lightmapMin[1] + scrollV + y));
            const uint8_t texel = texture->GetPixels()[textureY * texture->GetWidth() + textureX];
            std::size_t paletteOffset = static_cast<std::size_t>(texel) * 64 +
                ((static_cast<uint32_t>(value) & 0xff00) >> 7);
            paletteOffset = std::min(paletteOffset, paletteBytes / 2 - 1);
            uint16_t color;
            std::memcpy(&color, texture->GetPalette() + paletteOffset * 2, sizeof(color));
            face.lightmapPixels[static_cast<std::size_t>(y) * width + x] = color;
        }
    }
    return S3D_OK;
}

std::vector<Bsp::ProjectedVertex> Bsp::ProjectFace(const BspFace& face)
{
    std::vector<ClipVertex> polygon;
    polygon.reserve(face.edgeCount);
    for (uint32_t index = 0; index < face.edgeCount; ++index) {
        const int32_t faceEdge = faceEdges[face.firstEdge + index];
        const std::size_t edgeIndex = static_cast<std::size_t>(
            faceEdge < 0 ? -static_cast<int64_t>(faceEdge) : faceEdge);
        const BspEdge& edge = edges[edgeIndex];
        const Vec3& world = vertices[faceEdge < 0 ? edge.vertices[1] : edge.vertices[0]];
        polygon.push_back({world, Transform(renderState.worldToCamera, world)});
    }
    BspPlane nearPlane;
    nearPlane.normal = {0, 0, fixedOne};
    nearPlane.distance = -fixedOne;
    nearPlane.type = 2;
    polygon = ClipPolygon(polygon, nearPlane);
    for (const BspPlane& plane : renderState.clipPlanes) {
        polygon = ClipPolygon(polygon, plane);
        if (polygon.empty()) {
            break;
        }
    }
    std::vector<ProjectedVertex> projected;
    projected.reserve(polygon.size());
    for (const ClipVertex& vertex : polygon) {
        const Fixed z = std::max(vertex.camera.z, fixedOne);
        const uint32_t reciprocal = PerspectiveReciprocal(z);
        const Fixed x = AddFixed(FixedMultiply(
            FixedMultiply(vertex.camera.x, static_cast<Fixed>(reciprocal)) >> 8,
            renderState.screenScaleX), renderState.screenCenterX);
        const Fixed y = SubtractFixed(renderState.screenCenterY, FixedMultiply(
            FixedMultiply(vertex.camera.y, static_cast<Fixed>(reciprocal)) >> 8,
            renderState.screenScaleY));
        projected.push_back({vertex.world, x, y, static_cast<Fixed>(reciprocal >> 8)});
    }
    return projected;
}

void Bsp::RasterizeFace(const BspFace& face, const LightmapMapping& mapping,
                        const std::vector<ProjectedVertex>& polygon)
{
    if (polygon.size() < 3 || (face.texInfoIndex >= 0 && face.lightmapPixels.empty()) ||
        renderer == nullptr || renderer->GetCurrentTexture() == nullptr) {
        return;
    }
    Texture* frameTexture = renderer->GetCurrentTexture();
    const int width = static_cast<int>(frameTexture->GetWidth());
    const int height = static_cast<int>(frameTexture->GetHeight());
    uint32_t* output = reinterpret_cast<uint32_t*>(frameTexture->GetPixels());
    const BspPlane& plane = planes[face.planeIndex];
    const Vec3 rotatedNormal = TransformDirection(renderState.worldToCamera, plane.normal);
    const Fixed reciprocal = static_cast<Fixed>(
        static_cast<uint32_t>(ReciprocalSigned(-PlaneDistance(plane, renderState.cameraPosition))) << 8);
    Fixed depthX = FixedMultiply(FixedMultiply(rotatedNormal.x, reciprocal),
                                 renderState.inverseScreenScaleX);
    if (depthX == 0) {
        depthX = 1;
    }
    const Fixed depthY = -FixedMultiply(FixedMultiply(rotatedNormal.y, reciprocal),
                                        renderState.inverseScreenScaleY);
    const Fixed depthBase = FixedMultiply(rotatedNormal.z, reciprocal);
    for (std::size_t triangle = 1; triangle + 1 < polygon.size(); ++triangle) {
        const ProjectedVertex& first = polygon[0];
        const ProjectedVertex& second = polygon[triangle];
        const ProjectedVertex& third = polygon[triangle + 1];
        const int minX = std::max(0, static_cast<int>(std::min({first.x, second.x, third.x}) >> 16));
        const int maxX = std::min(width - 1,
            static_cast<int>(std::max({first.x, second.x, third.x}) >> 16));
        const int minY = std::max(0, static_cast<int>(std::min({first.y, second.y, third.y}) >> 16));
        const int maxY = std::min(height - 1,
            static_cast<int>(std::max({first.y, second.y, third.y}) >> 16));
        const int64_t area = Edge(first.x, first.y, second.x, second.y, third.x, third.y);
        if (area == 0) {
            continue;
        }
        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const int64_t sampleX = static_cast<int64_t>(x) << 16;
                const int64_t sampleY = static_cast<int64_t>(y) << 16;
                const int64_t w0 = Edge(second.x, second.y, third.x, third.y, sampleX, sampleY);
                const int64_t w1 = Edge(third.x, third.y, first.x, first.y, sampleX, sampleY);
                const int64_t w2 = Edge(first.x, first.y, second.x, second.y, sampleX, sampleY);
                if (area < 0) {
                    if (w0 > 0 || w1 > 0 || w2 > 0) {
                        continue;
                    }
                } else if (w0 < 0 || w1 < 0 || w2 < 0) {
                    continue;
                }
                const Fixed dx = static_cast<Fixed>(sampleX) - renderState.screenCenterX;
                const Fixed dy = static_cast<Fixed>(sampleY) - renderState.screenCenterY;
                const Fixed depth = AddFixed(depthBase, AddFixed(FixedMultiply(dx, depthX),
                                                                  FixedMultiply(dy, depthY)));
                if (depth <= 0) {
                    continue;
                }
                const uint16_t depth16 = face.texInfoIndex < 0 ? 1 : static_cast<uint16_t>(depth >> 8);
                uint32_t& pixel = output[static_cast<std::size_t>(y) * width + x];
                const uint16_t oldDepth = static_cast<uint16_t>(pixel >> 16);
                if (oldDepth != 0 && depth16 >= oldDepth) {
                    continue;
                }
                if (face.texInfoIndex < 0) {
                    pixel = 0x00010000;
                    continue;
                }
                const Fixed rayU = AddFixed(AddFixed(FixedMultiply(dx, mapping.rayUX),
                    FixedMultiply(dy, mapping.rayUY)), mapping.rayUZ);
                const Fixed rayV = AddFixed(AddFixed(FixedMultiply(dx, mapping.rayVX),
                    FixedMultiply(dy, mapping.rayVY)), mapping.rayVZ);
                const uint32_t perspective = ReciprocalTable(
                    (static_cast<uint32_t>(depth) >> 7) & 0xffff) << 3;
                Fixed u = AddFixed(FixedMultiply(static_cast<Fixed>(perspective), rayU), mapping.baseU);
                Fixed v = AddFixed(FixedMultiply(static_cast<Fixed>(perspective), rayV), mapping.baseV);
                u = std::clamp(u, 0, mapping.maxU);
                v = std::clamp(v, 0, mapping.maxV);
                const uint32_t mask = ~0U << face.lightmapWidthShift;
                const int32_t index = static_cast<int32_t>(
                    (mask & static_cast<uint32_t>(v >> (16 - face.lightmapWidthShift))) +
                    static_cast<uint32_t>(u >> 16));
                if (index < 0 || static_cast<std::size_t>(index) >= face.lightmapPixels.size()) {
                    continue;
                }
                pixel = static_cast<uint32_t>(face.lightmapPixels[index]) |
                        (static_cast<uint32_t>(depth16) << 16);
            }
        }
    }
}

void Bsp::Render()
{
    for (const uint32_t index : visibleFaces) {
        const BspFace& face = faces[index];
        const LightmapMapping mapping = face.texInfoIndex < 0 ? LightmapMapping{}
                                                              : BuildLightmapMapping(face);
        const std::vector<ProjectedVertex> polygon = ProjectFace(face);
        RasterizeFace(face, mapping, polygon);
    }
}

int32_t Bsp::GetCurrentCluster() const { return currentCluster; }
uint32_t Bsp::GetFrameCounter() const { return frameCounter; }
const std::vector<uint8_t>& Bsp::GetDecompressedPVS() const { return decompressedPVS; }
const std::vector<uint32_t>& Bsp::GetVisibleFaces() const { return visibleFaces; }

} // namespace soft3d

#include "soft3d/Bsp.hpp"

#include "soft3d/Engine.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/TextureRes.hpp"
#include "soft3d/s3dFilePak.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <new>
#include <utility>

namespace soft3d {
namespace {

uint16_t Read16(const uint8_t* input)
{
    return static_cast<uint16_t>(input[0]) |
           static_cast<uint16_t>(input[1] << 8);
}

int16_t ReadSigned16(const uint8_t* input)
{
    return static_cast<int16_t>(Read16(input));
}

uint32_t Read32(const uint8_t* input)
{
    return static_cast<uint32_t>(input[0]) |
           (static_cast<uint32_t>(input[1]) << 8) |
           (static_cast<uint32_t>(input[2]) << 16) |
           (static_cast<uint32_t>(input[3]) << 24);
}

int32_t ReadSigned32(const uint8_t* input)
{
    return static_cast<int32_t>(Read32(input));
}

float ReadFloat(const uint8_t* input)
{
    const uint32_t bits = Read32(input);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

Fixed FloatToFixed(float value)
{
    return static_cast<Fixed>(value * 65536.0f);
}

void Append32(std::vector<uint8_t>& output, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        output.push_back(static_cast<uint8_t>(value >> shift));
    }
}

bool Divisible(uint32_t value, uint32_t divisor)
{
    return value % divisor == 0;
}

bool RangeValid(uint64_t first, uint64_t count, std::size_t size)
{
    return first <= size && count <= static_cast<uint64_t>(size) - first;
}

Fixed AddFixed(Fixed left, Fixed right)
{
    return static_cast<Fixed>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

Fixed Dot(const Vec3& left, const std::array<Fixed, 3>& right)
{
    const int64_t product = static_cast<int64_t>(left.x) * right[0] +
                            static_cast<int64_t>(left.y) * right[1] +
                            static_cast<int64_t>(left.z) * right[2];
    const uint64_t bits = static_cast<uint64_t>(product);
    return static_cast<Fixed>(static_cast<uint32_t>(bits >> 16));
}

Fixed Divide(Fixed numerator, Fixed denominator)
{
    return static_cast<Fixed>((static_cast<int64_t>(numerator) * 0x10000) / denominator);
}

int32_t FloorFixed(Fixed value)
{
    return value >> 16;
}

int32_t CeilFixed(Fixed value)
{
    return static_cast<int32_t>(static_cast<uint32_t>(value) + 0xffffU) >> 16;
}

uint32_t NextPowerOfTwo(uint32_t value)
{
    if ((value & (~value + 1)) == value) {
        return value;
    }
    uint32_t result = 2;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

int32_t PowerOfTwoShift(uint32_t value)
{
    if (value == 1) {
        return 0;
    }
    int32_t shift = 1;
    uint32_t current = 2;
    while (current < value) {
        current <<= 1;
        ++shift;
    }
    return current == value ? shift : -1;
}

std::array<int32_t, 4> ReadPackedName(const uint8_t* input)
{
    return {ReadSigned32(input), ReadSigned32(input + 4), ReadSigned32(input + 8),
            ReadSigned32(input + 12)};
}

} // namespace

void BspEntity::SetObject(std::unique_ptr<Object> value)
{
    ownedObject = std::move(value);
    object = ownedObject.get();
}

void BspEntity::SetObject(Object* value)
{
    ownedObject.reset();
    object = value;
}

Object* BspEntity::GetObject() const
{
    return object;
}

void BspEntity::Render()
{
    if (object != nullptr) {
        object->Render();
    }
}

void BspEntity::Submit(RenderQueues& queues)
{
    if (object != nullptr) {
        object->AdvanceAnimation();
        queues.Submit(object, 3);
    }
}

Bsp::Bsp(Engine* value)
    : Bsp(&value->GetRenderer(), &value->GetResourceManager())
{
    SetEngine(value);
}

Bsp::Bsp(Renderer* mapRenderer, s3dResManager* manager)
    : renderer(mapRenderer)
    , resourceManager(manager)
{
    ResetLightStyles();
}

Bsp::~Bsp()
{
    FreeAll();
}

void Bsp::SetEngine(Engine* value)
{
    s3dBase::SetEngine(value);
}

void Bsp::SetEntityFactory(BspEntityFactory factory, void* context)
{
    entityFactory = factory;
    entityFactoryContext = context;
}

void Bsp::SetTextureDirectory(const char16_t* value)
{
    if (value == nullptr) {
        textureDirectory.clear();
        useTextureDirectory = false;
        return;
    }
    textureDirectory = value;
    useTextureDirectory = true;
}

void Bsp::FreeLastLevel()
{
    entities.clear();
    models.clear();
    vertices.clear();
    edges.clear();
    faceEdges.clear();
    planes.clear();
    texInfo.clear();
    faces.clear();
    brushes.clear();
    brushSides.clear();
    visBrushes.clear();
    visFaces.clear();
    vertexLights.clear();
    entityPairs.clear();
    entityBrushes.clear();
    entityLinks.clear();
    visibleEntityIndices.clear();
    entityDictionaryIndices.clear();
    leafBrushes.clear();
    leafVisibleBrushes.clear();
    pathPairs.clear();
    pathNodes.clear();
    leaves.clear();
    nodes.clear();
    pvs.clear();
    lightmapData.clear();
    source.clear();
    decompressedPVS.clear();
    visibleFaces.clear();
    visibleFaceFlags.clear();
    currentCluster = -1;
    previousCluster = -1;
    loaded = false;
    for (BspLightStyle& style : lightStyles) {
        style.light = 8;
        style.position = 0;
        style.enabled = 1;
    }
}

void Bsp::FreeAll()
{
    FreeLastLevel();
    ResetLightStyles();
}

void Bsp::ResetLightStyles()
{
    for (BspLightStyle& style : lightStyles) {
        style.light = 8;
        style.position = 0;
        style.frameCount = 0;
        style.enabled = 0;
        style.frames.clear();
    }
}

result Bsp::ValidateDirectory() const
{
    if (source.size() < sbpHeaderBytes) {
        return S3D_FORMAT;
    }
    for (const SbpChunkDescriptor& chunk : header.chunks) {
        if (chunk.length == 0) {
            continue;
        }
        if (chunk.offset < sbpHeaderBytes || chunk.offset > source.size() ||
            chunk.length > source.size() - chunk.offset) {
            return S3D_FORMAT;
        }
    }
    return S3D_OK;
}

result Bsp::LoadSource(const char16_t* mapDirectory, const char16_t* fileName)
{
    directory = mapDirectory;
    if (resourceManager == nullptr) {
        return S3D_FILE;
    }
    s3dFilePak* filePak = GetFilePakFromManager(resourceManager);
    if (filePak == nullptr) {
        return S3D_FILE;
    }
    const std::u16string path = directory + fileName;
    result status = filePak->Select(path.c_str());
    if (status != S3D_OK) {
        return status;
    }
    filePak->Seek(0, SEEK_END);
    const int byteCount = filePak->Tell();
    if (byteCount == S3D_EOF || byteCount < 0) {
        return S3D_UNKNOWN;
    }
    source.resize(static_cast<std::size_t>(byteCount));
    filePak->Seek(0, SEEK_SET);
    status = filePak->Read(source.data(), byteCount);
    if (status != S3D_OK) {
        return status;
    }
    if (source.size() < sbpHeaderBytes) {
        return S3D_FORMAT;
    }
    std::copy_n(source.data(), header.magic.size(), header.magic.begin());
    header.version = ReadFloat(source.data() + 4);
    if (header.version != sbpVersion) {
        return S3D_FORMAT;
    }
    for (std::size_t index = 0; index < header.values.size(); ++index) {
        const std::size_t offset = 8 + index * 4;
        header.values[index] = ReadFloat(source.data() + offset);
        headerValues[index] = FloatToFixed(header.values[index]);
    }
    for (std::size_t index = 0; index < header.chunks.size(); ++index) {
        const uint8_t* input = source.data() + 0x20 + index * 8;
        header.chunks[index] = {Read32(input), Read32(input + 4)};
    }
    return ValidateDirectory();
}

const SbpChunkDescriptor& Bsp::Chunk(std::size_t index) const
{
    return header.chunks[index];
}

const uint8_t* Bsp::ChunkData(std::size_t index) const
{
    return source.data() + Chunk(index).offset;
}

result Bsp::LoadVertices(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 12)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    vertices.resize(chunk.length / 12);
    for (Vec3& vertex : vertices) {
        vertex = {FloatToFixed(ReadFloat(input)), FloatToFixed(ReadFloat(input + 4)),
                  FloatToFixed(ReadFloat(input + 8))};
        input += 12;
    }
    return S3D_OK;
}

result Bsp::LoadEdges(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    edges.resize(chunk.length / 4);
    for (BspEdge& edge : edges) {
        edge.vertices = {Read16(input), Read16(input + 2)};
        if (!RangeValid(edge.vertices[0], 1, vertices.size()) ||
            !RangeValid(edge.vertices[1], 1, vertices.size())) {
            return S3D_FORMAT;
        }
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadFaceEdges(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    faceEdges.resize(chunk.length / 4);
    for (int32_t& faceEdge : faceEdges) {
        faceEdge = ReadSigned32(input);
        const int64_t edgeIndex = faceEdge < 0 ? -static_cast<int64_t>(faceEdge) : faceEdge;
        if (!RangeValid(edgeIndex, 1, edges.size())) {
            return S3D_FORMAT;
        }
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadPlanes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 20)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    planes.resize(chunk.length / 20);
    for (BspPlane& plane : planes) {
        plane.normal = {FloatToFixed(ReadFloat(input)), FloatToFixed(ReadFloat(input + 4)),
                        FloatToFixed(ReadFloat(input + 8))};
        plane.distance = FloatToFixed(ReadFloat(input + 12));
        plane.type = Read32(input + 16);
        plane.negativeComponents = (plane.normal.x < 0 ? 1U : 0U) |
                                   (plane.normal.y < 0 ? 2U : 0U) |
                                   (plane.normal.z < 0 ? 4U : 0U);
        input += 20;
    }
    return S3D_OK;
}

result Bsp::LoadTexInfo(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 48)) {
        return S3D_FORMAT;
    }
    if (resourceManager == nullptr) {
        return S3D_FILE;
    }
    const uint8_t* input = source.data() + chunk.offset;
    texInfo.resize(chunk.length / 48);
    for (BspTexInfo& info : texInfo) {
        for (std::size_t index = 0; index < info.textureMapping.size(); ++index) {
            info.textureMapping[index] = FloatToFixed(ReadFloat(input + index * 4));
        }
        info.textureName = DecodeSojTextureName(ReadPackedName(input + 32));
        TextureRes* resource = nullptr;
        if (useTextureDirectory) {
            resource = dynamic_cast<TextureRes*>(resourceManager->GetResource(
                ResourceTexture, 3, info.textureName.c_str(), textureDirectory.c_str()));
        }
        if (resource == nullptr) {
            resource = dynamic_cast<TextureRes*>(resourceManager->GetResource(
                ResourceTexture, 3, info.textureName.c_str()));
        }
        if (resource == nullptr) {
            return S3D_FILE;
        }
        info.texture = resource->GetTexture();
        input += 48;
    }
    return S3D_OK;
}

result Bsp::LoadLightmapData(const SbpChunkDescriptor& chunk)
{
    const uint8_t* input = source.data() + chunk.offset;
    lightmapData.assign(input, input + chunk.length);
    return S3D_OK;
}

result Bsp::BuildFaceLightmapInfo(BspFace& face)
{
    if (face.texInfoIndex < 0) {
        return S3D_OK;
    }
    const BspTexInfo& info = texInfo[static_cast<std::size_t>(face.texInfoIndex)];
    const std::array<Fixed, 3> uAxis = {info.textureMapping[0], info.textureMapping[1],
                                        info.textureMapping[2]};
    const std::array<Fixed, 3> vAxis = {info.textureMapping[4], info.textureMapping[5],
                                        info.textureMapping[6]};
    std::array<Fixed, 2> maximum = {static_cast<Fixed>(0xc0000000U),
                                    static_cast<Fixed>(0xc0000000U)};
    std::array<Fixed, 2> minimum = {0x40000000, 0x40000000};
    for (uint32_t index = 0; index < face.edgeCount; ++index) {
        const int32_t faceEdge = faceEdges[face.firstEdge + index];
        const std::size_t edgeIndex = static_cast<std::size_t>(
            faceEdge < 0 ? -static_cast<int64_t>(faceEdge) : faceEdge);
        const BspEdge& edge = edges[edgeIndex];
        const uint16_t vertexIndex = faceEdge < 0 ? edge.vertices[1] : edge.vertices[0];
        const Vec3& vertex = vertices[vertexIndex];
        const std::array<Fixed, 2> coordinates = {
            AddFixed(Dot(vertex, uAxis), info.textureMapping[3]),
            AddFixed(Dot(vertex, vAxis), info.textureMapping[7])
        };
        for (std::size_t axis = 0; axis < 2; ++axis) {
            minimum[axis] = std::min(minimum[axis], coordinates[axis]);
            maximum[axis] = std::max(maximum[axis], coordinates[axis]);
        }
    }
    for (std::size_t axis = 0; axis < 2; ++axis) {
        const int32_t maximumTexel = CeilFixed(Divide(maximum[axis], 0x100000));
        const int32_t minimumTexel = FloorFixed(Divide(minimum[axis], 0x100000));
        face.lightmapMin[axis] = static_cast<int16_t>(minimumTexel * 16);
        int32_t size = (static_cast<int16_t>(maximumTexel) -
                        static_cast<int16_t>(minimumTexel)) * 16;
        if (size < 16) {
            size = 16;
        }
        face.lightmapSize[axis] = static_cast<int16_t>(size);
    }
    const uint32_t width = static_cast<uint32_t>(face.lightmapSize[0]);
    face.lightmapPowerOfTwoWidth = static_cast<int16_t>(NextPowerOfTwo(width));
    face.lightmapWidthShift = static_cast<int16_t>(
        PowerOfTwoShift(static_cast<uint16_t>(face.lightmapPowerOfTwoWidth)));
    face.lightmapSampleCount = ((face.lightmapSize[0] >> 4) + 1) *
                               ((face.lightmapSize[1] >> 4) + 1);
    return S3D_OK;
}

result Bsp::LoadFaces(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 24)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    faces.resize(chunk.length / 24);
    for (BspFace& face : faces) {
        face.planeIndex = Read16(input);
        face.firstEdge = Read16(input + 2);
        face.edgeCount = Read16(input + 6);
        face.planeSide = ReadSigned16(input + 10);
        face.texInfoIndex = ReadSigned16(input + 8);
        face.textureMin = {static_cast<int32_t>(ReadSigned16(input + 12)) * 16,
                           static_cast<int32_t>(ReadSigned16(input + 14)) * 16};
        const int32_t lightmapOffset = ReadSigned32(input + 16);
        std::copy_n(input + 20, face.styles.size(), face.styles.begin());
        if (!RangeValid(face.planeIndex, 1, planes.size()) ||
            !RangeValid(face.firstEdge, face.edgeCount, faceEdges.size()) ||
            face.texInfoIndex < -1 ||
            (face.texInfoIndex >= 0 &&
             !RangeValid(face.texInfoIndex, 1, texInfo.size()))) {
            return S3D_FORMAT;
        }
        if (lightmapOffset == -1 || lightmapData.empty()) {
            face.lightmapOffset = -1;
        } else {
            if (lightmapOffset < 0 ||
                !RangeValid(static_cast<uint32_t>(lightmapOffset), 1, lightmapData.size())) {
                return S3D_FORMAT;
            }
            face.lightmapOffset = lightmapOffset;
        }
        const result status = BuildFaceLightmapInfo(face);
        if (status != S3D_OK) {
            return status;
        }
        input += 24;
    }
    return S3D_OK;
}

result Bsp::LoadBrushes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    brushes.resize(chunk.length / 4);
    for (BspBrush& brush : brushes) {
        brush.firstSide = Read16(input);
        brush.sideCount = Read16(input + 2);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadBrushSides(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    brushSides.resize(chunk.length / 4);
    for (uint32_t& side : brushSides) {
        side = Read32(input);
        input += 4;
    }
    for (const BspBrush& brush : brushes) {
        if (!RangeValid(brush.firstSide, brush.sideCount, brushSides.size())) {
            return S3D_FORMAT;
        }
    }
    return S3D_OK;
}

result Bsp::LoadVisBrushes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    visBrushes.resize(chunk.length / 4);
    for (BspVisBrush& brush : visBrushes) {
        brush.firstFace = Read16(input);
        brush.faceCount = Read16(input + 2);
        if (!RangeValid(brush.firstFace, brush.faceCount, faces.size())) {
            return S3D_FORMAT;
        }
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadVisFaces(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    visFaces.resize(chunk.length / 4);
    for (uint32_t& face : visFaces) {
        const int32_t index = ReadSigned32(input);
        if (index < 0 || !RangeValid(index, 1, faces.size())) {
            return S3D_FORMAT;
        }
        face = static_cast<uint32_t>(index);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadVertexLights(const SbpChunkDescriptor& chunk)
{
    const uint8_t* input = source.data() + chunk.offset;
    vertexLights.resize(chunk.length);
    for (std::size_t index = 0; index < vertexLights.size(); ++index) {
        vertexLights[index] = static_cast<Fixed>((256U - input[index]) * 0x1f0000U);
    }
    return S3D_OK;
}

result Bsp::LoadModels(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 88)) {
        return S3D_FORMAT;
    }
    if (resourceManager == nullptr || renderer == nullptr) {
        return S3D_FILE;
    }
    const uint8_t* input = source.data() + chunk.offset;
    models.resize(chunk.length / 88);
    for (BspModel& model : models) {
        const std::u16string modelName = DecodeSojTextureName(ReadPackedName(input));
        auto* resource = dynamic_cast<ObjectRes*>(
            resourceManager->GetResource(ResourceObject, 3, modelName.c_str()));
        if (resource == nullptr) {
            return S3D_FILE;
        }
        model.object = std::make_unique<Object>(renderer);
        model.object->SetModel(resource);
        result status = model.object->LoadMaterials(resourceManager, 3);
        if (status != S3D_OK) {
            return status;
        }
        for (std::size_t vector = 0; vector < model.transform.size(); ++vector) {
            const std::size_t offset = 0x20 + vector * 12;
            model.transform[vector] = {
                FloatToFixed(ReadFloat(input + offset)),
                FloatToFixed(ReadFloat(input + offset + 4)),
                FloatToFixed(ReadFloat(input + offset + 8))
            };
        }
        model.object->SetTransformVectors(model.transform);
        model.firstVertexLight = Read32(input + 0x50);
        model.vertexLightCount = Read16(input + 0x54);
        model.mode = input[0x56];
        model.flags = input[0x57];
        if (model.mode == 1 && model.vertexLightCount != 0 && !vertexLights.empty()) {
            if (!RangeValid(model.firstVertexLight, model.vertexLightCount,
                            vertexLights.size())) {
                return S3D_FORMAT;
            }
            model.vertexLights.format = 8;
            model.vertexLights.strideBytes = 4;
            model.vertexLights.vertexCount = model.vertexLightCount;
            model.vertexLights.dataByteSize = model.vertexLightCount * 4;
            model.vertexLights.data.reserve(model.vertexLights.dataByteSize);
            for (uint16_t index = 0; index < model.vertexLightCount; ++index) {
                Append32(model.vertexLights.data,
                         static_cast<uint32_t>(vertexLights[model.firstVertexLight + index]));
            }
        }
        if (Read32(input + 0x10) != 0x1f) {
            const std::u16string animationName =
                DecodeSojTextureName(ReadPackedName(input + 0x10));
            model.animation = dynamic_cast<const Animation*>(resourceManager->GetResource(
                ResourceAnimation, 3, animationName.c_str()));
            if (model.animation != nullptr) {
                model.object->SetAnimation(model.animation);
            }
        }
        input += 88;
    }
    return S3D_OK;
}

result Bsp::LoadEntityPairs(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 32)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    entityPairs.resize(chunk.length / 32);
    for (BspEntityPair& pair : entityPairs) {
        std::copy_n(input, pair.key.size(), pair.key.begin());
        std::copy_n(input + 16, pair.value.size(), pair.value.begin());
        input += 32;
    }
    return S3D_OK;
}

result Bsp::LoadEntityBrushes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 12)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    entityBrushes.resize(chunk.length / 12);
    for (BspEntityBrush& brush : entityBrushes) {
        for (std::size_t index = 0; index < brush.values.size(); ++index) {
            brush.values[index] = ReadSigned16(input + index * 2);
        }
        input += 12;
    }
    return S3D_OK;
}

result Bsp::LoadEntityLinks(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    entityLinks.resize(chunk.length / 4);
    for (uint32_t& link : entityLinks) {
        link = Read32(input);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadEntities(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 16)) {
        return S3D_FORMAT;
    }
    const std::size_t count = chunk.length / 16;
    if (count != 0 && entityFactory == nullptr) {
        return S3D_STATE;
    }
    const uint8_t* input = source.data() + chunk.offset;
    std::vector<BspEntityRecord> records(count);
    for (BspEntityRecord& record : records) {
        record.type = ReadSigned16(input);
        record.modelIndex = ReadSigned16(input + 2);
        if (!entityLinks.empty()) {
            record.firstLink = Read16(input + 4);
            record.linkCount = ReadSigned16(input + 6);
            if (record.linkCount < 0 ||
                !RangeValid(record.firstLink, static_cast<uint16_t>(record.linkCount),
                            entityLinks.size())) {
                return S3D_FORMAT;
            }
        }
        if (!entityPairs.empty()) {
            record.firstPair = Read16(input + 8);
            record.pairCount = ReadSigned16(input + 10);
            if (record.pairCount < 0 ||
                !RangeValid(record.firstPair, static_cast<uint16_t>(record.pairCount),
                            entityPairs.size())) {
                return S3D_FORMAT;
            }
        }
        if (!entityBrushes.empty()) {
            record.brushIndex = ReadSigned32(input + 12);
            if (record.brushIndex != -1 &&
                (record.brushIndex < 0 ||
                 !RangeValid(record.brushIndex, 1, entityBrushes.size()))) {
                return S3D_FORMAT;
            }
        }
        if (record.modelIndex != -1 &&
            (record.modelIndex < 0 || !RangeValid(record.modelIndex, 1, models.size()))) {
            return S3D_FORMAT;
        }
        input += 16;
    }
    entities.clear();
    entities.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        entities[index].reset(entityFactory(&records[index], entityFactoryContext,
                                            records.data()));
        if (entities[index] == nullptr) {
            return S3D_MEMORY;
        }
    }
    return S3D_OK;
}

result Bsp::LoadVisibleEntityIndices(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    visibleEntityIndices.resize(chunk.length / 4);
    for (uint32_t& entity : visibleEntityIndices) {
        const int32_t index = ReadSigned32(input);
        if (index < 0 || !RangeValid(index, 1, entities.size())) {
            return S3D_FORMAT;
        }
        entity = static_cast<uint32_t>(index);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadEntityDictionaryIndices(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    entityDictionaryIndices.resize(chunk.length / 4);
    for (uint32_t& entity : entityDictionaryIndices) {
        const int32_t index = ReadSigned32(input);
        if (index < 0 || !RangeValid(index, 1, entities.size())) {
            return S3D_FORMAT;
        }
        entity = static_cast<uint32_t>(index);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadLeafBrushes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    leafBrushes.resize(chunk.length / 4);
    for (uint32_t& brush : leafBrushes) {
        const int32_t index = ReadSigned32(input);
        if (index < 0 || !RangeValid(index, 1, brushes.size())) {
            return S3D_FORMAT;
        }
        brush = static_cast<uint32_t>(index);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadLeafVisibleBrushes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    leafVisibleBrushes.resize(chunk.length / 4);
    for (uint32_t& brush : leafVisibleBrushes) {
        const int32_t index = ReadSigned32(input);
        if (index < 0 || !RangeValid(index, 1, visBrushes.size())) {
            return S3D_FORMAT;
        }
        brush = static_cast<uint32_t>(index);
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadPathPairs(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 4)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    pathPairs.resize(chunk.length / 4);
    for (BspPathPair& pair : pathPairs) {
        pair.nodes = {Read16(input), Read16(input + 2)};
        input += 4;
    }
    return S3D_OK;
}

result Bsp::LoadPathNodes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 16)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    pathNodes.resize(chunk.length / 16);
    for (std::size_t index = 0; index < pathNodes.size(); ++index) {
        BspPathNode& node = pathNodes[index];
        node.position = {FloatToFixed(ReadFloat(input)), FloatToFixed(ReadFloat(input + 4)),
                         FloatToFixed(ReadFloat(input + 8))};
        node.pairCount = ReadSigned16(input + 14);
        if (node.pairCount < 0) {
            return S3D_FORMAT;
        }
        if (node.pairCount != 0) {
            node.firstPair = Read16(input + 12);
            if (!RangeValid(node.firstPair, static_cast<uint16_t>(node.pairCount),
                            pathPairs.size())) {
                return S3D_FORMAT;
            }
        }
        node.index = static_cast<uint16_t>(index);
        input += 16;
    }
    for (const BspPathPair& pair : pathPairs) {
        if (!RangeValid(pair.nodes[0], 1, pathNodes.size()) ||
            !RangeValid(pair.nodes[1], 1, pathNodes.size())) {
            return S3D_FORMAT;
        }
    }
    return S3D_OK;
}

result Bsp::LoadLeaves(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 32)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    leaves.resize(chunk.length / 32);
    for (BspLeaf& leaf : leaves) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            leaf.mins[axis] = ReadSigned16(input + axis * 2);
            leaf.maxs[axis] = ReadSigned16(input + 6 + axis * 2);
        }
        leaf.contents = Read16(input + 12);
        leaf.cluster = ReadSigned16(input + 14);
        if (!visFaces.empty()) {
            leaf.firstVisibleFace = Read16(input + 16);
            leaf.visibleFaceCount = Read16(input + 18);
            if (!RangeValid(leaf.firstVisibleFace, leaf.visibleFaceCount, visFaces.size())) {
                return S3D_FORMAT;
            }
        }
        if (!visibleEntityIndices.empty()) {
            leaf.firstVisibleEntity = Read16(input + 20);
            leaf.visibleEntityCount = Read16(input + 22);
            if (!RangeValid(leaf.firstVisibleEntity, leaf.visibleEntityCount,
                            visibleEntityIndices.size())) {
                return S3D_FORMAT;
            }
        }
        if (!leafBrushes.empty()) {
            leaf.firstBrush = Read16(input + 24);
            leaf.brushCount = Read16(input + 26);
            if (!RangeValid(leaf.firstBrush, leaf.brushCount, leafBrushes.size())) {
                return S3D_FORMAT;
            }
        }
        if (!leafVisibleBrushes.empty()) {
            leaf.firstVisibleBrush = Read16(input + 28);
            leaf.visibleBrushCount = Read16(input + 30);
            if (!RangeValid(leaf.firstVisibleBrush, leaf.visibleBrushCount,
                            leafVisibleBrushes.size())) {
                return S3D_FORMAT;
            }
            for (uint16_t index = 0; index < leaf.visibleBrushCount; ++index) {
                const uint32_t brushIndex = leafVisibleBrushes[leaf.firstVisibleBrush + index];
                visBrushes[brushIndex].leafIndex = static_cast<uint32_t>(&leaf - leaves.data());
            }
        }
        input += 32;
    }
    return S3D_OK;
}

result Bsp::LoadNodes(const SbpChunkDescriptor& chunk)
{
    if (!Divisible(chunk.length, 24)) {
        return S3D_FORMAT;
    }
    const uint8_t* input = source.data() + chunk.offset;
    nodes.resize(chunk.length / 24);
    for (BspNode& node : nodes) {
        node.planeIndex = Read32(input);
        for (std::size_t axis = 0; axis < 3; ++axis) {
            node.mins[axis] = ReadSigned16(input + 8 + axis * 2);
            node.maxs[axis] = ReadSigned16(input + 14 + axis * 2);
        }
        for (std::size_t child = 0; child < 2; ++child) {
            const int32_t index = ReadSigned16(input + 4 + child * 2);
            node.children[child] = index;
            if (index < 0) {
                const int64_t leaf = -static_cast<int64_t>(index) - 1;
                if (!RangeValid(leaf, 1, leaves.size())) {
                    return S3D_FORMAT;
                }
            } else if (!RangeValid(index, 1, nodes.size())) {
                return S3D_FORMAT;
            }
        }
        node.firstFace = Read16(input + 20);
        node.faceCount = Read16(input + 22);
        if (!RangeValid(node.planeIndex, 1, planes.size()) ||
            !RangeValid(node.firstFace, node.faceCount, faces.size())) {
            return S3D_FORMAT;
        }
        input += 24;
    }
    if (nodes.empty()) {
        return S3D_OK;
    }
    std::vector<uint8_t> state(nodes.size(), 0);
    std::function<result(uint32_t, int32_t)> link = [&](uint32_t index, int32_t parent) -> result {
        if (state[index] == 1) {
            return S3D_FORMAT;
        }
        if (state[index] == 2) {
            return S3D_OK;
        }
        state[index] = 1;
        nodes[index].parentIndex = parent;
        for (const int32_t child : nodes[index].children) {
            if (child >= 0) {
                const result status = link(static_cast<uint32_t>(child),
                                           static_cast<int32_t>(index));
                if (status != S3D_OK) {
                    return status;
                }
            }
        }
        state[index] = 2;
        return S3D_OK;
    };
    return link(0, -1);
}

result Bsp::LoadPVS(const SbpChunkDescriptor& chunk)
{
    const uint8_t* input = source.data() + chunk.offset;
    pvs.assign(input, input + chunk.length);
    return S3D_OK;
}

result Bsp::LoadNext(const char16_t* mapDirectory, const char16_t* fileName, int& progress)
{
    result status = S3D_OK;
    switch (progress) {
    case 0:
        status = LoadSource(mapDirectory, fileName);
        if (status == S3D_OK) {
            status = LoadVertices(Chunk(SbpVertices));
        }
        if (status == S3D_OK) {
            progress = 4;
        }
        break;
    case 4: status = LoadEdges(Chunk(SbpEdges)); if (status == S3D_OK) progress = 8; break;
    case 8: status = LoadFaceEdges(Chunk(SbpFaceEdges)); if (status == S3D_OK) progress = 12; break;
    case 12: status = LoadPlanes(Chunk(SbpPlanes)); if (status == S3D_OK) progress = 16; break;
    case 16: status = LoadTexInfo(Chunk(SbpTexInfo)); if (status == S3D_OK) progress = 20; break;
    case 20: status = LoadLightmapData(Chunk(SbpLightmapData)); if (status == S3D_OK) progress = 24; break;
    case 24: status = LoadFaces(Chunk(SbpFaces)); if (status == S3D_OK) progress = 28; break;
    case 28: status = LoadBrushes(Chunk(SbpBrushes)); if (status == S3D_OK) progress = 32; break;
    case 32: status = LoadBrushSides(Chunk(SbpBrushSides)); if (status == S3D_OK) progress = 36; break;
    case 36: status = LoadVisBrushes(Chunk(SbpVisBrushes)); if (status == S3D_OK) progress = 40; break;
    case 40: status = LoadVisFaces(Chunk(SbpVisFaces)); if (status == S3D_OK) progress = 44; break;
    case 44: status = LoadVertexLights(Chunk(SbpVertexLights)); if (status == S3D_OK) progress = 48; break;
    case 48: status = LoadModels(Chunk(SbpModels)); if (status == S3D_OK) progress = 52; break;
    case 52: status = LoadEntityPairs(Chunk(SbpEntityPairs)); if (status == S3D_OK) progress = 56; break;
    case 56: status = LoadEntityBrushes(Chunk(SbpEntityBrushes)); if (status == S3D_OK) progress = 60; break;
    case 60: status = LoadEntityLinks(Chunk(SbpEntityLinks)); if (status == S3D_OK) progress = 64; break;
    case 64: status = LoadEntities(Chunk(SbpEntities)); if (status == S3D_OK) progress = 68; break;
    case 68: status = LoadVisibleEntityIndices(Chunk(SbpVisibleEntityIndices)); if (status == S3D_OK) progress = 72; break;
    case 72: status = LoadEntityDictionaryIndices(Chunk(SbpEntityDictionaryIndices)); if (status == S3D_OK) progress = 76; break;
    case 76: status = LoadLeafBrushes(Chunk(SbpLeafBrushes)); if (status == S3D_OK) progress = 80; break;
    case 80: status = LoadLeafVisibleBrushes(Chunk(SbpLeafVisibleBrushes)); if (status == S3D_OK) progress = 84; break;
    case 84: status = LoadPathPairs(Chunk(SbpPathPairs)); if (status == S3D_OK) progress = 88; break;
    case 88: status = LoadPathNodes(Chunk(SbpPathNodes)); if (status == S3D_OK) progress = 92; break;
    case 92: status = LoadLeaves(Chunk(SbpLeaves)); if (status == S3D_OK) progress = 96; break;
    case 96: status = LoadNodes(Chunk(SbpNodes)); if (status == S3D_OK) progress = 98; break;
    case 98:
        status = LoadPVS(Chunk(SbpPVS));
        if (status == S3D_OK) {
            source.clear();
            vertexLights.clear();
            entityLinks.clear();
            for (std::size_t index = 1; index < lightStyles.size() - 1; ++index) {
                lightStyles[index].enabled = 1;
            }
            loaded = true;
            progress = 100;
        }
        break;
    default:
        progress = 0;
        return LoadNext(mapDirectory, fileName, progress);
    }
    return status;
}

result LoadSbpMapChunks(Bsp* bsp, const char16_t* directory, const char16_t* fileName,
                        int* progress)
{
    if (bsp == nullptr || directory == nullptr || fileName == nullptr) {
        return S3D_PARAM;
    }
    try {
        if (progress != nullptr) {
            const result status = bsp->LoadNext(directory, fileName, *progress);
            if (status != S3D_OK) {
                bsp->FreeLastLevel();
            }
            return status;
        }
        int step = 0;
        while (step != 100) {
            const result status = bsp->LoadNext(directory, fileName, step);
            if (status != S3D_OK) {
                bsp->FreeLastLevel();
                return status;
            }
        }
        return S3D_OK;
    } catch (const std::bad_alloc&) {
        bsp->FreeLastLevel();
        return S3D_MEMORY;
    }
}

bool Bsp::IsLoaded() const { return loaded; }
const SbpHeader& Bsp::GetHeader() const { return header; }
const std::array<Fixed, 6>& Bsp::GetHeaderValues() const { return headerValues; }
const std::u16string& Bsp::GetDirectory() const { return directory; }
const std::vector<Vec3>& Bsp::GetVertices() const { return vertices; }
const std::vector<BspEdge>& Bsp::GetEdges() const { return edges; }
const std::vector<int32_t>& Bsp::GetFaceEdges() const { return faceEdges; }
const std::vector<BspPlane>& Bsp::GetPlanes() const { return planes; }
const std::vector<BspTexInfo>& Bsp::GetTexInfo() const { return texInfo; }
const std::vector<BspFace>& Bsp::GetFaces() const { return faces; }
const std::vector<BspBrush>& Bsp::GetBrushes() const { return brushes; }
const std::vector<uint32_t>& Bsp::GetBrushSides() const { return brushSides; }
const std::vector<BspVisBrush>& Bsp::GetVisBrushes() const { return visBrushes; }
const std::vector<uint32_t>& Bsp::GetVisFaces() const { return visFaces; }
const std::vector<Fixed>& Bsp::GetVertexLights() const { return vertexLights; }
const std::vector<BspModel>& Bsp::GetModels() const { return models; }
const std::vector<BspEntityPair>& Bsp::GetEntityPairs() const { return entityPairs; }
const std::vector<BspEntityBrush>& Bsp::GetEntityBrushes() const { return entityBrushes; }
const std::vector<uint32_t>& Bsp::GetEntityLinks() const { return entityLinks; }
const std::vector<std::unique_ptr<BspEntity>>& Bsp::GetEntities() const { return entities; }
const std::vector<uint32_t>& Bsp::GetVisibleEntityIndices() const { return visibleEntityIndices; }
const std::vector<uint32_t>& Bsp::GetEntityDictionaryIndices() const { return entityDictionaryIndices; }
const std::vector<uint32_t>& Bsp::GetLeafBrushes() const { return leafBrushes; }
const std::vector<uint32_t>& Bsp::GetLeafVisibleBrushes() const { return leafVisibleBrushes; }
const std::vector<BspPathPair>& Bsp::GetPathPairs() const { return pathPairs; }
const std::vector<BspPathNode>& Bsp::GetPathNodes() const { return pathNodes; }
const std::vector<BspLeaf>& Bsp::GetLeaves() const { return leaves; }
const std::vector<BspNode>& Bsp::GetNodes() const { return nodes; }
const std::vector<std::uint8_t>& Bsp::GetPVS() const { return pvs; }
const std::vector<std::uint8_t>& Bsp::GetLightmapData() const { return lightmapData; }
const std::array<BspLightStyle, 256>& Bsp::GetLightStyles() const { return lightStyles; }

} // namespace soft3d

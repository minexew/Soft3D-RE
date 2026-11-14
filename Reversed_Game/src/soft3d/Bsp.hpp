#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/Object.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace soft3d {

class Engine;
class Renderer;
class Texture;

struct SbpChunkDescriptor {
    uint32_t offset = 0;
    uint32_t length = 0;
};

inline constexpr std::size_t sbpChunkCount = 27;
inline constexpr std::size_t sbpHeaderBytes = 0xf8;
inline constexpr float sbpVersion = 7.6f;

struct SbpHeader {
    std::array<char, 4> magic{};
    float version = 0.0f;
    std::array<float, 6> values{};
    std::array<SbpChunkDescriptor, sbpChunkCount> chunks{};
};

enum SbpChunk : std::size_t {
    SbpVertices = 0,
    SbpEdges = 1,
    SbpFaceEdges = 2,
    SbpPlanes = 3,
    SbpTexInfo = 4,
    SbpFaces = 5,
    SbpBrushes = 6,
    SbpBrushSides = 7,
    SbpVisBrushes = 8,
    SbpVisFaces = 9,
    SbpVertexLights = 10,
    SbpModels = 11,
    SbpEntityPairs = 12,
    SbpEntityBrushes = 13,
    SbpEntities = 14,
    SbpEntityLinks = 15,
    SbpVisibleEntityIndices = 16,
    SbpUnused17 = 17,
    SbpEntityDictionaryIndices = 18,
    SbpLeafBrushes = 19,
    SbpLeafVisibleBrushes = 20,
    SbpPathPairs = 21,
    SbpPathNodes = 22,
    SbpLeaves = 23,
    SbpNodes = 24,
    SbpPVS = 25,
    SbpLightmapData = 26,
};

struct BspEdge {
    std::array<uint16_t, 2> vertices{};
};

struct BspPlane {
    Vec3 normal;
    Fixed distance = 0;
    uint32_t type = 0;
    uint32_t negativeComponents = 0;
};

struct BspTexInfo {
    std::array<Fixed, 8> textureMapping{};
    std::u16string textureName;
    const Texture* texture = nullptr;
};

struct BspFace {
    int32_t renderMarker = -1;
    uint32_t planeIndex = 0;
    int16_t planeSide = 0;
    uint32_t firstEdge = 0;
    uint32_t edgeCount = 0;
    int32_t texInfoIndex = -1;
    std::array<int16_t, 2> lightmapMin{};
    std::array<int16_t, 2> lightmapSize{};
    int16_t lightmapPowerOfTwoWidth = 0;
    int16_t lightmapWidthShift = 0;
    int32_t lightmapSampleCount = 0;
    std::array<int32_t, 2> textureMin{};
    std::array<uint8_t, 4> styles{};
    int32_t lightmapOffset = -1;
    std::vector<uint16_t> lightmapPixels;
};

struct BspBrush {
    uint32_t flags = 0;
    uint16_t firstSide = 0;
    uint16_t sideCount = 0;
};

struct BspVisBrush {
    uint32_t leafIndex = 0;
    uint32_t renderMarker = 0;
    uint16_t firstFace = 0;
    uint16_t faceCount = 0;
    int16_t planeMask = 0;
};

class BspEntity : public Renderable {
public:
    ~BspEntity() override = default;

    void Render() override;
    virtual void Submit(RenderQueues& queues);
    void SetObject(std::unique_ptr<Object> value);
    void SetObject(Object* value);
    Object* GetObject() const;

private:
    friend class Bsp;
    std::unique_ptr<Object> ownedObject;
    Object* object = nullptr;
    uint32_t lastVisibleFrame = 0;
};

struct BspModel {
    std::unique_ptr<Object> object;
    const Animation* animation = nullptr;
    std::array<Vec3, 4> transform{};
    SojVertices vertexLights;
    uint32_t firstVertexLight = 0;
    uint16_t vertexLightCount = 0;
    uint8_t mode = 0;
    uint8_t flags = 0;
};

struct BspEntityPair {
    std::array<char, 16> key{};
    std::array<char, 16> value{};
};

struct BspEntityBrush {
    std::array<int16_t, 6> values{};
};

struct BspEntityRecord {
    int32_t type = 0;
    int32_t modelIndex = -1;
    uint32_t firstLink = 0;
    int16_t linkCount = 0;
    uint32_t firstPair = 0;
    int16_t pairCount = 0;
    int32_t brushIndex = -1;
};

using BspEntityFactory = BspEntity* (*)(BspEntityRecord* record, void* context,
                                       BspEntityRecord* recordBase);

struct BspPathPair {
    std::array<uint16_t, 2> nodes{};
};

struct BspPathNode {
    Vec3 position;
    uint32_t firstPair = 0;
    int16_t pairCount = 0;
    uint16_t index = 0;
};

struct BspLeaf {
    int32_t cluster = 0;
    int32_t renderMarker = -1;
    std::array<int16_t, 3> mins{};
    std::array<int16_t, 3> maxs{};
    uint32_t firstVisibleFace = 0;
    uint16_t visibleFaceCount = 0;
    uint32_t firstVisibleEntity = 0;
    uint16_t visibleEntityCount = 0;
    uint32_t firstBrush = 0;
    uint16_t brushCount = 0;
    uint32_t firstVisibleBrush = 0;
    uint16_t visibleBrushCount = 0;
    uint16_t contents = 0;
};

struct BspNode {
    int32_t renderMarker = -1;
    int32_t parentIndex = -1;
    std::array<int16_t, 3> mins{};
    std::array<int16_t, 3> maxs{};
    uint32_t planeIndex = 0;
    std::array<int32_t, 2> children{};
    uint16_t firstFace = 0;
    uint16_t faceCount = 0;
};

struct BspLightStyle {
    uint8_t light = 8;
    uint8_t position = 0;
    uint8_t frameCount = 0;
    uint8_t enabled = 0;
    std::vector<uint8_t> frames;
};

struct BspRenderState {
    Vec3 cameraPosition;
    Matrix4 worldToCamera = Matrix4::Identity();
    Fixed screenScaleX = 0;
    Fixed screenScaleY = 0;
    Fixed screenCenterX = 0;
    Fixed screenCenterY = 0;
    Fixed inverseScreenScaleX = 0;
    Fixed inverseScreenScaleY = 0;
    std::vector<BspPlane> clipPlanes;
};

class Bsp : public Renderable, public s3dBase {
public:
    explicit Bsp(Engine* engine);
    Bsp(Renderer* renderer, s3dResManager* resourceManager);
    ~Bsp() override;

    void SetEngine(Engine* engine) override;
    void Render() override;

    void SetEntityFactory(BspEntityFactory factory, void* context);
    void SetTextureDirectory(const char16_t* directory);
    void FreeLastLevel();
    void FreeAll();
    void ResetLightStyles();
    void AdvanceLightStyles();
    void SetLightStyleEnabled(uint8_t index, uint8_t enabled);
    void Update();
    void PrepareRender(const BspRenderState& state);
    void SubmitVisibleEntities(RenderQueues& queues);

    bool IsLoaded() const;
    int32_t GetCurrentCluster() const;
    uint32_t GetFrameCounter() const;
    const std::vector<uint8_t>& GetDecompressedPVS() const;
    const std::vector<uint32_t>& GetVisibleFaces() const;
    const SbpHeader& GetHeader() const;
    const std::array<Fixed, 6>& GetHeaderValues() const;
    const std::u16string& GetDirectory() const;
    const std::vector<Vec3>& GetVertices() const;
    const std::vector<BspEdge>& GetEdges() const;
    const std::vector<int32_t>& GetFaceEdges() const;
    const std::vector<BspPlane>& GetPlanes() const;
    const std::vector<BspTexInfo>& GetTexInfo() const;
    const std::vector<BspFace>& GetFaces() const;
    const std::vector<BspBrush>& GetBrushes() const;
    const std::vector<uint32_t>& GetBrushSides() const;
    const std::vector<BspVisBrush>& GetVisBrushes() const;
    const std::vector<uint32_t>& GetVisFaces() const;
    const std::vector<Fixed>& GetVertexLights() const;
    const std::vector<BspModel>& GetModels() const;
    const std::vector<BspEntityPair>& GetEntityPairs() const;
    const std::vector<BspEntityBrush>& GetEntityBrushes() const;
    const std::vector<uint32_t>& GetEntityLinks() const;
    const std::vector<std::unique_ptr<BspEntity>>& GetEntities() const;
    const std::vector<uint32_t>& GetVisibleEntityIndices() const;
    const std::vector<uint32_t>& GetEntityDictionaryIndices() const;
    const std::vector<uint32_t>& GetLeafBrushes() const;
    const std::vector<uint32_t>& GetLeafVisibleBrushes() const;
    const std::vector<BspPathPair>& GetPathPairs() const;
    const std::vector<BspPathNode>& GetPathNodes() const;
    const std::vector<BspLeaf>& GetLeaves() const;
    const std::vector<BspNode>& GetNodes() const;
    const std::vector<uint8_t>& GetPVS() const;
    const std::vector<uint8_t>& GetLightmapData() const;
    const std::array<BspLightStyle, 256>& GetLightStyles() const;

private:
    friend result LoadSbpMapChunks(Bsp*, const char16_t*, const char16_t*, int*);
    friend result LoadS3dStyleResource(Bsp*, const char16_t*, s3dFilePak*);

    result LoadNext(const char16_t* directory, const char16_t* fileName, int& progress);
    result LoadSource(const char16_t* directory, const char16_t* fileName);
    result ValidateDirectory() const;
    const uint8_t* ChunkData(std::size_t index) const;
    const SbpChunkDescriptor& Chunk(std::size_t index) const;

    result LoadVertices(const SbpChunkDescriptor& chunk);
    result LoadEdges(const SbpChunkDescriptor& chunk);
    result LoadFaceEdges(const SbpChunkDescriptor& chunk);
    result LoadPlanes(const SbpChunkDescriptor& chunk);
    result LoadTexInfo(const SbpChunkDescriptor& chunk);
    result LoadLightmapData(const SbpChunkDescriptor& chunk);
    result LoadFaces(const SbpChunkDescriptor& chunk);
    result BuildFaceLightmapInfo(BspFace& face);
    result LoadBrushes(const SbpChunkDescriptor& chunk);
    result LoadBrushSides(const SbpChunkDescriptor& chunk);
    result LoadVisBrushes(const SbpChunkDescriptor& chunk);
    result LoadVisFaces(const SbpChunkDescriptor& chunk);
    result LoadVertexLights(const SbpChunkDescriptor& chunk);
    result LoadModels(const SbpChunkDescriptor& chunk);
    result LoadEntityPairs(const SbpChunkDescriptor& chunk);
    result LoadEntityBrushes(const SbpChunkDescriptor& chunk);
    result LoadEntityLinks(const SbpChunkDescriptor& chunk);
    result LoadEntities(const SbpChunkDescriptor& chunk);
    result LoadVisibleEntityIndices(const SbpChunkDescriptor& chunk);
    result LoadEntityDictionaryIndices(const SbpChunkDescriptor& chunk);
    result LoadLeafBrushes(const SbpChunkDescriptor& chunk);
    result LoadLeafVisibleBrushes(const SbpChunkDescriptor& chunk);
    result LoadPathPairs(const SbpChunkDescriptor& chunk);
    result LoadPathNodes(const SbpChunkDescriptor& chunk);
    result LoadLeaves(const SbpChunkDescriptor& chunk);
    result LoadNodes(const SbpChunkDescriptor& chunk);
    result LoadPVS(const SbpChunkDescriptor& chunk);

    struct ProjectedVertex {
        Vec3 world;
        Fixed x = 0;
        Fixed y = 0;
        Fixed depth = 0;
    };
    struct LightmapMapping {
        Fixed rayUX = 0;
        Fixed rayUY = 0;
        Fixed rayUZ = 0;
        Fixed rayVX = 0;
        Fixed rayVY = 0;
        Fixed rayVZ = 0;
        Fixed baseU = 0;
        Fixed baseV = 0;
        Fixed maxU = 0;
        Fixed maxV = 0;
    };

    int32_t FindLeaf(const Vec3& position) const;
    std::vector<uint8_t> DecompressPVS(int32_t cluster) const;
    void MarkVisibleLeaves();
    void TraverseVisibleNode(int32_t index, uint32_t planeMask);
    void SubmitFace(uint32_t index, uint32_t planeMask);
    result BuildLightmap(BspFace& face);
    LightmapMapping BuildLightmapMapping(const BspFace& face) const;
    std::vector<ProjectedVertex> ProjectFace(const BspFace& face);
    void RasterizeFace(const BspFace& face, const LightmapMapping& mapping,
                       const std::vector<ProjectedVertex>& polygon);

    Renderer* renderer;
    s3dResManager* resourceManager;
    std::u16string directory;
    std::u16string textureDirectory;
    bool useTextureDirectory = false;
    BspEntityFactory entityFactory = nullptr;
    void* entityFactoryContext = nullptr;
    bool loaded = false;
    std::vector<uint8_t> source;
    SbpHeader header;
    std::array<Fixed, 6> headerValues{};
    std::vector<Vec3> vertices;
    std::vector<BspEdge> edges;
    std::vector<int32_t> faceEdges;
    std::vector<BspPlane> planes;
    std::vector<BspTexInfo> texInfo;
    std::vector<BspFace> faces;
    std::vector<BspBrush> brushes;
    std::vector<uint32_t> brushSides;
    std::vector<BspVisBrush> visBrushes;
    std::vector<uint32_t> visFaces;
    std::vector<Fixed> vertexLights;
    std::vector<BspModel> models;
    std::vector<BspEntityPair> entityPairs;
    std::vector<BspEntityBrush> entityBrushes;
    std::vector<uint32_t> entityLinks;
    std::vector<std::unique_ptr<BspEntity>> entities;
    std::vector<uint32_t> visibleEntityIndices;
    std::vector<uint32_t> entityDictionaryIndices;
    std::vector<uint32_t> leafBrushes;
    std::vector<uint32_t> leafVisibleBrushes;
    std::vector<BspPathPair> pathPairs;
    std::vector<BspPathNode> pathNodes;
    std::vector<BspLeaf> leaves;
    std::vector<BspNode> nodes;
    std::vector<uint8_t> pvs;
    std::vector<uint8_t> lightmapData;
    std::array<BspLightStyle, 256> lightStyles{};
    BspRenderState renderState;
    uint32_t frameCounter = 0;
    uint32_t leafMarkCounter = 0;
    int32_t currentCluster = -1;
    int32_t previousCluster = -1;
    std::vector<uint8_t> decompressedPVS;
    std::vector<uint32_t> visibleFaces;
    std::vector<uint8_t> visibleFaceFlags;
};

result LoadSbpMapChunks(Bsp* bsp, const char16_t* directory, const char16_t* fileName,
                        int* progress);
result LoadS3dStyleResource(Bsp* bsp, const char16_t* fileName, s3dFilePak* filePak);

} // namespace soft3d

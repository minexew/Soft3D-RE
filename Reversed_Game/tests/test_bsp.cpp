#include "soft3d/Bsp.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void AppendFloat(std::vector<uint8_t>& bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    Append32(bytes, bits);
}

void Write32(std::vector<uint8_t>& bytes, std::size_t offset, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.at(offset + shift / 8) = static_cast<uint8_t>(value >> shift);
    }
}

std::vector<uint8_t> Record16(std::initializer_list<uint16_t> values)
{
    std::vector<uint8_t> bytes;
    for (const uint16_t value : values) {
        Append16(bytes, value);
    }
    return bytes;
}

std::vector<uint8_t> Record32(std::initializer_list<uint32_t> values)
{
    std::vector<uint8_t> bytes;
    for (const uint32_t value : values) {
        Append32(bytes, value);
    }
    return bytes;
}

using Chunks = std::array<std::vector<uint8_t>, soft3d::sbpChunkCount>;

std::vector<uint8_t> MakeSbp(const Chunks& chunks)
{
    std::vector<uint8_t> bytes(soft3d::sbpHeaderBytes, 0);
    std::copy_n("S3D", 4, bytes.begin());
    const float version = 7.6f;
    std::memcpy(bytes.data() + 4, &version, 4);
    const std::array<float, 6> headerValues = {1.5f, -2.25f, 0.25f, 0.0f, 0.0f, 1.0f};
    for (std::size_t index = 0; index < headerValues.size(); ++index) {
        uint32_t bits;
        std::memcpy(&bits, &headerValues[index], 4);
        Write32(bytes, 8 + index * 4, bits);
    }
    uint32_t offset = soft3d::sbpHeaderBytes;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        Write32(bytes, 0x20 + index * 8, offset);
        Write32(bytes, 0x24 + index * 8, static_cast<uint32_t>(chunks[index].size()));
        bytes.insert(bytes.end(), chunks[index].begin(), chunks[index].end());
        offset += static_cast<uint32_t>(chunks[index].size());
    }
    return bytes;
}

std::vector<uint8_t> MakeArchive(const std::string& entryName,
                                 const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> bytes;
    Append16(bytes, 1);
    std::array<char, soft3d::pakPathBytes> name{};
    std::copy(entryName.begin(), entryName.end(), name.begin());
    bytes.insert(bytes.end(), name.begin(), name.end());
    Append32(bytes, static_cast<uint32_t>(2 + soft3d::pakEntryBytes));
    bytes.insert(bytes.end(), data.begin(), data.end());
    return bytes;
}

class TempArchive {
public:
    TempArchive(const std::string& entryName, const std::vector<uint8_t>& data)
        : path(std::filesystem::temp_directory_path() / "soft3d-bsp-tests.spk")
    {
        const std::vector<uint8_t> bytes = MakeArchive(entryName, data);
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.close();
        CHECK(output.good());
    }

    ~TempArchive()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

soft3d::result LoadBytes(const std::vector<uint8_t>& bytes)
{
    TempArchive archive(".\\common\\test.sbp", bytes);
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    soft3d::Bsp bsp(&renderer, &manager);
    return soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", nullptr);
}

std::vector<uint8_t> Vertex(float x, float y, float z)
{
    std::vector<uint8_t> bytes;
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
    return bytes;
}

std::vector<uint8_t> Plane(float x, float y, float z, float distance, uint32_t type = 0)
{
    std::vector<uint8_t> bytes;
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
    AppendFloat(bytes, distance);
    Append32(bytes, type);
    return bytes;
}

std::vector<uint8_t> PathNode(float x, float y, float z, uint16_t firstPair, int16_t count)
{
    std::vector<uint8_t> bytes = Vertex(x, y, z);
    Append16(bytes, firstPair);
    Append16(bytes, static_cast<uint16_t>(count));
    return bytes;
}

void TestIncrementalConversionAndCleanup()
{
    Chunks chunks;
    chunks[soft3d::SbpVertices] = Vertex(1.5f, -2.25f, 0.25f);
    const std::vector<uint8_t> second = Vertex(-1.0f, 2.0f, -3.0f);
    chunks[soft3d::SbpVertices].insert(chunks[soft3d::SbpVertices].end(),
                                      second.begin(), second.end());
    chunks[soft3d::SbpEdges] = Record16({0, 1, 0, 1});
    chunks[soft3d::SbpFaceEdges] = Record32({0, 0xffffffff});
    chunks[soft3d::SbpPlanes] = Plane(-1.0f, -2.0f, -3.0f, 4.5f, 7);
    chunks[soft3d::SbpBrushes] = Record16({0, 1});
    chunks[soft3d::SbpBrushSides] = Record32({0});
    chunks[soft3d::SbpVertexLights] = {0, 128, 255};
    chunks[soft3d::SbpPathPairs] = Record16({0, 1});
    chunks[soft3d::SbpPathNodes] = PathNode(2.0f, 3.0f, 4.0f, 0, 0);
    const std::vector<uint8_t> secondPath = PathNode(5.0f, 6.0f, 7.0f, 0, 1);
    chunks[soft3d::SbpPathNodes].insert(chunks[soft3d::SbpPathNodes].end(),
                                       secondPath.begin(), secondPath.end());
    chunks[soft3d::SbpPVS] = {0x12, 0x34, 0x56};
    chunks[soft3d::SbpLightmapData] = {9, 8, 7, 6};

    TempArchive archive(".\\common\\test.sbp", MakeSbp(chunks));
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    soft3d::Bsp bsp(&renderer, &manager);
    int progress = 0;
    CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", &progress) ==
          soft3d::S3D_OK);
    CHECK(progress == 4);
    CHECK(bsp.GetVertices().size() == 2);
    CHECK(bsp.GetVertices()[0].x == 0x18000);
    CHECK(bsp.GetVertices()[0].y == -0x24000);
    CHECK(bsp.GetVertices()[0].z == 0x4000);
    CHECK(bsp.GetHeaderValues()[0] == 0x18000);
    CHECK(bsp.GetHeaderValues()[1] == -0x24000);
    while (progress < 48) {
        CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", &progress) ==
              soft3d::S3D_OK);
    }
    CHECK(bsp.GetPlanes()[0].negativeComponents == 7);
    CHECK(bsp.GetPlanes()[0].type == 7);
    CHECK(bsp.GetPlanes()[0].distance == 0x48000);
    CHECK(bsp.GetVertexLights().size() == 3);
    CHECK(bsp.GetVertexLights()[0] == 0x1f000000);
    CHECK(bsp.GetVertexLights()[1] == 0x0f800000);
    CHECK(bsp.GetVertexLights()[2] == 0x001f0000);
    while (progress != 100) {
        CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", &progress) ==
              soft3d::S3D_OK);
    }
    CHECK(bsp.IsLoaded());
    CHECK(bsp.GetVertexLights().empty());
    CHECK(bsp.GetPathNodes().size() == 2);
    CHECK(bsp.GetPathNodes()[0].position.x == 0x20000);
    CHECK(bsp.GetPathNodes()[0].index == 0 && bsp.GetPathNodes()[1].index == 1);
    CHECK(bsp.GetPathNodes()[1].firstPair == 0 && bsp.GetPathNodes()[1].pairCount == 1);
    CHECK(bsp.GetPVS() == std::vector<uint8_t>({0x12, 0x34, 0x56}));
    CHECK(bsp.GetLightmapData() == std::vector<uint8_t>({9, 8, 7, 6}));
    CHECK(bsp.GetLightStyles()[0].enabled == 0);
    CHECK(bsp.GetLightStyles()[1].enabled == 1);
    CHECK(bsp.GetLightStyles()[254].enabled == 1);
    CHECK(bsp.GetLightStyles()[255].enabled == 0);
    bsp.FreeLastLevel();
    CHECK(!bsp.IsLoaded());
    CHECK(bsp.GetVertices().empty() && bsp.GetNodes().empty() && bsp.GetPVS().empty());
    CHECK(bsp.GetLightStyles()[1].enabled == 1);
    bsp.FreeAll();
    CHECK(bsp.GetLightStyles()[1].enabled == 0);
}

struct FactoryContext {
    int constructed = 0;
    int destroyed = 0;
    soft3d::BspEntityRecord record;
};

struct TestEntity final : soft3d::BspEntity {
    explicit TestEntity(FactoryContext* context) : context(context) {}
    ~TestEntity() override { ++context->destroyed; }
    FactoryContext* context;
};

soft3d::BspEntity* CreateEntity(soft3d::BspEntityRecord* record, void* opaque,
                                soft3d::BspEntityRecord* base)
{
    auto* context = static_cast<FactoryContext*>(opaque);
    ++context->constructed;
    context->record = *record;
    CHECK(base != nullptr && record >= base);
    return new TestEntity(context);
}

std::vector<uint8_t> EntityRecord()
{
    std::vector<uint8_t> bytes;
    Append16(bytes, 7);
    Append16(bytes, 0xffff);
    Append16(bytes, 0);
    Append16(bytes, 1);
    Append16(bytes, 0);
    Append16(bytes, 1);
    Append32(bytes, 0);
    return bytes;
}

void TestEntityFactoryAndLinks()
{
    Chunks chunks;
    chunks[soft3d::SbpEntityPairs].resize(32, 0);
    std::copy_n("classname", 10, chunks[soft3d::SbpEntityPairs].begin());
    std::copy_n("thing", 6, chunks[soft3d::SbpEntityPairs].begin() + 16);
    chunks[soft3d::SbpEntityBrushes] = Record16({1, 2, 3, 4, 5, 6});
    chunks[soft3d::SbpEntityLinks] = Record32({0x12345678});
    chunks[soft3d::SbpEntities] = EntityRecord();
    chunks[soft3d::SbpVisibleEntityIndices] = Record32({0});
    chunks[soft3d::SbpEntityDictionaryIndices] = Record32({0});

    TempArchive archive(".\\common\\test.sbp", MakeSbp(chunks));
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    FactoryContext context;
    {
        soft3d::Bsp bsp(&renderer, &manager);
        bsp.SetEntityFactory(CreateEntity, &context);
        CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", nullptr) ==
              soft3d::S3D_OK);
        CHECK(context.constructed == 1 && context.destroyed == 0);
        CHECK(context.record.type == 7 && context.record.modelIndex == -1);
        CHECK(context.record.firstLink == 0 && context.record.linkCount == 1);
        CHECK(context.record.firstPair == 0 && context.record.pairCount == 1);
        CHECK(context.record.brushIndex == 0);
        CHECK(bsp.GetEntities().size() == 1);
        CHECK(bsp.GetVisibleEntityIndices() == std::vector<uint32_t>({0}));
        CHECK(bsp.GetEntityDictionaryIndices() == std::vector<uint32_t>({0}));
        CHECK(bsp.GetEntityLinks().empty());
        CHECK(std::string(bsp.GetEntityPairs()[0].key.data()) == "classname");
        CHECK(std::string(bsp.GetEntityPairs()[0].value.data()) == "thing");
        bsp.FreeLastLevel();
        CHECK(context.destroyed == 1);
    }
}

void TestMalformedMaps()
{
    CHECK(LoadBytes(std::vector<uint8_t>(20, 0)) == soft3d::S3D_FORMAT);
    Chunks chunks;
    std::vector<uint8_t> valid = MakeSbp(chunks);
    CHECK(LoadBytes(valid) == soft3d::S3D_OK);
    std::vector<uint8_t> badMagic = valid;
    std::copy_n("XXXX", 4, badMagic.begin());
    CHECK(LoadBytes(badMagic) == soft3d::S3D_OK);
    std::vector<uint8_t> badVersion = valid;
    Write32(badVersion, 4, 0x40f00000);
    CHECK(LoadBytes(badVersion) == soft3d::S3D_FORMAT);
    std::vector<uint8_t> badDirectory = valid;
    Write32(badDirectory, 0x20, 0xfffffff0);
    Write32(badDirectory, 0x24, 0x20);
    CHECK(LoadBytes(badDirectory) == soft3d::S3D_FORMAT);
    chunks[soft3d::SbpVertices] = {0};
    CHECK(LoadBytes(MakeSbp(chunks)) == soft3d::S3D_FORMAT);
    chunks = Chunks{};
    chunks[soft3d::SbpVertices] = Vertex(0, 0, 0);
    chunks[soft3d::SbpEdges] = Record16({1, 0});
    CHECK(LoadBytes(MakeSbp(chunks)) == soft3d::S3D_FORMAT);
    chunks = Chunks{};
    chunks[soft3d::SbpVisibleEntityIndices] = Record32({0});
    CHECK(LoadBytes(MakeSbp(chunks)) == soft3d::S3D_FORMAT);
    chunks = Chunks{};
    chunks[soft3d::SbpEntities] = EntityRecord();
    CHECK(LoadBytes(MakeSbp(chunks)) == soft3d::S3D_STATE);
    chunks = Chunks{};
    chunks[soft3d::SbpPlanes] = Plane(0, 1, 0, 0);
    chunks[soft3d::SbpFaces].resize(24, 0);
    Write32(chunks[soft3d::SbpFaces], 8, 0);
    CHECK(LoadBytes(MakeSbp(chunks)) == soft3d::S3D_FORMAT);

    TempArchive archive(".\\common\\test.sbp", valid);
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    soft3d::Bsp bsp(&renderer, &manager);
    CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"missing.sbp", nullptr) ==
          soft3d::S3D_ACCESS);
    CHECK(soft3d::LoadSbpMapChunks(nullptr, u".\\common\\", u"test.sbp", nullptr) ==
          soft3d::S3D_PARAM);
}

void TestRealMenu()
{
#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
    std::cout << "SKIP: real archive uses the Chinese layout\n";
#else
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == soft3d::S3D_OK);
    soft3d::Renderer renderer(320, 240);
    soft3d::Bsp bsp(&renderer, &manager);
    FactoryContext entityContext;
    bsp.SetEntityFactory(CreateEntity, &entityContext);
    int progress = 0;
    int calls = 0;
    while (progress != 100) {
        const soft3d::result status =
            soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"menu.sbp", &progress);
        if (status != soft3d::S3D_OK) {
            std::cerr << "real map failed at progress " << progress << " with " << status << '\n';
        }
        CHECK(status == soft3d::S3D_OK);
        ++calls;
        CHECK(calls <= 26);
    }
    CHECK(calls == 26);
    CHECK(bsp.IsLoaded());
    CHECK(bsp.GetDirectory() == u".\\common\\");
    CHECK(bsp.GetHeaderValues()[0] == static_cast<soft3d::Fixed>(0xffefba31));
    CHECK(bsp.GetHeaderValues()[1] == -0x1c0000);
    CHECK(bsp.GetHeaderValues()[5] == 0x10000);
    CHECK(bsp.GetVertices().size() == 421);
    CHECK(bsp.GetVertices()[0].x == -0x240000);
    CHECK(bsp.GetVertices()[0].y == -0x110000);
    CHECK(bsp.GetVertices()[0].z == 0);
    CHECK(bsp.GetVertices()[1].z == -0x200000);
    CHECK(bsp.GetEdges().size() == 718);
    CHECK(bsp.GetFaceEdges().size() == 1419);
    CHECK(bsp.GetPlanes().size() == 406);
    CHECK(bsp.GetTexInfo().size() == 281);
    CHECK(bsp.GetFaces().size() == 316);
    CHECK(bsp.GetBrushes().size() == 47);
    CHECK(bsp.GetBrushSides().size() == 375);
    CHECK(bsp.GetVisBrushes().size() == 18);
    CHECK(bsp.GetVisFaces().size() == 212);
    CHECK(bsp.GetModels().size() == 5);
    CHECK(bsp.GetEntityPairs().size() == 63);
    CHECK(bsp.GetEntityBrushes().size() == 6);
    CHECK(entityContext.constructed == 6);
    CHECK(bsp.GetEntities().size() == 6);
    CHECK(bsp.GetVisibleEntityIndices().size() == 6);
    CHECK(bsp.GetEntityDictionaryIndices().size() == 1);
    CHECK(bsp.GetLeafBrushes().size() == 93);
    CHECK(bsp.GetLeafVisibleBrushes().size() == 3);
    CHECK(bsp.GetPathPairs().empty() && bsp.GetPathNodes().empty());
    CHECK(bsp.GetLeaves().size() == 178);
    CHECK(bsp.GetNodes().size() == 176);
    CHECK(bsp.GetPVS().size() == 378);
    CHECK(bsp.GetLightmapData().size() == 16563);
    CHECK(bsp.GetHeader().chunks[soft3d::SbpUnused17].length == 0);
    CHECK(bsp.GetHeader().chunks[soft3d::SbpPVS].length == 378);
    for (const soft3d::BspTexInfo& info : bsp.GetTexInfo()) {
        CHECK(info.texture != nullptr);
    }
    for (const soft3d::BspModel& model : bsp.GetModels()) {
        CHECK(model.object != nullptr && model.object->GetModel() != nullptr);
        CHECK(model.animation != nullptr);
    }
    CHECK(bsp.GetModels()[0].transform[0].x == static_cast<soft3d::Fixed>(0xffc4c50b));
    const soft3d::BspFace& face = bsp.GetFaces()[0];
    CHECK(face.planeIndex == 148 && face.edgeCount == 4 && face.texInfoIndex == 193);
    CHECK(face.planeSide == 0x800);
    CHECK(face.textureMin[0] == 0 && face.textureMin[1] == 0);
    CHECK(face.lightmapMin[0] == -448 && face.lightmapMin[1] == 112);
    CHECK(face.lightmapSize[0] == 16 && face.lightmapSize[1] == 64);
    CHECK(face.lightmapPowerOfTwoWidth == 16 && face.lightmapWidthShift == 4);
    CHECK(face.lightmapSampleCount == 10);
    CHECK(bsp.GetNodes()[0].parentIndex == -1);
    CHECK(bsp.GetEntityLinks().empty() && bsp.GetVertexLights().empty());
#endif
}

} // namespace

int main()
{
    try {
        TestIncrementalConversionAndCleanup();
        TestEntityFactoryAndLinks();
        TestMalformedMaps();
        TestRealMenu();
        std::cout << "BSP tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

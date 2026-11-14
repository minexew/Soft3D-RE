#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/GameEngine.hpp"

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

using Chunks = std::array<std::vector<uint8_t>, soft3d::sbpChunkCount>;

std::vector<uint8_t> MakeSbp(const Chunks& chunks)
{
    std::vector<uint8_t> bytes(soft3d::sbpHeaderBytes, 0);
    std::copy_n("S3D", 4, bytes.begin());
    const float version = 7.6f;
    std::memcpy(bytes.data() + 4, &version, 4);
    uint32_t offset = soft3d::sbpHeaderBytes;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        Write32(bytes, 0x20 + index * 8, offset);
        Write32(bytes, 0x24 + index * 8, static_cast<uint32_t>(chunks[index].size()));
        bytes.insert(bytes.end(), chunks[index].begin(), chunks[index].end());
        offset += static_cast<uint32_t>(chunks[index].size());
    }
    return bytes;
}

struct Entry {
    std::string name;
    std::vector<uint8_t> data;
};

std::vector<uint8_t> MakeArchive(const std::vector<Entry>& entries)
{
    std::vector<uint8_t> bytes;
    Append16(bytes, static_cast<uint16_t>(entries.size()));
    uint32_t offset = static_cast<uint32_t>(2 + entries.size() * soft3d::pakEntryBytes);
    for (const Entry& entry : entries) {
        std::array<char, soft3d::pakPathBytes> name{};
        std::copy(entry.name.begin(), entry.name.end(), name.begin());
        bytes.insert(bytes.end(), name.begin(), name.end());
        Append32(bytes, offset);
        offset += static_cast<uint32_t>(entry.data.size());
    }
    for (const Entry& entry : entries) {
        bytes.insert(bytes.end(), entry.data.begin(), entry.data.end());
    }
    return bytes;
}

class TempArchive {
public:
    explicit TempArchive(const std::vector<Entry>& entries)
        : path(std::filesystem::temp_directory_path() / "soft3d-bsp-render-tests.spk")
    {
        const std::vector<uint8_t> bytes = MakeArchive(entries);
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

void AppendVertex(std::vector<uint8_t>& bytes, float x, float y, float z)
{
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
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

std::vector<uint8_t> Plane(float x, float y, float z, float distance)
{
    std::vector<uint8_t> bytes;
    AppendFloat(bytes, x);
    AppendFloat(bytes, y);
    AppendFloat(bytes, z);
    AppendFloat(bytes, distance);
    Append32(bytes, 3);
    return bytes;
}

std::vector<uint8_t> PackedName(const std::string& text)
{
    const std::string alphabet = "abcdefghijklmnopqrstuvwxyz_1234";
    std::array<uint32_t, 4> packed{};
    std::size_t bit = 0;
    for (const char character : text) {
        packed[bit / 30] |= static_cast<uint32_t>(alphabet.find(character)) << (bit % 30);
        bit += 5;
    }
    packed[bit / 30] |= 0x1fU << (bit % 30);
    std::vector<uint8_t> bytes;
    for (const uint32_t value : packed) {
        Append32(bytes, value);
    }
    return bytes;
}

std::vector<uint8_t> MakeStx()
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 0xffff4040);
    Append32(bytes, 2);
    Append32(bytes, 2);
    for (uint32_t index = 0; index < 2048; ++index) {
        Append16(bytes, static_cast<uint16_t>(index));
    }
    bytes.insert(bytes.end(), 16, 0);
    return bytes;
}

Chunks OneFaceChunks()
{
    Chunks chunks;
    AppendVertex(chunks[soft3d::SbpVertices], 0, 0, 2);
    AppendVertex(chunks[soft3d::SbpVertices], 1, 0, 2);
    AppendVertex(chunks[soft3d::SbpVertices], 1, 1, 2);
    AppendVertex(chunks[soft3d::SbpVertices], 0, 1, 2);
    chunks[soft3d::SbpEdges] = Record16({0, 1, 1, 2, 2, 3, 3, 0});
    chunks[soft3d::SbpFaceEdges] = Record32({0, 1, 2, 3});
    chunks[soft3d::SbpPlanes] = Plane(0, 0, -1, 2);
    AppendFloat(chunks[soft3d::SbpTexInfo], 1);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    AppendFloat(chunks[soft3d::SbpTexInfo], 1);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    AppendFloat(chunks[soft3d::SbpTexInfo], 0);
    const std::vector<uint8_t> name = PackedName("tex");
    chunks[soft3d::SbpTexInfo].insert(chunks[soft3d::SbpTexInfo].end(), name.begin(), name.end());
    chunks[soft3d::SbpFaces] = Record16({0, 0, 0, 4, 0, 0, 0, 0});
    Append32(chunks[soft3d::SbpFaces], 0);
    chunks[soft3d::SbpFaces].insert(chunks[soft3d::SbpFaces].end(), {0, 0, 0, 0});
    chunks[soft3d::SbpVisFaces] = Record32({0});
    chunks[soft3d::SbpLightmapData] = {0, 0, 0, 0};
    chunks[soft3d::SbpLeaves] = Record16({0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0});
    chunks[soft3d::SbpNodes].resize(24, 0);
    Write32(chunks[soft3d::SbpNodes], 4, 0xffffffff);
    Write32(chunks[soft3d::SbpNodes], 6, 0xffffffff);
    Write32(chunks[soft3d::SbpNodes], 20, 0x00010000);
    chunks[soft3d::SbpPVS] = Record32({1, 8});
    chunks[soft3d::SbpPVS].push_back(1);
    return chunks;
}

soft3d::BspRenderState RenderState(int width, int height)
{
    soft3d::BspRenderState state;
    state.screenScaleX = width << 15;
    state.screenScaleY = height << 15;
    state.screenCenterX = width << 15;
    state.screenCenterY = height << 15;
    state.inverseScreenScaleX = static_cast<soft3d::Fixed>(
        (static_cast<int64_t>(1) << 32) / state.screenScaleX);
    state.inverseScreenScaleY = static_cast<soft3d::Fixed>(
        (static_cast<int64_t>(1) << 32) / state.screenScaleY);
    return state;
}

void TestLightmapAndPVS()
{
    TempArchive archive({
        {".\\common\\test.sbp", MakeSbp(OneFaceChunks())},
        {".\\common\\tex.stx", MakeStx()},
    });
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.path.u16string().c_str()) ==
          soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    renderer.Clear(0);
    soft3d::Bsp bsp(&renderer, &manager);
    CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"test.sbp", nullptr) == soft3d::S3D_OK);
    bsp.PrepareRender(RenderState(8, 8));
    CHECK(bsp.GetCurrentCluster() == 0);
    CHECK(bsp.GetDecompressedPVS() == std::vector<uint8_t>({1}));
    CHECK(bsp.GetVisibleFaces() == std::vector<uint32_t>({0}));
    const auto& lightmap = bsp.GetFaces()[0].lightmapPixels;
    CHECK(lightmap.size() == 256);
    CHECK(std::all_of(lightmap.begin(), lightmap.end(), [](uint16_t color) { return color == 62; }));
    bsp.Render();
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(renderer.GetCurrentTexture()->GetPixels());
    CHECK(pixels[4 * 8 + 4] != 0);
    CHECK(static_cast<uint16_t>(pixels[4 * 8 + 4]) == 62);
    CHECK((pixels[4 * 8 + 4] >> 16) != 0);
    CHECK(pixels[0] == 0);
}

void TestRealStyleResource()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    soft3d::Bsp bsp(&renderer, &manager);
    CHECK(soft3d::LoadS3dStyleResource(&bsp, u".\\common\\default.s3dsty",
                                       manager.GetFilePak()) == soft3d::S3D_OK);
    const auto& styles = bsp.GetLightStyles();
    CHECK(styles[0].frameCount == 1 && styles[0].frames == std::vector<uint8_t>({8}));
    CHECK(styles[1].frameCount == 203 && styles[1].frames.size() == 203);
    CHECK(styles[6].frameCount == 20);
    const std::array<uint8_t, 20> expected = {0, 1, 2, 1, 0, 1, 2, 3, 4, 5,
                                              6, 7, 8, 7, 6, 5, 4, 3, 2, 1};
    CHECK(std::equal(expected.begin(), expected.end(), styles[6].frames.begin()));
    bsp.AdvanceLightStyles();
    CHECK(bsp.GetLightStyles()[6].light == 0 && bsp.GetLightStyles()[6].position == 1);
    bsp.AdvanceLightStyles();
    CHECK(bsp.GetLightStyles()[6].light == 1 && bsp.GetLightStyles()[6].position == 2);
    bsp.AdvanceLightStyles();
    CHECK(bsp.GetLightStyles()[6].light == 2 && bsp.GetLightStyles()[6].position == 3);
    bsp.AdvanceLightStyles();
    CHECK(bsp.GetLightStyles()[6].light == 1 && bsp.GetLightStyles()[6].position == 4);
    for (int index = 4; index < 20; ++index) {
        bsp.AdvanceLightStyles();
    }
    CHECK(bsp.GetLightStyles()[6].light == 1 && bsp.GetLightStyles()[6].position == 0);
    bsp.AdvanceLightStyles();
    CHECK(bsp.GetLightStyles()[6].light == 0 && bsp.GetLightStyles()[6].position == 1);
    bsp.SetLightStyleEnabled(6, 0);
    CHECK(bsp.GetLightStyles()[6].light == 0);
#endif
}

struct FactoryContext {
    soft3d::Bsp* bsp = nullptr;
    int submittedModels = 0;
};

soft3d::BspEntity* CreateEntity(soft3d::BspEntityRecord* record, void* opaque,
                                soft3d::BspEntityRecord*)
{
    auto* context = static_cast<FactoryContext*>(opaque);
    auto* entity = new soft3d::BspEntity;
    if (record->modelIndex >= 0) {
        entity->SetObject(context->bsp->GetModels()[record->modelIndex].object.get());
        ++context->submittedModels;
    }
    return entity;
}

uint32_t HashPixels(const uint32_t* pixels, std::size_t count)
{
    uint32_t hash = 2166136261U;
    for (std::size_t index = 0; index < count; ++index) {
        hash ^= pixels[index];
        hash *= 16777619U;
    }
    return hash;
}

void TestRealMenuRender()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::EngineConfig config;
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    config.videoBackend = std::move(backend);
    soft3d::GameEngine engine(std::move(config));
    CHECK(soft3d::OpenGameResourceArchive(&engine.GetResourceManager(), SOFT3D_TEST_ARCHIVE) ==
          soft3d::S3D_OK);
    engine.Start();
    soft3d::Bsp bsp(&engine);
    FactoryContext context{&bsp, 0};
    bsp.SetEntityFactory(CreateEntity, &context);
    CHECK(soft3d::LoadS3dStyleResource(&bsp, u".\\common\\default.s3dsty",
                                       engine.GetResourceManager().GetFilePak()) == soft3d::S3D_OK);
    CHECK(soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"menu.sbp", nullptr) == soft3d::S3D_OK);
    soft3d::BspRenderState state = RenderState(320, 240);
    state.cameraPosition = {-66 * 0x10000, -22 * 0x10000, -43 * 0x10000};
    state.worldToCamera.values[12] = 66 << 16;
    state.worldToCamera.values[13] = 22 << 16;
    state.worldToCamera.values[14] = 43 << 16;
    engine.GetRenderer().SetViewMatrix(state.worldToCamera);
    engine.GetRenderer().Clear(0);
    bsp.PrepareRender(state);
    CHECK(bsp.GetCurrentCluster() == 0);
    CHECK(bsp.GetDecompressedPVS().size() == 5);
    CHECK(bsp.GetVisibleFaces().size() == 166);
    CHECK(context.submittedModels == 5);
    CHECK(bsp.GetModels()[0].object->GetModelMatrix().values[0] ==
          bsp.GetModels()[0].transform[2].x);
    CHECK(bsp.GetModels()[0].object->GetModelMatrix().values[12] ==
          bsp.GetModels()[0].transform[0].x);
    bsp.SubmitVisibleEntities(engine.GetRenderQueues());
    CHECK(engine.GetRenderQueues().GetCount(3) != 0);
    bsp.Render();
    engine.GetRenderQueues().RenderSceneQueues();
    const uint32_t* frame = reinterpret_cast<const uint32_t*>(
        engine.GetRenderer().GetCurrentTexture()->GetPixels());
    const uint32_t frameHash = HashPixels(frame, 320 * 240);
    CHECK(frameHash == 0xb10b52d9);
    engine.GetRenderer().Present();
    CHECK(engine.GetVideo().Update());
    CHECK(dummy->GetPresentedPixels().size() == 320 * 240);
    const uint32_t presentedHash = HashPixels(dummy->GetPresentedPixels().data(), 320 * 240);
    CHECK(presentedHash == 0x8112c675);
#endif
}

} // namespace

int main()
{
    try {
        TestLightmapAndPVS();
        TestRealStyleResource();
        TestRealMenuRender();
        std::cout << "BSP render tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

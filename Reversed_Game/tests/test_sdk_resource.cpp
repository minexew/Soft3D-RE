#include "soft3d/Bsp.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/TextureTarget.hpp"
#include "soft3d/s3dFilePak.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void Write32(std::vector<uint8_t>& bytes, std::size_t offset, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.at(offset + shift / 8) = static_cast<uint8_t>(value >> shift);
    }
}

class TempFile {
public:
    TempFile(const char* name, const std::vector<uint8_t>& bytes)
        : path(std::filesystem::temp_directory_path() / name)
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.close();
        CHECK(output.good());
    }

    ~TempFile()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

std::vector<uint8_t> MakeRawStx(uint32_t width, uint32_t height)
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 0xffff1200);
    Append32(bytes, width);
    Append32(bytes, height);
    for (uint32_t index = 0; index < width * height; ++index) {
        bytes.push_back(static_cast<uint8_t>(index));
    }
    return bytes;
}

std::vector<uint8_t> ReadRealStx()
{
    soft3d::s3dFilePak archive;
    CHECK(archive.Open(SOFT3D_TEST_ARCHIVE, 0) == soft3d::S3D_OK);
    CHECK(archive.Select(u".\\common\\aj_zhiwu_a.stx") == soft3d::S3D_OK);
    archive.Seek(0, SEEK_END);
    const int bytes = archive.Tell();
    CHECK(bytes > 0);
    std::vector<uint8_t> data(static_cast<std::size_t>(bytes));
    archive.Seek(0, SEEK_SET);
    CHECK(archive.Read(data.data(), bytes) == soft3d::S3D_OK);
    return data;
}

void TestTextureTargetSynthetic()
{
    const TempFile tiledFile("soft3d-sdk-target-tiled.stx", MakeRawStx(2, 4));
    soft3d::TextureTarget tiled(2, 2);
    CHECK(tiled.GetWidth() == 2 && tiled.GetHeight() == 2);
    CHECK(tiled.GetStrideBytes() == 8 && tiled.GetTiledHeight() == 3);
    CHECK(tiled.GetOffset().x == 0 && tiled.GetOffset().y == 0);
    CHECK(tiled.LoadStx(tiledFile.path.u16string().c_str()) == soft3d::S3D_OK);
    CHECK(tiled.HasTiledBuffer());
    CHECK(tiled.GetTexture()->GetPixels() == nullptr);
    const std::array<uint8_t, 24> expected = {
        0, 1, 2, 1, 2, 1, 2, 1,
        4, 3, 4, 3, 4, 3, 4, 3,
        6, 5, 6, 5, 6, 5, 6, 5,
    };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (expected[index] != tiled.GetPixels()[index]) {
            std::cerr << "tile mismatch at " << index << ": expected "
                      << static_cast<unsigned>(expected[index]) << ", got "
                      << static_cast<unsigned>(tiled.GetPixels()[index]) << '\n';
        }
    }
    CHECK(std::equal(expected.begin(), expected.end(), tiled.GetPixels()));
    const uint8_t* previous = tiled.GetPixels();
    CHECK(tiled.LoadStx(tiledFile.path.u16string().c_str()) == soft3d::S3D_OK);
    CHECK(std::equal(expected.begin(), expected.end(), tiled.GetPixels()));

    const std::filesystem::path missing = tiledFile.path.parent_path() / "soft3d-sdk-missing.stx";
    CHECK(tiled.LoadStx(missing.u16string().c_str()) == soft3d::S3D_FILE);
    CHECK(tiled.GetPixels() == previous);
    CHECK(std::equal(expected.begin(), expected.end(), tiled.GetPixels()));

    const TempFile matchingFile("soft3d-sdk-target-matching.stx", MakeRawStx(8, 1));
    soft3d::TextureTarget matching(2, 2);
    CHECK(matching.LoadStx(matchingFile.path.u16string().c_str()) == soft3d::S3D_OK);
    CHECK(!matching.HasTiledBuffer());
    CHECK(matching.GetPixels() == matching.GetTexture()->GetPixels());
    CHECK(matching.GetTexture()->GetWidth() == 8);
    const std::array<uint8_t, 8> source = {0, 1, 2, 3, 4, 5, 6, 7};
    CHECK(std::equal(source.begin(), source.end(), matching.GetPixels()));

    const TempFile shortFile("soft3d-sdk-target-short.stx", MakeRawStx(2, 1));
    soft3d::TextureTarget insufficient(2, 4);
    CHECK(insufficient.LoadStx(shortFile.path.u16string().c_str()) == soft3d::S3D_FORMAT);
    CHECK(!insufficient.HasTiledBuffer());
}

void TestTextureTargetRealResource()
{
#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
    std::cout << "SKIP: real STX uses the Chinese archive\n";
#else
    const TempFile realFile("soft3d-sdk-target-real.stx", ReadRealStx());
    soft3d::TextureTarget matching(32, 24);
    CHECK(matching.LoadStx(realFile.path.u16string().c_str()) == soft3d::S3D_OK);
    CHECK(!matching.HasTiledBuffer());
    CHECK(matching.GetTexture()->GetWidth() == 128);
    CHECK(matching.GetTexture()->GetHeight() == 128);

    soft3d::TextureTarget tiled(31, 24);
    CHECK(tiled.LoadStx(realFile.path.u16string().c_str()) == soft3d::S3D_OK);
    CHECK(tiled.HasTiledBuffer());
    CHECK(tiled.GetStrideBytes() == 124 && tiled.GetTiledHeight() == 36);
    const std::vector<uint8_t>& real = ReadRealStx();
    const uint8_t* source = matching.GetPixels();
    int32_t sourceX = 0;
    for (int32_t y = 0; y < tiled.GetTiledHeight(); ++y) {
        for (int32_t x = 0; x < tiled.GetStrideBytes(); ++x) {
            CHECK(tiled.GetPixels()[y * tiled.GetStrideBytes() + x] == source[y * 128 + sourceX]);
            ++sourceX;
            if (sourceX > 128) {
                sourceX -= 128;
            }
        }
    }
    CHECK(real.size() > 12);
#endif
}

std::unique_ptr<soft3d::Texture> MakeSpriteTexture()
{
    auto texture = std::make_unique<soft3d::Texture>();
    std::vector<uint8_t> bytes;
    Append32(bytes, 2U | (0x40U << 8) | (0xffffU << 16));
    Append32(bytes, 2);
    Append32(bytes, 2);
    const std::size_t palette = bytes.size();
    bytes.resize(palette + 128, 0);
    bytes.insert(bytes.end(), {0, 1, 1, 0});
    const auto setColor = [&](uint8_t index, uint8_t intensity, uint16_t color) {
        std::memcpy(bytes.data() + palette + (index * 32 + intensity) * 2, &color, 2);
    };
    setColor(0, 0, 0x07e0);
    setColor(0, 15, 0x001f);
    setColor(1, 0, 0x2104);
    setColor(1, 15, 0xf800);
    CHECK(texture->ParseStx(bytes.data()) == soft3d::S3D_OK);
    return texture;
}

uint32_t Pixel(soft3d::Renderer& renderer, int32_t x, int32_t y)
{
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    return pixels[y * 8 + x];
}

void TestSpanFills()
{
    soft3d::Renderer renderer(8, 8);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels());
    std::fill_n(pixels, 64, 0x12345678U);
    renderer.GetRaster().FillColor16(-2, 1, 5, 0xabcd);
    for (int32_t x = 0; x < 3; ++x) {
        CHECK(Pixel(renderer, x, 1) == 0x1234abcdU);
    }
    CHECK(Pixel(renderer, 3, 1) == 0x12345678U);
    renderer.GetRaster().FillColor16(0, 0, 0, 0);
    renderer.GetRaster().FillColor16(0, -1, 4, 0);
    CHECK(Pixel(renderer, 0, 0) == 0x12345678U);

    renderer.GetRaster().FillColor32(6, 2, 4, 0x89abcdefU);
    CHECK(Pixel(renderer, 5, 2) == 0x12345678U);
    CHECK(Pixel(renderer, 6, 2) == 0x89abcdefU);
    CHECK(Pixel(renderer, 7, 2) == 0x89abcdefU);
    renderer.GetRaster().FillColor32(0, 8, 4, 0);
    CHECK(Pixel(renderer, 0, 0) == 0x12345678U);
}

void TestSpriteSubmission()
{
    auto texture = MakeSpriteTexture();
    soft3d::Renderer renderer(8, 8);
    renderer.SetTexture(texture.get());
    renderer.Clear(0x4444);
    renderer.RenderSprite({0, 0, 0x30000}, nullptr, 0x0f00);
    CHECK(Pixel(renderer, 2, 2) == 0x5555001fU);
    CHECK(Pixel(renderer, 3, 2) == 0x5555f800U);
    CHECK(Pixel(renderer, 2, 3) == 0x5555f800U);
    CHECK(Pixel(renderer, 3, 3) == 0x5555001fU);
    CHECK(Pixel(renderer, 1, 2) == 0x4444 && Pixel(renderer, 4, 4) == 0x4444);

    renderer.Clear(0x4444);
    const soft3d::ScreenRect source = {1, 0, 2, 2};
    renderer.RenderSprite({0, 0, 0x30000}, &source, 0);
    CHECK(Pixel(renderer, 3, 2) == 0x55552104U);
    CHECK(Pixel(renderer, 3, 3) == 0x555507e0U);
    CHECK(Pixel(renderer, 2, 2) == 0x4444 && Pixel(renderer, 4, 3) == 0x4444);

    renderer.Clear(0x4444);
    renderer.RenderSprite({-0x2a000, 0, 0x30000}, nullptr, 0x0f00);
    CHECK(Pixel(renderer, 0, 2) == 0x5555f800U);
    CHECK(Pixel(renderer, 0, 3) == 0x5555001fU);
    CHECK(Pixel(renderer, 1, 2) == 0x4444 && Pixel(renderer, 1, 3) == 0x4444);

    for (soft3d::Fixed depth : {soft3d::fixedOne, 0x8c0000}) {
        renderer.Clear(0x4444);
        renderer.RenderSprite({0, 0, depth}, nullptr, 0x0f00);
        for (int32_t y = 0; y < 8; ++y) {
            for (int32_t x = 0; x < 8; ++x) {
                CHECK(Pixel(renderer, x, y) == 0x4444);
            }
        }
    }
}

void TestDynamicLights()
{
    soft3d::Bsp bsp(nullptr, nullptr);
    CHECK(bsp.GetDynamicLightCount() == 0 && bsp.GetDynamicLights() == nullptr);
    std::array<soft3d::BspDynamicLight*, 32> lights{};
    for (uint32_t index = 0; index < lights.size(); ++index) {
        lights[index] = bsp.AddDynamicLight();
        CHECK(lights[index] != nullptr);
        CHECK(lights[index]->plane.normal.x == 0 && lights[index]->plane.normal.y == 0 &&
              lights[index]->plane.normal.z == 0 && lights[index]->plane.distance == 0);
        CHECK(lights[index]->previous == nullptr);
        CHECK(bsp.GetDynamicLights() == lights[index]);
        CHECK(bsp.GetDynamicLightCount() == index + 1);
        lights[index]->plane.distance = static_cast<soft3d::Fixed>(index + 1);
        if (index != 0) {
            CHECK(lights[index]->next == lights[index - 1]);
            CHECK(lights[index - 1]->previous == lights[index]);
        } else {
            CHECK(lights[index]->next == nullptr);
        }
    }
    CHECK(bsp.AddDynamicLight() == nullptr);
    CHECK(bsp.GetDynamicLightCount() == 32);
    uint32_t expected = 32;
    for (const soft3d::BspDynamicLight* light = bsp.GetDynamicLights(); light != nullptr;
         light = light->next) {
        CHECK(light->plane.distance == static_cast<soft3d::Fixed>(expected--));
    }
    CHECK(expected == 0);

    bsp.ClearDynamicLights();
    bsp.ClearDynamicLights();
    CHECK(bsp.GetDynamicLightCount() == 0 && bsp.GetDynamicLights() == nullptr);
    soft3d::BspDynamicLight* reused = bsp.AddDynamicLight();
    CHECK(reused != nullptr && reused->next == nullptr && reused->previous == nullptr);
    bsp.FreeLastLevel();
    CHECK(bsp.GetDynamicLightCount() == 0 && bsp.GetDynamicLights() == nullptr);
}

void TestDynamicLightIntensity()
{
    std::vector<uint8_t> map(soft3d::sbpHeaderBytes + 1, 0);
    std::copy_n("S3D", 4, map.begin());
    const float version = soft3d::sbpVersion;
    std::memcpy(map.data() + 4, &version, sizeof(version));
    Write32(map, 0x20 + soft3d::SbpLightmapData * 8, soft3d::sbpHeaderBytes);
    Write32(map, 0x24 + soft3d::SbpLightmapData * 8, 1);
    map.back() = 0xff;

    std::vector<uint8_t> archive;
    archive.push_back(1);
    archive.push_back(0);
    std::array<char, soft3d::pakPathBytes> name{};
    const std::string entry = ".\\common\\lights.sbp";
    std::copy(entry.begin(), entry.end(), name.begin());
    archive.insert(archive.end(), name.begin(), name.end());
    Append32(archive, 2 + soft3d::pakEntryBytes);
    archive.insert(archive.end(), map.begin(), map.end());
    const TempFile mapFile("soft3d-sdk-dynamic-lights.spk", archive);
    soft3d::s3dResManager manager;
    CHECK(manager.GetFilePak()->Open(mapFile.path.u16string().c_str(), 0) == soft3d::S3D_OK);
    soft3d::Renderer renderer(8, 8);
    soft3d::Bsp bsp(&renderer, &manager);
    const soft3d::result loadStatus =
        soft3d::LoadSbpMapChunks(&bsp, u".\\common\\", u"lights.sbp", nullptr);
    if (loadStatus != soft3d::S3D_OK) {
        std::cerr << "lightmap fixture load status " << loadStatus << '\n';
    }
    CHECK(loadStatus == soft3d::S3D_OK);
    CHECK(bsp.GetLightIntensity({0, 0, 0}) == 0);
    soft3d::BspDynamicLight* light = bsp.AddDynamicLight();
    CHECK(light != nullptr);
    light->plane.distance = 0x20000;
    const soft3d::Fixed dynamicIntensity = bsp.GetLightIntensity({0, 0, 0});
    if (dynamicIntensity != 0x200) {
        std::cerr << std::hex << "dynamic intensity " << dynamicIntensity << std::dec << '\n';
    }
    CHECK(dynamicIntensity == 0x200);
    bsp.FreeLastLevel();
    CHECK(bsp.GetLightIntensity({0, 0, 0}) == soft3d::fixedOne);
}

} // namespace

int main()
{
    try {
        TestTextureTargetSynthetic();
        TestTextureTargetRealResource();
        TestSpanFills();
        TestSpriteSubmission();
        TestDynamicLights();
        TestDynamicLightIntensity();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "stage-32 resource and renderer API tests passed\n";
    return 0;
}

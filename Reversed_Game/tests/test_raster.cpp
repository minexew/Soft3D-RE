#include "soft3d/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

using soft3d::Fixed;
using soft3d::RasterVertex;
using soft3d::Renderer;
using soft3d::Texture;

RasterVertex Vertex(int32_t x, int32_t y, Fixed depth = 0x1000)
{
    RasterVertex result;
    result.x = x << 16;
    result.y = y << 16;
    result.inverseDepth = depth;
    return result;
}

void Triangle(Renderer& renderer, uint16_t color = 0xf800, Fixed depth = 0x1000)
{
    renderer.GetRaster().SetFlatColor(color);
    renderer.GetRaster().RasterizeTriangle(Vertex(1, 1, depth), Vertex(1, 4, depth),
                                           Vertex(4, 4, depth));
}

uint32_t Pixel(Renderer& renderer, int32_t x, int32_t y)
{
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    return pixels[y * 6 + x];
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    const std::size_t offset = bytes.size();
    bytes.resize(offset + 4);
    std::memcpy(bytes.data() + offset, &value, 4);
}

std::unique_ptr<Texture> MakeTexture(bool colorKey = false, bool detail = false)
{
    auto texture = std::make_unique<Texture>();
    std::vector<uint8_t> bytes;
    const uint32_t type = detail ? 0x10 : 0x40;
    const int16_t transparent = colorKey ? 0 : -1;
    Append32(bytes, (detail ? 0U : 2U) | (type << 8) |
                    (static_cast<uint32_t>(static_cast<uint16_t>(transparent)) << 16));
    Append32(bytes, 2);
    Append32(bytes, 2);
    const std::array<uint8_t, 4> pixels = detail
        ? std::array<uint8_t, 4>{0, 31, 2, 16}
        : std::array<uint8_t, 4>{0, 1, 1, 0};
    if (!detail) {
        const std::size_t palette = bytes.size();
        bytes.resize(palette + 128, 0);
        bytes.insert(bytes.end(), pixels.begin(), pixels.end());
        auto setColor = [&](uint8_t index, uint8_t intensity, uint16_t color) {
            std::memcpy(bytes.data() + palette + (index * 32 + intensity) * 2, &color, 2);
        };
        setColor(0, 0, 0x07e0);
        setColor(0, 15, 0x001f);
        setColor(0, 31, 0xffff);
        setColor(1, 0, 0x2104);
        setColor(1, 15, 0xf800);
        setColor(1, 31, 0xffe0);
    } else {
        bytes.insert(bytes.end(), pixels.begin(), pixels.end());
    }
    CHECK(texture->ParseStx(bytes.data()) == soft3d::S3D_OK);
    return texture;
}

void TestCoverageAndDepth()
{
    Renderer renderer(6, 6);
    renderer.Clear(0x1111);
    Triangle(renderer);
    CHECK(Pixel(renderer, 1, 2) == 0x0010f800);
    CHECK(Pixel(renderer, 1, 3) == 0x0010f800);
    CHECK(Pixel(renderer, 2, 3) == 0x0010f800);
    CHECK(Pixel(renderer, 1, 1) == 0x1111);
    CHECK(Pixel(renderer, 3, 3) == 0x1111);

    renderer.GetRaster().SetDepthWriteEnabled(false);
    Triangle(renderer, 0x07e0, 0x2000);
    CHECK(Pixel(renderer, 1, 2) == 0x001007e0);
    Triangle(renderer, 0x001f, 0x0800);
    CHECK(Pixel(renderer, 1, 2) == 0x001007e0);
    renderer.GetRaster().SetDepthTestEnabled(false);
    Triangle(renderer, 0x001f, 0x0800);
    CHECK(Pixel(renderer, 1, 2) == 0x0010001f);
    renderer.GetRaster().SetDepthWriteEnabled(true);
    Triangle(renderer, 0xffff, 0x0800);
    CHECK(Pixel(renderer, 1, 2) == 0x0008ffff);
}

void TestFlatBlends()
{
    constexpr std::array<uint16_t, 4> expected = {0xf800, 0xedd6, 0xf3ef, 0xe9e7};
    for (uint32_t mode = 0; mode < 4; ++mode) {
        Renderer renderer(6, 6);
        renderer.Clear(0);
        uint32_t* pixels = reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels());
        for (uint32_t index = 0; index < 36; ++index) {
            pixels[index] = 0x00300000U | 0xffffU;
        }
        renderer.GetRaster().SetDepthTestEnabled(false);
        renderer.GetRaster().SetDepthWriteEnabled(false);
        renderer.GetRaster().SetBlendMode(mode);
        Triangle(renderer, 0xf800);
        CHECK(Pixel(renderer, 1, 2) == (0x00300000U | expected[mode]));
    }
}

void TestTextureKeyLightingAndDetail()
{
    auto texture = MakeTexture();
    auto detail = MakeTexture(false, true);
    Renderer renderer(6, 6);
    renderer.Clear(0x4444);
    renderer.GetRaster().SetTexture(texture.get());
    Triangle(renderer);
    CHECK(Pixel(renderer, 1, 2) == 0x0010001f);

    renderer.Clear(0x4444);
    RasterVertex first = Vertex(1, 1);
    RasterVertex second = Vertex(1, 4);
    RasterVertex third = Vertex(4, 4);
    first.u = second.u = third.u = 0x10000;
    renderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(renderer, 1, 2) == 0x0010f800);
    first.u = second.u = third.u = -0x30000;
    renderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(renderer, 1, 2) == 0x0010f800);

    auto keyed = MakeTexture(true);
    Renderer keyRenderer(6, 6);
    keyRenderer.Clear(0x4444);
    keyRenderer.GetRaster().SetTexture(keyed.get());
    Triangle(keyRenderer, 0, 0x1000);
    CHECK(Pixel(keyRenderer, 1, 2) == 0x4444);
    first.u = second.u = third.u = 0x10000;
    keyRenderer.GetRaster().SetDepthWriteEnabled(false);
    keyRenderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(keyRenderer, 1, 2) == 0x0000f800);

    Renderer lightRenderer(6, 6);
    lightRenderer.Clear(0);
    lightRenderer.GetRaster().SetTexture(texture.get());
    lightRenderer.GetRaster().SetShadingEnabled(true);
    first.intensity = second.intensity = third.intensity = 0;
    first.u = second.u = third.u = 0;
    lightRenderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(lightRenderer, 1, 2) == 0x001007e0);
    lightRenderer.GetRaster().SetDepthTestEnabled(false);
    first.intensity = second.intensity = third.intensity = 0x1f000000;
    lightRenderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(lightRenderer, 1, 2) == 0x0010ffff);

    Renderer detailRenderer(6, 6);
    detailRenderer.Clear(0x4444);
    detailRenderer.GetRaster().SetTexture(texture.get());
    detailRenderer.GetRaster().SetDetailTexture(detail.get());
    detailRenderer.GetRaster().SetDepthWriteEnabled(false);
    Triangle(detailRenderer);
    CHECK(Pixel(detailRenderer, 1, 2) == 0x4444);
    first.u = second.u = third.u = 0x10000;
    detailRenderer.GetRaster().RasterizeTriangle(first, second, third);
    CHECK(Pixel(detailRenderer, 1, 2) == 0x0000f800);
}

void TestSpecializedDepthComparison()
{
    auto texture = MakeTexture();
    Renderer renderer(6, 6);
    renderer.Clear(0);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels());
    for (uint32_t index = 0; index < 36; ++index) {
        pixels[index] = 0x80004444U;
    }
    renderer.GetRaster().SetTexture(texture.get());
    Triangle(renderer);
    CHECK(Pixel(renderer, 1, 2) == 0x0010001f);
}

void FillSurface(soft3d::SojSurface& surface, uint32_t format, uint32_t primitiveType = 4)
{
    surface.primitiveType = primitiveType;
    surface.primitiveCount = 1;
    surface.vertices.format = format;
    surface.vertices.vertexCount = 3;
    surface.vertices.strideBytes = format == 1 ? 12 : format == 5 ? 20 : 32;
    surface.vertices.dataByteSize = surface.vertices.strideBytes * 3;
    surface.vertices.data.resize(surface.vertices.dataByteSize);
    const std::array<soft3d::Vec3, 3> positions = {{
        {-0x18000, -0x18000, 0x30000},
        {0x18000, -0x18000, 0x30000},
        {-0x18000, 0x18000, 0x30000},
    }};
    for (uint32_t index = 0; index < 3; ++index) {
        uint8_t* output = surface.vertices.data.data() + index * surface.vertices.strideBytes;
        std::memcpy(output, &positions[index], 12);
        if ((format & 2) != 0) {
            const soft3d::Vec3 normal = {0, -soft3d::fixedOne, 0};
            std::memcpy(output + 12, &normal, 12);
        }
        if ((format & 4) != 0) {
            std::memset(output + surface.vertices.strideBytes - 8, 0, 8);
        }
    }
    const std::array<uint16_t, 3> indices = {0, 1, 2};
    surface.indices.data.resize(6);
    std::memcpy(surface.indices.data.data(), indices.data(), 6);
}

void TestRendererStatesAndEntryPoints()
{
    auto texture = MakeTexture();
    for (uint32_t mode = 0; mode < 2; ++mode) {
        Renderer renderer(8, 8);
        renderer.Clear(0);
        soft3d::SojSurface surface;
        FillSurface(surface, 5);
        renderer.SetTexture(texture.get());
        renderer.SetDepthTestEnabled(false);
        renderer.SetTextureOffset(mode == 0 ? 0x10000 : 0, 0);
        renderer.DrawSequential(surface.vertices, 4, 0, 1);
        const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
            renderer.GetCurrentTexture()->GetPixels());
        bool found = false;
        for (uint32_t index = 0; index < 64; ++index) {
            if ((pixels[index] & 0xffff) == (mode == 0 ? 0xf800 : 0x001f)) {
                found = true;
            }
        }
        CHECK(found);
    }

    Renderer environment(8, 8);
    environment.Clear(0);
    soft3d::SojSurface normalSurface;
    FillSurface(normalSurface, 7);
    environment.SetTexture(texture.get());
    environment.SetTextureCoordinateMode(3);
    environment.SetLightingEnabled(false);
    environment.SetDepthTestEnabled(false);
    environment.DrawIndexed(normalSurface.vertices, normalSurface.indices, 4, 0, 1);
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        environment.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 64, [](uint32_t pixel) {
        return (pixel & 0xffff) == 0xf800;
    }));

    Renderer streams(8, 8);
    streams.Clear(0);
    soft3d::SojSurface primary;
    FillSurface(primary, 1);
    soft3d::SojVertices secondary;
    secondary.format = 4;
    secondary.strideBytes = 8;
    secondary.vertexCount = 3;
    secondary.dataByteSize = 24;
    secondary.data.resize(24);
    for (uint32_t index = 0; index < 3; ++index) {
        const Fixed uv[2] = {0x10000, 0};
        std::memcpy(secondary.data.data() + index * 8, uv, 8);
    }
    streams.SetTexture(texture.get());
    streams.SetDepthTestEnabled(false);
    streams.DrawIndexedTextured(primary.vertices, secondary, primary.indices, 4, 0, 1);
    pixels = reinterpret_cast<const uint32_t*>(streams.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 64, [](uint32_t pixel) {
        return (pixel & 0xffff) == 0xf800;
    }));

    for (uint32_t primitiveType = 4; primitiveType <= 6; ++primitiveType) {
        Renderer primitiveRenderer(8, 8);
        primitiveRenderer.Clear(0);
        soft3d::SojSurface surface;
        FillSurface(surface, 1, primitiveType);
        primitiveRenderer.SetFlatColor(0xf800);
        primitiveRenderer.SetDepthTestEnabled(false);
        primitiveRenderer.DrawIndexed(surface.vertices, surface.indices, primitiveType, 0, 1);
        pixels = reinterpret_cast<const uint32_t*>(
            primitiveRenderer.GetCurrentTexture()->GetPixels());
        CHECK(std::any_of(pixels, pixels + 64, [](uint32_t pixel) {
            return (pixel & 0xffff) == 0xf800;
        }));
    }
}

void TestClippingAndCulling()
{
    for (uint32_t cullMode = 0; cullMode < 3; ++cullMode) {
        Renderer renderer(8, 8);
        renderer.Clear(0);
        soft3d::SojSurface surface;
        FillSurface(surface, 1);
        soft3d::SojObject material{};
        material.color = 0xf800;
        material.cullMode = cullMode;
        renderer.DrawIndexedSurface(surface, &material, nullptr, nullptr);
        const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
            renderer.GetCurrentTexture()->GetPixels());
        const bool found = std::any_of(pixels, pixels + 64, [](uint32_t pixel) {
            return (pixel & 0xffff) == 0xf800;
        });
        CHECK(found == (cullMode != 2));
    }

    Renderer nearRenderer(8, 8);
    nearRenderer.Clear(0);
    soft3d::SojSurface surface;
    FillSurface(surface, 1);
    const std::array<soft3d::Vec3, 3> positions = {{
        {-0x8000, -0x8000, 0},
        {0x8000, -0x8000, 0x20000},
        {-0x8000, 0x8000, 0x20000},
    }};
    for (uint32_t index = 0; index < 3; ++index) {
        std::memcpy(surface.vertices.data.data() + index * 12, &positions[index], 12);
    }
    nearRenderer.SetFlatColor(0xf800);
    nearRenderer.DrawIndexed(surface.vertices, surface.indices, 4, 0, 1);
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        nearRenderer.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 64, [](uint32_t pixel) {
        return (pixel & 0xffff) == 0xf800;
    }));

    Renderer farRenderer(8, 8);
    farRenderer.Clear(0x4444);
    FillSurface(surface, 1);
    for (uint32_t index = 0; index < 3; ++index) {
        const soft3d::Vec3 position = {0, 0, 0x900000};
        std::memcpy(surface.vertices.data.data() + index * 12, &position, 12);
    }
    farRenderer.DrawIndexed(surface.vertices, surface.indices, 4, 0, 1);
    pixels = reinterpret_cast<const uint32_t*>(farRenderer.GetCurrentTexture()->GetPixels());
    CHECK(std::all_of(pixels, pixels + 64, [](uint32_t pixel) { return pixel == 0x4444; }));
}

void TestVerticalScreenClipping()
{
    const auto render = [](const std::array<soft3d::Vec3, 3>& positions) {
        Renderer renderer(8, 8);
        renderer.Clear(0x1111);
        renderer.SetCullMode(0);
        renderer.SetDepthTestEnabled(false);
        renderer.SetDepthWriteEnabled(false);
        renderer.SetFlatColor(0xf800);
        soft3d::SojSurface surface;
        FillSurface(surface, 1);
        for (uint32_t index = 0; index < 3; ++index) {
            std::memcpy(surface.vertices.data.data() + index * 12, &positions[index], 12);
        }
        renderer.DrawIndexed(surface.vertices, surface.indices, 4, 0, 1);
        const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
            renderer.GetCurrentTexture()->GetPixels());
        std::array<uint16_t, 64> colors{};
        for (std::size_t index = 0; index < colors.size(); ++index) {
            colors[index] = static_cast<uint16_t>(pixels[index]);
        }
        return colors;
    };
    const auto check = [&render](const std::array<soft3d::Vec3, 3>& positions,
                                 const char* expected) {
        const auto colors = render(positions);
        for (std::size_t index = 0; index < colors.size(); ++index) {
            CHECK((colors[index] == 0xf800) == (expected[index] == '#'));
        }
    };
    check({{{0, 0x30000, 0x30000}, {-0x10000, -0x10000, 0x30000},
            {0x10000, -0x10000, 0x30000}}},
          "........"
          "........"
          "....#..."
          "....#..."
          "....##.."
          "....##.."
          "........"
          "........");
    check({{{0, -0x30000, 0x30000}, {0x10000, 0x10000, 0x30000},
            {-0x10000, 0x10000, 0x30000}}},
          "........"
          "........"
          "........"
          "...###.."
          "....##.."
          "....##.."
          "....#..."
          "....#...");
}

void TestSelectedPathMatrix()
{
    auto texture = MakeTexture();
    auto keyed = MakeTexture(true);
    auto detail = MakeTexture(false, true);
    for (bool depthTest : {false, true}) {
        for (bool depthWrite : {false, true}) {
            for (bool textured : {false, true}) {
                for (bool colorKey : {false, true}) {
                    for (bool shading : {false, true}) {
                        for (bool useDetail : {false, true}) {
                            if (!textured && (colorKey || shading || useDetail)) continue;
                            if (!colorKey && !shading && useDetail) continue;
                            for (uint32_t blend = 0; blend < 4; ++blend) {
                                if (useDetail && blend != 0) continue;
                                Renderer renderer(6, 6);
                                renderer.Clear(0x4444);
                                renderer.GetRaster().SetDepthTestEnabled(depthTest);
                                renderer.GetRaster().SetDepthWriteEnabled(depthWrite);
                                renderer.GetRaster().SetTexture(textured
                                    ? (colorKey ? keyed.get() : texture.get()) : nullptr);
                                renderer.GetRaster().SetDetailTexture(useDetail ? detail.get() : nullptr);
                                renderer.GetRaster().SetShadingEnabled(shading);
                                renderer.GetRaster().SetBlendMode(blend);
                                Triangle(renderer, 0xf800, 0x1000);
                                const bool unchanged = colorKey || useDetail;
                                const uint32_t pixel = Pixel(renderer, 1, 2);
                                if ((((pixel & 0xffff) == 0x4444) != unchanged)) {
                                    std::cerr << "path mismatch: depthTest=" << depthTest
                                        << " depthWrite=" << depthWrite << " textured=" << textured
                                        << " key=" << colorKey << " shading=" << shading
                                        << " detail=" << useDetail << " blend=" << blend
                                        << std::hex << " pixel=" << pixel << "\n";
                                }
                                CHECK(((pixel & 0xffff) == 0x4444) == unchanged);
                            }
                        }
                    }
                }
            }
        }
    }
}

void TestGlobalLightDirectionSign()
{
    Renderer renderer(6, 6);
    soft3d::SojVertices vertices;
    vertices.format = 3;
    vertices.strideBytes = 24;
    vertices.vertexCount = 3;
    vertices.dataByteSize = 72;
    const soft3d::Vec3 positions[] = {{0, 0, 0x30000}, {0x10000, 0, 0x30000},
                                      {0, 0x10000, 0x30000}};
    for (const soft3d::Vec3& position : positions) {
        Append32(vertices.data, static_cast<uint32_t>(position.x));
        Append32(vertices.data, static_cast<uint32_t>(position.y));
        Append32(vertices.data, static_cast<uint32_t>(position.z));
        Append32(vertices.data, 0);
        Append32(vertices.data, 0);
        Append32(vertices.data, soft3d::fixedOne);
    }
    soft3d::SojIndices indices;
    indices.data = {0, 0, 1, 0, 2, 0};
    renderer.SetGlobalLightDirection({0, 0, soft3d::fixedOne});
    renderer.DrawIndexed(vertices, indices, 4, 0, 1);
    CHECK(renderer.GetLightDirection().z == -soft3d::fixedOne);
    renderer.SetGlobalLightDirection({soft3d::fixedOne, 0, 0});
    renderer.DrawIndexed(vertices, indices, 4, 0, 1);
    CHECK(renderer.GetLightDirection().x == -soft3d::fixedOne);
    CHECK(renderer.GetLightDirection().z == 0);
}

void TestOffscreenSpanClipping()
{
    Renderer renderer(6, 6);
    renderer.Clear(0x1111);
    renderer.GetRaster().SetFlatColor(0xf800);
    renderer.GetRaster().RasterizeTriangle(Vertex(-4, 1), Vertex(10, 1), Vertex(3, 8));
    CHECK(Pixel(renderer, 0, 0) == 0x1111);
    CHECK(Pixel(renderer, 1, 1) == 0x0010f800);
}

void TestEdgeOrderAfterSorting()
{
    Renderer renderer(6, 6);
    renderer.Clear(0x1111);
    renderer.GetRaster().SetFlatColor(0xf800);
    renderer.GetRaster().RasterizeTriangle(Vertex(2, 5), Vertex(3, 0), Vertex(1, 5));
    CHECK(Pixel(renderer, 2, 3) == 0x0010f800);
    CHECK(Pixel(renderer, 3, 3) == 0x1111);
}

} // namespace

int main()
{
    try {
        TestCoverageAndDepth();
        TestOffscreenSpanClipping();
        TestEdgeOrderAfterSorting();
        TestFlatBlends();
        TestTextureKeyLightingAndDetail();
        TestSpecializedDepthComparison();
        TestRendererStatesAndEntryPoints();
        TestClippingAndCulling();
        TestVerticalScreenClipping();
        TestSelectedPathMatrix();
        TestGlobalLightDirectionSign();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "raster and renderer tests passed\n";
    return 0;
}

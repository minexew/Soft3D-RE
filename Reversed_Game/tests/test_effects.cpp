#include "soft3d/Effects.hpp"
#include "soft3d/Camera.hpp"
#include "soft3d/Object.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
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

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    const std::size_t offset = bytes.size();
    bytes.resize(offset + 4);
    std::memcpy(bytes.data() + offset, &value, 4);
}

std::unique_ptr<soft3d::Texture> MakeWhiteTexture()
{
    auto texture = std::make_unique<soft3d::Texture>();
    std::vector<uint8_t> bytes;
    Append32(bytes, 2U | (0x40U << 8) | (0xffffU << 16));
    Append32(bytes, 2);
    Append32(bytes, 2);
    for (uint32_t index = 0; index < 64; ++index) {
        const uint16_t color = 0xffff;
        const std::size_t offset = bytes.size();
        bytes.resize(offset + 2);
        std::memcpy(bytes.data() + offset, &color, 2);
    }
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    CHECK(texture->ParseStx(bytes.data()) == soft3d::S3D_OK);
    return texture;
}

std::unique_ptr<soft3d::Texture> MakeAlphaTexture()
{
    auto texture = std::make_unique<soft3d::Texture>();
    std::vector<uint8_t> bytes;
    Append32(bytes, (0x10U << 8) | (0xffffU << 16));
    Append32(bytes, 2);
    Append32(bytes, 2);
    bytes.insert(bytes.end(), {31, 31, 31, 31});
    CHECK(texture->ParseStx(bytes.data()) == soft3d::S3D_OK);
    return texture;
}

uint32_t Pixel(soft3d::Renderer& renderer, int32_t x, int32_t y)
{
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    return pixels[y * renderer.GetWidth() + x];
}

void TestFixedMath()
{
    CHECK(soft3d::FixedSin(0) == 0);
    CHECK(soft3d::FixedSin(0x400) == 46340);
#ifdef SOFT3D_TRIG_USE_MATH
    CHECK(soft3d::FixedSin(0x800) == 65536);
    CHECK(soft3d::FixedCos(0) == 65536);
#else
    CHECK(soft3d::FixedSin(0x800) == 65535);
    CHECK(soft3d::FixedCos(0) == 65535);
    CHECK(soft3d::FixedTan(0x7ff) == 85450510);
#endif
    CHECK(soft3d::FixedTan(0) == 0);
    CHECK(soft3d::FixedTan(0x400) == 65536);
    CHECK(soft3d::FixedTan(0x800) == static_cast<soft3d::Fixed>(0x80000001U));
    CHECK(soft3d::FixedTan(0xc00) == -65536);
    CHECK(soft3d::FixedArcTangent(soft3d::fixedOne) == 0x41d);
    const soft3d::Vec3 rotated = soft3d::RotateY({soft3d::fixedOne, 0, 0}, 0x800);
#ifdef SOFT3D_TRIG_USE_MATH
    CHECK(rotated.x == 0 && rotated.y == 0 && rotated.z == -65536);
#else
    CHECK(rotated.x == 0 && rotated.y == 0 && rotated.z == -65535);
#endif
    soft3d::Matrix4 model = soft3d::Matrix4::Identity();
    model.values[0] = 0;
    model.values[1] = -soft3d::fixedOne;
    model.values[4] = soft3d::fixedOne;
    model.values[5] = 0;
    soft3d::Matrix4 view = soft3d::Matrix4::Identity();
    view.values[12] = 5 * soft3d::fixedOne;
    view.values[13] = 6 * soft3d::fixedOne;
    view.values[14] = 7 * soft3d::fixedOne;
    const soft3d::Vec3 point{soft3d::fixedOne, 2 * soft3d::fixedOne, 3 * soft3d::fixedOne};
    const soft3d::Vec3 combined = soft3d::Transform(soft3d::Multiply(model, view), point);
    const soft3d::Vec3 sequential = soft3d::Transform(view, soft3d::Transform(model, point));
    CHECK(combined.x == sequential.x && combined.y == sequential.y && combined.z == sequential.z);

    soft3d::Renderer renderer(8, 8);
    soft3d::Camera camera(8, 8, &renderer);
    camera.SetLookAt({0, 0, 0}, {soft3d::fixedOne, 0, 0}, {0, soft3d::fixedOne, 0});
    CHECK(camera.GetRollMatrix().values[0] == 0);
#ifdef SOFT3D_TRIG_USE_MATH
    CHECK(camera.GetRollMatrix().values[8] == 65536);
#else
    CHECK(camera.GetRollMatrix().values[8] == 65535);
#endif
}

void TestTextureResize()
{
    auto texture = MakeWhiteTexture();
    CHECK(texture->GetPalette() != nullptr);
    CHECK(texture->GetType() == 0x40);
    CHECK(texture->Resize(2, 2, 2) == soft3d::S3D_OK);
    CHECK(texture->GetType() == 0x40);
    CHECK(texture->GetPalette() == nullptr);
    CHECK(texture->GetPixelSize() == 16);
    CHECK(texture->GetPixels() != nullptr);
}

void TestLensflare()
{
    soft3d::Renderer renderer(16, 16);
    soft3d::ScreenBlitter blitter;
    soft3d::Lensflare flare(&renderer, &blitter);
    auto texture = MakeWhiteTexture();
    flare.SetSource({0, 0, 0x40000});
    flare.SetElement(0, texture.get(), 0);
    flare.SetElement(5, texture.get(), 0);
    CHECK(flare.IsElementEnabled(0));
    CHECK(!flare.IsElementEnabled(5));

    renderer.Clear(0x1111);
    flare.Render();
    CHECK(Pixel(renderer, 6, 6) == 0x7fffffff);
    CHECK(Pixel(renderer, 7, 7) == 0x7fffffff);
    CHECK(Pixel(renderer, 5, 5) == 0x1111);

    renderer.Clear(0x1111);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels());
    pixels[7 * 16 + 7] = 0xffff1111;
    flare.Render();
    CHECK(Pixel(renderer, 7, 7) == 0xffff1111);
    flare.ClearElements();
    flare.Render();
    CHECK(Pixel(renderer, 8, 8) == 0x1111);
}

void TestBillboardGroup()
{
    soft3d::Renderer renderer(16, 16);
    soft3d::Camera camera(16, 16, &renderer);
    soft3d::BillboardGroup group(&renderer, &camera, 1);
    auto texture = MakeWhiteTexture();
    group.SetTexture(texture.get());
    CHECK(group.AddBillboard({soft3d::fixedOne, soft3d::fixedOne, {0, 0, 0x40000}}));
    CHECK(!group.AddBillboard({soft3d::fixedOne, soft3d::fixedOne, {0, 0, 0x40000}}));
    renderer.Clear(0x1111);
    group.Render();
    CHECK(Pixel(renderer, 8, 7) == 0x54b8ffff);
    CHECK(Pixel(renderer, 0, 0) == 0x1111);

    group.ClearBillboards();
    CHECK(group.GetBillboardCount() == 0);
    CHECK(group.AddBillboard({soft3d::fixedOne, soft3d::fixedOne, {0, 0, -0x40000}}));
    renderer.Clear(0x1111);
    group.Render();
    CHECK(Pixel(renderer, 8, 8) == 0x1111);
}

void TestFakeShadow()
{
    soft3d::Renderer renderer(16, 16);
    soft3d::FakeShadow shadow(&renderer);
    shadow.Initialize(soft3d::fixedOne);
    CHECK(shadow.GetVertices().vertexCount == 12);
    CHECK(shadow.GetVertices().dataByteSize == 144);
    shadow.ScaleXZ(0x8000);
    soft3d::Matrix4 transform = soft3d::Matrix4::Identity();
    transform.values[6] = -soft3d::fixedOne;
    transform.values[9] = soft3d::fixedOne;
    transform.values[14] = 0x40000;
    shadow.SetTransform(transform);
    renderer.Clear(0xffff);
    shadow.Render();
    CHECK(Pixel(renderer, 8, 8) == 0x00000000);
    CHECK(Pixel(renderer, 0, 0) == 0xffff);
    const uint32_t before = Pixel(renderer, 8, 8);
    shadow.SetColor(255, 0, 0);
    shadow.Render();
    CHECK(Pixel(renderer, 8, 8) == 0x0000f800);
    CHECK(Pixel(renderer, 8, 8) != before);
}

void TestGunFire()
{
    soft3d::Renderer renderer(16, 16);
    soft3d::Camera camera(16, 16, &renderer);
    soft3d::GunFire fire(&renderer, &camera);
    auto texture = MakeWhiteTexture();
    auto alpha = MakeAlphaTexture();
    fire.SetTextures(texture.get(), alpha.get());
    fire.Trigger({0, 0, 0x40000}, 2);
    CHECK(fire.GetVertices().vertexCount == 4);
    renderer.Clear(0x1111);
    fire.Render();
    CHECK(fire.GetFramesRemaining() == 1);
    CHECK(Pixel(renderer, 8, 7) == 0x54b8ffff);
    fire.Render();
    CHECK(fire.GetFramesRemaining() == 0);
    renderer.Clear(0x1111);
    fire.Render();
    CHECK(Pixel(renderer, 8, 8) == 0x1111);
}

soft3d::Animation MakeAnimation()
{
    soft3d::Animation animation;
    std::vector<uint8_t> bytes(16 + 4 + 36, 0);
    const uint32_t start = 0;
    const uint32_t end = 100;
    const uint32_t tracks = 1;
    const uint32_t keys = 1;
    std::memcpy(bytes.data(), &start, 4);
    std::memcpy(bytes.data() + 4, &end, 4);
    std::memcpy(bytes.data() + 12, &tracks, 2);
    std::memcpy(bytes.data() + 16, &keys, 4);
    for (unsigned index = 0; index < 9; ++index) {
        const uint16_t magnitude = index == 0 || index == 4 || index == 8 ? 0xffff : 0;
        std::memcpy(bytes.data() + 26 + index * 2, &magnitude, 2);
    }
    CHECK(animation.ParseSai(bytes.data()) == soft3d::S3D_OK);
    return animation;
}

void TestObjectApi()
{
    soft3d::ObjectRes model;
    soft3d::Animation animation = MakeAnimation();
    soft3d::Renderer renderer(16, 16);
    soft3d::Object object(&renderer);
    object.SetModel(&model);
    object.SetAnimation(&animation);
    CHECK(object.GetAnimationTime() == 0);
    CHECK(object.GetAnimationSpeed() == 300);
    CHECK(object.GetMatrixPalette().size() == 1);
    object.SetAnimationPaused(true);
    CHECK(!object.AdvanceAnimation());
    CHECK(object.GetAnimationTime() == 0);
    object.SetAnimationPaused(false);
    CHECK(object.AdvanceAnimation());
    CHECK(object.GetAnimationTime() == 200);
    object.SetAnimationPlayback(-1, false);
    CHECK(object.GetAnimationTime() == 100);
    CHECK(object.AdvanceAnimation());
    CHECK(object.GetAnimationTime() == 0);

    soft3d::Matrix4 view = soft3d::Matrix4::Identity();
    view.values[14] = 0x40000;
    renderer.SetViewMatrix(view);
    object.SetAnimationTime(0);
    object.RenderNormal();
    CHECK(object.GetMatrixPalette()[0].values[14] == 0x40000);
    object.SetPosition({0x20000, 0x30000, 0x40000});
    object.SetAxisX({0, soft3d::fixedOne, 0});
    object.SetAxisY({-soft3d::fixedOne, 0, 0});
    object.UpdateTransform();
    CHECK(object.GetModelMatrix().values[1] == soft3d::fixedOne);
    CHECK(object.GetModelMatrix().values[4] == -soft3d::fixedOne);
    const soft3d::Vec3 transformed = soft3d::Transform(
        object.GetModelMatrix(), {soft3d::fixedOne, 0, 0});
    CHECK(transformed.x == 0x20000 && transformed.y == 0x40000 && transformed.z == 0x40000);
    soft3d::Matrix4 accumulated = soft3d::Matrix4::Identity();
    accumulated.values[0] = 0xb6;
    accumulated.values[1] = 0xb6;
    accumulated.values[4] = 0xb6;
    soft3d::Matrix4 coefficients;
    coefficients.values[0] = 0xb6;
    coefficients.values[4] = 0xb6;
    CHECK(soft3d::Multiply(accumulated, coefficients).values[0] == 1);
    CHECK(soft3d::Transform(accumulated, {0xb6, 0xb6, 0}).x == 1);
    object.RenderNormal();
    const soft3d::Matrix4 sampled = animation.SampleTransform(0, 0);
    const soft3d::Matrix4 expected = soft3d::Multiply(sampled,
        soft3d::Multiply(object.GetModelMatrix(), view));
    CHECK(object.GetMatrixPalette()[0].values == expected.values);
    object.SetBoneTransform(0, soft3d::Matrix4::Identity());
    object.SetAnimation(nullptr);
    CHECK(object.GetAnimation() == nullptr);
    CHECK(object.GetMatrixPalette().empty());
    CHECK(object.GetSurface(0) == nullptr);
    CHECK(object.GetVertexCount() == 0);
    CHECK(object.GetTriangleCount() == 0);
    CHECK(!object.HasTransparency());
}

void TestMatrixPaletteIndex()
{
    soft3d::Renderer renderer(16, 16);
    renderer.Clear(0);
    renderer.SetCullMode(0);
    renderer.SetDepthTestEnabled(false);
    renderer.SetFlatColor(0xf800);
    soft3d::SojVertices vertices;
    vertices.format = 0x11;
    vertices.strideBytes = 16;
    vertices.vertexCount = 3;
    vertices.dataByteSize = 48;
    const soft3d::Vec3 positions[] = {{0, 0, 0x30000}, {0x10000, 0, 0x30000},
                                      {0, 0x10000, 0x30000}};
    for (const soft3d::Vec3& position : positions) {
        Append32(vertices.data, 1);
        Append32(vertices.data, static_cast<uint32_t>(position.x));
        Append32(vertices.data, static_cast<uint32_t>(position.y));
        Append32(vertices.data, static_cast<uint32_t>(position.z));
    }
    soft3d::SojIndices indices;
    indices.data = {0, 0, 1, 0, 2, 0};
    soft3d::Matrix4 offscreen = soft3d::Matrix4::Identity();
    offscreen.values[12] = -0x40000;
    const std::vector<soft3d::Matrix4> palette = {offscreen, soft3d::Matrix4::Identity()};
    soft3d::SojSurface surface;
    surface.vertices = vertices;
    surface.indices = indices;
    surface.primitiveType = 4;
    surface.primitiveCount = 1;
    renderer.DrawIndexedSurface(surface, nullptr, nullptr, nullptr, palette);
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 16 * 16,
                      [](uint32_t pixel) { return (pixel & 0xffff) == 0xf800; }));
    soft3d::Matrix4 perspective = soft3d::Matrix4::Identity();
    perspective.values[10] = 0;
    perspective.values[11] = soft3d::fixedOne;
    perspective.values[15] = 0;
    renderer.Clear(0);
    renderer.DrawIndexedSurface(surface, nullptr, nullptr, nullptr, {offscreen, perspective});
    CHECK(std::any_of(pixels, pixels + 16 * 16,
                      [](uint32_t pixel) { return (pixel & 0xffff) == 0xf800; }));
}

void TestRealLamp()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    const std::filesystem::path archive = SOFT3D_TEST_ARCHIVE;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.u16string().c_str()) == soft3d::S3D_OK);
    soft3d::Renderer renderer(64, 64);
    soft3d::Camera camera(64, 64, &renderer);
    soft3d::Object object(&renderer);
    CHECK(object.Load(&manager, u"oszp_diaodeng_a", 0) == soft3d::S3D_OK);
    CHECK(object.LoadMaterials(0) == soft3d::S3D_OK);
    auto* animation = dynamic_cast<soft3d::Animation*>(manager.GetResource(
        soft3d::ResourceAnimation, 0, u"oszp_diaodeng_a"));
    CHECK(animation != nullptr);
    CHECK(animation->GetTrackCount() == 5);
    CHECK(object.GetModel()->GetObjects().size() == 3);
    for (const soft3d::SojObject& material : object.GetModel()->GetObjects()) {
        CHECK(material.cullMode == 2);
    }
    CHECK(object.GetModel()->GetSurfaces().size() == 5);
    CHECK(object.GetSurface(0)->primitiveCount == 12);
    CHECK(object.GetSurface(1)->primitiveCount == 4);
    CHECK(object.GetSurface(2)->primitiveCount == 4);
    CHECK(object.GetSurface(3)->primitiveCount == 2);
    CHECK(object.GetSurface(4)->primitiveCount == 2);
    object.SetAnimation(animation);
    object.SetPosition({0, 0, 0x80000});
    object.UpdateTransform();
    renderer.Clear(0x1111);
    object.Render();
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 64 * 64,
                      [](uint32_t pixel) { return (pixel & 0xffff) != 0x1111; }));
    const std::vector<uint32_t> firstFrame(pixels, pixels + 64 * 64);
    const auto hash = [&pixels] {
        uint32_t value = 2166136261U;
        for (std::size_t index = 0; index < 64 * 64; ++index) {
            value = (value ^ pixels[index]) * 16777619U;
        }
        return value;
    };
    const uint32_t firstHash = hash();
    object.SetAnimationTime(1200);
    renderer.Clear(0x1111);
    object.Render();
    CHECK(!std::equal(firstFrame.begin(), firstFrame.end(), pixels));
    const uint32_t secondHash = hash();
    CHECK(firstHash == 0x85d115c5);
    CHECK(secondHash == 0xf347549f);
#endif
}

void TestRealObject()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    const std::filesystem::path archive = SOFT3D_TEST_ARCHIVE;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.u16string().c_str()) == soft3d::S3D_OK);
    soft3d::Renderer renderer(64, 64);
    soft3d::Camera camera(64, 64, &renderer);
    soft3d::Object object(&renderer);
    CHECK(object.Load(&manager, u"cg_bully_tuomao_a", 0) == soft3d::S3D_OK);
    CHECK(object.LoadMaterials(0) == soft3d::S3D_OK);
    CHECK(object.GetSurface(0) != nullptr);
    CHECK(object.GetVertexCount() != 0);
    CHECK(object.GetTriangleCount() != 0);
    CHECK(object.GetPrimaryBounds().minimum.x == object.GetModel()->GetTrailer()[0]);
    CHECK(object.GetPrimaryBounds().maximum.z == object.GetModel()->GetTrailer()[5]);

    auto* animation = dynamic_cast<soft3d::Animation*>(manager.GetResource(
        soft3d::ResourceAnimation, 0, u"cg_bully_cc_aa"));
    CHECK(animation != nullptr);
    object.SetAnimation(animation);
    object.SetPosition({0, 0, 0x100000});
    object.UpdateTransform();
    renderer.Clear(0x1111);
    object.Render();
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    CHECK(std::any_of(pixels, pixels + 64 * 64,
                      [](uint32_t pixel) { return pixel != 0x1111; }));
    const int32_t time = object.GetAnimationTime();
    object.AdvanceAnimation();
    CHECK(object.GetAnimationTime() != time);
#endif
}

} // namespace

int main()
{
    try {
        TestFixedMath();
        TestTextureResize();
        TestLensflare();
        TestBillboardGroup();
        TestFakeShadow();
        TestGunFire();
        TestObjectApi();
        TestMatrixPaletteIndex();
        TestRealLamp();
        TestRealObject();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "effect and object tests passed\n";
    return 0;
}

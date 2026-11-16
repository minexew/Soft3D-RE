#include "soft3d/Effects.hpp"
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
    CHECK(soft3d::FixedSin(0x800) == 65535);
    CHECK(soft3d::FixedCos(0) == 65535);
    CHECK(soft3d::FixedArcTangent(soft3d::fixedOne) == 0x41d);
    const soft3d::Vec3 rotated = soft3d::RotateY({soft3d::fixedOne, 0, 0}, 0x800);
    CHECK(rotated.x == 0 && rotated.y == 0 && rotated.z == -65535);

    soft3d::Renderer renderer(8, 8);
    soft3d::Camera camera(8, 8, &renderer);
    camera.SetLookAt({0, 0, 0}, {soft3d::fixedOne, 0, 0}, {0, soft3d::fixedOne, 0});
    CHECK(camera.GetRollMatrix().values[0] == 0);
    CHECK(camera.GetRollMatrix().values[8] == 65535);
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
    CHECK(Pixel(renderer, 8, 7) == 0x4000ffff);
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
    CHECK(Pixel(renderer, 8, 7) == 0x4000ffff);
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
    CHECK(object.GetMatrixPalette()[0].values[14] == 0x3fffc);
    object.SetBoneTransform(0, soft3d::Matrix4::Identity());
    object.SetAnimation(nullptr);
    CHECK(object.GetAnimation() == nullptr);
    CHECK(object.GetMatrixPalette().empty());
    CHECK(object.GetSurface(0) == nullptr);
    CHECK(object.GetVertexCount() == 0);
    CHECK(object.GetTriangleCount() == 0);
    CHECK(!object.HasTransparency());
}

void TestRealObject()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    const std::filesystem::path archive = SOFT3D_TEST_ARCHIVE;
    CHECK(soft3d::OpenGameResourceArchive(&manager, archive.u16string().c_str()) == soft3d::S3D_OK);
    soft3d::Renderer renderer(64, 64);
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
    object.SetPosition({0, 0, 0x80000});
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
        TestRealObject();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "effect and object tests passed\n";
    return 0;
}

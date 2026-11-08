#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/Object.hpp"
#include "soft3d/Renderable.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
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

struct TestRenderable final : soft3d::Renderable {
    explicit TestRenderable(int value) { sortKey = value; }
    void Render() override { ++renderCount; }
    int renderCount = 0;
};

void TestRenderQueues()
{
    soft3d::RenderQueues queues;
    TestRenderable first(20);
    TestRenderable second(-10);
    TestRenderable third(5);
    CHECK(queues.Submit(&first, 0));
    CHECK(queues.Submit(&second, 0));
    CHECK(queues.Submit(&third, 0));
    CHECK(queues.Submit(&first, soft3d::RenderQueues::sceneQueueCount));
    CHECK(!queues.Submit(&first, soft3d::RenderQueues::queueCount));
    queues.SortSceneQueues(false);
    queues.RenderSceneQueues();
    queues.RenderOverlayQueue();
    CHECK(second.renderCount == 1 && third.renderCount == 1 && first.renderCount == 2);
    queues.Clear();
    CHECK(queues.GetCount(0) == 0 && queues.GetCount(6) == 0);
    for (std::size_t index = 0; index < soft3d::RenderQueues::queueCapacity; ++index) {
        CHECK(queues.Submit(&first, 1));
    }
    CHECK(!queues.Submit(&first, 1));
}

void TestBasicShader()
{
    soft3d::BasicShader shader(2, 2);
    const std::array<uint32_t, 4> source = {0xf800, 0x07e0, 0x001f, 0xffff};
    std::array<uint32_t, 4> destination{};
    shader.Render(source.data(), destination.data());
    CHECK(destination[0] == 0x0000fc00);
    CHECK(destination[1] == 0x00f8fcf8);
    CHECK(destination[2] == 0x00f80000);
    CHECK(destination[3] == 0x000000f8);
    shader.SetEnabled(0);
    CHECK(shader.enabled == 1);
}

void TestFogShader()
{
    soft3d::FogShader shader(1, 1);
    const std::array<uint32_t, 1> source = {0x0e30ffff};
    uint32_t destination = 0;
    shader.Render(source.data(), &destination);
    CHECK(destination == 0x00d0d0d0);
    const std::array<uint32_t, 1> nearSource = {0x0001ffff};
    shader.Render(nearSource.data(), &destination);
    CHECK(destination == 0);
}

void TestEnginePresentation()
{
    soft3d::EngineConfig config;
    config.width = 2;
    config.height = 2;
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    config.videoBackend = std::move(backend);
    soft3d::GameEngine engine(std::move(config));
    engine.Start();
    CHECK(engine.IsRunning());
    uint32_t* pixels = reinterpret_cast<uint32_t*>(
        engine.GetRenderer().GetCurrentTexture()->GetPixels());
    pixels[0] = 0xf800;
    pixels[1] = 0x07e0;
    pixels[2] = 0x001f;
    pixels[3] = 0xffff;
    CHECK(engine.RunFrame());
    CHECK(dummy->GetUpdateCount() == 1);
    CHECK(dummy->GetPresentedPixels().size() == 4);
    CHECK(dummy->GetPresentedPixels()[0] == 0x0000fc00);
    CHECK(dummy->GetPresentedPixels()[1] == 0x00f8fcf8);
    CHECK(dummy->GetPresentedPixels()[2] == 0x00f80000);
    CHECK(dummy->GetPresentedPixels()[3] == 0x000000f8);
    engine.Suspend();
    CHECK(!engine.IsRunning());
}

void TestRasterAndSoj()
{
    soft3d::Renderer renderer(8, 8);
    renderer.Clear(0);
    soft3d::SojSurface surface;
    surface.primitiveType = 4;
    surface.primitiveCount = 1;
    surface.indexCount = 3;
    surface.vertices.format = 1;
    surface.vertices.strideBytes = 12;
    surface.vertices.vertexCount = 3;
    surface.vertices.dataByteSize = 36;
    surface.vertices.data.resize(36);
    const std::array<soft3d::Vec3, 3> positions = {{
        {-0x8000, -0x8000, 0x20000},
        {0x8000, -0x8000, 0x20000},
        {-0x8000, 0x8000, 0x20000},
    }};
    for (std::size_t index = 0; index < positions.size(); ++index) {
        std::memcpy(surface.vertices.data.data() + index * 12, &positions[index], 12);
    }
    const std::array<uint16_t, 3> indices = {0, 1, 2};
    surface.indices.data.resize(6);
    std::memcpy(surface.indices.data.data(), indices.data(), 6);
    soft3d::SojObject material{};
    material.color = 0xf800;
    renderer.DrawIndexedSurface(surface, &material, nullptr, nullptr);
    const uint16_t* pixels = reinterpret_cast<const uint16_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    const bool found = std::any_of(pixels, pixels + 8 * 8 * 2,
                                   [](uint16_t value) { return value == 0xf800; });
    CHECK(found);
}

void TestAnimation()
{
    soft3d::Animation parsed;
    std::vector<uint8_t> bytes(12 + 4 + 4 + 36 * 2, 0);
    auto append32 = [&bytes](std::size_t offset, uint32_t value) {
        std::memcpy(bytes.data() + offset, &value, 4);
    };
    append32(0, 0);
    append32(4, 100);
    append32(8, 0);
    append32(12, 1);
    append32(16, 2);
    for (unsigned key = 0; key < 2; ++key) {
        const std::size_t base = 20 + key * 36;
        append32(base, key * 100);
        for (unsigned value = 0; value < 9; ++value) {
            const uint16_t magnitude = key == 0 ? 0 : 2;
            std::memcpy(bytes.data() + base + 6 + value * 2, &magnitude, 2);
        }
        append32(base + 24, key * 0x20000);
    }
    CHECK(parsed.ParseSai(bytes.data()) == soft3d::S3D_OK);
    const soft3d::Matrix4 matrix = parsed.SampleTransform(0, 50);
    CHECK(matrix.values[12] == 0xffdc);
    CHECK(matrix.values[0] == 0);
    CHECK(matrix.values[15] == 0x10000);
}

void TestPackedNameAndObjectTiming()
{
    std::array<int32_t, 4> packed{};
    const std::u16string text = u"abc_12";
    unsigned bit = 0;
    for (char16_t character : text) {
        const std::u16string alphabet = u"abcdefghijklmnopqrstuvwxyz_1234";
        const uint32_t code = static_cast<uint32_t>(alphabet.find(character));
        packed[bit / 30] |= code << (bit % 30);
        bit += 5;
    }
    packed[bit / 30] |= 0x1fU << (bit % 30);
    CHECK(soft3d::DecodeSojTextureName(packed) == text);

    soft3d::ObjectRes model;
    soft3d::Animation animation;
    std::array<uint8_t, 16> animationHeader{};
    const uint32_t endTime = 100;
    std::memcpy(animationHeader.data() + 4, &endTime, 4);
    CHECK(animation.ParseSai(animationHeader.data()) == soft3d::S3D_OK);
    soft3d::Renderer renderer(2, 2);
    soft3d::Object object(&renderer);
    object.SetModel(&model);
    object.SetAnimation(&animation);
    object.SetAnimationTime(90);
    CHECK(object.AdvanceAnimation());
    CHECK(object.GetAnimationTime() == 290);
}

} // namespace

int main()
{
    try {
        TestRenderQueues();
        TestBasicShader();
        TestFogShader();
        TestEnginePresentation();
        TestRasterAndSoj();
        TestAnimation();
        TestPackedNameAndObjectTiming();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "engine and rendering tests passed\n";
    return 0;
}

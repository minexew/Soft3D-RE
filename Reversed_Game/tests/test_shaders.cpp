#include "platform/dummy/DummyAudioBackend.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/Engine.hpp"
#include "soft3d/PresentationShaders.hpp"
#include "soft3d/Renderer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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

uint32_t Expand(uint16_t pixel)
{
    return ((pixel & 0xf800U) << 8) | ((pixel & 0x07e0U) << 5) |
           ((pixel & 0x001fU) << 3);
}

uint16_t Pack(uint32_t red, uint32_t green, uint32_t blue)
{
    red &= 0xff;
    green &= 0xff;
    blue &= 0xff;
    return static_cast<uint16_t>(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
}

std::size_t DestinationIndex(int width, int height, int orientation, int x, int y)
{
    return static_cast<std::size_t>(orientation == 1 ? height * (width - 1 - x) + y
                                                    : height - 1 - y + height * x);
}

uint32_t FadePixel(uint16_t pixel, uint16_t target, int alpha)
{
    const uint32_t sourceColor = ((pixel & 0xf800U) << 10) |
        ((pixel & 0x07e0U) << 5) | (pixel & 0x001fU);
    const uint32_t targetColor = ((target & 0xf800U) << 10) |
        ((target & 0x07e0U) << 5) | (target & 0x001fU);
    const uint32_t mixed = (targetColor - sourceColor) * static_cast<uint8_t>(alpha) +
        sourceColor * 32;
    return ((mixed & 0x7c000000U) >> 7) | ((mixed & 0x001f8000U) >> 5) |
           ((mixed & 0x000003e0U) >> 2);
}

class HookEngine final : public soft3d::Engine {
public:
    explicit HookEngine(soft3d::EngineConfig config) : Engine(std::move(config)) {}

    int Init() override { return 0; }
    void Shutdown() override {}
    void LoadGameState() override {}
    void SaveGameState() override {}
    void PreUpdate() override { ++preUpdates; }
    void PostUpdate() override { ++postUpdates; }
    void Update() override { ++updates; }
    void PrepareRender() override { ++prepares; }
    void SubmitRenderables() override { ++submits; }
    void PostRender() override {}

    int preUpdates = 0;
    int postUpdates = 0;
    int updates = 0;
    int prepares = 0;
    int submits = 0;
};

std::unique_ptr<HookEngine> MakeEngine(int width, int height)
{
    soft3d::EngineConfig config;
    config.width = width;
    config.height = height;
    config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    config.audioBackend = std::make_unique<soft3d::platform::DummyAudioBackend>();
    return std::make_unique<HookEngine>(std::move(config));
}

void TestMotionBlur()
{
    constexpr int width = 3;
    constexpr int height = 2;
    const std::array<uint16_t, 6> colors = {0xffff, 0xf800, 0x07e0, 0x001f, 0x8410, 0x1234};
    std::array<uint32_t, 6> source{};
    for (std::size_t index = 0; index < source.size(); ++index) {
        source[index] = colors[index] | 0x12340000U;
    }
    for (int orientation : {1, 2}) {
        soft3d::MotionBlurShader shader(width, height);
        std::array<uint32_t, 6> destination{};
        destination.fill(0x00808080);
        shader.SetEnabled(orientation);
        shader.ResetHistory();
        shader.Render(source.data(), destination.data());
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t sourceIndex = y * width + x;
                const std::size_t destinationIndex = DestinationIndex(width, height, orientation, x, y);
                const uint16_t pixel = colors[sourceIndex];
                const uint32_t red = std::min(255U, ((pixel & 0xf800U) >> 10) + 112);
                const uint32_t green = std::min(255U, (((pixel & 0x07e0U) >> 5) << 2) / 4 + 112);
                const uint32_t blue = std::min(255U, ((pixel & 0x001fU) << 3) / 4 + 112);
                CHECK(destination[destinationIndex] == ((red << 16) | (green << 8) | blue));
            }
        }
    }

    soft3d::MotionBlurShader shader(1, 1);
    uint32_t destination = 0x00ffffff;
    const uint32_t white = 0xffff;
    shader.ResetHistory();
    shader.Render(&white, &destination);
    CHECK(destination == 0x00ffffff);
    const uint32_t black = 0;
    shader.Render(&black, &destination);
    CHECK(destination == 0x00dfdfdf);
    shader.Render(&black, &destination);
    CHECK(destination == 0x00c3c3c3);
    destination = 0x00808080;
    shader.ResetHistory();
    shader.Render(&white, &destination);
    CHECK(destination == 0x00aeafae);
    shader.Render(&black, &destination);
    CHECK(destination == 0x00989998);
}

void TestGray()
{
    constexpr int width = 3;
    constexpr int height = 2;
    const std::array<uint32_t, 6> source = {
        0x0000ffff, 0x6ffff800, 0x7fff07e0, 0x0001001f, 0x12348410, 0x6fff0001
    };
    for (int orientation : {1, 2}) {
        soft3d::GrayShader shader(width, height);
        shader.SetEnabled(orientation);
        std::array<uint32_t, 6> destination{};
        shader.Render(source.data(), destination.data());
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const uint32_t pixel = source[y * width + x];
                const uint16_t color = pixel & 0xffff;
                const uint16_t depth = pixel >> 16;
                uint32_t expected = Expand(color);
                if (depth != 0x6fff && depth != 0x7fff) {
                    const uint32_t gray = color & 0x1f;
                    expected = (gray << 19) | (gray << 11) | (gray << 3);
                }
                CHECK(destination[DestinationIndex(width, height, orientation, x, y)] == expected);
            }
        }
    }
}

void TestFade()
{
    soft3d::Renderer renderer(1, 1);
    soft3d::FadeShader shader(1, 1, &renderer);
    CHECK(!shader.IsActive());
    CHECK(!shader.IsCompleted());
    uint32_t source = 0x0000ffff;
    uint32_t destination = 0xdeadbeef;
    shader.Render(&source, &destination);
    CHECK(destination == 0xdeadbeef);

    shader.StartFade(1, 0x0000, 2, 1);
    CHECK(shader.IsActive());
    shader.Render(&source, &destination);
    CHECK(destination == FadePixel(0xffff, 0, 15));
    CHECK(!shader.IsCompleted());
    shader.Render(&source, &destination);
    CHECK(destination == FadePixel(0xffff, 0, 31));
    CHECK(!shader.IsCompleted());
    CHECK(shader.IsActive());
    shader.Render(&source, &destination);
    CHECK(shader.IsCompleted());
    CHECK(!shader.IsActive());
    const uint32_t terminal = destination;
    source = 0;
    shader.Render(&source, &destination);
    CHECK(destination == terminal);

    shader.StartFade(0, 0x0000, 2, 0);
    source = 0x0000ffff;
    shader.Render(&source, &destination);
    CHECK(destination == FadePixel(0xffff, 0, 17));
    shader.Render(&source, &destination);
    CHECK(destination == FadePixel(0xffff, 0, 1));
    CHECK(!shader.IsCompleted());
    shader.Render(&source, &destination);
    CHECK(shader.IsCompleted());
    CHECK(shader.IsActive());

    constexpr int width = 3;
    constexpr int height = 2;
    const std::array<uint32_t, 6> orientedSource = {
        0x0000ffff, 0x6ffff800, 0x7fff07e0, 0x0001001f, 0x12348410, 0x6fff0001
    };
    for (int orientation : {1, 2}) {
        soft3d::FadeShader oriented(width, height);
        oriented.SetEnabled(orientation);
        oriented.StartFade(1, 0x001f, 4, 0);
        std::array<uint32_t, 6> output{};
        oriented.Render(orientedSource.data(), output.data());
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const uint32_t pixel = orientedSource[y * width + x];
                const uint16_t color = pixel & 0xffff;
                const uint32_t expected = (pixel >> 16) == 0x6fff
                    ? Expand(color) : FadePixel(color, 0x001f, 7);
                CHECK(output[DestinationIndex(width, height, orientation, x, y)] == expected);
            }
        }
    }

    uint32_t presented = 0;
    renderer.SetBackBufferPtr(&presented);
    *reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels()) = 0x0000ffff;
    shader.StartFade(0, 0, 1, 1);
    renderer.Present();
    CHECK(presented == FadePixel(0xffff, 0, 1));
    renderer.Present();
    CHECK(shader.IsCompleted());
    *reinterpret_cast<uint32_t*>(renderer.GetCurrentTexture()->GetPixels()) = 0x0000f800;
    renderer.Present();
    CHECK(presented == Expand(0xf800));
}

void TestInk()
{
    auto engine = MakeEngine(4, 3);
    soft3d::InkShader shader(4, 3);
    std::vector<uint32_t> previous(12, 0x00202020);
    std::vector<uint32_t> current(12, 0x00112233);
    shader.StartInk(engine.get(), 99, previous.data(), 0);
    CHECK(shader.IsActive());
    CHECK(!shader.IsCompleted());
    CHECK(engine->RunFrame());
    CHECK(engine->preUpdates == 1 && engine->postUpdates == 1 && engine->updates == 1);
    CHECK(engine->submits == 0 && engine->prepares == 0);
    shader.Render(nullptr, current.data());
    CHECK(shader.IsCompleted());
    CHECK(shader.IsActive());
    CHECK(current[1] == 0x00101010);
    CHECK(current[3] == 0x00101010);
    CHECK(current[4] == 0x00101010);
    CHECK(current[5] == 0x00101010);
    CHECK(current[7] == 0x00101010);
    CHECK(current[0] == 0x00112233 && current[11] == 0x00112233);
    CHECK(engine->RunFrame());
    CHECK(engine->preUpdates == 2 && engine->postUpdates == 2 && engine->updates == 2);
    CHECK(engine->submits == 1 && engine->prepares == 1);

    std::vector<uint32_t> next(12, 0x00112233);
    shader.Render(nullptr, next.data());
    CHECK(next[5] == 0x00000000);
    CHECK(next[8] == 0x00000000);

    soft3d::InkShader nullHistoryShader(4, 3);
    std::vector<uint32_t> inPlace(12, 0x00202020);
    nullHistoryShader.StartInk(engine.get(), 0, nullptr, 0);
    nullHistoryShader.Render(nullptr, inPlace.data());
    CHECK(nullHistoryShader.IsCompleted());
    CHECK(inPlace[1] == 0x00101010 && inPlace[8] == 0x00101010);

    for (int mode = 1; mode <= 3; ++mode) {
        soft3d::InkShader thresholdShader(4, 3);
        std::vector<uint32_t> thresholdPrevious(12, 0x00202020);
        thresholdPrevious[4] = mode == 2 ? 0x00202120 : 0x00212020;
        std::vector<uint32_t> thresholdCurrent(12, 0x00112233);
        thresholdShader.StartInk(engine.get(), 0, thresholdPrevious.data(), mode);
        thresholdShader.Render(nullptr, thresholdCurrent.data());
        CHECK(!thresholdShader.IsCompleted());
        CHECK(thresholdCurrent[5] == 0x00112233);
        CHECK(thresholdCurrent[8] != 0x00112233);
    }

    shader.Reset();
    CHECK(!shader.IsActive());
    CHECK(!shader.IsCompleted());
    const std::vector<uint32_t> unchanged = next;
    shader.Render(nullptr, next.data());
    CHECK(next == unchanged);
    shader.StartInk(engine.get(), 0, previous.data(), 0);
    shader.StopInk();
    CHECK(!shader.IsActive());
    CHECK(shader.IsCompleted());
}

void TestBlind()
{
    constexpr int width = 3;
    constexpr int height = 3;
    const std::array<uint32_t, 9> source = {
        0x1111ffff, 0x2222f800, 0x333307e0,
        0x4444001f, 0x55558410, 0x66661234,
        0x7777abcd, 0x6fff1357, 0x7fff2468
    };
    for (int orientation : {1, 2}) {
        soft3d::BlindShader shader(width, height);
        shader.SetEnabled(orientation);
        std::array<uint32_t, 9> destination{};
        destination.fill(0xa5a5a5a5);
        std::array<uint16_t, 18> expected{};
        expected.fill(0xa5a5);
        shader.Render(source.data(), destination.data());
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const uint16_t pixel = source[y * width + x] & 0xffff;
                const int coordinate = orientation == 1 ? y : x;
                const uint32_t red = ((pixel & 0xf800U) >> 8) * coordinate / 2;
                const uint32_t green = (((pixel & 0x07e0U) >> 5) << 2) * coordinate / 2;
                const uint32_t blue = ((pixel & 0x001fU) << 3) * coordinate / 2;
                expected[DestinationIndex(width, height, orientation, x, y)] = Pack(red, green, blue);
            }
        }
        CHECK(std::equal(expected.begin(), expected.end(),
                         reinterpret_cast<const uint16_t*>(destination.data())));
    }

    soft3d::BlindShader wrapShader(3, 2);
    wrapShader.SetEnabled(2);
    const std::array<uint32_t, 6> white = {
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff
    };
    std::array<uint32_t, 6> wrapped{};
    wrapShader.Render(white.data(), wrapped.data());
    CHECK(reinterpret_cast<const uint16_t*>(wrapped.data())[DestinationIndex(3, 2, 2, 2, 0)] ==
          Pack(496, 504, 496));
}

void TestTwist()
{
    constexpr int width = 4;
    constexpr int height = 2;
    std::array<uint32_t, 8> source{};
    for (std::size_t index = 0; index < source.size(); ++index) {
        source[index] = static_cast<uint32_t>(index + 1) | 0x12340000U;
    }
    const std::array<std::array<int, 4>, 2> firstOffsets = {{ {{1, 2, 3, 3}}, {{2, 3, 3, 3}} }};
    const std::array<std::array<int, 4>, 2> secondOffsets = {{ {{2, 3, 3, 3}}, {{1, 2, 3, 3}} }};
    const std::array<std::array<int, 4>, 2> thirdOffsets = {{ {{3, 3, 3, 3}}, {{2, 3, 3, 3}} }};
    soft3d::TwistShader shader(width, height);
    std::array<uint32_t, 8> destination{};
    shader.Render(source.data(), destination.data());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            CHECK(destination[DestinationIndex(width, height, 1, x, y)] ==
                  Expand(source[y * width + firstOffsets[y][x]] & 0xffff));
        }
    }
    shader.Render(source.data(), destination.data());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            CHECK(destination[DestinationIndex(width, height, 1, x, y)] ==
                  Expand(source[y * width + secondOffsets[y][x]] & 0xffff));
        }
    }
    shader.SetEnabled(2);
    shader.Render(source.data(), destination.data());
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            CHECK(destination[DestinationIndex(width, height, 2, x, y)] ==
                  Expand(source[y * width + thirdOffsets[y][x]] & 0xffff));
        }
    }

    int offset = 1;
    int velocity = -1;
    for (int frame = 3; frame < 12; ++frame) {
        int rowOffset = offset;
        int rowStep = 1;
        std::array<uint32_t, 8> expected{};
        for (int y = 0; y < height; ++y) {
            rowOffset += rowStep;
            if (rowOffset >= 2 || rowOffset <= -2) {
                rowStep = -rowStep;
            }
            for (int x = 0; x < width; ++x) {
                const int sample = std::clamp(x + rowOffset, 0, width - 1);
                expected[DestinationIndex(width, height, 2, x, y)] =
                    Expand(source[y * width + sample] & 0xffff);
            }
        }
        shader.Render(source.data(), destination.data());
        CHECK(destination == expected);
        offset += velocity;
        if (offset >= 2 || offset <= -2) {
            velocity = -velocity;
        }
    }
}

} // namespace

int main()
{
    try {
        TestMotionBlur();
        TestGray();
        TestFade();
        TestInk();
        TestBlind();
        TestTwist();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "presentation shader tests passed\n";
    return 0;
}

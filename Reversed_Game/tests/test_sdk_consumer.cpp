#include "soft3d/Bsp.hpp"
#include "soft3d/Color.hpp"
#include "soft3d/Dialog.hpp"
#include "soft3d/Geometry.hpp"
#include "soft3d/Input.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/SoundInstance.hpp"
#include "soft3d/TextureTarget.hpp"
#include "soft3d/s3dBaseMath.hpp"
#include "soft3d/s3dIO.hpp"

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

struct SortRecord {
    int16_t key;
    int16_t serial;
};
static_assert(sizeof(SortRecord) == 4);

int CompareRecords(const void* left, const void* right)
{
    const auto* a = static_cast<const SortRecord*>(left);
    const auto* b = static_cast<const SortRecord*>(right);
    return static_cast<int>(a->key) - static_cast<int>(b->key);
}

int CompareBytes(const void* left, const void* right)
{
    return static_cast<int>(*static_cast<const uint8_t*>(left)) -
           static_cast<int>(*static_cast<const uint8_t*>(right));
}

int CompareWords(const void* left, const void* right)
{
    return static_cast<int>(*static_cast<const uint16_t*>(left)) -
           static_cast<int>(*static_cast<const uint16_t*>(right));
}

void TestPowerOfTwo()
{
    CHECK(soft3d::GetPow2(0) == -1);
    CHECK(soft3d::GetPow2(1) == 0);
    CHECK(soft3d::GetPow2(3) == -1);
    CHECK(soft3d::GetPow2(0x7fffffffU) == -1);
    CHECK(soft3d::GetPow2(0x80000001U) == -1);
    CHECK(soft3d::GetPow2(0xffffffffU) == -1);
    for (uint32_t exponent = 0; exponent < 32; ++exponent) {
        const uint32_t value = 1U << exponent;
        CHECK(soft3d::GetPow2(value) == static_cast<int32_t>(exponent));
        CHECK(soft3d::ConvPow2(value) == value);
    }
    CHECK(soft3d::ConvPow2(0) == 0);
    CHECK(soft3d::ConvPow2(3) == 4);
    CHECK(soft3d::ConvPow2(5) == 8);
    CHECK(soft3d::ConvPow2(0x7fffffffU) == 0x80000000U);
    CHECK(soft3d::ConvPow2(0x80000001U) == 0);
    CHECK(soft3d::ConvPow2(0xffffffffU) == 0);
}

void TestRandomGenerator()
{
    soft3d::RandomGenerator random{};
    random.SetSeed(0);
    constexpr int32_t expected[] = {38, 7719, 21238, 2437, 8855, 11797};
    for (int32_t value : expected) {
        CHECK(random.Next() == value);
    }
    random.SetSeed(0);
    CHECK(random.Next() == 38);
    random.SetSeed(0xffffffffU);
    CHECK(random.Next() == 35);
}

void TestQsort()
{
    SortRecord small[] = {{2, 0}, {1, 1}, {2, 2}, {1, 3}};
    soft3d::Qsort(small, 4, sizeof(SortRecord), CompareRecords);
    constexpr SortRecord smallExpected[] = {{1, 1}, {1, 3}, {2, 2}, {2, 0}};
    CHECK(std::memcmp(small, smallExpected, sizeof(small)) == 0);

    SortRecord partitioned[] = {{4, 0}, {1, 1}, {3, 2}, {2, 3}, {4, 4},
                                {0, 5}, {3, 6}, {2, 7}, {1, 8}};
    soft3d::Qsort(partitioned, 9, sizeof(SortRecord), CompareRecords);
    constexpr SortRecord partitionedExpected[] = {
        {0, 5}, {1, 1}, {1, 8}, {2, 7}, {2, 3}, {3, 6}, {3, 2}, {4, 4}, {4, 0}
    };
    CHECK(std::memcmp(partitioned, partitionedExpected, sizeof(partitioned)) == 0);

    SortRecord equal[9];
    for (int32_t index = 0; index < 9; ++index) {
        equal[index] = {7, static_cast<int16_t>(index)};
    }
    const SortRecord equalOriginal[9] = {
        {7, 0}, {7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7}, {7, 8}
    };
    soft3d::Qsort(equal, 9, sizeof(SortRecord), CompareRecords);
    CHECK(std::memcmp(equal, equalOriginal, sizeof(equal)) == 0);

    uint8_t bytes[] = {8, 7, 6, 5, 4, 3, 2, 1, 0};
    soft3d::Qsort(bytes, 9, 1, CompareBytes);
    for (uint8_t index = 0; index < 9; ++index) {
        CHECK(bytes[index] == index);
    }

    uint16_t words[] = {300, 1, 65535, 2};
    soft3d::Qsort(words, 4, 2, CompareWords);
    CHECK(words[0] == 1 && words[1] == 2 && words[2] == 300 && words[3] == 65535);

    uint8_t unsupported[] = {2, 1};
    soft3d::Qsort(unsupported, 2, 3, CompareBytes);
    CHECK(unsupported[0] == 2 && unsupported[1] == 1);
    soft3d::Qsort(nullptr, 0, 4, CompareRecords);
    soft3d::Qsort(nullptr, 1, 4, CompareRecords);
}

void TestConversion()
{
    char output[8] = {};
    CHECK(soft3d::ConvertUtf16ToAscii(output, u"A\u1234z") == output);
    CHECK(std::memcmp(output, "A\x34z", 4) == 0);
    CHECK(soft3d::ConvertUtf16ToAscii(output, u"") == output);
    CHECK(output[0] == '\0');

    char16_t wide[5] = {};
    CHECK(soft3d::ConvertAsciiToWide(wide, "A\xff") == wide);
    CHECK(wide[0] == u'A' && wide[1] == 0xff && wide[2] == 0);
}

void TestRecoveredEngineSurface()
{
    CHECK(soft3d::FixedToIntCeil(0x18000) == 2);
    CHECK(soft3d::Color4Negate({0, soft3d::fixedOne, 0x8000, 0}).x == soft3d::fixedOne);
    CHECK(soft3d::LerpVec3({0, 0, 0}, {soft3d::fixedOne, 0, 0}, 0x8000).x == 0x8000);
    CHECK(soft3d::Inverse(soft3d::Matrix4::Identity()).values[0] == soft3d::fixedOne);

    const soft3d::Plane plane = soft3d::PlaneFromPointNormal({0, 0, 0}, {0, soft3d::fixedOne, 0});
    CHECK(soft3d::PlaneDistance(plane, {0, 2 * soft3d::fixedOne, 0}) == 2 * soft3d::fixedOne);
    soft3d::PointCloud cloud(std::vector<soft3d::Vec3>{{0, 0, 0}, {soft3d::fixedOne, 0, 0},
                                                       {0, 0, soft3d::fixedOne}});
    soft3d::PointCloud copy = cloud;
    copy.GetPoints()[0].x = 7;
    CHECK(cloud.GetPoints()[0].x == 0 && copy.GetPoints()[0].x == 7);
    CHECK(cloud.Reversed().GetPoints()[0].z == soft3d::fixedOne);

    soft3d::TextureTarget target(2, 2);
    CHECK(target.GetStrideBytes() == 8 && target.GetTiledHeight() == 3);
    CHECK(target.GetTexture() != nullptr && !target.HasTiledBuffer());

    soft3d::Renderer renderer(2, 2);
    renderer.GetRaster().FillColor32(0, 0, 2, 0x12345678);
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(
        renderer.GetCurrentTexture()->GetPixels());
    CHECK(pixels[0] == 0x12345678 && pixels[1] == 0x12345678);

    soft3d::Bsp bsp(static_cast<soft3d::Renderer*>(nullptr), nullptr);
    soft3d::BspDynamicLight* light = bsp.AddDynamicLight();
    CHECK(light != nullptr && bsp.GetDynamicLightCount() == 1);
    bsp.ClearDynamicLights();
    CHECK(bsp.GetDynamicLights() == nullptr && bsp.GetDynamicLightCount() == 0);

    soft3d::Input input;
    input.KeyDown(soft3d::Key::Accept);
    CHECK(input.IsAnyKeyFirstDown() && input.IsAnyKeyDownOrClick());
    input.SetAnyKeyDown(1);
    CHECK(input.IsKeyUp(soft3d::Key::Accept));

    soft3d::SoundInstance sound;
    sound.Reset();
    CHECK(sound.GetVoice() == -1 && sound.GetListIndex() == -1);

    soft3d::Dialog dialog(nullptr, 1);
    dialog.AddElement(1, 2, 3, 4, 5, 6);
    CHECK(dialog.GetElementTemplate(0) != nullptr && dialog.GetElementTemplate(0)->id == 2);
    dialog.SetLinkedDialog(nullptr);
    CHECK(dialog.GetNextDialog() == &dialog && dialog.GetPreviousDialog() == &dialog);
    soft3d::SdtVisual visual;
    visual.x = 17;
    dialog.SetCurrentVisual(visual);
    visual.x = 99;
    CHECK(dialog.GetCurrentVisual() != nullptr && dialog.GetCurrentVisual()->x == 17);

    soft3d::ScreenRect rectangle(soft3d::S3dPoint{2, 3}, soft3d::S3dPoint{4, 5});
    CHECK(rectangle.right == 6 && rectangle.bottom == 8);
    CHECK(rectangle.Contains({6, 8}));
}

} // namespace

int main()
{
    try {
        TestPowerOfTwo();
        TestRandomGenerator();
        TestQsort();
        TestConversion();
        TestRecoveredEngineSurface();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "standalone soft3d SDK consumer passed\n";
    return 0;
}

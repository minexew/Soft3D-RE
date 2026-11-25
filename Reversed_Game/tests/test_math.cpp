#include "soft3d/Color.hpp"
#include "soft3d/Fixed.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void CheckVec2(const soft3d::Vec2& actual, const soft3d::Vec2& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y, line);
}

void CheckVec3(const soft3d::Vec3& actual, const soft3d::Vec3& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z, line);
}

void CheckVec4(const soft3d::Vec4& actual, const soft3d::Vec4& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z &&
          actual.w == expected.w, line);
}

#define CHECK_VEC2(actual, ...) CheckVec2((actual), soft3d::Vec2 __VA_ARGS__, __LINE__)
#define CHECK_VEC3(actual, ...) CheckVec3((actual), soft3d::Vec3 __VA_ARGS__, __LINE__)
#define CHECK_VEC4(actual, ...) CheckVec4((actual), soft3d::Vec4 __VA_ARGS__, __LINE__)

constexpr soft3d::Fixed one = soft3d::fixedOne;

void TestScalars()
{
    CHECK(soft3d::IntPow(-2, -3) == 0);
    CHECK(soft3d::IntPow(-2, -1) == 0);
    CHECK(soft3d::IntPow(-2, 0) == 1);
    CHECK(soft3d::IntPow(-2, 3) == -8);
    CHECK(soft3d::IntPow(0, -3) == 0);
    CHECK(soft3d::IntPow(0, 0) == 1);
    CHECK(soft3d::IntPow(1, -3) == 1);
    CHECK(soft3d::IntPow(-1, -3) == -1);
    CHECK(soft3d::IntPow(2, -3) == 0);
    CHECK(soft3d::IntPow(2, 10) == 1024);
    CHECK(soft3d::IntPow(0x10000, 2) == 0);
    CHECK(soft3d::IntPow(0x10000, -2) == 0);
    CHECK(soft3d::IntPow(0x10001, 2) == 0x20001);

    CHECK(soft3d::FixedToInt(0x18000) == 1);
    CHECK(soft3d::FixedToInt(-0x18000) == -2);
    CHECK(soft3d::FixedToInt(std::numeric_limits<int32_t>::max()) == 32767);
    CHECK(soft3d::FixedToInt(std::numeric_limits<int32_t>::min()) == -32768);
    CHECK(soft3d::FixedToIntCeil(0x18000) == 2);
    CHECK(soft3d::FixedToIntCeil(-0x18000) == -1);
    CHECK(soft3d::FixedToIntCeil(one) == 1);
    CHECK(soft3d::FixedToIntCeil(-one) == -1);
    CHECK(soft3d::FixedToIntCeil(std::numeric_limits<int32_t>::max()) == -32768);
    CHECK(soft3d::FixedToUnsignedByte(0x12345678) == 0x34);
    CHECK(soft3d::FixedToUnsignedByte(-one) == 0xff);
    CHECK(soft3d::FixedToUnsignedByte(std::numeric_limits<int32_t>::min()) == 0);
    CHECK(soft3d::FixedToAngle(0x12345678) == 0x1234);
    CHECK(soft3d::FixedToAngle(static_cast<soft3d::Fixed>(0xffff0000U)) == -1);
    CHECK(soft3d::FixedToAngle(std::numeric_limits<int32_t>::min()) == -32768);
    CHECK(soft3d::FixedToAngle(0xffff) == 0);

    CHECK(soft3d::FixedReciprocalQuarter(one) == one);
    CHECK(soft3d::FixedReciprocalQuarter(-one) == -one);
    CHECK(soft3d::FixedReciprocalQuarter(0) == 0);
    CHECK(soft3d::FixedReciprocalQuarter(1) == 0);
    CHECK(soft3d::FixedReciprocalQuarter(std::numeric_limits<int32_t>::max()) == 2);
    CHECK(soft3d::FixedReciprocalQuarter(std::numeric_limits<int32_t>::min()) == -2);

    constexpr uint32_t inputs[] = {
        0, 1, 0xffff, 0x10000, 0x3ffff, 0x40000, 0xfffff, 0x100000,
        0x3fffff, 0x400000, 0xffffff, 0x1000000, 0x3ffffff, 0x4000000,
        0x7fffffff, 0x80000000, 0xffffffff,
    };
    constexpr soft3d::Fixed expected16[] = {
        -4, 0, 65536, 65536, 16384, 16384, 4096, 4096, 1024,
        1024, 256, 256, 64, 64, 16, 2097151, 16,
    };
    constexpr soft3d::Fixed expected17[] = {
        -2, -2, 65538, 65536, 16384, 16384, 4096, 4096, 1024,
        1024, 256, 256, 128, 16777215, 128, 16777215, 128,
    };
    for (std::size_t index = 0; index < std::size(inputs); ++index) {
        CHECK(soft3d::FixedReciprocalUnsigned16(inputs[index]) == expected16[index]);
        CHECK(soft3d::FixedReciprocalUnsigned17(inputs[index]) == expected17[index]);
    }
    CHECK(soft3d::FixedReciprocalSigned16(0) == 4);
    CHECK(soft3d::FixedReciprocalSigned17(0) == 2);
    CHECK(soft3d::FixedReciprocalSigned16(-1) == 0);
    CHECK(soft3d::FixedReciprocalSigned17(-1) == 2);
    CHECK(soft3d::FixedReciprocalSigned16(-0x40000) == -16384);
    CHECK(soft3d::FixedReciprocalSigned17(-0x40000) == -16384);
    CHECK(soft3d::FixedReciprocalSigned16(std::numeric_limits<int32_t>::min()) == -2097151);
    CHECK(soft3d::FixedReciprocalSigned17(std::numeric_limits<int32_t>::min()) == -16777215);

    CHECK(soft3d::FixedArcSine(0) == 0);
    CHECK(soft3d::FixedArcSine(0x8000) == 0x2aa);
    CHECK(soft3d::FixedArcSine(-0x8000) == 0xfd55);
    CHECK(soft3d::FixedArcSine(one) == 0x716);
    CHECK(soft3d::FixedArcSine(-one) == 0xf8e9);
    CHECK(soft3d::FixedArcSine(2 * one) == 0x716);
    CHECK(soft3d::FixedArcSine(-2 * one) == 0xf8e9);
    CHECK(soft3d::FixedArcCosine(one) == 0xea);
    CHECK(soft3d::FixedArcCosine(-one) == 0xf17);
}

void TestColors()
{
    soft3d::Color4 color{-1, 0x8000, 0x10001, 0x20000};
    CHECK(&soft3d::Color4Clamp(color) == &color);
    CHECK_VEC4(color, {0, 0x8000, one, one});
    CHECK_VEC4(soft3d::Color4Negate({0, 0x8000, one, 0x20000}),
               {one, 0x8000, 0, -one});
    CHECK_VEC4(soft3d::Color4AdjustContrast({0, 0x4000, 0xc000, 0x12345}, 0x20000),
               {0, 0, one, one});
    CHECK_VEC4(soft3d::Color4AdjustContrast({0, 0x4000, 0xc000, 0x4000}, 0x20000),
               {0, 0, one, 0x4000});
    CHECK_VEC4(soft3d::Color4Lerp({0, 0x4000, 0x8000, 0xc000},
                                  {one, 0xc000, 0, 0x4000}, 0x8000),
               {0x8000, 0x8000, 0x4000, 0x8000});
    CHECK_VEC4(soft3d::Color4Lerp({0, 0x4000, 0x8000, 0xc000},
                                  {one, 0xc000, 0, 0x4000}, 0x20000),
               {one, one, 0, 0});
    color = {0, 0x4000, 0x8000, 0xc000};
    color = soft3d::Color4Lerp(color, {one, 0xc000, 0, 0x4000}, 0x8000);
    CHECK_VEC4(color, {0x8000, 0x8000, 0x4000, 0x8000});
}

soft3d::Matrix4 MakeTransform()
{
    soft3d::Matrix4 matrix = soft3d::Matrix4::Identity();
    matrix.values = {
        0x20000, 0x50000, 0x10000, 0,
        0x30000, 0x60000, 0x10000, 0,
        0x10000, 0x20000, 0x10000, 0,
        0x40000, 0x70000, 0x10000, one,
    };
    return matrix;
}

void TestVec2AndVec3()
{
    CHECK(soft3d::Dot(soft3d::Vec2{one, 2 * one}, soft3d::Vec2{3 * one, 4 * one}) == 11 * one);
    CHECK(soft3d::SquaredLength(soft3d::Vec2{3 * one, 4 * one}) == 25 * one);
    CHECK(soft3d::Length(soft3d::Vec2{3 * one, 4 * one}) == 5 * one);
    CHECK_VEC2(soft3d::Normalize(soft3d::Vec2{3 * one, 4 * one}), {39324, 52432});
    CHECK_VEC2(soft3d::Normalize(soft3d::Vec2{0, 0}), {0, 0});
    CHECK(soft3d::SquaredLength(soft3d::Vec2{0x70000000, 0x70000000}) == 0);

    soft3d::Matrix4 matrix = MakeTransform();
    soft3d::Vec2 vector2{0x8000, 0x4000};
    CHECK_VEC2(soft3d::Transform(matrix, vector2), {0x5c000, 0xb0000});
    CHECK_VEC2(soft3d::TransformDirection(matrix, vector2), {0x1c000, 0x40000});
    CHECK_VEC2(soft3d::TransformPerspective(matrix, vector2), {0x5c000, 0xb0000});
    vector2 = soft3d::Transform(matrix, vector2);
    CHECK_VEC2(vector2, {0x5c000, 0xb0000});
    vector2 = {0x8000, 0x4000};
    matrix.values[15] = 0x20000;
    CHECK_VEC2(soft3d::TransformPerspective(matrix, vector2), {0x2e000, 0x58000});
    matrix.values[15] = 0;
    CHECK_VEC2(soft3d::TransformPerspective(matrix, vector2), {0, 0});
    matrix.values[15] = 0x8000;
    CHECK_VEC2(soft3d::TransformPerspective(matrix, vector2), {0xb8000, 0x160000});

    const soft3d::Vec3 lerpStart{one, 2 * one, 3 * one};
    const soft3d::Vec3 lerpEnd{3 * one, 4 * one, 5 * one};
    CHECK_VEC3(soft3d::LerpVec3(lerpStart, lerpEnd, 0x8000), {2 * one, 3 * one, 4 * one});
    CHECK_VEC3(soft3d::LerpVec3(lerpStart, lerpEnd, -0x8000), {0, one, 2 * one});
    CHECK_VEC3(soft3d::LerpVec3(soft3d::Vec3{0, 0, 0}, soft3d::Vec3{1, 1, 1}, 0x8000), {0, 0, 0});
    CHECK_VEC3(soft3d::LerpVec3(soft3d::Vec3{0, 0, 0}, soft3d::Vec3{1, 1, 1}, -0x8000), {-1, -1, -1});
    CHECK_VEC3(soft3d::MultiplyComponents({2 * one, 3 * one, 4 * one}, 2 * one, 3 * one, 4 * one),
               {4 * one, 6 * one, 8 * one});
    CHECK_VEC3(soft3d::AddComponents({2 * one, 3 * one, 4 * one}, 2 * one, 3 * one, 4 * one),
               {4 * one, 5 * one, 6 * one});
    CHECK(soft3d::SquaredDistance({one, 2 * one, 3 * one}, {-one, 0, one}) == 12 * one);
    CHECK(soft3d::SquaredDistance({0x70000000, 0, 0}, {0, 0, 0}) == 0);

    matrix = MakeTransform();
    const soft3d::Vec3 vector3{0x8000, 0x4000, 0x2000};
    CHECK_VEC3(soft3d::TransformDirection(matrix, vector3), {0x1e000, 0x44000, 0xe000});
    CHECK_VEC3(soft3d::TransformPerspective(matrix, vector3), {0x5e000, 0xb4000, 0x1e000});
    matrix.values[15] = 0x20000;
    CHECK_VEC3(soft3d::TransformPerspective(matrix, vector3), {0x2f000, 0x5a000, 0xf000});
    matrix.values[15] = 0;
    CHECK_VEC3(soft3d::TransformPerspective(matrix, vector3), {0, 0, 0});
    matrix.values[15] = 0x8000;
    CHECK_VEC3(soft3d::TransformPerspective(matrix, vector3), {0xbc000, 0x168000, 0x3c000});

    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({one, 0, 0})) == 0xea);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({0, 0, one})) == 0x716);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({-one, 0, 0})) == 0xf16);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({0, 0, -one})) == 0x1716);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({0x8000, 0, 0x8000})) == 0x2aa);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({-0x8000, 0, 0x8000})) == 0xd56);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({-0x8000, 0, -0x8000})) == 0x12aa);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({0x8000, 0, -0x8000})) == 0x1d56);
    CHECK(static_cast<uint16_t>(soft3d::Vec3AngleXZ({one, 0, 0x8000})) == 0xea);
}

void TestVec4()
{
    const soft3d::Vec4 value{one, 2 * one, 3 * one, 4 * one};
    CHECK(soft3d::Dot(value, {4 * one, 3 * one, 2 * one, one}) == 20 * one);
    CHECK(soft3d::SquaredLength(value) == 30 * one);
    CHECK(soft3d::Length(value) == 358912);
    CHECK_VEC4(soft3d::NormalizeVec4(value), {11964, 23928, 35892, 47856});
    CHECK_VEC4(soft3d::NormalizeVec4({0, 0, 0, 0}), {0, 0, 0, 0});

    soft3d::Vec4 output{1, 2, 3, 4};
    CHECK(&soft3d::DivideVec4ByW(output, {one, 2 * one, 3 * one, one}) == &output);
    CHECK_VEC4(output, {1, 2, 3, 4});
    soft3d::DivideVec4ByW(output, {one, 2 * one, 3 * one, 2 * one});
    CHECK_VEC4(output, {0x8000, one, 0x18000, one});
    output = {one, 2 * one, 3 * one, 2 * one};
    soft3d::DivideVec4ByW(output, output);
    CHECK_VEC4(output, {0x8000, one, 0x18000, one});
    output = {1, 2, 3, 4};
    soft3d::DivideVec4ByW(output, {one, 2 * one, 3 * one, 0});
    CHECK_VEC4(output, {0, 0, 0, one});

    const soft3d::Vec4 e1{one, 0, 0, 0};
    const soft3d::Vec4 e2{0, one, 0, 0};
    const soft3d::Vec4 e3{0, 0, one, 0};
    CHECK_VEC4(soft3d::Cross3(e1, e2, e3), {0, 0, 0, -one});
    const soft3d::Vec4 cross = soft3d::Cross3(e1, e2, e3);
    CHECK(soft3d::Dot(cross, e1) == 0);
    CHECK(soft3d::Dot(cross, e2) == 0);
    CHECK(soft3d::Dot(cross, e3) == 0);
    CHECK_VEC4(soft3d::LerpVec4(e1, {-one, 0, 0, 0}, 0x8000), {0, 0, 0, 0});

    CHECK_VEC4(soft3d::NormalizeRobust({one, 0, 0, 2 * one}), {one, 0, 0, 2 * one});
    CHECK_VEC4(soft3d::NormalizeRobust({256, 0, 0, 512}), {32767, 0, 0, 65534});
    CHECK_VEC4(soft3d::NormalizeRobust({0x1000000, 0, 0, 0x2000000}),
               {one, 0, 0, 2 * one});
    CHECK_VEC4(soft3d::NormalizeRobust({0, 0, 0, 0}), {0, 0, 0, 0});
}

void TestQuaternions()
{
    CHECK_VEC4(soft3d::QuaternionIdentity(), {0, 0, 0, one});
    CHECK_VEC4(soft3d::QuaternionConjugate({one, 2 * one, 3 * one, 4 * one}),
               {-one, -2 * one, -3 * one, 4 * one});
    CHECK_VEC4(soft3d::QuaternionInverse(soft3d::QuaternionIdentity()), {0, 0, 0, one});
    CHECK_VEC4(soft3d::QuaternionInverse({0, 0, 0, 2 * one}), {0, 0, 0, one});
    CHECK_VEC4(soft3d::QuaternionInverse({0, 0, 0, 0}), {0, 0, 0, 0});

    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0x800), {0, 46340, 0, 46340});
#ifdef SOFT3D_TRIG_USE_MATH
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0x1000), {0, one, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, 2 * one, 0}, 0x1000), {0, 2 * one, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0), {0, 0, 0, one});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0x2000), {0, 0, 0, -one});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0, 0, 0), {0, 0, 0, one});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0x1000, 0, 0), {0, one, 0, 0});
#else
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0x1000), {0, 65535, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, 2 * one, 0}, 0x1000), {0, 131070, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0), {0, 0, 0, 65535});
    CHECK_VEC4(soft3d::QuaternionFromAxisAngle({0, one, 0}, 0x2000), {0, 0, 0, -65535});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0, 0, 0), {0, 0, 0, 65533});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0x1000, 0, 0), {0, 65533, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0, 0x1000, 0), {65533, 0, 0, 0});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0, 0, 0x1000), {0, 0, 65533, 0});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0x400, 0x800, 0xc00),
               {32766, -32767, 32767, 32766});
    CHECK_VEC4(soft3d::QuaternionFromEuler(0x1234, static_cast<int16_t>(0xfedc), 0x2222),
               {-14759, -61891, -4090, 15156});
#endif

    CHECK_VEC4(soft3d::QuaternionFromMatrix(soft3d::Matrix4::Identity()), {0, 0, 0, one});
    soft3d::Matrix4 matrix = soft3d::Matrix4::Identity();
    matrix.values[5] = -one;
    matrix.values[10] = -one;
    CHECK_VEC4(soft3d::QuaternionFromMatrix(matrix), {one, 0, 0, 0});
    matrix.values = {-one, 0, 0, 0, 0, one, 0, 0, 0, 0, -one, 0, 0, 0, 0, one};
    CHECK_VEC4(soft3d::QuaternionFromMatrix(matrix), {0, one, 0, 0});
    matrix.values = {-one, 0, 0, 0, 0, -one, 0, 0, 0, 0, one, 0, 0, 0, 0, one};
    CHECK_VEC4(soft3d::QuaternionFromMatrix(matrix), {0, 0, one, 0});

    matrix = soft3d::Matrix4::Identity();
    matrix.values[0] = 0x70000000;
    matrix.values[1] = one;
    matrix.values[4] = 0;
    matrix.values[5] = 0x70000000;
    matrix.values[10] = one - 6;
    CHECK_VEC4(soft3d::QuaternionFromMatrix(matrix), {256, 512, 0, 0});
    matrix.values[10] = one - 7;
    CHECK_VEC4(soft3d::QuaternionFromMatrix(matrix), {384, 2796202, 0, 0});

    CHECK_VEC4(soft3d::QuaternionLerp({one, 0, 0, 0}, {-one, 0, 0, 0}, 0x8000),
               {one, 0, 0, 0});
    CHECK_VEC4(soft3d::QuaternionLerp({one, 0, 0, 0}, {0, one, 0, 0}, 0x8000),
               {0x8000, 0x8000, 0, 0});

    soft3d::Vec3 axis;
    int16_t angle;
    soft3d::QuaternionToAxisAngle({0, 65535, 0, 0}, axis, angle);
    CHECK_VEC3(axis, {0, 65535, 0});
    CHECK(static_cast<uint16_t>(angle) == 0x1000);
    soft3d::QuaternionToAxisAngle({0, 0, 0, 65535}, axis, angle);
    CHECK_VEC3(axis, {0, one, 0});
    CHECK(static_cast<uint16_t>(angle) == 0x1d4);
    soft3d::QuaternionToAxisAngle({0, 0, 0, -one}, axis, angle);
    CHECK_VEC3(axis, {0, one, 0});
    CHECK(static_cast<uint16_t>(angle) == 0x1e2e);
}

} // namespace

int main()
{
    try {
        TestScalars();
        TestColors();
        TestVec2AndVec3();
        TestVec4();
        TestQuaternions();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "fixed-point math API tests passed\n";
    return 0;
}

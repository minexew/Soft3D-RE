#include "soft3d/Fixed.hpp"

#include <algorithm>

#ifdef SOFT3D_TRIG_USE_MATH
#include <cmath>
#include <limits>
#endif

namespace soft3d {
namespace {

#ifndef SOFT3D_TRIG_USE_MATH
constexpr Fixed sineTable[] = {
#include "soft3d/FixedSinTable.inc"
};

constexpr Fixed tangentTable[] = {
#include "soft3d/FixedTanTable.inc"
};
#endif

Fixed FixedArcTangentUnit(Fixed value)
{
    const Fixed squared = FixedMultiply(value, value);
    Fixed power = FixedMultiply(value, squared);
    Fixed result = SubtractFixed(value, FixedMultiply(power, 0x5555));
    constexpr Fixed coefficients[] = {
        0x3333, 0x2492, 0x1c71, 0x1745, 0x13b1,
        0x1111, 0x0f0f, 0x0d79, 0x0c30,
    };
    for (std::size_t index = 0; index < std::size(coefficients); ++index) {
        power = FixedMultiply(power, squared);
        const Fixed term = FixedMultiply(power, coefficients[index]);
        result = (index & 1) == 0 ? AddFixed(result, term) : SubtractFixed(result, term);
    }
    return static_cast<uint16_t>(static_cast<uint32_t>(
        FixedMultiply(result, 0x0517cc18)) >> 16) & 0x1fff;
}

uint32_t ReciprocalTable(uint16_t index)
{
    return index == 0 ? 0x7fffffffU : static_cast<uint32_t>(0x40000000U / index);
}

Fixed ShiftRightArm(Fixed value, unsigned shift)
{
    shift &= 0xff;
    if (shift >= 32) {
        return value < 0 ? -1 : 0;
    }
    return shift == 0 ? value : value >> shift;
}

} // namespace

int32_t IntPow(int32_t base, int32_t exponent)
{
    if (exponent == 0) {
        return 1;
    }
    if (exponent < 0 && base == 0) {
        return 0;
    }
    uint32_t result = static_cast<uint32_t>(base);
    uint32_t remaining = exponent < 0
        ? 0U - static_cast<uint32_t>(exponent)
        : static_cast<uint32_t>(exponent);
    while (--remaining != 0) {
        result *= static_cast<uint32_t>(base);
    }
    if (exponent > 0) {
        return static_cast<int32_t>(result);
    }
    const int32_t denominator = static_cast<int32_t>(result);
    return denominator == 0 ? 0 : 1 / denominator;
}

Fixed FixedReciprocalQuarter(Fixed value)
{
    const Fixed denominator = value >> 2;
    return denominator == 0 ? 0 : 0x40000000 / denominator;
}

Fixed FixedReciprocalUnsigned16(uint32_t value)
{
    if (value < 0x10000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value)) << 2);
    }
    if (value < 0x40000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 2)));
    }
    if (value < 0x100000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 4)) >> 2);
    }
    if (value < 0x400000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 6)) >> 4);
    }
    if (value < 0x1000000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 8)) >> 6);
    }
    if (value < 0x4000000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 10)) >> 8);
    }
    return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 12)) >> 10);
}

Fixed FixedReciprocalSigned16(Fixed value)
{
    if (value < 1) {
        return NegateFixed(FixedReciprocalUnsigned16(0U - static_cast<uint32_t>(value)));
    }
    return FixedReciprocalUnsigned16(static_cast<uint32_t>(value));
}

Fixed FixedReciprocalUnsigned17(uint32_t value)
{
    if (value < 0x20000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 1)) << 1);
    }
    if (value < 0x80000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 3)) >> 1);
    }
    if (value < 0x200000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 5)) >> 3);
    }
    if (value < 0x800000) {
        return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 7)) >> 5);
    }
    return static_cast<Fixed>(ReciprocalTable(static_cast<uint16_t>(value >> 9)) >> 7);
}

Fixed FixedReciprocalSigned17(Fixed value)
{
    if (value < 1) {
        return NegateFixed(FixedReciprocalUnsigned17(0U - static_cast<uint32_t>(value)));
    }
    return FixedReciprocalUnsigned17(static_cast<uint32_t>(value));
}

Fixed FixedSin(uint16_t angle)
{
    const unsigned index = (angle & 0x800) == 0
        ? angle & 0x7ff
        : 0x800 - (angle & 0x7ff);
#ifdef SOFT3D_TRIG_USE_MATH
    constexpr double pi = 3.14159265358979323846;
    const Fixed value = static_cast<Fixed>(
        std::sin(static_cast<double>(index) * pi / 4096.0) * 65536.0);
#else
    const Fixed value = sineTable[index];
#endif
    return (angle & 0x1000) == 0 ? value : NegateFixed(value);
}

Fixed FixedCos(int16_t angle)
{
    return FixedSin(static_cast<uint16_t>(angle + 0x800) & 0x1fff);
}

Fixed FixedTan(uint16_t angle)
{
    const unsigned index = (angle & 0x800) == 0
        ? angle & 0x7ff
        : 0x800 - (angle & 0x7ff);
#ifdef SOFT3D_TRIG_USE_MATH
    constexpr double pi = 3.14159265358979323846;
    Fixed value = std::numeric_limits<Fixed>::max();
    if (index == 1024) {
        value = fixedOne;
    } else if (index != 2048) {
        const double tangent = std::tan(static_cast<double>(index) * pi / 4096.0) * 65536.0;
        if (tangent < static_cast<double>(std::numeric_limits<Fixed>::max())) {
            value = static_cast<Fixed>(tangent);
        }
    }
#else
    const Fixed value = tangentTable[index];
#endif
    return (angle & 0x800) == 0 ? value : NegateFixed(value);
}

uint16_t FixedArcTangent(Fixed value)
{
    if (value > fixedOne) {
        return static_cast<uint16_t>(0x800 - FixedArcTangentUnit(FixedDivide(fixedOne, value)));
    }
    if (value < -fixedOne) {
        return static_cast<uint16_t>(-0x800 - FixedArcTangentUnit(FixedDivide(fixedOne, value)));
    }
    return FixedArcTangentUnit(value);
}

uint16_t FixedArcSine(Fixed input)
{
    const Fixed value = std::clamp(input, -fixedOne, fixedOne);
    const Fixed squared = FixedMultiply(value, value);
    Fixed power = FixedMultiply(value, squared);
    Fixed result = value;
    constexpr Fixed coefficients[] = {
        0x2aaa, 0x1333, 0x0b6d, 0x07c7, 0x05ba, 0x0471, 0x0393, 0x02f5, 0x027f,
    };
    for (Fixed coefficient : coefficients) {
        result = AddFixed(result, FixedMultiply(power, coefficient));
        power = FixedMultiply(power, squared);
    }
    const int16_t angle = FixedToAngle(FixedMultiply(result, 0x0517cc18));
    if (input < 0) {
        return static_cast<uint16_t>((static_cast<uint16_t>(angle) & 0x1fff) - 0x2000);
    }
    return static_cast<uint16_t>(angle) & 0x1fff;
}

uint16_t FixedArcCosine(Fixed input)
{
    return static_cast<uint16_t>(0x800 - FixedArcSine(input));
}

Vec3 RotateX(const Vec3& value, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    return {
        value.x,
        SubtractFixed(FixedMultiply(value.y, cosine), FixedMultiply(value.z, sine)),
        AddFixed(FixedMultiply(value.y, sine), FixedMultiply(value.z, cosine)),
    };
}

Vec3 RotateY(const Vec3& value, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    return {
        AddFixed(FixedMultiply(value.x, cosine), FixedMultiply(value.z, sine)),
        value.y,
        SubtractFixed(FixedMultiply(value.z, cosine), FixedMultiply(value.x, sine)),
    };
}

Vec3 RotateZ(const Vec3& value, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    return {
        SubtractFixed(FixedMultiply(value.x, cosine), FixedMultiply(value.y, sine)),
        AddFixed(FixedMultiply(value.x, sine), FixedMultiply(value.y, cosine)),
        value.z,
    };
}

Vec3 Rotate(const Vec3& value, uint16_t xAngle, uint16_t yAngle, uint16_t zAngle)
{
    const Fixed sinX = FixedSin(xAngle);
    const Fixed cosX = FixedCos(static_cast<int16_t>(xAngle));
    const Fixed sinY = FixedSin(yAngle);
    const Fixed cosY = FixedCos(static_cast<int16_t>(yAngle));
    const Fixed sinZ = FixedSin(zAngle);
    const Fixed cosZ = FixedCos(static_cast<int16_t>(zAngle));

    const Fixed m00 = AddFixed(FixedMultiply(FixedMultiply(sinY, sinX), sinZ),
                               FixedMultiply(cosX, cosZ));
    const Fixed m01 = SubtractFixed(FixedMultiply(FixedMultiply(sinY, cosX), sinZ),
                                    FixedMultiply(sinX, cosZ));
    const Fixed m02 = FixedMultiply(cosY, sinX);
    const Fixed m10 = FixedMultiply(cosY, sinZ);
    const Fixed m11 = FixedMultiply(cosY, cosZ);
    const Fixed m12 = NegateFixed(sinY);
    const Fixed m20 = SubtractFixed(FixedMultiply(FixedMultiply(sinY, sinX), cosZ),
                                    FixedMultiply(cosX, sinZ));
    const Fixed m21 = AddFixed(FixedMultiply(FixedMultiply(sinY, cosX), cosZ),
                               FixedMultiply(sinX, sinZ));
    const Fixed m22 = FixedMultiply(cosY, cosX);
    return {
        AddFixed(AddFixed(FixedMultiply(value.x, m00), FixedMultiply(value.y, m20)),
                 FixedMultiply(value.z, m02)),
        AddFixed(AddFixed(FixedMultiply(value.x, m10), FixedMultiply(value.y, m11)),
                 FixedMultiply(value.z, m12)),
        AddFixed(AddFixed(FixedMultiply(value.x, m01), FixedMultiply(value.y, m21)),
                 FixedMultiply(value.z, m22)),
    };
}

Vec3 LerpVec3(const Vec3& start, const Vec3& end, Fixed fraction)
{
    const auto interpolate = [fraction](Fixed first, Fixed last) {
        const int64_t product = static_cast<int64_t>(SubtractFixed(last, first)) * fraction;
        const Fixed delta = static_cast<Fixed>(static_cast<uint32_t>(
            static_cast<uint64_t>(product) >> 16));
        return AddFixed(first, delta);
    };
    return {interpolate(start.x, end.x), interpolate(start.y, end.y),
            interpolate(start.z, end.z)};
}

Vec3 MultiplyComponents(const Vec3& input, Fixed x, Fixed y, Fixed z)
{
    return {FixedMultiply(input.x, x), FixedMultiply(input.x, y), FixedMultiply(input.x, z)};
}

Vec3 AddComponents(const Vec3& input, Fixed x, Fixed y, Fixed z)
{
    return {AddFixed(input.x, x), AddFixed(input.x, y), AddFixed(input.x, z)};
}

Fixed SquaredDistance(const Vec3& first, const Vec3& second)
{
    return SquaredLength(Subtract(first, second));
}

int16_t Vec3AngleXZ(const Vec3& value)
{
    if (value.x >= 0 && value.z >= 0) {
        if (value.x > value.z) {
            return static_cast<int16_t>(FixedArcCosine(value.x));
        }
        return static_cast<int16_t>(FixedArcSine(value.z));
    }
    if (value.x <= 0 && value.z >= 0) {
        if (NegateFixed(value.x) > value.z) {
            return static_cast<int16_t>(0x1000 - FixedArcCosine(NegateFixed(value.x)));
        }
        return static_cast<int16_t>(0x1000 - FixedArcSine(value.z));
    }
    if (value.x <= 0 && value.z <= 0) {
        if (NegateFixed(value.x) > NegateFixed(value.z)) {
            return static_cast<int16_t>(FixedArcCosine(NegateFixed(value.x)) + 0x1000);
        }
        return static_cast<int16_t>(FixedArcSine(NegateFixed(value.z)) + 0x1000);
    }
    if (value.x > NegateFixed(value.z)) {
        return static_cast<int16_t>(0x2000 - FixedArcCosine(value.x));
    }
    return static_cast<int16_t>(0x2000 - FixedArcSine(NegateFixed(value.z)));
}

Vec2 Transform(const Matrix4& matrix, const Vec2& value)
{
    return {
        AddFixed(AddFixed(FixedMultiply(matrix.values[0], value.x),
                          FixedMultiply(matrix.values[4], value.y)), matrix.values[12]),
        AddFixed(AddFixed(FixedMultiply(matrix.values[1], value.x),
                          FixedMultiply(matrix.values[5], value.y)), matrix.values[13])
    };
}

Vec2 TransformPerspective(const Matrix4& matrix, const Vec2& value)
{
    const Fixed w = AddFixed(AddFixed(FixedMultiply(matrix.values[3], value.x),
                                      FixedMultiply(matrix.values[7], value.y)),
                             matrix.values[15]);
    const Fixed inverseW = FixedDivide(fixedOne, w);
    const Vec2 affine = Transform(matrix, value);
    return {FixedMultiply(affine.x, inverseW), FixedMultiply(affine.y, inverseW)};
}

Vec2 TransformDirection(const Matrix4& matrix, const Vec2& value)
{
    return {
        AddFixed(FixedMultiply(matrix.values[0], value.x),
                 FixedMultiply(matrix.values[4], value.y)),
        AddFixed(FixedMultiply(matrix.values[1], value.x),
                 FixedMultiply(matrix.values[5], value.y))
    };
}

Vec3 TransformPerspective(const Matrix4& matrix, const Vec3& value)
{
    const Fixed w = AddFixed(
        AddFixed(AddFixed(FixedMultiply(matrix.values[3], value.x),
                          FixedMultiply(matrix.values[7], value.y)),
                 FixedMultiply(matrix.values[11], value.z)), matrix.values[15]);
    const Fixed inverseW = FixedDivide(fixedOne, w);
    const auto transformRow = [&matrix, &value, inverseW](unsigned row) {
        const Fixed affine = AddFixed(
            AddFixed(AddFixed(FixedMultiply(matrix.values[row], value.x),
                              FixedMultiply(matrix.values[row + 4], value.y)),
                     FixedMultiply(matrix.values[row + 8], value.z)),
            matrix.values[row + 12]);
        return FixedMultiply(affine, inverseW);
    };
    return {transformRow(0), transformRow(1), transformRow(2)};
}

Vec3 TransformDirection(const Matrix4& matrix, const Vec3& value)
{
    const auto transformRow = [&matrix, &value](unsigned row) {
        return AddFixed(AddFixed(FixedMultiply(matrix.values[row], value.x),
                                 FixedMultiply(matrix.values[row + 4], value.y)),
                        FixedMultiply(matrix.values[row + 8], value.z));
    };
    return {transformRow(0), transformRow(1), transformRow(2)};
}

Vec4 NormalizeVec4(const Vec4& value)
{
    const Fixed inverseLength = FixedInverseSqrt(SquaredLength(value));
    return {FixedMultiply(value.x, inverseLength), FixedMultiply(value.y, inverseLength),
            FixedMultiply(value.z, inverseLength), FixedMultiply(value.w, inverseLength)};
}

Vec4 NormalizeRobust(const Vec4& value)
{
    const Fixed magnitude = AddFixed(AddFixed(AbsFixed(value.x), AbsFixed(value.y)),
                                     AbsFixed(value.z));
    Fixed threshold = 0xb50000;
    unsigned shift = 0;
    while (magnitude > threshold) {
        threshold = ShiftLeftFixed(threshold, 1);
        shift += 2;
    }
    const Fixed x = ShiftRightArm(value.x, shift);
    const Fixed y = ShiftRightArm(value.y, shift);
    const Fixed z = ShiftRightArm(value.z, shift);
    Fixed squaredLength = AddFixed(AddFixed(FixedMultiply(x, x), FixedMultiply(y, y)),
                                   FixedMultiply(z, z));
    if (squaredLength < 0x4000) {
        squaredLength = ShiftRightArm(FixedInverseSqrt(ShiftLeftFixed(squaredLength, 12)), shift);
        return {
            FixedMultiply(ShiftLeftFixed(value.x, 6), squaredLength),
            FixedMultiply(ShiftLeftFixed(value.y, 6), squaredLength),
            FixedMultiply(ShiftLeftFixed(value.z, 6), squaredLength),
            FixedMultiply(ShiftLeftFixed(value.w, 6), squaredLength)
        };
    }
    squaredLength = ShiftRightArm(FixedInverseSqrt(squaredLength), shift);
    return {FixedMultiply(value.x, squaredLength), FixedMultiply(value.y, squaredLength),
            FixedMultiply(value.z, squaredLength), FixedMultiply(value.w, squaredLength)};
}

Vec4& DivideVec4ByW(Vec4& output, const Vec4& input)
{
    if (input.w != fixedOne) {
        const Fixed inverseW = FixedDivide(fixedOne, input.w);
        output.x = FixedMultiply(input.x, inverseW);
        output.y = FixedMultiply(input.y, inverseW);
        output.z = FixedMultiply(input.z, inverseW);
        output.w = fixedOne;
    }
    return output;
}

Vec4 Cross3(const Vec4& first, const Vec4& second, const Vec4& third)
{
    const Fixed xy = SubtractFixed(FixedMultiply(second.x, third.y),
                                   FixedMultiply(second.y, third.x));
    const Fixed xz = SubtractFixed(FixedMultiply(second.x, third.z),
                                   FixedMultiply(second.z, third.x));
    const Fixed xw = SubtractFixed(FixedMultiply(second.x, third.w),
                                   FixedMultiply(second.w, third.x));
    const Fixed yz = SubtractFixed(FixedMultiply(second.y, third.z),
                                   FixedMultiply(second.z, third.y));
    const Fixed yw = SubtractFixed(FixedMultiply(second.y, third.w),
                                   FixedMultiply(second.w, third.y));
    const Fixed zw = SubtractFixed(FixedMultiply(second.z, third.w),
                                   FixedMultiply(second.w, third.z));
    return {
        AddFixed(SubtractFixed(FixedMultiply(zw, first.y), FixedMultiply(yw, first.z)),
                 FixedMultiply(yz, first.w)),
        AddFixed(SubtractFixed(FixedMultiply(NegateFixed(zw), first.x),
                               FixedMultiply(xw, first.z)), FixedMultiply(xz, first.w)),
        AddFixed(SubtractFixed(FixedMultiply(yw, first.x), FixedMultiply(xw, first.y)),
                 FixedMultiply(xy, first.w)),
        AddFixed(SubtractFixed(FixedMultiply(NegateFixed(yz), first.x),
                               FixedMultiply(xz, first.y)), FixedMultiply(xy, first.z))
    };
}

Vec4 LerpVec4(const Vec4& start, const Vec4& end, Fixed fraction)
{
    const auto interpolate = [fraction](Fixed first, Fixed last) {
        return AddFixed(first, FixedMultiply(SubtractFixed(last, first), fraction));
    };
    return {interpolate(start.x, end.x), interpolate(start.y, end.y),
            interpolate(start.z, end.z), interpolate(start.w, end.w)};
}

Vec4 QuaternionIdentity()
{
    return {0, 0, 0, fixedOne};
}

Vec4 QuaternionConjugate(const Vec4& value)
{
    return {NegateFixed(value.x), NegateFixed(value.y), NegateFixed(value.z), value.w};
}

Vec4 QuaternionInverse(const Vec4& value)
{
    const Fixed inverseLength = FixedInverseSqrt(SquaredLength(value));
    return {FixedMultiply(NegateFixed(value.x), inverseLength),
            FixedMultiply(NegateFixed(value.y), inverseLength),
            FixedMultiply(NegateFixed(value.z), inverseLength),
            FixedMultiply(value.w, inverseLength)};
}

Vec4 QuaternionFromAxisAngle(const Vec3& axis, int16_t angle)
{
    const Fixed sine = FixedSin(static_cast<uint16_t>(angle >> 1));
    return {FixedMultiply(axis.x, sine), FixedMultiply(axis.y, sine),
            FixedMultiply(axis.z, sine), FixedCos(static_cast<int16_t>(angle >> 1))};
}

Vec4 QuaternionFromMatrix(const Matrix4& matrix)
{
    const Fixed trace = AddFixed(AddFixed(matrix.values[0], matrix.values[5]), matrix.values[10]);
    if (trace > 0) {
        Fixed root = FixedSqrt(AddFixed(trace, fixedOne));
        const Fixed w = FixedMultiply(root, 0x8000);
        root = FixedDivide(0x8000, root);
        return {FixedMultiply(SubtractFixed(matrix.values[9], matrix.values[6]), root),
                FixedMultiply(SubtractFixed(matrix.values[2], matrix.values[8]), root),
                FixedMultiply(SubtractFixed(matrix.values[4], matrix.values[1]), root), w};
    }

    Fixed root;
    Vec4 result;
    if (matrix.values[5] > matrix.values[0]) {
        if (matrix.values[10] <= matrix.values[5]) {
            root = FixedSqrt(AddFixed(SubtractFixed(FixedMultiply(matrix.values[5], 0x20000),
                                                   trace), fixedOne));
            result.y = FixedMultiply(root, 0x8000);
            if (root >= 0x28f) {
                root = FixedDivide(0x8000, root);
            }
            result.x = FixedMultiply(AddFixed(matrix.values[1], matrix.values[4]), root);
            result.z = FixedMultiply(AddFixed(matrix.values[9], matrix.values[6]), root);
            result.w = FixedMultiply(SubtractFixed(matrix.values[2], matrix.values[8]), root);
            return result;
        }
    } else if (matrix.values[10] <= matrix.values[0]) {
        root = FixedSqrt(AddFixed(SubtractFixed(FixedMultiply(matrix.values[0], 0x20000),
                                               trace), fixedOne));
        result.x = FixedMultiply(root, 0x8000);
        if (root >= 0x28f) {
            root = FixedDivide(0x8000, root);
        }
        result.y = FixedMultiply(AddFixed(matrix.values[4], matrix.values[1]), root);
        result.z = FixedMultiply(AddFixed(matrix.values[8], matrix.values[2]), root);
        result.w = FixedMultiply(SubtractFixed(matrix.values[9], matrix.values[6]), root);
        return result;
    }

    root = FixedSqrt(AddFixed(SubtractFixed(FixedMultiply(matrix.values[10], 0x20000), trace),
                              fixedOne));
    result.z = FixedMultiply(root, 0x8000);
    if (root >= 0x28f) {
        root = FixedDivide(0x8000, root);
    }
    result.x = FixedMultiply(AddFixed(matrix.values[2], matrix.values[8]), root);
    result.y = FixedMultiply(AddFixed(matrix.values[6], matrix.values[9]), root);
    result.w = FixedMultiply(SubtractFixed(matrix.values[4], matrix.values[1]), root);
    return result;
}

Vec4 QuaternionFromEuler(int16_t xAngle, int16_t yAngle, int16_t zAngle)
{
    const Fixed sx = FixedSin(static_cast<uint16_t>(xAngle >> 1));
    const Fixed cx = FixedCos(static_cast<int16_t>(xAngle >> 1));
    const Fixed sy = FixedSin(static_cast<uint16_t>(yAngle >> 1));
    const Fixed cy = FixedCos(static_cast<int16_t>(yAngle >> 1));
    const Fixed sz = FixedSin(static_cast<uint16_t>(zAngle >> 1));
    const Fixed cz = FixedCos(static_cast<int16_t>(zAngle >> 1));
    return {
        AddFixed(FixedMultiply(FixedMultiply(cz, sy), cx),
                 FixedMultiply(FixedMultiply(sz, cy), sx)),
        SubtractFixed(FixedMultiply(FixedMultiply(cz, cy), sx),
                      FixedMultiply(FixedMultiply(sz, sy), cx)),
        SubtractFixed(FixedMultiply(FixedMultiply(sz, cy), cx),
                      FixedMultiply(FixedMultiply(cz, sy), sx)),
        AddFixed(FixedMultiply(FixedMultiply(cz, cy), cx),
                 FixedMultiply(FixedMultiply(sz, sy), sx))
    };
}

Vec4 QuaternionLerp(const Vec4& start, const Vec4& end, Fixed fraction)
{
    const Vec4 adjusted = Dot(start, end) < 0
        ? Vec4{NegateFixed(end.x), NegateFixed(end.y), NegateFixed(end.z), NegateFixed(end.w)}
        : end;
    return LerpVec4(start, adjusted, fraction);
}

void QuaternionToAxisAngle(const Vec4& value, Vec3& axis, int16_t& angle)
{
    Fixed scale = FixedSqrt(SubtractFixed(fixedOne, FixedMultiply(value.w, value.w)));
    if (scale >= 0x28f) {
        scale = FixedDivide(fixedOne, scale);
        axis = {FixedMultiply(value.x, scale), FixedMultiply(value.y, scale),
                FixedMultiply(value.z, scale)};
    } else {
        axis = {0, fixedOne, 0};
    }
    angle = static_cast<int16_t>(static_cast<uint16_t>(FixedArcCosine(value.w) << 1));
}

Matrix4 LookAt(const Vec3& position, const Vec3& target, const Vec3& up)
{
    const Vec3 forward = Normalize(Subtract(target, position));
    Vec3 right = Cross(up, forward);
    if (AbsFixed(right.x) <= 0x4000 && AbsFixed(right.y) <= 0x4000 &&
        AbsFixed(right.z) <= 0x4000) {
        right = ShiftLeft(right, 8);
    }
    right = Normalize(right);
    const Vec3 cameraUp = Cross(forward, right);
    Matrix4 result;
    result.values = {
        right.x, cameraUp.x, forward.x, 0,
        right.y, cameraUp.y, forward.y, 0,
        right.z, cameraUp.z, forward.z, 0,
        NegateFixed(Dot(right, position)), NegateFixed(Dot(cameraUp, position)),
        NegateFixed(Dot(forward, position)), fixedOne
    };
    return result;
}

Matrix4 RotateXMatrix(uint16_t angle)
{
    Matrix4 result = Matrix4::Identity();
    result.values[5] = FixedCos(static_cast<int16_t>(angle));
    result.values[6] = FixedSin(angle);
    result.values[9] = NegateFixed(result.values[6]);
    result.values[10] = result.values[5];
    return result;
}

void TranslateMatrix(Matrix4& matrix, Fixed x, Fixed y, Fixed z)
{
    matrix.values[12] = AddFixed(matrix.values[12], x);
    matrix.values[13] = AddFixed(matrix.values[13], y);
    matrix.values[14] = AddFixed(matrix.values[14], z);
}

Matrix4 RotateYMatrix(uint16_t angle)
{
    Matrix4 result = Matrix4::Identity();
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    result.values[0] = cosine;
    result.values[2] = NegateFixed(sine);
    result.values[8] = sine;
    result.values[10] = cosine;
    return result;
}

Matrix4 Transpose(const Matrix4& value)
{
    Matrix4 result;
    for (unsigned column = 0; column < 4; ++column) {
        for (unsigned row = 0; row < 4; ++row) {
            result.values[column * 4 + row] = value.values[row * 4 + column];
        }
    }
    return result;
}

} // namespace soft3d

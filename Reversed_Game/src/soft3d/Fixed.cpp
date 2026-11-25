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

Matrix4 RotateZMatrix(uint16_t angle)
{
    Matrix4 result = Matrix4::Identity();
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    result.values[0] = cosine;
    result.values[1] = sine;
    result.values[4] = NegateFixed(sine);
    result.values[5] = cosine;
    return result;
}

Matrix4 EulerMatrix(uint16_t xAngle, uint16_t yAngle, uint16_t zAngle)
{
    const Fixed sx = FixedSin(xAngle);
    const Fixed cx = FixedCos(static_cast<int16_t>(xAngle));
    const Fixed sy = FixedSin(yAngle);
    const Fixed cy = FixedCos(static_cast<int16_t>(yAngle));
    const Fixed sz = FixedSin(zAngle);
    const Fixed cz = FixedCos(static_cast<int16_t>(zAngle));
    const Fixed sysx = FixedMultiply(sy, sx);
    const Fixed sycx = FixedMultiply(sy, cx);
    Matrix4 result = Matrix4::Identity();
    result.values[0] = AddFixed(FixedMultiply(sysx, sz), FixedMultiply(cx, cz));
    result.values[1] = FixedMultiply(cy, sz);
    result.values[2] = SubtractFixed(FixedMultiply(sycx, sz), FixedMultiply(sx, cz));
    result.values[4] = SubtractFixed(FixedMultiply(sysx, cz), FixedMultiply(cx, sz));
    result.values[5] = FixedMultiply(cy, cz);
    result.values[6] = AddFixed(FixedMultiply(sycx, cz), FixedMultiply(sx, sz));
    result.values[8] = FixedMultiply(cy, sx);
    result.values[9] = NegateFixed(sy);
    result.values[10] = FixedMultiply(cy, cx);
    return result;
}

Matrix4 AxisRotationMatrix(const Vec3& axis, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    const Fixed oneMinusCosine = SubtractFixed(fixedOne, cosine);
    const Fixed xy = FixedMultiply(FixedMultiply(oneMinusCosine, axis.y), axis.x);
    const Fixed xz = FixedMultiply(FixedMultiply(oneMinusCosine, axis.z), axis.x);
    const Fixed xs = FixedMultiply(axis.x, sine);
    const Fixed ys = FixedMultiply(axis.y, sine);
    const Fixed zs = FixedMultiply(axis.z, sine);
    Matrix4 result = Matrix4::Identity();
    result.values[0] = AddFixed(
        FixedMultiply(FixedMultiply(oneMinusCosine, axis.x), axis.x), cosine);
    result.values[1] = SubtractFixed(xy, zs);
    result.values[2] = AddFixed(xz, ys);
    result.values[4] = AddFixed(xy, zs);
    result.values[5] = AddFixed(
        FixedMultiply(FixedMultiply(oneMinusCosine, axis.y), axis.y), cosine);
    result.values[6] = SubtractFixed(xz, xs);
    result.values[8] = SubtractFixed(xz, ys);
    result.values[9] = AddFixed(FixedMultiply(FixedMultiply(oneMinusCosine, axis.z), axis.y), xs);
    result.values[10] = AddFixed(
        FixedMultiply(FixedMultiply(oneMinusCosine, axis.z), axis.z), cosine);
    return result;
}

Matrix4 TranslationMatrix(Fixed x, Fixed y, Fixed z)
{
    Matrix4 result = Matrix4::Identity();
    result.values[12] = x;
    result.values[13] = y;
    result.values[14] = z;
    return result;
}

Matrix4& ScaleMatrix(Matrix4& matrix, Fixed x, Fixed y, Fixed z)
{
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        matrix.values[offset] = FixedMultiply(matrix.values[offset], x);
        matrix.values[offset + 1] = FixedMultiply(matrix.values[offset + 1], y);
        matrix.values[offset + 2] = FixedMultiply(matrix.values[offset + 2], z);
    }
    return matrix;
}

Matrix4 ScaleMatrix(Fixed x, Fixed y, Fixed z)
{
    Matrix4 result;
    result.values = {x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, fixedOne};
    return result;
}

Matrix4& RotateX(Matrix4& matrix, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        const Fixed first = matrix.values[offset + 1];
        const Fixed second = matrix.values[offset + 2];
        matrix.values[offset + 1] = SubtractFixed(FixedMultiply(first, cosine),
                                                  FixedMultiply(second, sine));
        matrix.values[offset + 2] = AddFixed(FixedMultiply(first, sine),
                                             FixedMultiply(second, cosine));
    }
    return matrix;
}

Matrix4& RotateY(Matrix4& matrix, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        const Fixed first = matrix.values[offset];
        const Fixed second = matrix.values[offset + 2];
        matrix.values[offset] = AddFixed(FixedMultiply(first, cosine),
                                         FixedMultiply(second, sine));
        matrix.values[offset + 2] = SubtractFixed(FixedMultiply(second, cosine),
                                                  FixedMultiply(first, sine));
    }
    return matrix;
}

Matrix4& RotateZ(Matrix4& matrix, uint16_t angle)
{
    const Fixed sine = FixedSin(angle);
    const Fixed cosine = FixedCos(static_cast<int16_t>(angle));
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        const Fixed first = matrix.values[offset];
        const Fixed second = matrix.values[offset + 1];
        matrix.values[offset] = SubtractFixed(FixedMultiply(first, cosine),
                                              FixedMultiply(second, sine));
        matrix.values[offset + 1] = AddFixed(FixedMultiply(first, sine),
                                             FixedMultiply(second, cosine));
    }
    return matrix;
}

Matrix4& RotateEuler(Matrix4& matrix, uint16_t xAngle, uint16_t yAngle, uint16_t zAngle)
{
    const Matrix4 rotation = EulerMatrix(xAngle, yAngle, zAngle);
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        const Fixed x = matrix.values[offset];
        const Fixed y = matrix.values[offset + 1];
        const Fixed z = matrix.values[offset + 2];
        matrix.values[offset] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[0]),
                                                  FixedMultiply(y, rotation.values[4])),
                                         FixedMultiply(z, rotation.values[8]));
        matrix.values[offset + 1] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[1]),
                                                      FixedMultiply(y, rotation.values[5])),
                                             FixedMultiply(z, rotation.values[9]));
        matrix.values[offset + 2] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[2]),
                                                      FixedMultiply(y, rotation.values[6])),
                                             FixedMultiply(z, rotation.values[10]));
    }
    return matrix;
}

Matrix4& RotateAxis(Matrix4& matrix, const Vec3& axis, uint16_t angle)
{
    const Matrix4 rotation = AxisRotationMatrix(axis, angle);
    for (unsigned column = 0; column < 4; ++column) {
        const unsigned offset = column * 4;
        const Fixed x = matrix.values[offset];
        const Fixed y = matrix.values[offset + 1];
        const Fixed z = matrix.values[offset + 2];
        matrix.values[offset] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[0]),
                                                  FixedMultiply(y, rotation.values[4])),
                                         FixedMultiply(z, rotation.values[8]));
        matrix.values[offset + 1] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[1]),
                                                      FixedMultiply(y, rotation.values[5])),
                                             FixedMultiply(z, rotation.values[9]));
        matrix.values[offset + 2] = AddFixed(AddFixed(FixedMultiply(x, rotation.values[2]),
                                                      FixedMultiply(y, rotation.values[6])),
                                             FixedMultiply(z, rotation.values[10]));
    }
    return matrix;
}

Fixed Determinant2x2(Fixed a, Fixed b, Fixed c, Fixed d)
{
    return SubtractFixed(FixedMultiply(a, d), FixedMultiply(b, c));
}

Fixed Determinant3x3(Fixed a, Fixed b, Fixed c, Fixed d, Fixed e, Fixed f,
                     Fixed g, Fixed h, Fixed i)
{
    return AddFixed(SubtractFixed(FixedMultiply(a, Determinant2x2(e, f, h, i)),
                                  FixedMultiply(d, Determinant2x2(b, c, h, i))),
                    FixedMultiply(g, Determinant2x2(b, c, e, f)));
}

Fixed Determinant(const Matrix4& m)
{
    const auto& v = m.values;
    return SubtractFixed(
        AddFixed(SubtractFixed(
            FixedMultiply(v[0], Determinant3x3(v[5], v[9], v[13], v[6], v[10], v[14],
                                               v[7], v[11], v[15])),
            FixedMultiply(v[1], Determinant3x3(v[4], v[8], v[12], v[6], v[10], v[14],
                                               v[7], v[11], v[15]))),
            FixedMultiply(v[2], Determinant3x3(v[4], v[8], v[12], v[5], v[9], v[13],
                                               v[7], v[11], v[15]))),
        FixedMultiply(v[3], Determinant3x3(v[4], v[8], v[12], v[5], v[9], v[13],
                                           v[6], v[10], v[14])));
}

Matrix4 Inverse(const Matrix4& matrix)
{
    const auto& v = matrix.values;
    const Fixed inverseDeterminant = FixedDivide(fixedOne, Determinant(matrix));
    Matrix4 result;
    result.values[0] = FixedMultiply(Determinant3x3(v[5], v[9], v[13], v[6], v[10], v[14],
                                                    v[7], v[11], v[15]), inverseDeterminant);
    result.values[4] = FixedMultiply(NegateFixed(Determinant3x3(
        v[4], v[8], v[12], v[6], v[10], v[14], v[7], v[11], v[15])), inverseDeterminant);
    result.values[8] = FixedMultiply(Determinant3x3(v[4], v[8], v[12], v[5], v[9], v[13],
                                                    v[7], v[11], v[15]), inverseDeterminant);
    result.values[12] = FixedMultiply(NegateFixed(Determinant3x3(
        v[4], v[8], v[12], v[5], v[9], v[13], v[6], v[10], v[14])), inverseDeterminant);
    result.values[1] = FixedMultiply(NegateFixed(Determinant3x3(
        v[1], v[9], v[13], v[2], v[10], v[14], v[3], v[11], v[15])), inverseDeterminant);
    result.values[5] = FixedMultiply(Determinant3x3(v[0], v[8], v[12], v[2], v[10], v[14],
                                                    v[3], v[11], v[15]), inverseDeterminant);
    result.values[9] = FixedMultiply(NegateFixed(Determinant3x3(
        v[0], v[8], v[12], v[1], v[9], v[13], v[3], v[11], v[15])), inverseDeterminant);
    result.values[13] = FixedMultiply(Determinant3x3(v[0], v[8], v[12], v[1], v[9], v[13],
                                                     v[2], v[10], v[14]), inverseDeterminant);
    result.values[2] = FixedMultiply(Determinant3x3(v[1], v[5], v[13], v[2], v[6], v[14],
                                                    v[3], v[7], v[15]), inverseDeterminant);
    result.values[6] = FixedMultiply(NegateFixed(Determinant3x3(
        v[0], v[4], v[12], v[2], v[6], v[14], v[3], v[7], v[15])), inverseDeterminant);
    result.values[10] = FixedMultiply(Determinant3x3(v[0], v[4], v[12], v[1], v[5], v[13],
                                                     v[3], v[7], v[15]), inverseDeterminant);
    result.values[14] = FixedMultiply(NegateFixed(Determinant3x3(
        v[0], v[4], v[12], v[1], v[5], v[13], v[2], v[6], v[14])), inverseDeterminant);
    result.values[3] = FixedMultiply(NegateFixed(Determinant3x3(
        v[1], v[5], v[9], v[2], v[6], v[10], v[3], v[7], v[11])), inverseDeterminant);
    result.values[7] = FixedMultiply(Determinant3x3(v[0], v[4], v[8], v[2], v[6], v[10],
                                                    v[3], v[7], v[11]), inverseDeterminant);
    result.values[11] = FixedMultiply(NegateFixed(Determinant3x3(
        v[0], v[4], v[8], v[1], v[5], v[9], v[3], v[7], v[11])), inverseDeterminant);
    result.values[15] = FixedMultiply(Determinant3x3(v[0], v[4], v[8], v[1], v[5], v[9],
                                                     v[2], v[6], v[10]), inverseDeterminant);
    return result;
}

Matrix4 Inverse3x3(const Matrix4& matrix)
{
    const auto& v = matrix.values;
    const Fixed c00 = SubtractFixed(FixedMultiply(v[5], v[10]), FixedMultiply(v[6], v[9]));
    const Fixed c10 = SubtractFixed(FixedMultiply(v[6], v[8]), FixedMultiply(v[4], v[10]));
    const Fixed c20 = SubtractFixed(FixedMultiply(v[4], v[9]), FixedMultiply(v[5], v[8]));
    const Fixed determinant = AddFixed(AddFixed(FixedMultiply(v[0], c00),
                                                FixedMultiply(v[1], c10)),
                                       FixedMultiply(v[2], c20));
    if (AbsFixed(determinant) < 0x28f) {
        return matrix;
    }
    const Fixed inverseDeterminant = FixedDivide(fixedOne, determinant);
    Matrix4 result;
    result.values[0] = FixedMultiply(c00, inverseDeterminant);
    result.values[1] = FixedMultiply(SubtractFixed(FixedMultiply(v[2], v[9]),
                                                   FixedMultiply(v[1], v[10])), inverseDeterminant);
    result.values[2] = FixedMultiply(SubtractFixed(FixedMultiply(v[1], v[6]),
                                                   FixedMultiply(v[2], v[5])), inverseDeterminant);
    result.values[4] = FixedMultiply(c10, inverseDeterminant);
    result.values[5] = FixedMultiply(SubtractFixed(FixedMultiply(v[0], v[10]),
                                                   FixedMultiply(v[2], v[8])), inverseDeterminant);
    result.values[6] = FixedMultiply(SubtractFixed(FixedMultiply(v[2], v[4]),
                                                   FixedMultiply(v[0], v[6])), inverseDeterminant);
    result.values[8] = FixedMultiply(c20, inverseDeterminant);
    result.values[9] = FixedMultiply(SubtractFixed(FixedMultiply(v[1], v[8]),
                                                   FixedMultiply(v[0], v[9])), inverseDeterminant);
    result.values[10] = FixedMultiply(SubtractFixed(FixedMultiply(v[0], v[5]),
                                                    FixedMultiply(v[1], v[4])), inverseDeterminant);
    return result;
}

Matrix4 OrthographicMatrix(Fixed width, Fixed height, Fixed nearDistance, Fixed farDistance)
{
    const Fixed depth = FixedDivide(fixedOne, SubtractFixed(farDistance, nearDistance));
    Matrix4 result;
    result.values = {FixedDivide(0x20000, width), 0, 0, 0,
                     0, FixedDivide(0x20000, height), 0, 0,
                     0, 0, depth, 0,
                     0, 0, FixedMultiply(NegateFixed(depth), nearDistance), fixedOne};
    return result;
}

Matrix4 OrthographicMatrixNegativeDepth(Fixed width, Fixed height, Fixed nearDistance,
                                        Fixed farDistance)
{
    const Fixed depth = FixedDivide(fixedOne, SubtractFixed(nearDistance, farDistance));
    Matrix4 result;
    result.values = {FixedDivide(0x20000, width), 0, 0, 0,
                     0, FixedDivide(0x20000, height), 0, 0,
                     0, 0, depth, 0,
                     0, 0, FixedMultiply(depth, nearDistance), fixedOne};
    return result;
}

Matrix4 OrthographicBoundsMatrix(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                 Fixed nearDistance, Fixed farDistance)
{
    const Fixed inverseWidth = FixedDivide(fixedOne, SubtractFixed(left, right));
    const Fixed inverseHeight = FixedDivide(fixedOne, SubtractFixed(bottom, top));
    const Fixed depth = FixedDivide(fixedOne, SubtractFixed(farDistance, nearDistance));
    Matrix4 result;
    result.values = {FixedMultiply(0xfffe0000, inverseWidth), 0, 0, 0,
                     0, FixedMultiply(0xfffe0000, inverseHeight), 0, 0,
                     0, 0, depth, 0,
                     FixedMultiply(AddFixed(left, right), inverseWidth),
                     FixedMultiply(AddFixed(bottom, top), inverseHeight),
                     FixedMultiply(NegateFixed(depth), nearDistance), fixedOne};
    return result;
}

Matrix4 OrthographicBoundsMatrixNegativeDepth(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                              Fixed nearDistance, Fixed farDistance)
{
    Matrix4 result = OrthographicBoundsMatrix(left, right, bottom, top, nearDistance, farDistance);
    const Fixed depth = FixedDivide(fixedOne, SubtractFixed(nearDistance, farDistance));
    result.values[10] = depth;
    result.values[14] = FixedMultiply(depth, nearDistance);
    return result;
}

Matrix4 PerspectiveTanMatrix(int16_t angle, Fixed aspectRatio, Fixed nearDistance,
                             Fixed farDistance)
{
    const Fixed scaleY = FixedDivide(fixedOne, FixedTan(static_cast<uint16_t>(angle >> 1)));
    const Fixed depth = FixedDivide(farDistance, SubtractFixed(nearDistance, farDistance));
    Matrix4 result;
    result.values = {FixedDivide(scaleY, aspectRatio), 0, 0, 0,
                     0, scaleY, 0, 0,
                     0, 0, depth, NegateFixed(fixedOne),
                     0, 0, FixedMultiply(nearDistance, depth), 0};
    return result;
}

Matrix4 PerspectiveMatrix(Fixed width, Fixed height, Fixed nearDistance, Fixed farDistance)
{
    const Fixed doubleNear = FixedMultiply(0x20000, nearDistance);
    const Fixed depth = FixedDivide(farDistance, SubtractFixed(farDistance, nearDistance));
    Matrix4 result;
    result.values = {FixedDivide(doubleNear, width), 0, 0, 0,
                     0, FixedDivide(doubleNear, height), 0, 0,
                     0, 0, depth, fixedOne,
                     0, 0, FixedMultiply(NegateFixed(nearDistance), depth), 0};
    return result;
}

Matrix4 PerspectiveMatrixNegativeDepth(Fixed width, Fixed height, Fixed nearDistance,
                                       Fixed farDistance)
{
    const Fixed doubleNear = FixedMultiply(0x20000, nearDistance);
    const Fixed depth = FixedDivide(farDistance, SubtractFixed(nearDistance, farDistance));
    Matrix4 result;
    result.values = {FixedDivide(doubleNear, width), 0, 0, 0,
                     0, FixedDivide(doubleNear, height), 0, 0,
                     0, 0, depth, NegateFixed(fixedOne),
                     0, 0, FixedMultiply(nearDistance, depth), 0};
    return result;
}

Matrix4 FrustumMatrix(Fixed left, Fixed right, Fixed bottom, Fixed top,
                      Fixed nearDistance, Fixed farDistance)
{
    const Fixed doubleNear = FixedMultiply(0xfffe0000, nearDistance);
    const Fixed inverseWidth = FixedDivide(fixedOne, SubtractFixed(left, right));
    const Fixed inverseHeight = FixedDivide(fixedOne, SubtractFixed(bottom, top));
    const Fixed depth = FixedDivide(farDistance, SubtractFixed(farDistance, nearDistance));
    Matrix4 result;
    result.values = {FixedMultiply(doubleNear, inverseWidth), 0, 0, 0,
                     0, FixedMultiply(doubleNear, inverseHeight), 0, 0,
                     FixedMultiply(AddFixed(left, right), inverseWidth),
                     FixedMultiply(AddFixed(bottom, top), inverseHeight),
                     depth, fixedOne,
                     0, 0, FixedMultiply(NegateFixed(nearDistance), depth), 0};
    return result;
}

Matrix4 FrustumMatrixNegativeDepth(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                   Fixed nearDistance, Fixed farDistance)
{
    const Fixed doubleNear = FixedMultiply(0x20000, nearDistance);
    const Fixed inverseWidth = FixedDivide(fixedOne, SubtractFixed(right, left));
    const Fixed inverseHeight = FixedDivide(fixedOne, SubtractFixed(top, bottom));
    const Fixed depth = FixedDivide(farDistance, SubtractFixed(nearDistance, farDistance));
    Matrix4 result;
    result.values = {FixedMultiply(doubleNear, inverseWidth), 0, 0, 0,
                     0, FixedMultiply(doubleNear, inverseHeight), 0, 0,
                     FixedMultiply(AddFixed(left, right), inverseWidth),
                     FixedMultiply(AddFixed(bottom, top), inverseHeight),
                     depth, NegateFixed(fixedOne),
                     0, 0, FixedMultiply(nearDistance, depth), 0};
    return result;
}

Matrix4 ViewportMatrix(int32_t left, int32_t top, uint32_t width, uint32_t height,
                       Fixed nearDistance, Fixed farDistance)
{
    const Fixed halfWidth = static_cast<Fixed>(((width >> 1) & 0xffffU) << 16);
    const Fixed halfHeight = static_cast<Fixed>(((height >> 1) & 0xffffU) << 16);
    Matrix4 result;
    result.values = {halfWidth, 0, 0, 0,
                     0, NegateFixed(halfHeight), 0, 0,
                     0, 0, SubtractFixed(farDistance, nearDistance), 0,
                     AddFixed(ShiftLeftFixed(left, 16), halfWidth),
                     AddFixed(ShiftLeftFixed(top, 16), halfHeight),
                     nearDistance, fixedOne};
    return result;
}

Matrix4 ReflectionMatrix(const Vec4& plane)
{
    const Fixed nx = FixedMultiply(0xfffe0000, plane.x);
    const Fixed ny = FixedMultiply(0xfffe0000, plane.y);
    const Fixed nw = FixedMultiply(0xfffe0000, plane.w);
    const Fixed xy = FixedMultiply(nx, plane.y);
    const Fixed xz = FixedMultiply(nx, plane.z);
    const Fixed yz = FixedMultiply(ny, plane.z);
    Matrix4 result;
    result.values = {AddFixed(FixedMultiply(nx, plane.x), fixedOne), xy, xz, 0,
                     xy, AddFixed(FixedMultiply(ny, plane.y), fixedOne), yz, 0,
                     xz, yz, AddFixed(FixedMultiply(FixedMultiply(0xfffe0000, plane.z), plane.z),
                                      fixedOne), 0,
                     FixedMultiply(nw, plane.x), FixedMultiply(nw, plane.y),
                     FixedMultiply(nw, plane.z), fixedOne};
    return result;
}

Matrix4 PlaneProjectionMatrix(const Vec3& normal, const Vec4& coefficients)
{
    Matrix4 result;
    result.values = {
        AddFixed(FixedMultiply(NegateFixed(coefficients.x), normal.x), fixedOne),
        FixedMultiply(NegateFixed(coefficients.x), normal.y),
        FixedMultiply(NegateFixed(coefficients.x), normal.z), 0,
        FixedMultiply(NegateFixed(coefficients.y), normal.x),
        AddFixed(FixedMultiply(NegateFixed(coefficients.y), normal.y), fixedOne),
        FixedMultiply(NegateFixed(coefficients.y), normal.z), 0,
        FixedMultiply(NegateFixed(coefficients.z), normal.x),
        FixedMultiply(NegateFixed(coefficients.z), normal.y),
        AddFixed(FixedMultiply(NegateFixed(coefficients.z), normal.z), fixedOne), 0,
        FixedMultiply(NegateFixed(coefficients.w), normal.x),
        FixedMultiply(NegateFixed(coefficients.w), normal.y),
        FixedMultiply(NegateFixed(coefficients.w), normal.z), fixedOne
    };
    return result;
}

Matrix4 AxesMatrix(const Vec3& x, const Vec3& y, const Vec3& z)
{
    Matrix4 result;
    result.values = {x.x, y.x, z.x, 0, x.y, y.y, z.y, 0,
                     x.z, y.z, z.z, 0, 0, 0, 0, fixedOne};
    return result;
}

Matrix4 MatrixFromValues(Fixed m0, Fixed m1, Fixed m2, Fixed m3,
                         Fixed m4, Fixed m5, Fixed m6, Fixed m7,
                         Fixed m8, Fixed m9, Fixed m10, Fixed m11,
                         Fixed m12, Fixed m13, Fixed m14, Fixed m15)
{
    Matrix4 result;
    result.values = {m0, m1, m2, m3, m4, m5, m6, m7,
                     m8, m9, m10, m11, m12, m13, m14, m15};
    return result;
}

Vec4 TransformVec4(const Matrix4& matrix, const Vec4& value)
{
    const auto transformRow = [&matrix, &value](unsigned row) {
        return AddFixed(AddFixed(AddFixed(FixedMultiply(matrix.values[row], value.x),
                                          FixedMultiply(matrix.values[row + 4], value.y)),
                                 FixedMultiply(matrix.values[row + 8], value.z)),
                        FixedMultiply(matrix.values[row + 12], value.w));
    };
    return {transformRow(0), transformRow(1), transformRow(2), transformRow(3)};
}

Vec4 TransformInverse(const Matrix4& matrix, const Vec4& value)
{
    return TransformVec4(Inverse(matrix), value);
}

Vec3 Project(const Vec3& input, const Matrix4& world, const Matrix4& view,
             const Matrix4& projection, const Matrix4& viewport)
{
    Vec4 current{input.x, input.y, input.z, fixedOne};
    current = TransformVec4(world, current);
    current = TransformVec4(view, current);
    current = TransformVec4(projection, current);
    const Fixed inverseW = FixedDivide(fixedOne, current.w);
    return Transform(viewport, Vec3{FixedMultiply(current.x, inverseW),
                                    FixedMultiply(current.y, inverseW),
                                    FixedMultiply(current.z, inverseW)});
}

Vec3 Unproject(const Vec3& input, const Matrix4& world, const Matrix4& view,
               const Matrix4& projection, const Matrix4& viewport)
{
    Vec3 result = Transform(Inverse(viewport), input);
    result = Transform(Inverse(projection), result);
    result = Transform(Inverse(view), result);
    return Transform(Inverse(world), result);
}

Matrix4 QuaternionToMatrix(const Vec4& q)
{
    const Fixed x2 = FixedMultiply(0x20000, q.x);
    const Fixed y2 = FixedMultiply(0x20000, q.y);
    const Fixed z2 = FixedMultiply(0x20000, q.z);
    const Fixed wx = FixedMultiply(q.w, x2);
    const Fixed wy = FixedMultiply(q.w, y2);
    const Fixed wz = FixedMultiply(q.w, z2);
    const Fixed xx = FixedMultiply(q.x, x2);
    const Fixed xy = FixedMultiply(q.x, y2);
    const Fixed xz = FixedMultiply(q.x, z2);
    const Fixed yy = FixedMultiply(q.y, y2);
    const Fixed yz = FixedMultiply(q.y, z2);
    const Fixed zz = FixedMultiply(q.z, z2);
    Matrix4 result;
    result.values = {
        SubtractFixed(fixedOne, AddFixed(yy, zz)),
        AddFixed(xy, wz),
        SubtractFixed(xz, wy), 0,
        SubtractFixed(xy, wz),
        SubtractFixed(fixedOne, AddFixed(xx, zz)),
        AddFixed(yz, wx), 0,
        AddFixed(xz, wy),
        SubtractFixed(yz, wx),
        SubtractFixed(fixedOne, AddFixed(xx, yy)), 0,
        0, 0, 0, fixedOne
    };
    return result;
}

} // namespace soft3d

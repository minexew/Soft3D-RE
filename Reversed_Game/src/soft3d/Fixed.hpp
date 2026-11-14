#pragma once

#include <array>
#include <cstdint>

namespace soft3d {

using Fixed = int32_t;

constexpr Fixed fixedOne = 0x10000;

inline Fixed FixedMultiply(Fixed left, Fixed right)
{
    const uint64_t product = static_cast<uint64_t>(static_cast<int64_t>(left) *
                                                  static_cast<int64_t>(right));
    return static_cast<Fixed>(static_cast<uint32_t>(product >> 16));
}

inline Fixed FixedDivide(Fixed numerator, Fixed denominator)
{
    if (denominator == 0) {
        return 0;
    }
    return static_cast<Fixed>((static_cast<int64_t>(numerator) << 16) / denominator);
}

inline Fixed AddFixed(Fixed left, Fixed right)
{
    return static_cast<Fixed>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

inline Fixed SubtractFixed(Fixed left, Fixed right)
{
    return static_cast<Fixed>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

inline Fixed NegateFixed(Fixed value)
{
    return static_cast<Fixed>(0U - static_cast<uint32_t>(value));
}

inline Fixed AbsFixed(Fixed value)
{
    return value < 0 ? NegateFixed(value) : value;
}

inline Fixed ShiftLeftFixed(Fixed value, unsigned shift)
{
    return static_cast<Fixed>(static_cast<uint32_t>(value) << (shift & 0xff));
}

inline Fixed FixedSqrt(Fixed value)
{
    int32_t remainder = value;
    int32_t root = 0;
    if (remainder >= 0x40000000) {
        remainder = SubtractFixed(remainder, 0x40000000);
        root = 0x40000000;
    }
    for (int32_t bit : {0x10000000, 0x04000000, 0x01000000, 0x00400000, 0x00100000,
                        0x00040000, 0x00010000, 0x00004000, 0x00001000, 0x00000400,
                        0x00000100, 0x00000040, 0x00000010, 0x00000004}) {
        const int32_t trial = AddFixed(root, bit);
        root >>= 1;
        if (trial <= remainder) {
            remainder = SubtractFixed(remainder, trial);
            root |= bit;
        }
    }
    const int32_t trial = AddFixed(root, 1);
    root >>= 1;
    if (trial <= remainder) {
        remainder = SubtractFixed(remainder, trial);
        root |= 1;
    }
    if (root < remainder) {
        root = AddFixed(root, 1);
    }
    return ShiftLeftFixed(root, 8);
}

inline Fixed FixedInverseSqrt(Fixed value)
{
    int32_t result = 0;
    int32_t sum = 0;
    int32_t estimate = 0;
    const int32_t quarter = value >> 2;
    if (quarter < 0x4001) {
        result = fixedOne;
        sum = value >> 1;
        estimate = quarter;
    }
    for (unsigned step = 1; step <= 13; ++step) {
        const int32_t trial = AddFixed(AddFixed(sum >> step, estimate),
                                       value >> (step * 2 + 2));
        if (trial < 0x4001) {
            result = AddFixed(result, static_cast<Fixed>(fixedOne >> step));
            sum = AddFixed(sum, step == 1 ? quarter : value >> (step + 1));
            estimate = trial;
        }
    }
    if (AddFixed(AddFixed(sum >> 14, estimate), value >> 30) < 0x4001) {
        result = AddFixed(result, 4);
    }
    return result;
}

struct Vec3 {
    Fixed x = 0;
    Fixed y = 0;
    Fixed z = 0;
};

inline Vec3 Add(const Vec3& left, const Vec3& right)
{
    return {AddFixed(left.x, right.x), AddFixed(left.y, right.y), AddFixed(left.z, right.z)};
}

inline Vec3 Subtract(const Vec3& left, const Vec3& right)
{
    return {SubtractFixed(left.x, right.x), SubtractFixed(left.y, right.y),
            SubtractFixed(left.z, right.z)};
}

inline Vec3 Scale(const Vec3& value, Fixed scale)
{
    return {FixedMultiply(value.x, scale), FixedMultiply(value.y, scale),
            FixedMultiply(value.z, scale)};
}

inline Vec3 ShiftLeft(const Vec3& value, unsigned shift)
{
    return {ShiftLeftFixed(value.x, shift), ShiftLeftFixed(value.y, shift),
            ShiftLeftFixed(value.z, shift)};
}

inline Vec3 ShiftRight(const Vec3& value, unsigned shift)
{
    return {value.x >> (shift & 0xff), value.y >> (shift & 0xff), value.z >> (shift & 0xff)};
}

inline Fixed Dot(const Vec3& left, const Vec3& right)
{
    const int64_t product = static_cast<int64_t>(left.x) * right.x +
                            static_cast<int64_t>(left.y) * right.y +
                            static_cast<int64_t>(left.z) * right.z;
    const uint64_t bits = static_cast<uint64_t>(product);
    return static_cast<Fixed>(static_cast<uint32_t>(bits >> 16));
}

inline Vec3 Cross(const Vec3& left, const Vec3& right)
{
    return {
        SubtractFixed(FixedMultiply(left.y, right.z), FixedMultiply(left.z, right.y)),
        SubtractFixed(FixedMultiply(left.z, right.x), FixedMultiply(left.x, right.z)),
        SubtractFixed(FixedMultiply(left.x, right.y), FixedMultiply(left.y, right.x))
    };
}

inline Fixed SquaredLength(const Vec3& value)
{
    return AddFixed(AddFixed(FixedMultiply(value.x, value.x),
                             FixedMultiply(value.y, value.y)),
                    FixedMultiply(value.z, value.z));
}

inline Fixed Length(const Vec3& value)
{
    Fixed magnitude = AddFixed(AddFixed(AbsFixed(value.x), AbsFixed(value.y)), AbsFixed(value.z));
    Fixed threshold = 0xb50000;
    unsigned shift = 0;
    while (magnitude > threshold) {
        threshold = ShiftLeftFixed(threshold, 1);
        shift += 2;
    }
    const Vec3 scaled = ShiftRight(value, shift);
    return ShiftLeftFixed(FixedSqrt(SquaredLength(scaled)), shift);
}

inline Vec3 Normalize(const Vec3& value)
{
    Fixed magnitude = AddFixed(AddFixed(AbsFixed(value.x), AbsFixed(value.y)), AbsFixed(value.z));
    Fixed threshold = 0xb50000;
    unsigned shift = 0;
    while (magnitude > threshold) {
        threshold = ShiftLeftFixed(threshold, 1);
        shift += 2;
    }
    const Vec3 scaled = ShiftRight(value, shift);
    Fixed squaredLength = SquaredLength(scaled);
    if (squaredLength < 0x4000) {
        squaredLength = FixedInverseSqrt(ShiftLeftFixed(squaredLength, 12)) >> (shift & 0xff);
        return Scale(ShiftLeft(value, 6), squaredLength);
    }
    squaredLength = FixedInverseSqrt(squaredLength) >> (shift & 0xff);
    return Scale(value, squaredLength);
}

struct Matrix4 {
    std::array<Fixed, 16> values{};

    static Matrix4 Identity()
    {
        Matrix4 result;
        result.values[0] = fixedOne;
        result.values[5] = fixedOne;
        result.values[10] = fixedOne;
        result.values[15] = fixedOne;
        return result;
    }
};

inline Matrix4 Multiply(const Matrix4& left, const Matrix4& right)
{
    Matrix4 result;
    for (unsigned column = 0; column < 4; ++column) {
        for (unsigned row = 0; row < 4; ++row) {
            Fixed value = 0;
            for (unsigned index = 0; index < 4; ++index) {
                value = static_cast<Fixed>(static_cast<uint32_t>(value) +
                    static_cast<uint32_t>(FixedMultiply(left.values[column * 4 + row],
                                                        right.values[index * 4 + row])));
            }
            result.values[column * 4 + row] = value;
        }
    }
    return result;
}

inline Vec3 Transform(const Matrix4& matrix, const Vec3& value)
{
    return {
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[0], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[4], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[8], value.z)) +
                           static_cast<uint32_t>(matrix.values[12])),
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[1], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[5], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[9], value.z)) +
                           static_cast<uint32_t>(matrix.values[13])),
        static_cast<Fixed>(static_cast<uint32_t>(FixedMultiply(matrix.values[2], value.x)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[6], value.y)) +
                           static_cast<uint32_t>(FixedMultiply(matrix.values[10], value.z)) +
                           static_cast<uint32_t>(matrix.values[14]))
    };
}

} // namespace soft3d

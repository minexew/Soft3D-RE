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

struct Vec3 {
    Fixed x = 0;
    Fixed y = 0;
    Fixed z = 0;
};

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

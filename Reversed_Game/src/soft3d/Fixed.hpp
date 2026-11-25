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
    return static_cast<Fixed>((static_cast<int64_t>(numerator) * 0x10000) / denominator);
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

int32_t IntPow(int32_t base, int32_t exponent);

inline Fixed FixedToInt(Fixed value)
{
    return value >> 16;
}

inline Fixed FixedToIntCeil(Fixed value)
{
    return AddFixed(value, 0xffff) >> 16;
}

inline uint32_t FixedToUnsignedByte(Fixed value)
{
    return static_cast<uint32_t>(value >> 16) & 0xff;
}

inline int16_t FixedToAngle(Fixed value)
{
    return static_cast<int16_t>(static_cast<uint32_t>(value) >> 16);
}

Fixed FixedReciprocalQuarter(Fixed value);
Fixed FixedReciprocalUnsigned16(uint32_t value);
Fixed FixedReciprocalSigned16(Fixed value);
Fixed FixedReciprocalUnsigned17(uint32_t value);
Fixed FixedReciprocalSigned17(Fixed value);

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

struct Vec2 {
    Fixed x = 0;
    Fixed y = 0;
};

struct Vec3 {
    Fixed x = 0;
    Fixed y = 0;
    Fixed z = 0;
};

struct Vec4 {
    Fixed x = 0;
    Fixed y = 0;
    Fixed z = 0;
    Fixed w = 0;
};

inline Fixed Dot(const Vec2& left, const Vec2& right)
{
    return AddFixed(FixedMultiply(left.x, right.x), FixedMultiply(left.y, right.y));
}

inline Fixed SquaredLength(const Vec2& value)
{
    return Dot(value, value);
}

inline Fixed Length(const Vec2& value)
{
    return FixedSqrt(SquaredLength(value));
}

inline Vec2 Normalize(const Vec2& value)
{
    const Fixed inverseLength = FixedInverseSqrt(SquaredLength(value));
    return {FixedMultiply(value.x, inverseLength), FixedMultiply(value.y, inverseLength)};
}

Vec3 LerpVec3(const Vec3& start, const Vec3& end, Fixed fraction);
Vec3 MultiplyComponents(const Vec3& input, Fixed x, Fixed y, Fixed z);
Vec3 AddComponents(const Vec3& input, Fixed x, Fixed y, Fixed z);
Fixed SquaredDistance(const Vec3& first, const Vec3& second);
int16_t Vec3AngleXZ(const Vec3& value);

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

Fixed FixedSin(uint16_t angle);
Fixed FixedCos(int16_t angle);
Fixed FixedTan(uint16_t angle);
uint16_t FixedArcTangent(Fixed value);
uint16_t FixedArcSine(Fixed value);
uint16_t FixedArcCosine(Fixed value);
Vec3 RotateX(const Vec3& value, uint16_t angle);
Vec3 RotateY(const Vec3& value, uint16_t angle);
Vec3 RotateZ(const Vec3& value, uint16_t angle);
Vec3 Rotate(const Vec3& value, uint16_t xAngle, uint16_t yAngle, uint16_t zAngle);

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
            int64_t value = 0;
            for (unsigned index = 0; index < 4; ++index) {
                value += static_cast<int64_t>(left.values[column * 4 + index]) *
                         right.values[index * 4 + row];
            }
            const uint64_t bits = static_cast<uint64_t>(value);
            result.values[column * 4 + row] =
                static_cast<Fixed>(static_cast<uint32_t>(bits >> 16));
        }
    }
    return result;
}

inline Vec3 Transform(const Matrix4& matrix, const Vec3& value)
{
    return {
        AddFixed(Dot({matrix.values[0], matrix.values[4], matrix.values[8]}, value),
                 matrix.values[12]),
        AddFixed(Dot({matrix.values[1], matrix.values[5], matrix.values[9]}, value),
                 matrix.values[13]),
        AddFixed(Dot({matrix.values[2], matrix.values[6], matrix.values[10]}, value),
                 matrix.values[14])
    };
}

Vec2 Transform(const Matrix4& matrix, const Vec2& value);
Vec2 TransformPerspective(const Matrix4& matrix, const Vec2& value);
Vec2 TransformDirection(const Matrix4& matrix, const Vec2& value);
Vec3 TransformPerspective(const Matrix4& matrix, const Vec3& value);
Vec3 TransformDirection(const Matrix4& matrix, const Vec3& value);

inline Fixed Dot(const Vec4& left, const Vec4& right)
{
    return AddFixed(AddFixed(FixedMultiply(left.x, right.x), FixedMultiply(left.y, right.y)),
                    AddFixed(FixedMultiply(left.z, right.z), FixedMultiply(left.w, right.w)));
}

inline Fixed SquaredLength(const Vec4& value)
{
    return Dot(value, value);
}

inline Fixed Length(const Vec4& value)
{
    return FixedSqrt(SquaredLength(value));
}

Vec4 NormalizeVec4(const Vec4& value);
Vec4 NormalizeRobust(const Vec4& value);
Vec4& DivideVec4ByW(Vec4& output, const Vec4& input);
Vec4 Cross3(const Vec4& first, const Vec4& second, const Vec4& third);
Vec4 LerpVec4(const Vec4& start, const Vec4& end, Fixed fraction);

Vec4 QuaternionIdentity();
Vec4 QuaternionConjugate(const Vec4& value);
Vec4 QuaternionInverse(const Vec4& value);
Vec4 QuaternionFromAxisAngle(const Vec3& axis, int16_t angle);
Vec4 QuaternionFromMatrix(const Matrix4& matrix);
Vec4 QuaternionFromEuler(int16_t xAngle, int16_t yAngle, int16_t zAngle);
Vec4 QuaternionLerp(const Vec4& start, const Vec4& end, Fixed fraction);
void QuaternionToAxisAngle(const Vec4& value, Vec3& axis, int16_t& angle);

Matrix4 LookAt(const Vec3& position, const Vec3& target, const Vec3& up);
Matrix4 RotateXMatrix(uint16_t angle);
Matrix4 RotateYMatrix(uint16_t angle);
Matrix4 RotateZMatrix(uint16_t angle);
Matrix4 EulerMatrix(uint16_t xAngle, uint16_t yAngle, uint16_t zAngle);
Matrix4 AxisRotationMatrix(const Vec3& axis, uint16_t angle);
void TranslateMatrix(Matrix4& matrix, Fixed x, Fixed y, Fixed z);
Matrix4 TranslationMatrix(Fixed x, Fixed y, Fixed z);
Matrix4& ScaleMatrix(Matrix4& matrix, Fixed x, Fixed y, Fixed z);
Matrix4 ScaleMatrix(Fixed x, Fixed y, Fixed z);
Matrix4& RotateX(Matrix4& matrix, uint16_t angle);
Matrix4& RotateY(Matrix4& matrix, uint16_t angle);
Matrix4& RotateZ(Matrix4& matrix, uint16_t angle);
Matrix4& RotateEuler(Matrix4& matrix, uint16_t xAngle, uint16_t yAngle, uint16_t zAngle);
Matrix4& RotateAxis(Matrix4& matrix, const Vec3& axis, uint16_t angle);
Matrix4 Transpose(const Matrix4& value);
Fixed Determinant2x2(Fixed a, Fixed b, Fixed c, Fixed d);
Fixed Determinant3x3(Fixed a, Fixed b, Fixed c, Fixed d, Fixed e, Fixed f,
                     Fixed g, Fixed h, Fixed i);
Fixed Determinant(const Matrix4& matrix);
Matrix4 Inverse(const Matrix4& matrix);
Matrix4 Inverse3x3(const Matrix4& matrix);
Matrix4 OrthographicMatrix(Fixed width, Fixed height, Fixed nearDistance, Fixed farDistance);
Matrix4 OrthographicMatrixNegativeDepth(Fixed width, Fixed height, Fixed nearDistance,
                                        Fixed farDistance);
Matrix4 OrthographicBoundsMatrix(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                 Fixed nearDistance, Fixed farDistance);
Matrix4 OrthographicBoundsMatrixNegativeDepth(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                              Fixed nearDistance, Fixed farDistance);
Matrix4 PerspectiveTanMatrix(int16_t angle, Fixed aspectRatio, Fixed nearDistance,
                             Fixed farDistance);
Matrix4 PerspectiveMatrix(Fixed width, Fixed height, Fixed nearDistance, Fixed farDistance);
Matrix4 PerspectiveMatrixNegativeDepth(Fixed width, Fixed height, Fixed nearDistance,
                                       Fixed farDistance);
Matrix4 FrustumMatrix(Fixed left, Fixed right, Fixed bottom, Fixed top,
                      Fixed nearDistance, Fixed farDistance);
Matrix4 FrustumMatrixNegativeDepth(Fixed left, Fixed right, Fixed bottom, Fixed top,
                                   Fixed nearDistance, Fixed farDistance);
Matrix4 ViewportMatrix(int32_t left, int32_t top, uint32_t width, uint32_t height,
                       Fixed nearDistance, Fixed farDistance);
Matrix4 ReflectionMatrix(const Vec4& plane);
Matrix4 PlaneProjectionMatrix(const Vec3& normal, const Vec4& coefficients);
Matrix4 AxesMatrix(const Vec3& x, const Vec3& y, const Vec3& z);
Matrix4 MatrixFromValues(Fixed m0, Fixed m1, Fixed m2, Fixed m3,
                         Fixed m4, Fixed m5, Fixed m6, Fixed m7,
                         Fixed m8, Fixed m9, Fixed m10, Fixed m11,
                         Fixed m12, Fixed m13, Fixed m14, Fixed m15);
Vec4 TransformVec4(const Matrix4& matrix, const Vec4& value);
Vec4 TransformInverse(const Matrix4& matrix, const Vec4& value);
Vec3 Project(const Vec3& input, const Matrix4& world, const Matrix4& view,
             const Matrix4& projection, const Matrix4& viewport);
Vec3 Unproject(const Vec3& input, const Matrix4& world, const Matrix4& view,
               const Matrix4& projection, const Matrix4& viewport);
Matrix4 QuaternionToMatrix(const Vec4& quaternion);

} // namespace soft3d

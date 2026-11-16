#include "soft3d/Fixed.hpp"

namespace soft3d {
namespace {

constexpr Fixed sineTable[] = {
#include "soft3d/FixedSinTable.inc"
};

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

} // namespace

Fixed FixedSin(uint16_t angle)
{
    const unsigned index = (angle & 0x800) == 0
        ? angle & 0x7ff
        : 0x800 - (angle & 0x7ff);
    const Fixed value = sineTable[index];
    return (angle & 0x1000) == 0 ? value : NegateFixed(value);
}

Fixed FixedCos(int16_t angle)
{
    return FixedSin(static_cast<uint16_t>(angle + 0x800) & 0x1fff);
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

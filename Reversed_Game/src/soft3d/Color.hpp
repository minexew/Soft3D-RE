#pragma once

#include "soft3d/Fixed.hpp"

#include <cstdint>

namespace soft3d {

using Color4 = Vec4;

inline Color4& Color4Clamp(Color4& value)
{
    const auto clamp = [](Fixed component) {
        return component > fixedOne ? fixedOne : component < 0 ? 0 : component;
    };
    value.x = clamp(value.x);
    value.y = clamp(value.y);
    value.z = clamp(value.z);
    value.w = clamp(value.w);
    return value;
}

inline Color4 Color4Negate(const Color4& value)
{
    return {SubtractFixed(fixedOne, value.x), SubtractFixed(fixedOne, value.y),
            SubtractFixed(fixedOne, value.z), SubtractFixed(fixedOne, value.w)};
}

inline Color4 Color4AdjustContrast(const Color4& value, Fixed contrast)
{
    Color4 result{
        AddFixed(0x8000, FixedMultiply(SubtractFixed(value.x, 0x8000), contrast)),
        AddFixed(0x8000, FixedMultiply(SubtractFixed(value.y, 0x8000), contrast)),
        AddFixed(0x8000, FixedMultiply(SubtractFixed(value.z, 0x8000), contrast)),
        value.w
    };
    return Color4Clamp(result);
}

inline Color4 Color4Lerp(const Color4& start, const Color4& end, Fixed fraction)
{
    Color4 result{
        AddFixed(start.x, FixedMultiply(SubtractFixed(end.x, start.x), fraction)),
        AddFixed(start.y, FixedMultiply(SubtractFixed(end.y, start.y), fraction)),
        AddFixed(start.z, FixedMultiply(SubtractFixed(end.z, start.z), fraction)),
        AddFixed(start.w, FixedMultiply(SubtractFixed(end.w, start.w), fraction))
    };
    return Color4Clamp(result);
}

inline uint16_t PackRgb565(uint32_t red, uint32_t green, uint32_t blue)
{
    return static_cast<uint16_t>(((red & 0xff) >> 3) << 11 |
                                 ((green & 0xff) >> 2) << 5 | ((blue & 0xff) >> 3));
}

inline uint32_t ExpandRgb565(uint16_t pixel)
{
    return (static_cast<uint32_t>(pixel & 0xf800) << 8) |
           (static_cast<uint32_t>(pixel & 0x07e0) << 5) |
           (static_cast<uint32_t>(pixel & 0x001f) << 3);
}

} // namespace soft3d

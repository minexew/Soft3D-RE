#pragma once

#include <cstdint>

namespace soft3d {

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

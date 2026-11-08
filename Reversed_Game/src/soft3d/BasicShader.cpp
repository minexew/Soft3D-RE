#include "soft3d/BasicShader.hpp"

#include "soft3d/Fixed.hpp"

#include <cstddef>

namespace soft3d {

BasicShader::BasicShader(int shaderWidth, int shaderHeight)
    : width(shaderWidth)
    , height(shaderHeight)
    , enabled(1)
{
}

uint32_t BasicShader::ExpandRgb565(uint16_t pixel)
{
    return (static_cast<uint32_t>(pixel & 0xf800) << 8) |
           (static_cast<uint32_t>(pixel & 0x07e0) << 5) |
           (static_cast<uint32_t>(pixel & 0x001f) << 3);
}

void BasicShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    if (enabled == 1) {
        destination += static_cast<std::size_t>(width) * (height - 1);
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                *destination = ExpandRgb565(*input);
                input += 2;
                destination -= width;
            }
            destination += width * height + 1;
        }
    } else {
        --destination;
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                *destination = ExpandRgb565(*input);
                input += 2;
                destination += width;
            }
            destination -= width * height + 1;
        }
    }
}

void BasicShader::SetEnabled(int value)
{
    enabled = value == 0 ? 1 : value;
}

FogShader::FogShader(int shaderWidth, int shaderHeight)
    : BasicShader(shaderWidth, shaderHeight)
    , rangeStart(fixedOne)
    , rangeEnd(0x8c0000)
    , colors(0x10000)
{
    BuildAttenuationTable();
    BuildColorTable();
}

void FogShader::SetFog(int start, int end, uint16_t color)
{
    rangeStart = start;
    rangeEnd = end;
    fogColor = color;
    BuildAttenuationTable();
    BuildColorTable();
}

void FogShader::BuildAttenuationTable()
{
    const int range = rangeEnd - rangeStart;
    const int fullEnd = rangeEnd - FixedMultiply(0xf000, range);
    fullFogDepth = fullEnd == 0 ? 0 : 0x40000000 / (fullEnd >> 2);
    if (fullFogDepth > 0xffff) {
        fullFogDepth = 0xffff;
    }
    const int fogStart = rangeEnd - FixedMultiply(0x1000, range);
    fogStartDepth = fogStart == 0 ? 0 : 0x40000000 / (fogStart >> 2);
    const int count = fullFogDepth - fogStartDepth;
    attenuation.clear();
    if (count <= 0) {
        return;
    }
    attenuation.resize(static_cast<std::size_t>(count));
    for (int depth = fogStartDepth; depth < fullFogDepth; ++depth) {
        const Fixed reciprocalDepth = depth == 0 ? 0 : 0x40000000 / (depth >> 2);
        const Fixed reciprocalRange = range == 0 ? 0 : 0x40000000 / (range >> 2);
        Fixed value = FixedMultiply(rangeEnd - reciprocalDepth, reciprocalRange);
        value = FixedMultiply(value, 0x100000);
        if (value < fixedOne) {
            value = fixedOne;
        } else if (value > 0xf0000) {
            value = 0xf0000;
        }
        attenuation[static_cast<std::size_t>(depth - fogStartDepth)] =
            static_cast<uint8_t>((static_cast<uint32_t>(value) >> 16) & 0xff);
    }
}

void FogShader::BuildColorTable()
{
    const unsigned red = (fogColor >> 12) & 0xf;
    const unsigned green = (fogColor >> 7) & 0xf;
    const unsigned blue = (fogColor & 0x001f) >> 1;
    for (unsigned intensity = 0; intensity < 16; ++intensity) {
        for (unsigned r = 0; r < 16; ++r) {
            for (unsigned g = 0; g < 16; ++g) {
                for (unsigned b = 0; b < 16; ++b) {
                    const unsigned outR = r * intensity + (16 - intensity) * red;
                    const unsigned outG = g * intensity + (16 - intensity) * green;
                    const unsigned outB = b * intensity + (16 - intensity) * blue;
                    colors[(intensity << 12) | (r << 8) | (g << 4) | b] =
                        static_cast<uint16_t>(((outR & 0xf8) << 8) |
                                              ((outG & 0xfc) << 3) | (outB >> 3));
                }
            }
        }
    }
}

void FogShader::Render(const void* source, uint32_t* destination)
{
    const uint16_t* input = static_cast<const uint16_t*>(source);
    const int direction = enabled == 1 ? -1 : 1;
    if (enabled == 1) {
        destination += static_cast<std::size_t>(width) * (height - 1);
    } else {
        --destination;
    }
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            uint16_t color = input[0];
            const uint16_t depth = input[1];
            if (depth != 0 && depth < fullFogDepth) {
                if (depth > fogStartDepth) {
                    const unsigned index = static_cast<unsigned>(color & 0x1e) |
                        ((color & 0x780) >> 2) | ((color & 0xf000) >> 3) |
                        (static_cast<unsigned>(attenuation[depth - fogStartDepth]) << 13);
                    color = colors[index >> 1];
                } else {
                    color = fogColor;
                }
            }
            *destination = ExpandRgb565(color);
            input += 2;
            destination += direction * width;
        }
        destination += direction == -1 ? width * height + 1 : -(width * height + 1);
    }
}

} // namespace soft3d

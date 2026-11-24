#pragma once

#include <cstdint>
#include <vector>

namespace soft3d {

inline uint16_t Read16BE(const uint8_t* data)
{
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

inline uint32_t Read32BE(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

inline uint32_t Read32LE(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

inline void Append16BE(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

} // namespace soft3d

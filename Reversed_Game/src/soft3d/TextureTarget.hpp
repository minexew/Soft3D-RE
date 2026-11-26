#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/Texture.hpp"

#include <cstdint>
#include <vector>

namespace soft3d {

class TextureTarget {
public:
    TextureTarget(int32_t width, int32_t height);
    TextureTarget(const TextureTarget&) = delete;
    TextureTarget& operator=(const TextureTarget&) = delete;

    result LoadStx(const char16_t* path);

    int32_t GetWidth() const;
    int32_t GetHeight() const;
    int32_t GetStrideBytes() const;
    int32_t GetTiledHeight() const;
    const Vec2& GetOffset() const;
    Texture* GetTexture();
    const Texture* GetTexture() const;
    uint8_t* GetPixels();
    const uint8_t* GetPixels() const;
    bool HasTiledBuffer() const;

private:
    int32_t width;
    int32_t height;
    int32_t strideBytes;
    int32_t tiledHeight;
    Vec2 offset{};
    Texture texture;
    std::vector<uint8_t> tiledPixels;
};

} // namespace soft3d

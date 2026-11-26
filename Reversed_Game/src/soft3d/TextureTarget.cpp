#include "soft3d/TextureTarget.hpp"

#include <new>

namespace soft3d {

TextureTarget::TextureTarget(int32_t targetWidth, int32_t targetHeight)
    : width(targetWidth)
    , height(targetHeight)
    , strideBytes(ShiftLeftFixed(targetWidth, 2))
    , tiledHeight(AddFixed(targetHeight, targetHeight >> 1))
{
}

result TextureTarget::LoadStx(const char16_t* path)
{
    const result status = texture.LoadStxFile(path);
    if (status != S3D_OK) {
        return status;
    }
    tiledPixels.clear();
    if (strideBytes == static_cast<int32_t>(texture.GetWidth())) {
        return S3D_OK;
    }
    if (width <= 0 || height <= 0 || texture.GetWidth() == 0 ||
        texture.GetPixels() == nullptr) {
        return S3D_PARAM;
    }

    const int32_t sourceWidth = static_cast<int32_t>(texture.GetWidth());
    try {
        tiledPixels.resize(static_cast<std::size_t>(strideBytes) * tiledHeight);
    } catch (const std::bad_alloc&) {
        return S3D_MEMORY;
    }

    const uint8_t* source = texture.GetPixels();
    int32_t sourceX = 0;
    for (int32_t y = 0; y < tiledHeight; ++y) {
        for (int32_t x = 0; x < strideBytes; ++x) {
            const int64_t sourceOffset = static_cast<int64_t>(y) * sourceWidth + sourceX;
            if (sourceOffset >= texture.GetPixelSize()) {
                tiledPixels.clear();
                return S3D_FORMAT;
            }
            tiledPixels[static_cast<std::size_t>(y) * strideBytes + x] = source[sourceOffset];
            ++sourceX;
            if (sourceX > sourceWidth) {
                sourceX -= sourceWidth;
            }
        }
    }
    texture.Reset();
    return S3D_OK;
}

int32_t TextureTarget::GetWidth() const { return width; }
int32_t TextureTarget::GetHeight() const { return height; }
int32_t TextureTarget::GetStrideBytes() const { return strideBytes; }
int32_t TextureTarget::GetTiledHeight() const { return tiledHeight; }
const Vec2& TextureTarget::GetOffset() const { return offset; }
Texture* TextureTarget::GetTexture() { return &texture; }
const Texture* TextureTarget::GetTexture() const { return &texture; }

uint8_t* TextureTarget::GetPixels()
{
    return tiledPixels.empty() ? texture.GetPixels() : tiledPixels.data();
}

const uint8_t* TextureTarget::GetPixels() const
{
    return tiledPixels.empty() ? texture.GetPixels() : tiledPixels.data();
}

bool TextureTarget::HasTiledBuffer() const { return !tiledPixels.empty(); }

} // namespace soft3d

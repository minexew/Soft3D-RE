#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstdint>

namespace soft3d {

class s3dFilePak;

class Texture : public s3dBase {
public:
    Texture();
    ~Texture() override;

    result LoadStx(const char16_t* entryName, s3dFilePak* filePak);
    result ParseStx(const void* data);
    void Reset();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetPixelSize() const;
    const uint8_t* GetPixels() const;
    const uint8_t* GetPalette() const;
    uint32_t GetPaletteSize() const;

private:
    s3dIO io;
    s3dLog log;
    uint32_t width;
    uint32_t height;
    int32_t fixedWidth;
    int32_t fixedHeight;
    uint32_t widthLog2;
    uint16_t type;
    uint16_t padding3e;
    uint32_t pixelSize;
    uint8_t bytesPerPixel;
    uint8_t hasTransparentColor;
    uint8_t transparentColor;
    uint8_t padding47;
    uint8_t* pixelsAllocation;
    uint8_t* pixels;
    uint32_t paletteSize;
    uint8_t* palette;
};

} // namespace soft3d

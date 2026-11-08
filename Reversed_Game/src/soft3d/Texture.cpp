#include "soft3d/Texture.hpp"

#include "soft3d/s3dFilePak.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace soft3d {
namespace {

uint32_t Read32(const uint8_t*& input)
{
    uint32_t value;
    std::memcpy(&value, input, sizeof(value));
    input += sizeof(value);
    return value;
}

int32_t IntegerLog2(uint32_t value)
{
    int32_t result = 1;
    uint32_t power = 2;
    if (value == 1) {
        return 0;
    }
    for (; power < value; power <<= 1) {
        ++result;
    }
    return power == value ? result : -1;
}

} // namespace

Texture::Texture()
    : width(0)
    , height(0)
    , fixedWidth(0)
    , fixedHeight(0)
    , widthLog2(0)
    , type(0)
    , padding3e(0)
    , pixelSize(0)
    , bytesPerPixel(1)
    , hasTransparentColor(0)
    , transparentColor(0)
    , padding47(0)
    , pixelsAllocation(nullptr)
    , pixels(nullptr)
    , paletteSize(0)
    , palette(nullptr)
{
}

Texture::~Texture()
{
    std::free(pixelsAllocation);
    std::free(palette);
}

result Texture::LoadStx(const char16_t* entryName, s3dFilePak* filePak)
{
    Reset();
    result status = filePak->Select(entryName);
    if (status != S3D_OK) {
        S3D_LOG(log, u"Texture::CreateFromPak %s: can't find!\n", entryName);
        return S3D_FILE;
    }

    filePak->Seek(0, SEEK_END);
    int32_t bytes = filePak->Tell();
    if (bytes == S3D_EOF) {
        S3D_LOG(log, u"Texture::CreateFromFile %s: ftell return EOF!\n", entryName);
        return S3D_UNKNOWN;
    }
    void* data = std::malloc(static_cast<size_t>(bytes));
    if (data == nullptr) {
        S3D_LOG(log, u"Texture::CreateFromFile %s: memory out!\n", entryName);
        return S3D_MEMORY;
    }
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data, bytes);
    status = ParseStx(data);
    std::free(data);
    return status;
}

result Texture::ParseStx(const void* data)
{
    const uint8_t* input = static_cast<const uint8_t*>(data);
    uint32_t header = Read32(input);
    paletteSize = header & 0xff;
    type = static_cast<uint16_t>(header >> 8) & 0xff;
    int16_t transparent = static_cast<int16_t>(header >> 16);
    uint8_t typePixelBytes = static_cast<uint8_t>((type & 0x30) >> 4);
    bytesPerPixel = typePixelBytes == 0 ? 4 : typePixelBytes;
    width = Read32(input);
    height = Read32(input);
    fixedWidth = static_cast<int32_t>(width << 16);
    fixedHeight = static_cast<int32_t>(height << 16);
    if (transparent < 0) {
        hasTransparentColor = 0;
    } else {
        hasTransparentColor = 1;
        transparentColor = static_cast<uint8_t>(transparent);
    }

    if ((type & 0xc0) != 0) {
        if (paletteSize == 0) {
            paletteSize = 0x100;
        }
        uint32_t paletteBytes = (type & 0xc0) == 0x40 ? paletteSize << 6
                                                       : paletteSize << 1;
        palette = static_cast<uint8_t*>(std::malloc((paletteBytes + 3) & 0xfffffffc));
        std::memcpy(palette, input, paletteBytes);
        input += paletteBytes & 0xfffffffc;
        if ((paletteBytes & 3) != 0) {
            input += 4;
        }
    }

    pixelSize = height * width * bytesPerPixel;
    pixelsAllocation = static_cast<uint8_t*>(std::malloc(pixelSize + 4));
    if (pixelsAllocation == nullptr) {
        S3D_LOG(log, u"Texture::Create: failed to alloc %d memory!\n", pixelSize + 4);
        return S3D_MEMORY;
    }
    pixels = reinterpret_cast<uint8_t*>(
        (reinterpret_cast<uintptr_t>(pixelsAllocation) + 3) & ~uintptr_t(3));
    std::memcpy(pixels, input, pixelSize);
    widthLog2 = IntegerLog2(width);
    return S3D_OK;
}

void Texture::Reset()
{
    if (pixelsAllocation != nullptr) {
        std::free(pixelsAllocation);
        pixelsAllocation = nullptr;
    }
    if (palette != nullptr) {
        std::free(palette);
        palette = nullptr;
    }
    width = 0;
    height = 0;
    bytesPerPixel = 0;
    fixedWidth = 0;
    fixedHeight = 0;
}

uint32_t Texture::GetWidth() const { return width; }
uint32_t Texture::GetHeight() const { return height; }
uint32_t Texture::GetPixelSize() const { return pixelSize; }
const uint8_t* Texture::GetPixels() const { return pixels; }
const uint8_t* Texture::GetPalette() const { return palette; }
uint32_t Texture::GetPaletteSize() const { return paletteSize; }

} // namespace soft3d

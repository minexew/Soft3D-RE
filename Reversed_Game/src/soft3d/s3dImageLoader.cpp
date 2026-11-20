#include "soft3d/s3dImageLoader.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

namespace soft3d {
namespace {

uint16_t Read16(const uint8_t* input)
{
    return static_cast<uint16_t>(input[0]) | static_cast<uint16_t>(input[1] << 8);
}

void StorePixel(uint8_t* output, const uint8_t* input, uint32_t depth)
{
    output[0] = input[2];
    output[1] = input[1];
    output[2] = input[0];
    if (depth == 4) {
        output[3] = input[3];
    }
}

} // namespace

ImageInfo::~ImageInfo()
{
    Free();
}

void ImageInfo::Free()
{
    std::free(palette);
    std::free(data);
    palette = nullptr;
    data = nullptr;
    width = 0;
    height = 0;
    bitDepth = 0;
}

void s3dImageLoader::SetEngine(Engine* value)
{
    io.SetEngine(value);
    log.SetEngine(value);
    s3dBase::SetEngine(value);
}

result s3dImageLoader::LoadTGA(const char16_t* path, ImageInfo& image)
{
    image.Free();
    if (!io.Open(path, u"rb")) {
        S3D_LOG(log, u"LoadTGA %s: can't find!\n", path);
        return S3D_FILE;
    }
    io.Seek(0, SEEK_END);
    const uint32_t bytes = io.Tell();
    if (bytes == static_cast<uint32_t>(S3D_EOF)) {
        S3D_LOG(log, u"LoadTGA %s: s3dftell returns EOF!\n", path);
        io.Close();
        return S3D_UNKNOWN;
    }
    std::vector<uint8_t> data(bytes);
    io.Seek(0, SEEK_SET);
    const uint32_t read = data.empty() ? 0 : io.Read(data.data(), bytes);
    io.Close();
    if (read != bytes) {
        return S3D_FILE;
    }
    return ParseTga(data.data(), data.size(), image);
}

result s3dImageLoader::ParseTga(const void* source, size_t size, ImageInfo& image)
{
    image.Free();
    if (source == nullptr || size < 18) {
        return S3D_FORMAT;
    }
    const uint8_t* input = static_cast<const uint8_t*>(source);
    const uint8_t idLength = input[0];
    const uint8_t colorMapType = input[1];
    const uint8_t imageType = input[2];
    const uint32_t width = Read16(input + 12);
    const uint32_t height = Read16(input + 14);
    const uint32_t depth = input[16] >> 3;
    if ((imageType != 2 && imageType != 10) || colorMapType != 0 ||
        (depth != 3 && depth != 4)) {
        return S3D_FORMAT;
    }
    if (width == 0 || height == 0) {
        return S3D_FORMAT;
    }
    const size_t pixelCount = static_cast<size_t>(width) * height;
    if (pixelCount / height != width || pixelCount > std::numeric_limits<size_t>::max() / depth) {
        return S3D_FORMAT;
    }
    const size_t outputBytes = pixelCount * depth;
    size_t position = 18U + idLength;
    if (position > size) {
        return S3D_FORMAT;
    }
    auto output = static_cast<uint8_t*>(std::malloc(outputBytes));
    if (output == nullptr) {
        return S3D_MEMORY;
    }
    size_t outputIndex = 0;
    auto append = [&](const uint8_t* pixel) {
        const size_t row = outputIndex / width;
        const size_t column = outputIndex % width;
        uint8_t* destination = output + ((height - 1 - row) * width + column) * depth;
        StorePixel(destination, pixel, depth);
        ++outputIndex;
    };
    if (imageType == 2) {
        if (outputBytes > size - position) {
            std::free(output);
            return S3D_FORMAT;
        }
        for (size_t index = 0; index < pixelCount; ++index) {
            append(input + position);
            position += depth;
        }
    } else {
        while (outputIndex < pixelCount) {
            if (position >= size) {
                std::free(output);
                return S3D_FORMAT;
            }
            const uint8_t packet = input[position++];
            const size_t count = static_cast<size_t>(packet & 0x7f) + 1;
            if (count > pixelCount - outputIndex) {
                std::free(output);
                return S3D_FORMAT;
            }
            if ((packet & 0x80) != 0) {
                if (depth > size - position) {
                    std::free(output);
                    return S3D_FORMAT;
                }
                for (size_t index = 0; index < count; ++index) {
                    append(input + position);
                }
                position += depth;
            } else {
                if (count > (size - position) / depth) {
                    std::free(output);
                    return S3D_FORMAT;
                }
                for (size_t index = 0; index < count; ++index) {
                    append(input + position);
                    position += depth;
                }
            }
        }
    }
    image.data = output;
    image.width = static_cast<int32_t>(width);
    image.height = static_cast<int32_t>(height);
    image.bitDepth = static_cast<int32_t>(depth);
    return S3D_OK;
}

} // namespace soft3d

#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstddef>
#include <cstdint>

namespace soft3d {

struct ImageInfo {
    ImageInfo() = default;
    ImageInfo(const ImageInfo&) = delete;
    ImageInfo& operator=(const ImageInfo&) = delete;
    ~ImageInfo();

    void Free();

    uint8_t* palette = nullptr;
    uint8_t* data = nullptr;
    int32_t width = 0;
    int32_t height = 0;
    int32_t bitDepth = 0;
};

class s3dImageLoader : public s3dBase {
public:
    s3dImageLoader() = default;
    ~s3dImageLoader() override = default;

    void SetEngine(Engine* engine) override;
    result LoadTGA(const char16_t* path, ImageInfo& image);
    result ParseTga(const void* data, size_t size, ImageInfo& image);

private:
    s3dIO io;
    s3dLog log;
};

} // namespace soft3d

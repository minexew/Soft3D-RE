#pragma once

#include "soft3d/s3dBase.hpp"

#include <cstdint>

namespace soft3d {

namespace platform {
class File;
}

class s3dIO : public s3dBase {
public:
    s3dIO();
    ~s3dIO() override = default;

    bool Open(const char16_t* path, const char16_t* mode);
    void Close();
    void Seek(uint32_t offset, int whence);
    uint32_t Tell() const;
    uint32_t Read(void* destination, uint32_t bytes);
    uint32_t Read(void* destination, uint32_t size, uint32_t count);

private:
    platform::File* file;
};

} // namespace soft3d

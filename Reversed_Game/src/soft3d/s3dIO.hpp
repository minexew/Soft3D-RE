#pragma once

#include "soft3d/s3dBase.hpp"

#include <cstdarg>
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
    int Write(const void* source, int size, int count);
    int Printf(const char16_t* format, ...);
    int VPrintf(const char16_t* format, va_list arguments);
    bool Reopen(const char16_t* path, const char16_t* mode);

private:
    platform::File* file;
};

int FormatWideString(char16_t* output, const char16_t* format, va_list arguments);
int WideSprintf(char16_t* output, const char16_t* format, ...);

} // namespace soft3d

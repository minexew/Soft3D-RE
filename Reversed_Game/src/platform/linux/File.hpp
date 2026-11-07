#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace soft3d::platform {

class File {
public:
    File() = default;
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool Open(const char16_t* path, const char16_t* mode);
    void Close();
    void Seek(uint32_t offset, int whence);
    uint32_t Tell() const;
    uint32_t Read(void* destination, uint32_t bytes);
    uint32_t Read(void* destination, uint32_t size, uint32_t count);
    int Write(const void* source, uint32_t bytes);

private:
    std::FILE* handle = nullptr;
};

} // namespace soft3d::platform

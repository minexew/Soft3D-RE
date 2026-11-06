#include "soft3d/s3dIO.hpp"

#include "platform/linux/File.hpp"

#include <new>

namespace soft3d {

s3dIO::s3dIO() = default;

bool s3dIO::Open(const char16_t* path, const char16_t* mode)
{
    platform::File* opened = new (std::nothrow) platform::File;
    file = opened;
    if (opened == nullptr) {
        return false;
    }
    if (!opened->Open(path, mode)) {
        delete opened;
        file = nullptr;
        return false;
    }
    return true;
}

void s3dIO::Close()
{
    delete file;
    file = nullptr;
}

void s3dIO::Seek(uint32_t offset, int whence)
{
    file->Seek(offset, whence);
}

uint32_t s3dIO::Tell() const
{
    return file->Tell();
}

uint32_t s3dIO::Read(void* destination, uint32_t bytes)
{
    return file->Read(destination, bytes);
}

uint32_t s3dIO::Read(void* destination, uint32_t size, uint32_t count)
{
    return file->Read(destination, size, count);
}

} // namespace soft3d

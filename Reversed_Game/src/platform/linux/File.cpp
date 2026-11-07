#include "platform/linux/File.hpp"

#include <string>

namespace soft3d::platform {
namespace {

std::string Narrow(const char16_t* text, bool path)
{
    std::string result;
    for (; *text != u'\0'; ++text) {
        char c = static_cast<char>(*text);
        if (path && c == '\\') {
            c = '/';
        }
        result.push_back(c);
    }
    return result;
}

} // namespace

File::~File()
{
    Close();
}

bool File::Open(const char16_t* path, const char16_t* mode)
{
    std::FILE* opened = std::fopen(Narrow(path, true).c_str(), Narrow(mode, false).c_str());
    if (opened == nullptr) {
        handle = nullptr;
        return false;
    }

    handle = opened;
    return true;
}

void File::Close()
{
    if (handle != nullptr) {
        std::fclose(handle);
        handle = nullptr;
    }
}

void File::Seek(uint32_t offset, int whence)
{
    std::fseek(handle, static_cast<long>(offset), whence);
}

uint32_t File::Tell() const
{
    return static_cast<uint32_t>(std::ftell(handle));
}

uint32_t File::Read(void* destination, uint32_t bytes)
{
    return static_cast<uint32_t>(std::fread(destination, 1, bytes, handle));
}

uint32_t File::Read(void* destination, uint32_t size, uint32_t count)
{
    return Read(destination, size * count);
}

int File::Write(const void* source, uint32_t bytes)
{
    return std::fwrite(source, 1, bytes, handle) == bytes ? 0 : -1;
}

} // namespace soft3d::platform

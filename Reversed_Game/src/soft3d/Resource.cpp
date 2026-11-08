#include "soft3d/Resource.hpp"

#include "soft3d/s3dFilePak.hpp"

#include <algorithm>

namespace soft3d {
namespace {

void CopyString(char16_t* destination, const char16_t* source)
{
    do {
        *destination++ = *source;
    } while (*source++ != u'\0');
}

void AppendString(char16_t* destination, const char16_t* source)
{
    while (*destination != u'\0') {
        ++destination;
    }
    CopyString(destination, source);
}

size_t StringLength(const char16_t* text)
{
    const char16_t* end = text;
    while (*end != u'\0') {
        ++end;
    }
    return static_cast<size_t>(end - text);
}

} // namespace

Resource::Resource()
    : path{}
    , fileName{}
    , directory{}
    , unknown(0)
{
}

void Resource::SetEngine(Engine* value)
{
    io.SetEngine(value);
    log.SetEngine(value);
    s3dBase::SetEngine(value);
}

result Resource::Initialize(const char16_t* name, const char16_t* pathPrefix,
                            s3dFilePak* filePak)
{
    CopyString(directory, pathPrefix);
    CopyString(fileName, name);
    CopyString(path, pathPrefix);
    AppendString(path, name);
    fileName[StringLength(fileName) - 4] = u'\0';
    return Load(path, filePak);
}

const char16_t* Resource::GetPath() const
{
    return path;
}

const char16_t* Resource::GetFileName() const
{
    return fileName;
}

const char16_t* Resource::GetDirectory() const
{
    return directory;
}

} // namespace soft3d

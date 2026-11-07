#include "soft3d/ObjectRes.hpp"

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

int ParseObject(SojObject& object, const uint8_t*& input)
{
    object.unknown00 = static_cast<int32_t>(Read32(input));
    object.unknown04 = static_cast<int32_t>(Read32(input));
    object.color = static_cast<uint16_t>((input[0] >> 3) << 11)
                   | static_cast<uint16_t>((input[1] >> 2) << 5)
                   | static_cast<uint16_t>(input[2] >> 3);
    object.unknown0c = input[3];
    input += 4;
    uint32_t texture = Read32(input);
    if (static_cast<int16_t>(texture) == 0xff) {
        object.unknown16 = 1;
        object.textureIndex = 0;
    } else {
        object.unknown16 = 0;
        object.textureIndex = texture & 0xffff;
    }
    object.unknown14 = static_cast<uint16_t>(Read32(input));
    for (int32_t& value : object.unknown18) {
        value = static_cast<int32_t>(Read32(input));
    }
    object.unknown28 = static_cast<int32_t>(Read32(input));
    object.unknown2c = static_cast<int32_t>(Read32(input));
    uint32_t first = Read32(input);
    uint32_t second = Read32(input);
    if (first == 0) {
        object.primitiveType = second == 0 ? 0 : 4;
    } else {
        object.primitiveType = 3;
    }
    return 0;
}

int ParseVertices(SojVertices& vertices, const uint8_t*& input)
{
    vertices.format = Read32(input);
    vertices.count = Read32(input);
    vertices.byteSize = Read32(input);
    vertices.unknown0c = Read32(input);
    vertices.data.assign(input, input + vertices.byteSize);
    input += vertices.byteSize & 0xfffffffc;
    vertices.count >>= 2;
    return 0;
}

int ParseIndices(SojIndices& indices, const uint8_t*& input)
{
    indices.format = Read32(input);
    indices.byteSize = Read32(input);
    indices.data.assign(input, input + indices.byteSize);
    input += indices.byteSize & 0xfffffffc;
    return 0;
}

int ParseSurface(SojSurface& surface, const uint8_t*& input)
{
    surface.unknown00 = Read32(input);
    surface.primitiveType = Read32(input);
    surface.primitiveCount = Read32(input);
    int status = ParseVertices(surface.vertices, input);
    if (status == 0) {
        status = ParseIndices(surface.indices, input);
        if (status == 0) {
            surface.indexCount = surface.primitiveType == 4
                                     ? surface.primitiveCount * 3
                                     : surface.primitiveCount + 2;
        }
    }
    return status;
}

} // namespace

int ObjectRes::Load(const char16_t* entryName, s3dFilePak* filePak)
{
    int status = filePak->Select(entryName);
    if (status != 0) {
        S3D_LOG(log, u"file %s not found!\n", entryName);
        return 3;
    }
    filePak->Seek(0, SEEK_END);
    int32_t bytes = filePak->Tell();
    if (bytes == -1) {
        return 8;
    }
    void* data = std::malloc(static_cast<size_t>(bytes));
    if (data == nullptr) {
        return 4;
    }
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data, bytes);
    status = ParseSoj(data);
    std::free(data);
    return status;
}

int ObjectRes::ParseSoj(const void* data)
{
    objects.clear();
    surfaces.clear();
    const uint8_t* input = static_cast<const uint8_t*>(data);
    objectCount = Read32(input);
    if (objectCount != 0) {
        objects.resize(objectCount);
        for (uint32_t i = 0; i < objectCount; ++i) {
            if (ParseObject(objects[i], input) != 0) {
                S3D_LOG(log, u"Object::LoadFromMemory: failed 1!\n");
                objects.clear();
                surfaces.clear();
                return 6;
            }
        }
    }

    surfaceCount = Read32(input);
    unknown138 = Read32(input);
    unknown13c = Read32(input);
    if (surfaceCount != 0) {
        surfaces.resize(surfaceCount);
        for (uint32_t i = 0; i < surfaceCount; ++i) {
            ParseSurface(surfaces[i], input);
        }
    }
    for (int32_t& value : trailer) {
        value = static_cast<int32_t>(Read32(input));
    }
    if (surfaceCount == 0) {
        objects.clear();
        surfaces.clear();
    }
    return 0;
}

uint32_t ObjectRes::GetObjectCount() const { return objectCount; }
uint32_t ObjectRes::GetSurfaceCount() const { return surfaceCount; }
const std::vector<SojObject>& ObjectRes::GetObjects() const { return objects; }
const std::vector<SojSurface>& ObjectRes::GetSurfaces() const { return surfaces; }
const std::array<int32_t, 6>& ObjectRes::GetTrailer() const { return trailer; }

} // namespace soft3d

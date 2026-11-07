#pragma once

#include "soft3d/Resource.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace soft3d {

struct SojObject {
    int32_t unknown00;
    int32_t unknown04;
    uint16_t color;
    uint16_t padding0a;
    uint32_t unknown0c;
    uint32_t textureIndex;
    uint16_t unknown14;
    uint16_t unknown16;
    std::array<int32_t, 4> unknown18;
    int32_t unknown28;
    int32_t unknown2c;
    uint32_t primitiveType;
};

struct SojVertices {
    uint32_t format;
    uint32_t count;
    uint32_t byteSize;
    uint32_t unknown0c;
    std::vector<uint8_t> data;
};

struct SojIndices {
    uint32_t format;
    uint32_t byteSize;
    std::vector<uint8_t> data;
};

struct SojSurface {
    uint32_t unknown00;
    uint32_t primitiveType;
    uint32_t primitiveCount;
    uint32_t indexCount;
    SojVertices vertices;
    SojIndices indices;
};

class ObjectRes : public Resource {
public:
    ObjectRes() = default;

    int Load(const char16_t* entryName, s3dFilePak* filePak) override;
    int ParseSoj(const void* data);

    uint32_t GetObjectCount() const;
    uint32_t GetSurfaceCount() const;
    const std::vector<SojObject>& GetObjects() const;
    const std::vector<SojSurface>& GetSurfaces() const;
    const std::array<int32_t, 6>& GetTrailer() const;

private:
    uint32_t objectCount = 0;
    std::vector<SojObject> objects;
    uint32_t surfaceCount = 0;
    uint32_t unknown138 = 0;
    uint32_t unknown13c = 0;
    std::vector<SojSurface> surfaces;
    std::array<int32_t, 6> trailer{};
};

} // namespace soft3d

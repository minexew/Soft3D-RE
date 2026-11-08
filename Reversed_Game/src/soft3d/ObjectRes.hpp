#pragma once

#include "soft3d/Resource.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace soft3d {

std::u16string DecodeSojTextureName(const std::array<int32_t, 4>& packedName);

struct SojObject {
    int32_t unknown00;
    int32_t unknown04;
    uint16_t color;
    uint16_t padding0a;
    uint32_t blendMode;
    uint32_t textureIndex;
    uint16_t hasTexture;
    uint16_t hasDetailTexture;
    std::array<int32_t, 4> packedTextureName;
    int32_t unknown28;
    int32_t unknown2c;
    uint32_t textureCoordinateMode;
};

struct SojVertices {
    uint32_t format;
    uint32_t strideBytes;
    uint32_t vertexCount;
    uint32_t dataByteSize;
    std::vector<uint8_t> data;
};

struct SojIndices {
    uint32_t format;
    uint32_t byteSize;
    std::vector<uint8_t> data;
};

struct SojSurface {
    int32_t materialIndex;
    uint32_t primitiveType;
    uint32_t primitiveCount;
    uint32_t indexCount;
    SojVertices vertices;
    SojIndices indices;
};

class ObjectRes : public Resource {
public:
    ObjectRes() = default;

    result Load(const char16_t* entryName, s3dFilePak* filePak) override;
    result ParseSoj(const void* data);

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

#pragma once

#include "soft3d/Raster.hpp"
#include "soft3d/s3dBase.hpp"

#include <array>

namespace soft3d {

class Renderer : public s3dBase {
public:
    Renderer(int width, int height);

    void SetBackBufferPtr(uint32_t* address);
    void SetModelMatrix(const Matrix4& matrix);
    void SetViewMatrix(const Matrix4& matrix);
    void SetProjectionMatrix(const Matrix4& matrix);
    void SetOrientation(int orientation);
    void SetCullMode(uint32_t mode);
    void SetBlendMode(uint32_t mode);
    void SetTexture(const Texture* texture);
    void SetDetailTexture(const Texture* texture);
    void SetTextureOffset(Fixed u, Fixed v);
    void SetTextureCoordinateMode(uint32_t mode);
    void SetDepthTestEnabled(bool enabled);
    void SetDepthWriteEnabled(bool enabled);
    void SetDepthBias(Fixed bias);
    void SetFlatColor(uint16_t color);
    void SetLightingEnabled(bool enabled);
    void SetAmbientLightOnly(bool enabled);
    void SetMaterialLighting(Fixed directionalScale, Fixed ambient,
                             Fixed objectIntensity = 0, Fixed animatedIntensity = 0,
                             Fixed minimumIntensity = 0);

    void DrawSequential(const SojVertices& vertices, uint32_t primitiveType,
                        uint32_t firstVertex, uint32_t primitiveCount);
    void DrawIndexed(const SojVertices& vertices, const SojIndices& indices,
                     uint32_t primitiveType, uint32_t firstIndex, uint32_t primitiveCount,
                     uint32_t firstVertex = 0);
    void DrawIndexedTextured(const SojVertices& vertices, const SojIndices& indices,
                             uint32_t primitiveType, uint32_t firstIndex,
                             uint32_t primitiveCount, uint32_t firstVertex = 0);
    void DrawIndexedTextured(const SojVertices& vertices, const SojVertices& secondaryVertices,
                             const SojIndices& indices, uint32_t primitiveType,
                             uint32_t firstIndex, uint32_t primitiveCount,
                             uint32_t firstVertex = 0);
    void DrawIndexedSurface(const SojSurface& surface, const SojObject* material,
                            const Texture* texture, const Texture* detailTexture,
                            const std::vector<Matrix4>& matrixPalette = {},
                            Fixed textureOffsetU = 0, Fixed textureOffsetV = 0,
                            Fixed materialIntensity = 0);
    void Clear(uint16_t color);
    void Present();

    Texture* GetCurrentTexture() const;
    Raster& GetRaster();

    struct Vertex {
        uint16_t clipFlags = 0;
        Vec3 camera;
        int32_t x = 0;
        int32_t y = 0;
        Fixed inverseDepth = 0;
        Fixed u = 0;
        Fixed v = 0;
        Fixed intensity = 0;
    };

private:
    int width;
    int height;
    Matrix4 modelMatrix;
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    Matrix4 modelViewMatrix;
    std::vector<Vertex> transformed;
    uint32_t cullMode = 0;
    uint32_t textureCoordinateMode = 0;
    Fixed textureOffsetU = 0;
    Fixed textureOffsetV = 0;
    const Texture* texture = nullptr;
    bool lightingEnabled = true;
    bool ambientLightOnly = false;
    Fixed directionalScale = 0;
    Fixed ambientIntensity = 0;
    Fixed objectIntensity = 0;
    Fixed animatedIntensity = 0;
    Fixed minimumIntensity = 0;
    Vec3 lightDirection = {0, 0, fixedOne};
    Raster raster;
    Texture frameTexture;

    void PrepareDrawState(uint32_t vertexFormat);
    void DrawIndexedBuffers(const SojVertices& vertices, const SojVertices* secondaryVertices,
                            const SojIndices& indices, uint32_t primitiveType,
                            uint32_t firstIndex, uint32_t primitiveCount,
                            uint32_t firstVertex);
    Vertex TransformVertex(const SojVertices& vertices, uint32_t index,
                           const std::vector<Matrix4>& matrixPalette,
                           const SojVertices* secondaryVertices = nullptr);
    void ProjectVertex(Vertex& vertex) const;
    void ClipAndRasterizeTriangle(const Vertex& first, const Vertex& second,
                                  const Vertex& third);
    void RasterizePolygon(const std::vector<Vertex>& polygon);
};

} // namespace soft3d

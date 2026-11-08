#pragma once

#include "soft3d/Raster.hpp"
#include "soft3d/s3dBase.hpp"

namespace soft3d {

class Renderer : public s3dBase {
public:
    Renderer(int width, int height);

    void SetBackBufferPtr(uint32_t* address);
    void SetModelMatrix(const Matrix4& matrix);
    void SetViewMatrix(const Matrix4& matrix);
    void SetProjectionMatrix(const Matrix4& matrix);
    void DrawIndexedSurface(const SojSurface& surface, const SojObject* material,
                            const Texture* texture, const Texture* detailTexture,
                            const std::vector<Matrix4>& matrixPalette = {});
    void Clear(uint16_t color);
    void Present();

    Texture* GetCurrentTexture() const;
    Raster& GetRaster();

private:
    int width;
    int height;
    Matrix4 modelMatrix;
    Matrix4 viewMatrix;
    Matrix4 projectionMatrix;
    Raster raster;
    Texture frameTexture;
};

} // namespace soft3d

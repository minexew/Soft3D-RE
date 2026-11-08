#pragma once

#include "soft3d/BasicShader.hpp"
#include "soft3d/Fixed.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/Texture.hpp"

#include <cstdint>
#include <vector>

namespace soft3d {

struct RasterVertex {
    Fixed x = 0;
    Fixed y = 0;
    Fixed z = fixedOne;
    Fixed u = 0;
    Fixed v = 0;
    uint16_t color = 0xffff;
};

class Raster {
public:
    Raster(int width, int height);

    void SetCurrentTexture(Texture* texture);
    Texture* GetCurrentTexture() const;
    void SetBackBufferPtr(uint32_t* address);
    void SelectShader(BasicShader* shader);
    void SetFogEnabled(bool enabled);
    void SetDepthTestEnabled(bool enabled);
    void Clear(uint16_t color);

    void RasterizeTriangle(const RasterVertex& first, const RasterVertex& second,
                           const RasterVertex& third, const Texture* texture);
    void Present();

    BasicShader& GetBasicShader();
    FogShader& GetFogShader();

private:
    int width;
    int height;
    Texture* previousTexture = nullptr;
    Texture* currentTexture = nullptr;
    uint32_t* backBuffer = nullptr;
    bool depthTest = true;
    bool fogEnabled = false;
    BasicShader basicShader;
    FogShader fogShader;
    BasicShader* activeShader;
};

} // namespace soft3d

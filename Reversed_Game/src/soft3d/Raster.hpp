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
    Fixed inverseDepth = fixedOne;
    Fixed u = 0;
    Fixed v = 0;
    Fixed intensity = 0;
    uint16_t color = 0xffff;
};

class Raster {
public:
    Raster(int width, int height);

    void SetCurrentTexture(Texture* texture);
    Texture* GetCurrentTexture() const;
    void SetBackBufferPtr(uint32_t* address);
    void SelectShader(BasicShader* shader);
    void SetOrientation(int orientation);
    void SetFogEnabled(bool enabled);
    void SetTexture(const Texture* texture);
    void SetDetailTexture(const Texture* texture);
    void SetBlendMode(uint32_t mode);
    void SetDepthTestEnabled(bool enabled);
    void SetDepthWriteEnabled(bool enabled);
    void SetDepthBias(Fixed bias);
    void SetFlatColor(uint16_t color);
    void SetShadingEnabled(bool enabled);
    void Clear(uint16_t color);

    void RasterizeTriangle(const RasterVertex& first, const RasterVertex& second,
                           const RasterVertex& third);
    void Present();

    BasicShader& GetBasicShader();
    FogShader& GetFogShader();
    BasicShader* GetActiveShader() const;

private:
    struct Vertex {
        int32_t x = 0;
        int32_t y = 0;
        Fixed inverseDepth = 0;
        Fixed u = 0;
        Fixed v = 0;
        int32_t intensity = 0;
    };

    struct AttributeDeltas {
        Fixed depthX = 0;
        Fixed depthY = 0;
        Fixed uX = 0;
        Fixed uY = 0;
        Fixed vX = 0;
        Fixed vY = 0;
        Fixed intensityX = 0;
        Fixed intensityY = 0;
    };

    struct Edge {
        int32_t x = 0;
        int32_t error = -1;
        int32_t step = 0;
        int32_t errorStep = 0;
        int32_t height = 0;
        Fixed inverseDepth = 0;
        Fixed u = 0;
        Fixed v = 0;
        int32_t intensity = 0;
        void Advance(const AttributeDeltas& deltas);
    };

    int width;
    int height;
    Texture* previousTexture = nullptr;
    Texture* currentTexture = nullptr;
    const Texture* texture = nullptr;
    const Texture* detailTexture = nullptr;
    uint32_t* backBuffer = nullptr;
    uint32_t blendMode = 0;
    Fixed depthBias = 0;
    uint16_t flatColor = 0;
    bool depthTest = true;
    bool depthWrite = true;
    bool shading = false;
    bool fogEnabled = false;
    BasicShader basicShader;
    FogShader fogShader;
    BasicShader* activeShader;

    AttributeDeltas ComputeAttributeDeltas(const Vertex& first, const Vertex& second,
                                           const Vertex& third, int32_t area) const;
    static Edge SetupEdge(const Vertex& from, const Vertex& to);
    void DrawSpan(int32_t y, const Edge& left, const Edge& right,
                  const AttributeDeltas& deltas);
};

} // namespace soft3d

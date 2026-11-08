#pragma once

#include <cstdint>
#include <vector>

namespace soft3d {

class BasicShader {
public:
    BasicShader(int width, int height);
    virtual ~BasicShader() = default;

    virtual void Render(const void* source, uint32_t* destination);
    void SetEnabled(int enabled);

    int width;
    int height;
    int enabled;

protected:
    static uint32_t ExpandRgb565(uint16_t pixel);
};

class FogShader : public BasicShader {
public:
    FogShader(int width, int height);

    void Render(const void* source, uint32_t* destination) override;
    void SetFog(int start, int end, uint16_t color);

private:
    void BuildAttenuationTable();
    void BuildColorTable();

    int fullFogDepth = 0;
    int fogStartDepth = 0;
    int rangeStart = 0;
    int rangeEnd = 0;
    std::vector<uint8_t> attenuation;
    std::vector<uint16_t> colors;
    uint16_t fogColor = 0;
};

} // namespace soft3d

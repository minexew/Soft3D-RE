#pragma once

#include "soft3d/Texture.hpp"

#include <cstdint>

namespace soft3d {

struct ScreenRect {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;
};

class ScreenBlitter {
public:
    void SetTexture(Texture* texture);
    Texture* GetTexture() const;
    void Clear(uint8_t red, uint8_t green, uint8_t blue);
    void Blit(const Texture* source, ScreenRect destination, ScreenRect sourceRect,
              uint32_t mode = 0);

private:
    Texture* target = nullptr;
};

} // namespace soft3d

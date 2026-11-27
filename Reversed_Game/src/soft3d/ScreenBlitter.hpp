#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/Texture.hpp"

#include <cstdint>

namespace soft3d {

struct SdtAnimationKey;

struct S3dPoint {
    int32_t x = 0;
    int32_t y = 0;

    void Set(int32_t x, int32_t y);
};

struct ScreenRect {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;

    ScreenRect() = default;
    ScreenRect(int32_t left, int32_t top, int32_t right, int32_t bottom);
    ScreenRect(const S3dPoint& position, const S3dPoint& size);

    void Zero();
    void Set(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void AssignPosition(const S3dPoint& position);
    void AssignSdtPosition(const SdtAnimationKey& key);
    void Offset(int32_t x, int32_t y);
    void OffsetMirrored(int32_t x, int32_t y);
    void OffsetMirroredBoth(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void OffsetReverse(int32_t x, int32_t y);
    void OffsetReverseBoth(int32_t left, int32_t top, int32_t right, int32_t bottom);
    void SetLeft(int32_t left);
    void SetTop(int32_t top);
    void SetPosition(int32_t x, int32_t y);
    bool IsEmpty() const;
    bool IsZero() const;
    bool Contains(const S3dPoint& point) const;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    void Include(const ScreenRect& other);
    void Clamp(const ScreenRect& other);

    bool operator==(const ScreenRect& other) const;
    bool operator!=(const ScreenRect& other) const;
};

class ScreenBlitter {
public:
    void SetTexture(Texture* texture);
    Texture* GetTexture() const;
    void Clear(uint8_t red, uint8_t green, uint8_t blue);
    void FillRect(uint16_t color, ScreenRect rectangle);
    void Blit(const Texture* source, ScreenRect destination, ScreenRect sourceRect,
              uint32_t mode = 0, const Texture* secondary = nullptr);
    void BlitUnscaled(const Texture* source, int32_t x, int32_t y, ScreenRect sourceRect,
                      uint32_t mode = 0, const Texture* secondary = nullptr);

private:
    Texture* target = nullptr;
};

} // namespace soft3d

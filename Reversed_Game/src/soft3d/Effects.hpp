#pragma once

#include "soft3d/Camera.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/ScreenBlitter.hpp"
#include "soft3d/TextureRes.hpp"

#include <vector>

namespace soft3d {

class s3dResManager;

class Lensflare : public Renderable {
public:
    Lensflare(Renderer* renderer, ScreenBlitter* blitter, s3dResManager* manager = nullptr);

    void SetSource(const Vec3& source);
    result LoadElement(uint32_t index, const char16_t* name, Fixed offset,
                       int cacheLevel = 3, const char16_t* directory = u".\\ui\\");
    void SetElement(uint32_t index, const Texture* texture, Fixed offset);
    void ClearElements();
    bool IsElementEnabled(uint32_t index) const;
    void Render() override;

private:
    struct Element {
        bool enabled = false;
        Fixed offset = 0;
        const Texture* texture = nullptr;
        const TextureRes* resource = nullptr;
    };

    Renderer* renderer;
    ScreenBlitter* blitter;
    s3dResManager* resourceManager;
    Vec3 source;
    std::array<Element, 5> elements{};
};

struct Billboard {
    Fixed halfWidth = 0;
    Fixed halfHeight = 0;
    Vec3 position;
};

class BillboardGroup : public Renderable {
public:
    BillboardGroup(Renderer* renderer, Camera* camera, uint16_t capacity);

    bool AddBillboard(const Billboard& billboard);
    void ClearBillboards();
    void SetTexture(const Texture* texture);
    void SetBlendMode(uint32_t mode);
    uint16_t GetBillboardCount() const;
    void Render() override;

private:
    Renderer* renderer;
    Camera* camera;
    uint16_t capacity;
    std::vector<Billboard> billboards;
    const Texture* texture = nullptr;
    uint32_t blendMode = 0;
    SojVertices vertices;
};

class FakeShadow : public Renderable {
public:
    explicit FakeShadow(Renderer* renderer);

    void Initialize(Fixed radius);
    void ScaleXZ(Fixed scale);
    void SetColor(uint8_t red, uint8_t green, uint8_t blue);
    void SetTransform(const Matrix4& matrix);
    void SetSecondaryTransform(const Matrix4& matrix);
    const SojVertices& GetVertices() const;
    void Render() override;

private:
    Renderer* renderer;
    SojVertices vertices;
    Matrix4 transform = Matrix4::Identity();
    Matrix4 secondaryTransform = Matrix4::Identity();
    uint16_t color = 0;
};

class GunFire : public Renderable {
public:
    GunFire(Renderer* renderer, Camera* camera);

    result LoadTextures(s3dResManager* manager);
    void SetTextures(const Texture* texture, const Texture* detailTexture);
    void SetPosition(const Vec3& position);
    void Trigger(const Vec3& position, int32_t frames = 2);
    int32_t GetFramesRemaining() const;
    const SojVertices& GetVertices() const;
    void Render() override;

private:
    Renderer* renderer;
    Camera* camera;
    SojVertices vertices;
    Vec3 position;
    const Texture* texture = nullptr;
    const Texture* detailTexture = nullptr;
    const TextureRes* textureResource = nullptr;
    const TextureRes* detailTextureResource = nullptr;
    int32_t framesRemaining = 0;
};

} // namespace soft3d

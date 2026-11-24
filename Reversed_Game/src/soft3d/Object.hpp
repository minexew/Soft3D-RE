#pragma once

#include "soft3d/Animation.hpp"
#include "soft3d/Geometry.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/TextureRes.hpp"

namespace soft3d {

class s3dResManager;

enum class ObjectRenderMode {
    Normal,
    SecondaryStreams,
    Wireframe,
};

class Object : public Renderable {
public:
    explicit Object(Renderer* renderer);

    result Load(s3dResManager* manager, const char16_t* entryName, int cacheLevel,
                const char16_t* directory = nullptr);
    void SetModel(const ObjectRes* model);
    result LoadMaterials(s3dResManager* manager, int cacheLevel,
                         const char16_t* directory = nullptr);
    result LoadMaterials(int cacheLevel, const char16_t* directory = nullptr);

    void SetTransformVectors(const std::array<Vec3, 4>& transform);
    void ResetTransform();
    void RestoreAxes(const Vec3& axisZ, const Vec3& axisX, const Vec3& axisY);
    void SetModelMatrix(const Matrix4& matrix);
    void SetSecondaryTransform(const Matrix4& matrix);
    void SetTransformMode(int mode);
    void SetPosition(const Vec3& position);
    void SetAxisX(const Vec3& axis);
    void SetAxisY(const Vec3& axis);
    void SetAxisZ(const Vec3& axis);
    void RotateX(uint16_t angle);
    void RotateY(uint16_t angle);
    void RotateZ(uint16_t angle);
    void Rotate(uint16_t xAngle, uint16_t yAngle, uint16_t zAngle);
    void Scale(Fixed x, Fixed y, Fixed z);
    void UpdateTransform();
    void UpdateFrustumVisibility();
    const Matrix4& GetModelMatrix() const;

    void SetAnimation(const Animation* animation);
    bool AdvanceAnimation();
    void SetAnimationTime(int32_t time);
    void SetAnimationPlayback(int32_t direction, bool loop);
    void SetAnimationSpeed(int32_t speed);
    void SetAnimationPaused(bool paused);
    void SetBoneTransform(int32_t track, const Matrix4& transform);
    Vec3 SampleMorph(uint32_t target) const;
    result LoadTextureOverride(uint32_t material, const char16_t* name, int cacheLevel);

    void SetLightingEnabled(bool enabled);
    void SetAmbientLightOnly(bool enabled);
    void SetExternalTransformEnabled(bool enabled);
    void SetFrustumCulled(bool culled) { frustumCulled = culled; }
    void SetObjectIntensity(Fixed intensity);
    void SetDepthBias(Fixed bias);
    void SetSecondaryVertices(const SojVertices* vertices);
    void SetRenderMode(ObjectRenderMode mode);
    void SetMaterialIntensityBounce(uint32_t material, Fixed minimum, Fixed maximum,
                                    int32_t period);
    void SetMaterialBlendMode(uint32_t material, uint32_t mode);
    void SetMaterialTextureScroll(uint32_t material, Fixed u, Fixed v);
    void SetMaterialTextureOffset(uint32_t material, Fixed u, Fixed v);

    void Render() override;
    void RenderNormal();
    void RenderSecondaryStreams();
    void RenderWireframe();

    bool HasTransparency() const;
    const SojSurface* GetSurface(uint32_t index) const;
    uint32_t GetVertexCount() const;
    uint32_t GetTriangleCount() const;
    Fixed GetCameraDepth() const;
    const ObjectBounds& GetPrimaryBounds() const;
    const ObjectBounds& GetSecondaryBounds() const;
    const ObjectBounds& GetWorldBounds() const;
    const std::vector<Matrix4>& GetMatrixPalette() const;
    const Matrix4& GetSecondaryTransform() const { return secondaryTransform; }
    const ObjectRes* GetModel() const;
    const Animation* GetAnimation() const;
    int32_t GetAnimationTime() const;
    int32_t GetAnimationDirection() const;
    int32_t GetAnimationSpeed() const;
    bool IsAnimationLooping() const;
    bool IsAnimationPaused() const;
    const Vec3& GetPosition() const;
    const Vec3& GetAxisX() const;
    const Vec3& GetAxisY() const;
    const Vec3& GetAxisZ() const;
    Fixed GetObjectIntensity() const;
    Fixed GetDepthBias() const;
    bool IsLightingEnabled() const;
    bool IsAmbientLightOnly() const;
    bool IsExternalTransformEnabled() const;
    bool IsFrustumCulled() const { return frustumCulled; }
    std::array<Fixed, 4> GetMaterialIntensityState(uint32_t material) const;
    void SetMaterialIntensityState(uint32_t material, Fixed minimum, Fixed maximum,
                                   Fixed intensity, Fixed speed);
    void RestoreAnimationState(int32_t paused, int32_t time, int32_t direction,
                               int32_t loop, int32_t speed);
    uint32_t GetMaterialBlendMode(uint32_t material) const;
    std::array<Fixed, 4> GetMaterialTextureState(uint32_t material) const;
    ObjectRenderMode GetRenderMode() const;

private:
    struct MaterialState {
        Fixed textureU = 0;
        Fixed textureV = 0;
        Fixed textureScrollU = 0;
        Fixed textureScrollV = 0;
        Fixed minimumIntensity = 0x1999;
        Fixed maximumIntensity = fixedOne;
        Fixed intensity = 0x1999;
        Fixed intensitySpeed = 0;
        uint32_t blendMode = 0;
    };

    void RebuildLocalCorners();
    void UpdateMatrixPalette();
    void UpdateMaterial(MaterialState& state, const SojObject& material,
                        const Texture* texture);

    Renderer* renderer;
    s3dResManager* resourceManager = nullptr;
    const ObjectRes* model = nullptr;
    const Animation* animation = nullptr;
    Matrix4 modelMatrix = Matrix4::Identity();
    Matrix4 secondaryTransform;
    Matrix4 boneTransform = Matrix4::Identity();
    int transformMode = 0;
    int32_t boneTrack = -1;
    Vec3 position;
    Vec3 axisX = {fixedOne, 0, 0};
    Vec3 axisY = {0, fixedOne, 0};
    Vec3 axisZ = {0, 0, fixedOne};
    Fixed axisLengthX = fixedOne;
    Fixed axisLengthY = fixedOne;
    Fixed axisLengthZ = fixedOne;
    std::array<Vec3, 8> localCorners{};
    ObjectBounds primaryBounds;
    ObjectBounds secondaryBounds;
    ObjectBounds worldBounds;
    std::vector<Matrix4> matrixPalette;
    std::vector<const Texture*> textures;
    std::vector<const TextureRes*> textureResources;
    std::vector<const Texture*> detailTextures;
    std::vector<const TextureRes*> detailTextureResources;
    std::vector<MaterialState> materialStates;
    const SojVertices* secondaryVertices = nullptr;
    ObjectRenderMode renderMode = ObjectRenderMode::Normal;
    int32_t animationTime = 0;
    int32_t animationDirection = 1;
    int32_t animationSpeed = 300;
    bool loopAnimation = true;
    bool animationPaused = false;
    bool lightingEnabled = true;
    bool ambientLightOnly = false;
    bool externalTransformEnabled = false;
    bool frustumCulled = false;
    Fixed objectIntensity = 0;
    Fixed depthBias = 0;
};

} // namespace soft3d

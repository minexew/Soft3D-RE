#pragma once

#include "soft3d/Animation.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/Renderer.hpp"
#include "soft3d/TextureRes.hpp"

namespace soft3d {

class s3dResManager;

class Object : public Renderable {
public:
    explicit Object(Renderer* renderer);

    void SetModel(const ObjectRes* model);
    void SetTransformVectors(const std::array<Vec3, 4>& transform);
    const Matrix4& GetModelMatrix() const;
    result LoadMaterials(s3dResManager* manager, int cacheLevel,
                         const char16_t* directory = nullptr);
    void SetAnimation(const Animation* animation);
    bool AdvanceAnimation();
    void SetAnimationTime(int32_t time);
    void Render() override;

    const ObjectRes* GetModel() const;
    const Animation* GetAnimation() const;
    int32_t GetAnimationTime() const;

private:
    void UpdateMatrixPalette();

    Renderer* renderer;
    const ObjectRes* model = nullptr;
    const Animation* animation = nullptr;
    Matrix4 modelMatrix = Matrix4::Identity();
    std::vector<Matrix4> matrixPalette;
    std::vector<const Texture*> textures;
    std::vector<const Texture*> detailTextures;
    int32_t animationTime = 0;
    int32_t animationDirection = 1;
    int32_t animationSpeed = 300;
    bool loopAnimation = true;
};

} // namespace soft3d

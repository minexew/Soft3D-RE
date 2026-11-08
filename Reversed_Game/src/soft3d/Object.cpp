#include "soft3d/Object.hpp"

#include "soft3d/s3dResManager.hpp"

namespace soft3d {

Object::Object(Renderer* objectRenderer)
    : renderer(objectRenderer)
{
}

void Object::SetModel(const ObjectRes* resource)
{
    model = resource;
    matrixPalette.clear();
    textures.assign(resource != nullptr ? resource->GetObjects().size() : 0, nullptr);
    detailTextures.assign(textures.size(), nullptr);
}

result Object::LoadMaterials(s3dResManager* manager, int cacheLevel,
                             const char16_t* directory)
{
    if (model == nullptr) {
        return S3D_FILE;
    }
    for (std::size_t index = 0; index < model->GetObjects().size(); ++index) {
        const SojObject& material = model->GetObjects()[index];
        if (material.hasTexture == 0) {
            continue;
        }
        std::u16string name = DecodeSojTextureName(material.packedTextureName);
        auto* resource = dynamic_cast<TextureRes*>(
            manager->GetResource(ResourceTexture, cacheLevel, name.c_str(), directory));
        if (resource == nullptr) {
            return S3D_FILE;
        }
        textures[index] = resource->GetTexture();
        if (material.hasDetailTexture != 0) {
            name += u"_alp";
            resource = dynamic_cast<TextureRes*>(
                manager->GetResource(ResourceTexture, cacheLevel, name.c_str(), directory));
            if (resource == nullptr) {
                return S3D_FILE;
            }
            detailTextures[index] = resource->GetTexture();
        }
    }
    return S3D_OK;
}

void Object::SetAnimation(const Animation* resource)
{
    if (model == nullptr) {
        return;
    }
    animation = resource;
    animationTime = animation != nullptr ? animation->GetStartTime() : 0;
    if (animation == nullptr) {
        matrixPalette.clear();
        return;
    }
    if (animation->GetBoundingVolumeCount() == 0) {
        return;
    }
    matrixPalette.resize(static_cast<uint16_t>(animation->GetTrackCount()));
}

bool Object::AdvanceAnimation()
{
    if (model == nullptr || animation == nullptr) {
        return false;
    }
    animationTime += animationSpeed * animationDirection;
    bool completed = false;
    if (animationDirection == 1 && animationTime >= animation->GetEndTime()) {
        completed = true;
    }
    if (animationDirection == -1 && animationTime <= animation->GetStartTime()) {
        completed = true;
    }
    if (animationTime >= animation->GetEndTime()) {
        if (loopAnimation) {
            animationTime = animation->GetStartTime() +
                (animationTime - animation->GetEndTime());
        } else {
            animationTime = animation->GetEndTime();
        }
    }
    if (animationTime < animation->GetStartTime()) {
        if (loopAnimation) {
            animationTime = animation->GetEndTime() -
                (animation->GetStartTime() - animationTime);
        } else {
            animationTime = animation->GetStartTime();
        }
    }
    return completed;
}

void Object::SetAnimationTime(int32_t time)
{
    if (model == nullptr || animation == nullptr) {
        return;
    }
    animationTime = time;
    if (animationTime == -1) {
        animationTime = animation->GetEndTime();
    }
    if (animationTime > animation->GetEndTime()) {
        animationTime = animation->GetStartTime();
    }
}

void Object::UpdateMatrixPalette()
{
    if (animation == nullptr) {
        return;
    }
    for (uint32_t track = 0; track < matrixPalette.size(); ++track) {
        matrixPalette[track] = Multiply(modelMatrix,
                                        animation->SampleTransform(track, animationTime));
    }
}

void Object::Render()
{
    if (model == nullptr) {
        return;
    }
    renderer->SetModelMatrix(modelMatrix);
    UpdateMatrixPalette();
    for (const SojSurface& surface : model->GetSurfaces()) {
        const SojObject* material = nullptr;
        const Texture* texture = nullptr;
        const Texture* detail = nullptr;
        if (surface.materialIndex >= 0 &&
            static_cast<std::size_t>(surface.materialIndex) < model->GetObjects().size()) {
            material = &model->GetObjects()[static_cast<std::size_t>(surface.materialIndex)];
            const std::size_t index = static_cast<std::size_t>(surface.materialIndex);
            if (index < textures.size()) {
                texture = textures[index];
            }
            if (index < detailTextures.size()) {
                detail = detailTextures[index];
            }
        }
        renderer->DrawIndexedSurface(surface, material, texture, detail, matrixPalette);
    }
}

const ObjectRes* Object::GetModel() const
{
    return model;
}

const Animation* Object::GetAnimation() const
{
    return animation;
}

int32_t Object::GetAnimationTime() const
{
    return animationTime;
}

} // namespace soft3d

#include "soft3d/Object.hpp"

#include "soft3d/s3dResManager.hpp"

#include <algorithm>

namespace soft3d {
namespace {

ObjectBounds ToBounds(const std::array<int32_t, 6>& values)
{
    return {{values[0], values[1], values[2]}, {values[3], values[4], values[5]}};
}

} // namespace

Object::Object(Renderer* objectRenderer)
    : renderer(objectRenderer)
{
}

result Object::Load(s3dResManager* manager, const char16_t* entryName, int cacheLevel,
                    const char16_t* directory)
{
    resourceManager = manager;
    auto* resource = dynamic_cast<const ObjectRes*>(manager->GetResource(
        ResourceObject, cacheLevel, entryName, directory));
    if (resource == nullptr) {
        return S3D_FILE;
    }
    SetModel(resource);
    return S3D_OK;
}

void Object::SetModel(const ObjectRes* resource)
{
    model = resource;
    animation = nullptr;
    animationPaused = false;
    boneTrack = -1;
    matrixPalette.clear();
    const std::size_t count = model != nullptr ? model->GetObjects().size() : 0;
    textures.assign(count, nullptr);
    textureResources.assign(count, nullptr);
    detailTextures.assign(count, nullptr);
    detailTextureResources.assign(count, nullptr);
    materialStates.assign(count, MaterialState{});
    for (std::size_t index = 0; index < count; ++index) {
        materialStates[index].blendMode = model->GetObjects()[index].blendMode;
    }
    if (model != nullptr) {
        primaryBounds = ToBounds(model->GetTrailer());
        secondaryBounds = primaryBounds;
        worldBounds = primaryBounds;
        RebuildLocalCorners();
    }
}

result Object::LoadMaterials(s3dResManager* manager, int cacheLevel,
                             const char16_t* directory)
{
    resourceManager = manager;
    return LoadMaterials(cacheLevel, directory);
}

result Object::LoadMaterials(int cacheLevel, const char16_t* directory)
{
    if (model == nullptr || resourceManager == nullptr) {
        return S3D_FILE;
    }
    for (std::size_t index = 0; index < model->GetObjects().size(); ++index) {
        const SojObject& material = model->GetObjects()[index];
        if (material.hasTexture == 0) {
            continue;
        }
        std::u16string name = DecodeSojTextureName(material.packedTextureName);
        auto* resource = dynamic_cast<TextureRes*>(resourceManager->GetResource(
            ResourceTexture, cacheLevel, name.c_str(), directory));
        if (resource == nullptr) {
            return S3D_FILE;
        }
        textureResources[index] = resource;
        textures[index] = resource->GetTexture();
        if (material.hasDetailTexture != 0) {
            name += u"_alp";
            resource = dynamic_cast<TextureRes*>(resourceManager->GetResource(
                ResourceTexture, cacheLevel, name.c_str(), directory));
            if (resource == nullptr) {
                return S3D_FILE;
            }
            detailTextureResources[index] = resource;
            detailTextures[index] = resource->GetTexture();
        }
    }
    return S3D_OK;
}

void Object::SetTransformVectors(const std::array<Vec3, 4>& transform)
{
    Matrix4 matrix = Matrix4::Identity();
    for (std::size_t axis = 0; axis < 3; ++axis) {
        matrix.values[axis] = (&transform[2].x)[axis];
        matrix.values[4 + axis] = (&transform[3].x)[axis];
        matrix.values[8 + axis] = (&transform[1].x)[axis];
        matrix.values[12 + axis] = (&transform[0].x)[axis];
    }
    SetModelMatrix(matrix);
}

void Object::SetModelMatrix(const Matrix4& matrix)
{
    modelMatrix = matrix;
}

void Object::SetSecondaryTransform(const Matrix4& matrix)
{
    secondaryTransform = matrix;
}

void Object::SetTransformMode(int mode)
{
    transformMode = mode;
}

void Object::SetPosition(const Vec3& newPosition)
{
    position = newPosition;
    UpdateTransform();
}

void Object::SetAxisX(const Vec3& axis)
{
    axisX = axis;
    axisLengthX = Length(axis);
}

void Object::SetAxisY(const Vec3& axis)
{
    axisY = axis;
    axisLengthY = Length(axis);
}

void Object::SetAxisZ(const Vec3& axis)
{
    axisZ = axis;
    axisLengthZ = Length(axis);
}

void Object::RotateX(uint16_t angle)
{
    axisX = soft3d::Scale(Normalize(soft3d::RotateX(axisX, angle)), axisLengthX);
    axisY = soft3d::Scale(Normalize(soft3d::RotateX(axisY, angle)), axisLengthY);
    axisZ = soft3d::Scale(Normalize(soft3d::RotateX(axisZ, angle)), axisLengthZ);
}

void Object::RotateY(uint16_t angle)
{
    axisX = soft3d::Scale(Normalize(soft3d::RotateY(axisX, angle)), axisLengthX);
    axisY = soft3d::Scale(Normalize(soft3d::RotateY(axisY, angle)), axisLengthY);
    axisZ = soft3d::Scale(Normalize(soft3d::RotateY(axisZ, angle)), axisLengthZ);
}

void Object::RotateZ(uint16_t angle)
{
    axisX = soft3d::Scale(Normalize(soft3d::RotateZ(axisX, angle)), axisLengthX);
    axisY = soft3d::Scale(Normalize(soft3d::RotateZ(axisY, angle)), axisLengthY);
    axisZ = soft3d::Scale(Normalize(soft3d::RotateZ(axisZ, angle)), axisLengthZ);
}

void Object::Rotate(uint16_t xAngle, uint16_t yAngle, uint16_t zAngle)
{
    axisX = soft3d::Scale(Normalize(soft3d::Rotate(axisX, xAngle, yAngle, zAngle)), axisLengthX);
    axisY = soft3d::Scale(Normalize(soft3d::Rotate(axisY, xAngle, yAngle, zAngle)), axisLengthY);
    axisZ = soft3d::Scale(Normalize(soft3d::Rotate(axisZ, xAngle, yAngle, zAngle)), axisLengthZ);
}

void Object::Scale(Fixed x, Fixed y, Fixed z)
{
    axisX = soft3d::Scale(axisX, x);
    axisY = soft3d::Scale(axisY, y);
    axisZ = soft3d::Scale(axisZ, z);
    axisLengthX = FixedMultiply(axisLengthX, x);
    axisLengthY = FixedMultiply(axisLengthY, y);
    axisLengthZ = FixedMultiply(axisLengthZ, z);
}

void Object::UpdateTransform()
{
    if (model == nullptr) {
        return;
    }
    if (cullingDisabled) {
        modelMatrix = secondaryTransform;
        for (unsigned column = 0; column < 4; ++column) {
            modelMatrix.values[column * 4] = FixedMultiply(modelMatrix.values[column * 4],
                                                           axisLengthX);
            modelMatrix.values[column * 4 + 1] = FixedMultiply(modelMatrix.values[column * 4 + 1],
                                                               axisLengthY);
        }
    } else {
        modelMatrix = Matrix4::Identity();
        for (unsigned axis = 0; axis < 3; ++axis) {
            modelMatrix.values[axis] = (&axisX.x)[axis];
            modelMatrix.values[4 + axis] = (&axisY.x)[axis];
            modelMatrix.values[8 + axis] = (&axisZ.x)[axis];
        }
        modelMatrix = Transpose(modelMatrix);
    }
    modelMatrix.values[12] = AddFixed(modelMatrix.values[12], position.x);
    modelMatrix.values[13] = AddFixed(modelMatrix.values[13], position.y);
    modelMatrix.values[14] = AddFixed(modelMatrix.values[14], position.z);
    if (cullingDisabled) {
        return;
    }
    worldBounds.minimum = worldBounds.maximum = Transform(modelMatrix, localCorners[0]);
    for (std::size_t index = 1; index < localCorners.size(); ++index) {
        const Vec3 point = Transform(modelMatrix, localCorners[index]);
        worldBounds.minimum.x = std::min(worldBounds.minimum.x, point.x);
        worldBounds.minimum.y = std::min(worldBounds.minimum.y, point.y);
        worldBounds.minimum.z = std::min(worldBounds.minimum.z, point.z);
        worldBounds.maximum.x = std::max(worldBounds.maximum.x, point.x);
        worldBounds.maximum.y = std::max(worldBounds.maximum.y, point.y);
        worldBounds.maximum.z = std::max(worldBounds.maximum.z, point.z);
    }
}

const Matrix4& Object::GetModelMatrix() const
{
    return modelMatrix;
}

void Object::SetAnimation(const Animation* resource)
{
    if (model == nullptr) {
        return;
    }
    if (resource == nullptr) {
        animation = nullptr;
        primaryBounds = ToBounds(model->GetTrailer());
        animationPaused = false;
        matrixPalette.clear();
        return;
    }
    if (animation == nullptr || animation->GetTrackCount() < resource->GetTrackCount()) {
        matrixPalette.resize(static_cast<uint16_t>(resource->GetTrackCount()));
    }
    if (resource->GetBoundingVolumeCount() != 0) {
        primaryBounds = ToBounds(resource->GetFirstBounds());
        secondaryBounds = ToBounds(resource->GetSecondBounds());
    }
    animationTime = resource->GetStartTime();
    animation = resource;
    animationSpeed = 300;
}

bool Object::AdvanceAnimation()
{
    if (model == nullptr || animation == nullptr || animationPaused) {
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
        animationTime = loopAnimation
            ? animation->GetStartTime() + (animationTime - animation->GetEndTime())
            : animation->GetEndTime();
    }
    if (animationTime < animation->GetStartTime()) {
        animationTime = loopAnimation
            ? animation->GetEndTime() - (animation->GetStartTime() - animationTime)
            : animation->GetStartTime();
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

void Object::SetAnimationPlayback(int32_t direction, bool loop)
{
    if (animation == nullptr) {
        return;
    }
    animationDirection = direction;
    loopAnimation = loop;
    animationTime = direction == 1 ? animation->GetStartTime() : animation->GetEndTime();
}

void Object::SetAnimationSpeed(int32_t speed)
{
    animationSpeed = speed;
}

void Object::SetAnimationPaused(bool paused)
{
    animationPaused = paused;
}

void Object::SetBoneTransform(int32_t track, const Matrix4& transform)
{
    boneTrack = track;
    boneTransform = transform;
}

void Object::SetLightingEnabled(bool enabled) { lightingEnabled = enabled; }
void Object::SetAmbientLightOnly(bool enabled) { ambientLightOnly = enabled; }
void Object::SetCullingDisabled(bool disabled) { cullingDisabled = disabled; }
void Object::SetObjectIntensity(Fixed intensity) { objectIntensity = intensity; }
void Object::SetDepthBias(Fixed bias) { depthBias = bias; }
void Object::SetSecondaryVertices(const SojVertices* vertices) { secondaryVertices = vertices; }
void Object::SetRenderMode(ObjectRenderMode mode) { renderMode = mode; }

void Object::SetMaterialIntensityBounce(uint32_t material, Fixed minimum, Fixed maximum,
                                        int32_t period)
{
    if (material >= materialStates.size()) {
        return;
    }
    MaterialState& state = materialStates[material];
    state.minimumIntensity = minimum;
    state.maximumIntensity = maximum;
    state.intensity = 0x1999;
    state.intensitySpeed = period == 0 ? 0 : FixedMultiply(
        SubtractFixed(maximum, minimum), FixedDivide(fixedOne, period << 16));
}

void Object::SetMaterialBlendMode(uint32_t material, uint32_t mode)
{
    if (material < materialStates.size()) {
        materialStates[material].blendMode = mode;
    }
}

void Object::RebuildLocalCorners()
{
    const Vec3 center = ShiftRight(Add(primaryBounds.minimum, primaryBounds.maximum), 1);
    const Vec3 extent = ShiftRight(Subtract(primaryBounds.maximum, primaryBounds.minimum), 1);
    localCorners = {{
        {center.x - extent.x, center.y - extent.y, center.z - extent.z},
        {center.x - extent.x, center.y - extent.y, center.z + extent.z},
        {center.x + extent.x, center.y - extent.y, center.z + extent.z},
        {center.x + extent.x, center.y - extent.y, center.z - extent.z},
        {center.x - extent.x, center.y + extent.y, center.z - extent.z},
        {center.x - extent.x, center.y + extent.y, center.z + extent.z},
        {center.x + extent.x, center.y + extent.y, center.z + extent.z},
        {center.x + extent.x, center.y + extent.y, center.z - extent.z},
    }};
}

void Object::UpdateMatrixPalette()
{
    if (model == nullptr || animation == nullptr) {
        return;
    }
    Matrix4 transform = transformMode == 2
        ? Multiply(Multiply(modelMatrix, secondaryTransform), renderer->GetViewMatrix())
        : Multiply(modelMatrix, renderer->GetViewMatrix());
    const int32_t trackCount = animation->GetTrackCount();
    for (int32_t track = 0; track < trackCount; ++track) {
        Matrix4 sampled = animation->SampleTransform(static_cast<uint32_t>(track), animationTime);
        if (track == boneTrack) {
            sampled = Multiply(boneTransform, sampled);
        }
        matrixPalette[static_cast<std::size_t>(track)] = Multiply(sampled, transform);
    }
}

void Object::UpdateMaterial(MaterialState& state, const SojObject& material,
                            const Texture* texture)
{
    state.intensity = AddFixed(state.intensity, state.intensitySpeed);
    if (state.intensity >= state.maximumIntensity) {
        state.intensity = state.maximumIntensity;
        state.intensitySpeed = NegateFixed(state.intensitySpeed);
    } else if (state.intensity <= state.minimumIntensity) {
        state.intensity = state.minimumIntensity;
        state.intensitySpeed = NegateFixed(state.intensitySpeed);
    }
    if (texture == nullptr) {
        return;
    }
    state.textureU = AddFixed(state.textureU, material.textureScrollU);
    const Fixed width = texture->GetFixedWidth();
    if (state.textureU >= width) {
        state.textureU = SubtractFixed(state.textureU, width);
    } else if (state.textureU <= NegateFixed(width)) {
        state.textureU = AddFixed(state.textureU, width);
    }
    state.textureV = AddFixed(state.textureV, material.textureScrollV);
    const Fixed height = texture->GetFixedHeight();
    if (state.textureV >= height) {
        state.textureV = SubtractFixed(state.textureV, height);
    } else if (state.textureV <= NegateFixed(height)) {
        state.textureV = AddFixed(state.textureV, height);
    }
}

void Object::Render()
{
    switch (renderMode) {
    case ObjectRenderMode::Normal:
        RenderNormal();
        break;
    case ObjectRenderMode::SecondaryStreams:
        RenderSecondaryStreams();
        break;
    case ObjectRenderMode::Wireframe:
        RenderWireframe();
        break;
    }
}

void Object::RenderNormal()
{
    if (model == nullptr) {
        return;
    }
    const Fixed previousMinimum = renderer->GetMinimumIntensity();
    const Fixed previousObject = renderer->GetObjectIntensity();
    renderer->SetModelMatrix(modelMatrix);
    if (animation != nullptr) {
        UpdateMatrixPalette();
    }
    renderer->SetObjectIntensity(objectIntensity);
    renderer->SetLightingEnabled(lightingEnabled);
    renderer->SetDepthTestEnabled(true);
    renderer->SetAmbientLightOnly(ambientLightOnly);
    renderer->SetDepthBias(depthBias);
    for (const SojSurface& surface : model->GetSurfaces()) {
        const SojObject* material = nullptr;
        const Texture* texture = nullptr;
        const Texture* detail = nullptr;
        MaterialState* state = nullptr;
        if (surface.materialIndex >= 0 &&
            static_cast<std::size_t>(surface.materialIndex) < model->GetObjects().size()) {
            const std::size_t index = static_cast<std::size_t>(surface.materialIndex);
            material = &model->GetObjects()[index];
            texture = textures[index];
            detail = detailTextures[index];
            state = &materialStates[index];
        }
        renderer->DrawIndexedSurface(surface, material, texture, detail, matrixPalette,
            state != nullptr ? state->textureU : 0, state != nullptr ? state->textureV : 0,
            state != nullptr ? state->intensity : 0, cullingDisabled);
        if (material != nullptr && state != nullptr) {
            UpdateMaterial(*state, *material, texture);
        }
    }
    renderer->SetTexture(nullptr);
    renderer->SetDetailTexture(nullptr);
    renderer->SetDepthBias(0);
    renderer->SetLightingEnabled(true);
    renderer->SetAmbientLightOnly(false);
    renderer->SetDepthWriteEnabled(true);
    renderer->SetMinimumIntensity(previousMinimum);
    renderer->SetObjectIntensity(previousObject);
}

void Object::RenderSecondaryStreams()
{
    if (model == nullptr) {
        return;
    }
    const Fixed previousMinimum = renderer->GetMinimumIntensity();
    const Fixed previousObject = renderer->GetObjectIntensity();
    renderer->SetModelMatrix(modelMatrix);
    if (animation != nullptr) {
        UpdateMatrixPalette();
    }
    renderer->SetObjectIntensity(objectIntensity);
    renderer->SetLightingEnabled(lightingEnabled);
    renderer->SetDepthTestEnabled(true);
    renderer->SetAmbientLightOnly(ambientLightOnly);
    renderer->SetDepthBias(depthBias);
    for (const SojSurface& surface : model->GetSurfaces()) {
        const SojObject* material = nullptr;
        const Texture* texture = nullptr;
        const Texture* detail = nullptr;
        MaterialState* state = nullptr;
        if (surface.materialIndex >= 0 &&
            static_cast<std::size_t>(surface.materialIndex) < model->GetObjects().size()) {
            const std::size_t index = static_cast<std::size_t>(surface.materialIndex);
            material = &model->GetObjects()[index];
            texture = textures[index];
            detail = detailTextures[index];
            state = &materialStates[index];
            renderer->SetMaterialLighting(material->directionalScale, material->ambientIntensity);
            renderer->SetCullMode(cullingDisabled ? 0 : material->cullMode);
            renderer->SetTexture(texture);
            renderer->SetDetailTexture(detail);
            renderer->SetTextureCoordinateMode(material->textureCoordinateMode);
            renderer->SetBlendMode(materialStates[index].blendMode);
            renderer->SetDepthWriteEnabled(materialStates[index].blendMode == 0);
            renderer->SetMaterialLighting(material->directionalScale, material->ambientIntensity,
                objectIntensity, state->intensity, renderer->GetMinimumIntensity());
            if (texture == nullptr) {
                renderer->SetFlatColor(material->color);
            }
            if (texture != nullptr) {
                renderer->SetTextureOffset(state->textureU, state->textureV);
            }
            UpdateMaterial(*state, *material, texture);
        }
        if (secondaryVertices != nullptr) {
            renderer->DrawIndexedTextured(surface.vertices, *secondaryVertices, surface.indices,
                surface.primitiveType, 0, surface.primitiveCount, 0, matrixPalette);
        } else {
            renderer->DrawIndexed(surface.vertices, surface.indices, surface.primitiveType, 0,
                                  surface.primitiveCount);
        }
    }
    renderer->SetTexture(nullptr);
    renderer->SetDetailTexture(nullptr);
    renderer->SetDepthBias(0);
    renderer->SetLightingEnabled(true);
    renderer->SetAmbientLightOnly(false);
    renderer->SetDepthWriteEnabled(true);
    renderer->SetMinimumIntensity(previousMinimum);
    renderer->SetObjectIntensity(previousObject);
}

void Object::RenderWireframe()
{
    if (model == nullptr) {
        return;
    }
    if (animation != nullptr) {
        UpdateMatrixPalette();
    } else {
        renderer->SetModelMatrix(Multiply(modelMatrix, secondaryTransform));
    }
    renderer->SetLightingEnabled(false);
    renderer->SetDepthTestEnabled(true);
    renderer->SetDepthWriteEnabled(false);
    renderer->SetTexture(nullptr);
    renderer->SetFlatColor(0);
    renderer->SetBlendMode(0);
    for (const SojSurface& surface : model->GetSurfaces()) {
        renderer->DrawIndexedSurface(surface, nullptr, nullptr, nullptr, matrixPalette);
    }
    renderer->SetLightingEnabled(true);
    renderer->SetDepthWriteEnabled(true);
    renderer->SetBlendMode(0);
}

bool Object::HasTransparency() const
{
    if (model == nullptr) {
        return false;
    }
    for (std::size_t index = 0; index < materialStates.size(); ++index) {
        if (materialStates[index].blendMode != 0 || detailTextures[index] != nullptr) {
            return true;
        }
    }
    return false;
}

const SojSurface* Object::GetSurface(uint32_t index) const
{
    return model != nullptr && index < model->GetSurfaces().size()
        ? &model->GetSurfaces()[index] : nullptr;
}

uint32_t Object::GetVertexCount() const
{
    uint32_t count = 0;
    if (model != nullptr) {
        for (const SojSurface& surface : model->GetSurfaces()) {
            count += surface.vertices.vertexCount;
        }
    }
    return count;
}

uint32_t Object::GetTriangleCount() const
{
    uint32_t count = 0;
    if (model != nullptr) {
        for (const SojSurface& surface : model->GetSurfaces()) {
            count += surface.primitiveCount;
        }
    }
    return count;
}

Fixed Object::GetCameraDepth() const
{
    const Vec3 center = ShiftRight(Add(worldBounds.minimum, worldBounds.maximum), 1);
    return Transform(renderer->GetViewMatrix(), center).z;
}

const ObjectBounds& Object::GetPrimaryBounds() const { return primaryBounds; }
const ObjectBounds& Object::GetSecondaryBounds() const { return secondaryBounds; }
const ObjectBounds& Object::GetWorldBounds() const { return worldBounds; }
const std::vector<Matrix4>& Object::GetMatrixPalette() const { return matrixPalette; }
const ObjectRes* Object::GetModel() const { return model; }
const Animation* Object::GetAnimation() const { return animation; }
int32_t Object::GetAnimationTime() const { return animationTime; }
int32_t Object::GetAnimationDirection() const { return animationDirection; }
int32_t Object::GetAnimationSpeed() const { return animationSpeed; }
bool Object::IsAnimationLooping() const { return loopAnimation; }
bool Object::IsAnimationPaused() const { return animationPaused; }
uint32_t Object::GetMaterialBlendMode(uint32_t material) const
{
    return material < materialStates.size() ? materialStates[material].blendMode : 0;
}
ObjectRenderMode Object::GetRenderMode() const { return renderMode; }

} // namespace soft3d

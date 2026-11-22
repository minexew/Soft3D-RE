#include "game/Sword.hpp"

#include "game/GameData.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>

namespace game {
namespace {

void ResetBounds(soft3d::ObjectBounds& bounds)
{
    bounds.minimum = {INT32_MAX, INT32_MAX, INT32_MAX};
    bounds.maximum = {INT32_MIN, INT32_MIN, INT32_MIN};
}

void Expand(soft3d::ObjectBounds& bounds, const soft3d::Vec3& point)
{
    bounds.minimum.x = std::min(bounds.minimum.x, point.x);
    bounds.minimum.y = std::min(bounds.minimum.y, point.y);
    bounds.minimum.z = std::min(bounds.minimum.z, point.z);
    bounds.maximum.x = std::max(bounds.maximum.x, point.x);
    bounds.maximum.y = std::max(bounds.maximum.y, point.y);
    bounds.maximum.z = std::max(bounds.maximum.z, point.z);
}

bool Equal(const soft3d::Vec3& left, const soft3d::Vec3& right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

} // namespace

Sword::Sword(soft3d::s3dResManager* resourceManager, soft3d::Renderer* objectRenderer)
    : resources(resourceManager)
    , renderer(objectRenderer)
{
    unitType = 7;
    SetType(0);
    ResetBounds(hitBounds);
}

soft3d::result Sword::LoadModel(int typeValue, const char16_t* name)
{
    if (typeValue < 1 || typeValue > 4 || resources == nullptr) {
        return soft3d::S3D_PARAM;
    }
    const std::size_t index = static_cast<std::size_t>(typeValue);
    if (models[index] == nullptr) {
        models[index] = std::make_unique<soft3d::Object>(renderer);
        models[index]->SetTransformMode(0);
    }
    soft3d::result status = models[index]->Load(resources, name, 2);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    status = models[index]->LoadMaterials(resources, 2);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    if (shadows[index] == nullptr) {
        shadows[index] = std::make_unique<soft3d::Object>(renderer);
        shadows[index]->SetTransformMode(2);
    }
    return shadows[index]->Load(resources, name, 2);
}

void Sword::LoadAnimations(const GameData& data, int typeValue)
{
    if (typeValue < 1 || typeValue > 4) {
        return;
    }
    static constexpr int ordinaryNames[4][10] = {
        {44, 45, 46, 47, 48, 49, 50, 51, 52, 53},
        {54, 55, 56, 57, 58, 59, 60, 61, 62, 63},
        {68, 69, 70, 71, 72, 73, 74, 75, 76, 77},
        {79, 80, -1, -1, -1, 81, 82, 83, 84, 86},
    };
    auto& table = animations[static_cast<std::size_t>(typeValue)];
    for (std::size_t action = 0; action < std::size(ordinaryNames[typeValue - 1]); ++action) {
        const int name = ordinaryNames[typeValue - 1][action];
        table[action] = name < 0 ? nullptr : dynamic_cast<const soft3d::Animation*>(
            resources->GetResource(soft3d::ResourceAnimation, 2, data.GetAnimationName(name)));
    }
    primary = models[0].get();
    shadow = shadows[0].get();
}

void Sword::LoadQteAnimations(const GameData& data, int typeValue, bool alternate)
{
    if (typeValue == 2) {
        for (int action = 12; action <= 15; ++action) {
            animations[2][static_cast<std::size_t>(action)] = dynamic_cast<const soft3d::Animation*>(
                resources->GetResource(soft3d::ResourceAnimation, 3,
                                       data.GetAnimationName(64 + action - 12)));
        }
    } else if (!alternate && typeValue == 3) {
        animations[3][12] = dynamic_cast<const soft3d::Animation*>(resources->GetResource(
            soft3d::ResourceAnimation, 3, data.GetAnimationName(77)));
    } else if (alternate && typeValue == 4) {
        animations[4][12] = dynamic_cast<const soft3d::Animation*>(resources->GetResource(
            soft3d::ResourceAnimation, 3, data.GetAnimationName(85)));
    }
}

void Sword::UseStandForSpecial(int typeValue)
{
    if (typeValue >= 0 && typeValue < 5) {
        animations[static_cast<std::size_t>(typeValue)][12] =
            animations[static_cast<std::size_t>(typeValue)][0];
    }
}

void Sword::SetType(int typeValue)
{
    type = static_cast<int16_t>(typeValue);
    const std::size_t index = type >= 0 && type < 5 ? static_cast<std::size_t>(type) : 0;
    primary = models[index].get();
    shadow = shadows[index].get();
}

void Sword::Reset()
{
    hitActive = 0;
    hitHistoryIndex = 0;
    SetType(type);
}

void Sword::SetAnimation(int action, int32_t direction, bool loop, int32_t time)
{
    if (primary == nullptr) {
        return;
    }
    const std::size_t typeIndex = static_cast<std::size_t>(std::clamp(type, 0, 4));
    if (action < 0 || action >= 20) {
        return;
    }
    currentAnimation = animations[typeIndex][static_cast<std::size_t>(action)];
    primary->SetAnimation(currentAnimation);
    primary->SetAnimationPlayback(direction, loop);
    primary->SetAnimationTime(time);
    shadow->SetAnimation(currentAnimation);
    shadow->SetAnimationPlayback(direction, loop);
    shadow->SetAnimationTime(time);
    localBounds = primary->GetPrimaryBounds();
    animationFinished = false;
}

void Sword::SetAnimationSpeed(int32_t speed)
{
    if (primary != nullptr) {
        primary->SetAnimationSpeed(speed);
        shadow->SetAnimationSpeed(speed);
    }
}

void Sword::SetHitActive(bool active) { hitActive = active ? 1 : 0; }

void Sword::SetPosition(const soft3d::Vec3& value)
{
    if (primary != nullptr) {
        primary->SetPosition(value);
        shadow->SetPosition(value);
    }
}

void Sword::SetRight(const soft3d::Vec3& value)
{
    if (primary != nullptr) {
        primary->SetAxisX(value);
        shadow->SetAxisX(value);
    }
}

void Sword::SetUp(const soft3d::Vec3& value)
{
    if (primary != nullptr) {
        primary->SetAxisY(value);
        shadow->SetAxisY(value);
    }
}

void Sword::SetForward(const soft3d::Vec3& value)
{
    if (primary != nullptr) {
        primary->SetAxisZ(value);
        shadow->SetAxisZ(value);
    }
}

void Sword::SetObjectIntensity(soft3d::Fixed value)
{
    if (primary != nullptr) {
        primary->SetObjectIntensity(value);
    }
}

void Sword::SetShadowSecondaryTransform(const soft3d::Matrix4& value)
{
    if (shadow != nullptr) {
        shadow->SetSecondaryTransform(value);
    }
}

soft3d::Vec3 Sword::SampleMorph(uint32_t target) const
{
    return primary != nullptr ? primary->SampleMorph(target) : soft3d::Vec3{};
}

void Sword::Update()
{
    if (primary == nullptr) {
        return;
    }
    primary->UpdateTransform();
    shadow->SetModelMatrix(primary->GetModelMatrix());
    UpdateHitBounds();
}

void Sword::PostUpdate()
{
    if (primary != nullptr && currentAnimation != nullptr) {
        animationFinished = primary->AdvanceAnimation();
        shadow->AdvanceAnimation();
    }
}

void Sword::SubmitRenderables(soft3d::RenderQueues& queues)
{
    if (primary == nullptr) {
        return;
    }
    const soft3d::Fixed depth = primary->GetCameraDepth();
    primary->sortKey = primary->HasTransparency() ? depth : -depth;
    queues.Submit(primary, 3);
    queues.Submit(shadow, 2);
}

void Sword::UpdateHitBounds()
{
    ResetBounds(hitBounds);
    if (primary == nullptr) {
        return;
    }
    if (hitActive == 0) {
        --hitHistoryIndex;
        if (hitHistoryIndex > 0) {
            firstHistory[0] = firstHistory[1];
            secondHistory[0] = secondHistory[1];
            firstHistory[1] = firstHistory[2];
            secondHistory[1] = secondHistory[2];
            firstHistory[2] = firstHistory[3];
            secondHistory[2] = secondHistory[3];
        }
        if (hitHistoryIndex < -1) {
            hitHistoryIndex = -1;
        }
        return;
    }

    const soft3d::Vec3 first = SampleMorph(0);
    const soft3d::Vec3 second = SampleMorph(1);
    if (Equal(first, previousFirst)) {
        return;
    }
    hitVelocity = soft3d::Normalize(soft3d::Scale(soft3d::Subtract(first, previousFirst), 0xa0000));
    previousFirst = first;
    Expand(hitBounds, first);
    Expand(hitBounds, second);
    if (hitHistoryIndex >= 0) {
        const std::size_t index = static_cast<std::size_t>(hitHistoryIndex);
        Expand(hitBounds, firstHistory[index]);
        Expand(hitBounds, secondHistory[index]);
    }
    ++hitHistoryIndex;
    if (hitHistoryIndex > 3) {
        hitHistoryIndex = 3;
        firstHistory[0] = firstHistory[1];
        secondHistory[0] = secondHistory[1];
        firstHistory[1] = firstHistory[2];
        secondHistory[1] = secondHistory[2];
        firstHistory[2] = firstHistory[3];
        secondHistory[2] = secondHistory[3];
    }
    firstHistory[static_cast<std::size_t>(hitHistoryIndex)] = first;
    secondHistory[static_cast<std::size_t>(hitHistoryIndex)] = second;
}

} // namespace game

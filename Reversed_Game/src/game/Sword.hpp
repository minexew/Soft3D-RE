#pragma once

#include "soft3d/Units.hpp"

#include <array>
#include <memory>

namespace game {

class GameData;

class Sword final : public soft3d::GameUnit {
public:
    Sword(soft3d::s3dResManager* resources, soft3d::Renderer* renderer);

    soft3d::result LoadModel(int type, const char16_t* name);
    void LoadAnimations(const GameData& data, int type);
    void LoadQteAnimations(const GameData& data, int type, bool alternate);
    void UseStandForSpecial(int type);

    void Update() override;
    void PostUpdate() override;
    void SubmitRenderables(soft3d::RenderQueues& queues) override;

    void SetType(int type);
    void Reset();
    void SetAnimation(int action, int32_t direction, bool loop, int32_t time = 0);
    void SetAnimationSpeed(int32_t speed);
    void SetHitActive(bool active);
    void BeginHitWindow() { hitActive = 1; hitHistoryIndex = -1; }
    void SetPosition(const soft3d::Vec3& value);
    void SetRight(const soft3d::Vec3& value);
    void SetUp(const soft3d::Vec3& value);
    void SetForward(const soft3d::Vec3& value);
    void SetObjectIntensity(soft3d::Fixed value);
    void SetShadowSecondaryTransform(const soft3d::Matrix4& value);

    int GetType() const { return type; }
    bool IsAnimationFinished() const { return animationFinished; }
    bool IsHitActive() const { return hitActive != 0; }
    int16_t GetHitHistoryIndex() const { return hitHistoryIndex; }
    const soft3d::ObjectBounds& GetHitBounds() const { return hitBounds; }
    const soft3d::Vec3& GetHitVelocity() const { return hitVelocity; }
    soft3d::Object* GetObject() const { return primary; }
    soft3d::Vec3 SampleMorph(uint32_t target) const;

private:
    void UpdateHitBounds();

    soft3d::s3dResManager* resources;
    soft3d::Renderer* renderer;
    std::array<std::unique_ptr<soft3d::Object>, 5> models{};
    std::array<std::unique_ptr<soft3d::Object>, 5> shadows{};
    std::array<std::array<const soft3d::Animation*, 20>, 5> animations{};
    soft3d::Object* primary = nullptr;
    soft3d::Object* shadow = nullptr;
    const soft3d::Animation* currentAnimation = nullptr;
    soft3d::ObjectBounds hitBounds;
    std::array<soft3d::Vec3, 4> firstHistory{};
    std::array<soft3d::Vec3, 4> secondHistory{};
    soft3d::Vec3 previousFirst;
    soft3d::Vec3 hitVelocity;
    int16_t hitActive = 0;
    int16_t hitHistoryIndex = 0;
    int type = 0;
    bool animationFinished = false;
};

} // namespace game

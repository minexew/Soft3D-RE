#pragma once

#include "soft3d/Bsp.hpp"
#include "soft3d/Camera.hpp"
#include "soft3d/CameraSpot.hpp"
#include "soft3d/GameConfig.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace game {

class CameraTrigger;

class CameraController {
public:
    enum Mode : int32_t {
        FixedMode = 1,
        FrontFollow = 2,
        OrbitFollow = 3,
        OffsetFollow = 4,
        Path = 5,
        FirstPerson = 6,
        SideFollow = 7,
        FreeLook = 8,
    };

    CameraController();

    void SetServices(soft3d::Camera* camera, soft3d::Bsp* bsp,
                     soft3d::ControlMap* controls, const soft3d::Input* input,
                     uint32_t* randomState);
    void Bind(soft3d::CameraSpot* spot, bool preservePosition);
    void Reset();
    void SetFrozen(bool frozen);
    void SetInstant(bool instant);
    void ClearTriggers() { triggers.clear(); }

    void SetMode(int32_t mode, const soft3d::Vec3& position, const soft3d::Vec3& direction,
                 soft3d::Fixed time, const soft3d::CameraPathCollection* paths,
                 soft3d::Fixed pathSpeed, bool snap, int32_t targetMode);
    void Update(const soft3d::Vec3* target = nullptr, bool immediate = false);
    bool MoveCamera(const soft3d::Vec3& position, const soft3d::Vec3& target,
                    soft3d::Fixed speed);
    void LookAt(const soft3d::Vec3& position, const soft3d::Vec3& target);
    void SetCurrentTarget(const soft3d::Vec3& value) { target = value; }

    void Shake(int32_t frames, soft3d::Fixed amplitude);
    void SetDirectFollow(bool enabled, soft3d::Fixed speed);
    void SetCollisionEnabled(bool enabled) { collisionEnabled = enabled; }
    void SetPathLoop(bool enabled) { pathLoop = enabled; }
    void ConfigureOrbit(int32_t frames, const soft3d::Vec3* center,
                        const soft3d::Vec3* target, bool vertical, bool clockwise);
    void SetOrbitRotationEnabled(bool enabled) { orbitRotationEnabled = enabled; }
    void SetOrbitTarget(const soft3d::Vec3* target);
    void SetFirstPerson(const soft3d::Vec3& direction, bool snap, bool lock,
                        soft3d::Fixed horizontalLimit, soft3d::Fixed verticalLimit);
    void UpdateTriggers();

    int32_t GetMode() const { return mode; }
    int32_t GetPreviousMode() const { return previousMode; }
    const soft3d::Vec3& GetPosition() const { return position; }
    const soft3d::Vec3& GetTarget() const { return target; }
    const soft3d::Vec3& GetRight() const { return right; }
    const soft3d::Vec3& GetUp() const { return up; }
    const soft3d::Vec3& GetDirection() const { return direction; }
    const soft3d::Vec3& GetSavedDirection() const { return savedDirection; }
    const soft3d::Vec3& GetDesiredPosition() const { return desiredPosition; }
    const soft3d::Vec3& GetDesiredTarget() const { return desiredTarget; }
    soft3d::Fixed GetTargetDistance() const {
        return soft3d::Length(soft3d::Subtract(target, position));
    }
    soft3d::Fixed GetSavedFollowDistance() const { return savedFollowDistance; }
    soft3d::Camera* GetCamera() const { return camera; }
    bool IsBound() const { return bound; }
    bool IsFrozen() const { return frozen; }
    bool IsInstant() const { return instant; }
    bool IsPathCompleted() const { return pathCompleted; }
    bool IsInputCancelled() const { return inputCancelled; }
    bool IsDirectFollow() const { return directFollow; }
    bool HasSavedState() const { return hasSavedState; }
    void ClearSavedState() { hasSavedState = false; }
    void* GetUnitManager() const { return unitManager; }
    void SetUnitManager(void* manager) { unitManager = manager; }
    int32_t GetShakeFrames() const { return shakeFrames; }
    soft3d::Fixed GetPathTime() const { return pathTime; }
    soft3d::Fixed GetPathSpeed() const { return pathSpeed; }
    soft3d::Fixed GetFollowDistance() const { return followDistance; }
    std::size_t GetTriggerCount() const { return triggers.size(); }
    CameraTrigger* GetTrigger(std::size_t index) const {
        return index < triggers.size() ? triggers[index] : nullptr;
    }

private:
    struct SavedState {
        soft3d::Vec3 position;
        soft3d::Vec3 target;
        soft3d::Vec3 direction;
        soft3d::Vec3 desiredPosition;
        soft3d::Fixed followDistance = 0;
        soft3d::Fixed heightDelta = 0;
        soft3d::Fixed pathSpeed = 0;
        int32_t targetMode = 0;
        int32_t mode = 0;
        soft3d::CameraSpot* spot = nullptr;
        const soft3d::CameraPathCollection* paths = nullptr;
    };

    void BeginMode(int32_t newMode, bool snap);
    void SaveForTrigger();
    void RestoreForTrigger(bool snap);
    void AddTrigger(CameraTrigger* trigger);
    void RemoveTrigger(CameraTrigger* trigger);
    void UpdateFixed(bool immediate);
    void UpdateFrontFollow(bool immediate);
    void UpdateOrbitFollow(bool immediate);
    void UpdateOffsetFollow(bool immediate);
    void UpdatePath(bool immediate);
    void UpdateFirstPerson(bool immediate);
    void UpdateSideFollow(bool immediate);
    void UpdateFreeLook(bool immediate);
    void ClipCamera();
    void UpdateSmoothing();
    void UpdateOrientation();
    void UpdateOrbit();
    bool MovementInput() const;
    soft3d::Fixed ProjectOntoPath(bool initializing);
    uint32_t Random();
    int32_t RandomShakeComponent();

    std::vector<CameraTrigger*> triggers;
    bool resumeFromTrigger = false;
    bool frozen = false;
    bool pathLoop = false;
    soft3d::Vec3 position;
    soft3d::Vec3 target;
    soft3d::Vec3 up{0, soft3d::fixedOne, 0};
    soft3d::Vec3 desiredPosition;
    soft3d::Vec3 right;
    soft3d::Vec3 forward;
    soft3d::Vec3 previousPosition;
    soft3d::Vec3 previousTarget;
    soft3d::Vec3 previousBoundPosition;
    soft3d::Vec3 previousDesiredPosition;
    soft3d::Vec3 previousDesiredTarget;
    soft3d::Vec3 previousDirection;
    soft3d::Vec3 desiredTarget;
    soft3d::Vec3 direction;
    soft3d::Vec3 fixedTarget;
    soft3d::Fixed heightDelta = 0;
    soft3d::Fixed nearestPathTime = 0;
    soft3d::Vec3 positionVelocity;
    soft3d::Vec3 targetVelocity;
    soft3d::Vec3 acceleration;
    soft3d::Fixed followDistance = 0;
    int32_t previousMode = 0;
    int32_t mode = 0;
    soft3d::CameraSpot* spot = nullptr;
    bool inputCancelled = false;
    bool instant = false;
    bool hasSavedState = false;
    SavedState saved;
    int32_t shakeFrames = 0;
    soft3d::Fixed shakeAmplitude = 0;
    uint32_t* randomState = nullptr;
    uint32_t fallbackRandomState = 0;
    int16_t orbitFrames = -8;
    int16_t orbitInitialFrames = 0;
    int16_t orbitAngle = 0;
    int16_t orbitAngularVelocity = 0;
    soft3d::Fixed orbitRadius = 0;
    soft3d::Fixed orbitVertical = 0;
    int16_t orbitFlags = 0;
    int16_t orbitEndFrames = -8;
    bool orbitRotationEnabled = false;
    bool orbitTargetEnabled = false;
    soft3d::Vec3 orbitTarget;
    soft3d::Vec3 orbitPosition;
    soft3d::Vec3 savedDirection;
    soft3d::Fixed savedFollowDistance = 0;
    soft3d::Renderer* renderer = nullptr;
    bool snapMode = false;
    bool directFollow = false;
    soft3d::Fixed directFollowSpeed = 0xcccc;
    bool firstPersonLock = false;
    soft3d::Vec3 freeRight;
    soft3d::Vec3 freeUp;
    soft3d::Fixed firstPersonNear = 0;
    soft3d::Fixed firstPersonFar = 0;
    bool freeLookLocked = false;
    soft3d::Vec3 freeReference;
    soft3d::Fixed freeHorizontalLimit = 0;
    soft3d::Fixed freeVerticalLimit = 0;
    soft3d::Bsp* bsp = nullptr;
    soft3d::ControlMap* controls = nullptr;
    const soft3d::Input* controlInput = nullptr;
    soft3d::Camera* camera = nullptr;
    void* unitManager = nullptr;
    bool bound = false;
    bool collisionEnabled = true;
    const soft3d::CameraPathCollection* paths = nullptr;
    bool pathReverse = false;
    soft3d::Fixed pathTime = 0;
    soft3d::Fixed pathSpeed = 0;
    int32_t targetMode = 0;
    bool pathCompleted = false;

    friend class CameraTrigger;
};

} // namespace game

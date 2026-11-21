#pragma once

#include "soft3d/BezierCurve3.hpp"
#include "soft3d/CameraSpot.hpp"
#include "soft3d/Renderer.hpp"

#include <array>
#include <cstdint>

namespace soft3d {

class Camera {
public:
    Camera(int width, int height, Renderer* renderer);

    void SetLookAt(const Vec3& position, const Vec3& target, const Vec3& up);
    void UpdateRoll();
    void SetProjection(int16_t angle, Fixed aspectRatio, Fixed nearDistance, Fixed farDistance);
    void SetViewport(int32_t left, int32_t top, uint32_t width, uint32_t height);

    int GetWidth() const;
    int GetHeight() const;
    Fixed GetAspectRatio() const;
    Fixed GetScreenScaleX() const;
    Fixed GetScreenScaleY() const;
    Fixed GetInverseScreenScaleX() const;
    Fixed GetInverseScreenScaleY() const;
    const Vec3& GetPosition() const;
    const Vec3& GetTarget() const;
    const Vec3& GetUp() const;
    const Matrix4& GetWorldToCamera() const;
    const Matrix4& GetProjection() const;
    const Matrix4& GetRollMatrix() const;
    const Viewport& GetViewport() const;

private:
    void ApplyProjection(bool multiplyAspect);

    int width;
    int height;
    Fixed aspectRatio;
    Fixed projectionAspectRatio;
    int16_t angle = 0x400;
    int16_t verticalAngle = 0;
    Fixed nearDistance = fixedOne;
    Fixed farDistance = 0x8c0000;
    Vec3 position;
    Vec3 target;
    Vec3 up = {0, fixedOne, 0};
    Matrix4 worldToCamera = Matrix4::Identity();
    Matrix4 projection = Matrix4::Identity();
    Matrix4 rollMatrix = Matrix4::Identity();
    Viewport viewport;
    std::array<int32_t, 4> viewportBounds{};
    Fixed screenScaleX = 0;
    Fixed screenScaleY = 0;
    Fixed inverseScreenScaleX = 0;
    Fixed inverseScreenScaleY = 0;
    Renderer* renderer;
};

class CameraPathController {
public:
    void Start(const CameraPathCollection& paths, CameraSpot* spot, Fixed speed,
               bool loop, bool snapToStart);
    void Update(Camera& camera);

    Fixed GetTime() const;
    Fixed GetSpeed() const;
    bool IsFinished() const;
    const Vec3& GetPosition() const;
    const Vec3& GetTarget() const;
    const Vec3& GetDesiredPosition() const;
    const Vec3& GetDesiredTarget() const;

private:
    const CameraPathCollection* paths = nullptr;
    CameraSpot* spot = nullptr;
    Fixed speed = 0;
    Fixed time = 0;
    bool loop = false;
    bool finished = false;
    Vec3 position;
    Vec3 target;
    Vec3 desiredPosition;
    Vec3 desiredTarget;
    Vec3 up = {0, fixedOne, 0};
};

} // namespace soft3d

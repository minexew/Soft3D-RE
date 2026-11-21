#include "soft3d/Camera.hpp"

#include "soft3d/Renderer.hpp"

#include <algorithm>

namespace soft3d {

Camera::Camera(int cameraWidth, int cameraHeight, Renderer* cameraRenderer)
    : width(cameraWidth)
    , height(cameraHeight)
    , aspectRatio(FixedDivide(width << 16, height << 16))
    , projectionAspectRatio(aspectRatio)
    , renderer(cameraRenderer)
{
    SetLookAt(position, {0, 0, fixedOne}, up);
    ApplyProjection(true);
    SetViewport(0, 0, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void Camera::SetLookAt(const Vec3& newPosition, const Vec3& newTarget, const Vec3& newUp)
{
    position = newPosition;
    target = newTarget;
    up = newUp;
    const Vec3 forward = Normalize(Subtract(target, position));
    Vec3 right = Cross(up, forward);
    if (AbsFixed(right.x) <= 0x4000 && AbsFixed(right.y) <= 0x4000 &&
        AbsFixed(right.z) <= 0x4000) {
        right = ShiftLeft(right, 8);
    }
    right = Normalize(right);
    const Vec3 cameraUp = Cross(forward, right);
    worldToCamera.values = {
        right.x, cameraUp.x, forward.x, 0,
        right.y, cameraUp.y, forward.y, 0,
        right.z, cameraUp.z, forward.z, 0,
        NegateFixed(Dot(right, position)), NegateFixed(Dot(cameraUp, position)),
        NegateFixed(Dot(forward, position)), fixedOne
    };
    const Vec3 delta = Subtract(target, position);
    uint16_t roll = 0;
    if (AbsFixed(delta.x) > AbsFixed(delta.z)) {
        const uint16_t tangent = FixedArcTangent(FixedDivide(delta.z, delta.x));
        roll = delta.x > 0 ? static_cast<uint16_t>(0x800 - tangent)
                           : static_cast<uint16_t>(-0x800 - tangent);
    } else {
        const uint16_t tangent = FixedArcTangent(FixedDivide(delta.x, delta.z));
        roll = delta.z > 0 ? tangent : static_cast<uint16_t>(tangent - 0x1000);
    }
    rollMatrix = RotateYMatrix(roll);
    if (renderer != nullptr) {
        renderer->SetViewMatrix(worldToCamera);
    }
}

void Camera::UpdateRoll()
{
    SetLookAt(position, target, up);
}

void Camera::SetProjection(int16_t newAngle, Fixed newAspectRatio, Fixed newNearDistance,
                           Fixed newFarDistance)
{
    (void)newNearDistance;
    (void)newFarDistance;
    angle = newAngle;
    projectionAspectRatio = newAspectRatio;
    nearDistance = fixedOne;
    farDistance = 0x8c0000;
    ApplyProjection(false);
}

void Camera::SetViewport(int32_t left, int32_t top, uint32_t newWidth, uint32_t newHeight)
{
    viewportBounds = {left, top, static_cast<int32_t>(newWidth), static_cast<int32_t>(newHeight)};
    viewport.SetBounds(left, top, newWidth, newHeight);
    if (renderer != nullptr) {
        renderer->SetViewport(viewport);
    }
}

void Camera::ApplyProjection(bool multiplyAspect)
{
    const Fixed angleFixed = static_cast<Fixed>(angle) << 16;
    verticalAngle = static_cast<int16_t>((multiplyAspect
        ? FixedMultiply(angleFixed, projectionAspectRatio)
        : FixedDivide(angleFixed, projectionAspectRatio)) >> 16);
    const Fixed tangentX = FixedTan(static_cast<uint16_t>(verticalAngle) >> 1);
    const Fixed tangentY = FixedTan(static_cast<uint16_t>(angle) >> 1);
    const Fixed doubleTangentX = FixedMultiply(0x20000, tangentX);
    const Fixed doubleTangentY = FixedMultiply(0x20000, tangentY);
    screenScaleX = FixedDivide(width << 16, doubleTangentX);
    screenScaleY = FixedDivide(height << 16, doubleTangentY);
    inverseScreenScaleX = FixedDivide(doubleTangentX, width << 16);
    inverseScreenScaleY = FixedDivide(doubleTangentY, height << 16);

    const Fixed scaleY = FixedDivide(fixedOne, tangentY);
    const Fixed scaleX = FixedDivide(scaleY, projectionAspectRatio);
    const Fixed scaleZ = FixedDivide(farDistance, SubtractFixed(farDistance, nearDistance));
    projection.values = {
        scaleX, 0, 0, 0,
        0, scaleY, 0, 0,
        0, 0, scaleZ, fixedOne,
        0, 0, FixedMultiply(NegateFixed(nearDistance), scaleZ), 0
    };
    if (renderer != nullptr) {
        renderer->SetProjectionMatrix(projection);
    }
}

int Camera::GetWidth() const { return width; }
int Camera::GetHeight() const { return height; }
Fixed Camera::GetAspectRatio() const { return aspectRatio; }
Fixed Camera::GetScreenScaleX() const { return screenScaleX; }
Fixed Camera::GetScreenScaleY() const { return screenScaleY; }
Fixed Camera::GetInverseScreenScaleX() const { return inverseScreenScaleX; }
Fixed Camera::GetInverseScreenScaleY() const { return inverseScreenScaleY; }
const Vec3& Camera::GetPosition() const { return position; }
const Vec3& Camera::GetTarget() const { return target; }
const Vec3& Camera::GetUp() const { return up; }
const Matrix4& Camera::GetWorldToCamera() const { return worldToCamera; }
const Matrix4& Camera::GetProjection() const { return projection; }
const Matrix4& Camera::GetRollMatrix() const { return rollMatrix; }
const Viewport& Camera::GetViewport() const { return viewport; }

void CameraPathController::Start(const CameraPathCollection& newPaths, CameraSpot* newSpot,
                                 Fixed newSpeed, bool newLoop, bool snapToStart)
{
    paths = &newPaths;
    spot = newSpot;
    speed = newSpeed;
    loop = newLoop;
    time = 0;
    finished = false;
    desiredPosition = paths->EvaluatePosition(0);
    desiredTarget = Add(desiredPosition, Scale(paths->EvaluateDerivative(0), 0xa0000));
    if (!snapToStart) {
        position = desiredPosition;
        target = desiredTarget;
    }
}

void CameraPathController::Update(Camera& camera)
{
    if (paths == nullptr || paths->GetCount() == 0) {
        return;
    }
    desiredPosition = paths->EvaluatePosition(time);
    if (speed != 0) {
        const Fixed step = FixedDivide(fixedOne, FixedMultiply(0x100000, speed));
        time = AddFixed(time, step);
        if (time > fixedOne) {
            finished = true;
            time = loop ? 0 : fixedOne;
        }
    }
    desiredTarget = Add(position, Scale(paths->EvaluateDerivative(time), 0xa0000));
    position = Add(position, Scale(Subtract(desiredPosition, position), 0x1999));
    target = desiredTarget;
    camera.SetLookAt(position, target, up);
}

Fixed CameraPathController::GetTime() const { return time; }
Fixed CameraPathController::GetSpeed() const { return speed; }
bool CameraPathController::IsFinished() const { return finished; }
const Vec3& CameraPathController::GetPosition() const { return position; }
const Vec3& CameraPathController::GetTarget() const { return target; }
const Vec3& CameraPathController::GetDesiredPosition() const { return desiredPosition; }
const Vec3& CameraPathController::GetDesiredTarget() const { return desiredTarget; }

} // namespace soft3d

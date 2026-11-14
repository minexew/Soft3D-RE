#include "soft3d/Camera.hpp"

#include "soft3d/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace soft3d {
namespace {

Fixed FixedTan(uint16_t angle)
{
    const uint16_t index = angle & 0x7ff;
    constexpr double pi = 3.14159265358979323846;
    const double value = std::tan(static_cast<double>(index) * pi / 4096.0) * 65536.0;
    Fixed result = std::numeric_limits<Fixed>::max();
    if (index == 1024) {
        result = fixedOne;
    } else if (value < static_cast<double>(std::numeric_limits<Fixed>::max())) {
        result = static_cast<Fixed>(value);
    }
    return (angle & 0x800) != 0 ? NegateFixed(result) : result;
}

} // namespace

Camera::Camera(int cameraWidth, int cameraHeight, Renderer* cameraRenderer)
    : width(cameraWidth)
    , height(cameraHeight)
    , aspectRatio(FixedDivide(width << 16, height << 16))
    , renderer(cameraRenderer)
{
    SetLookAt(position, {0, 0, fixedOne}, up);
    ApplyProjection(true);
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
    if (renderer != nullptr) {
        renderer->SetViewMatrix(worldToCamera);
    }
}

void Camera::SetProjection(int16_t newAngle, Fixed newAspectRatio, Fixed newNearDistance,
                           Fixed newFarDistance)
{
    angle = newAngle;
    aspectRatio = newAspectRatio;
    nearDistance = newNearDistance;
    farDistance = newFarDistance;
    ApplyProjection(false);
}

void Camera::ApplyProjection(bool multiplyAspect)
{
    const Fixed angleFixed = static_cast<Fixed>(angle) << 16;
    verticalAngle = static_cast<int16_t>((multiplyAspect
        ? FixedMultiply(angleFixed, aspectRatio)
        : FixedDivide(angleFixed, aspectRatio)) >> 16);
    const Fixed tangentX = FixedTan(static_cast<uint16_t>(verticalAngle) >> 1);
    const Fixed tangentY = FixedTan(static_cast<uint16_t>(angle) >> 1);
    const Fixed doubleTangentX = FixedMultiply(0x20000, tangentX);
    const Fixed doubleTangentY = FixedMultiply(0x20000, tangentY);
    screenScaleX = FixedDivide(width << 16, doubleTangentX);
    screenScaleY = FixedDivide(height << 16, doubleTangentY);
    inverseScreenScaleX = FixedDivide(doubleTangentX, width << 16);
    inverseScreenScaleY = FixedDivide(doubleTangentY, height << 16);

    const Fixed scaleY = FixedDivide(fixedOne, tangentY);
    const Fixed scaleX = FixedDivide(scaleY, aspectRatio);
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
    if (snapToStart) {
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

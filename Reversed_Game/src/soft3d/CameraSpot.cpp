#include "soft3d/CameraSpot.hpp"

namespace soft3d {

DummySpot::DummySpot() = default;

const Vec3& DummySpot::GetPosition() const { return position; }
const Vec3& DummySpot::GetRight() const { return right; }
const Vec3& DummySpot::GetUp() const { return up; }
const Vec3& DummySpot::GetForward() const { return forward; }
void DummySpot::SetPosition(const Vec3& value) { position = value; }
void DummySpot::SetCameraController(CameraPathController* value) { controller = value; }
void DummySpot::SetRight(const Vec3& value) { right = value; }
void DummySpot::SetUp(const Vec3& value) { up = value; }
void DummySpot::SetForward(const Vec3& value) { forward = value; }

} // namespace soft3d

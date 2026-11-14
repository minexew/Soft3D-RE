#pragma once

#include "soft3d/Fixed.hpp"

namespace soft3d {

class CameraPathController;

class CameraSpot {
public:
    virtual ~CameraSpot() = default;

    virtual const Vec3& GetPosition() const = 0;
    virtual const Vec3& GetRight() const = 0;
    virtual const Vec3& GetUp() const = 0;
    virtual const Vec3& GetForward() const = 0;
    virtual void SetRight(const Vec3& value) = 0;
    virtual void SetUp(const Vec3& value) = 0;
    virtual void SetForward(const Vec3& value) = 0;
};

class DummySpot final : public CameraSpot {
public:
    DummySpot();

    const Vec3& GetPosition() const override;
    const Vec3& GetRight() const override;
    const Vec3& GetUp() const override;
    const Vec3& GetForward() const override;
    void SetPosition(const Vec3& value);
    void SetCameraController(CameraPathController* value);
    void SetRight(const Vec3& value) override;
    void SetUp(const Vec3& value) override;
    void SetForward(const Vec3& value) override;

private:
    CameraPathController* controller = nullptr;
    Vec3 position;
    Vec3 right = {fixedOne, 0, 0};
    Vec3 up = {0, fixedOne, 0};
    Vec3 forward = {0, 0, fixedOne};
};

} // namespace soft3d

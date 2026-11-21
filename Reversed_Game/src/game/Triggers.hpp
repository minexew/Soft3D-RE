#pragma once

#include "game/CameraController.hpp"
#include "soft3d/Units.hpp"

#include <cstdint>
#include <memory>

namespace game {

class CameraTrigger final : public soft3d::GameUnit {
public:
    CameraTrigger(CameraController* controller = nullptr, soft3d::Creature* player = nullptr);

    void Configure(CameraController* controller, soft3d::Creature* player);
    soft3d::result LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp,
                              soft3d::s3dResManager& resources);
    void SetMode(int32_t mode, const soft3d::Vec3& position, const soft3d::Vec3& direction,
                 soft3d::Fixed time, std::unique_ptr<soft3d::CameraPathCollection> paths,
                 soft3d::Fixed pathSpeed, bool snap, int32_t targetMode);

    void Update() override;
    void SaveState(soft3d::OutputStream& stream) const override;
    void LoadState(soft3d::InputStream& stream) override;

    int32_t GetMode() const { return mode; }
    const soft3d::Vec3& GetPosition() const { return cameraPosition; }
    const soft3d::Vec3& GetDirection() const { return direction; }
    soft3d::Fixed GetTime() const { return time; }
    const soft3d::CameraPathCollection* GetPaths() const { return paths.get(); }
    soft3d::Fixed GetPathSpeed() const { return pathSpeed; }
    bool GetSnap() const { return snap; }
    int32_t GetTargetMode() const { return targetMode; }
    bool IsOutside() const { return outside; }

private:
    CameraController* controller;
    soft3d::Creature* player;
    int32_t mode = 0;
    soft3d::Vec3 cameraPosition;
    soft3d::Vec3 direction;
    soft3d::Fixed time = 0;
    std::unique_ptr<soft3d::CameraPathCollection> paths;
    bool snap = false;
    soft3d::Fixed pathSpeed = 0;
    bool active = false;
    bool outside = true;
    int32_t targetMode = 0;
};

class LightTrigger final : public soft3d::GameUnit {
public:
    explicit LightTrigger(soft3d::Creature* player = nullptr);

    soft3d::result LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp);
    void Update() override;

    bool UsesPlayerDirection() const { return usePlayerDirection; }
    const soft3d::Vec3& GetLightDirection() const { return lightDirection; }

private:
    soft3d::Creature* player;
    bool usePlayerDirection = false;
    soft3d::Vec3 lightDirection;
};

} // namespace game

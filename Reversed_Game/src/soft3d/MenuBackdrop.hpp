#pragma once

#include "soft3d/Bsp.hpp"
#include "soft3d/Camera.hpp"
#include "soft3d/CameraSpot.hpp"

#include <memory>

namespace soft3d {

class Engine;

class MenuBackdrop {
public:
    explicit MenuBackdrop(Engine* engine);

    result Load();
    void Update();
    void PrepareRender();
    bool IsLoaded() const;

    Bsp& GetBsp();
    CameraPathCollection& GetPaths();
    CameraPathController& GetCameraController();
    DummySpot& GetCameraSpot();

private:
    static BspEntity* CreateEntity(BspEntityRecord* record, void* context,
                                   BspEntityRecord* recordBase);

    Engine* engine;
    Bsp bsp;
    CameraPathCollection paths;
    CameraPathController cameraController;
    DummySpot cameraSpot;
    bool loaded = false;
};

} // namespace soft3d

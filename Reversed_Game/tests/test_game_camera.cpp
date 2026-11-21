#include "game/CameraController.hpp"
#include "game/Triggers.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

#define CHECK(condition) Check((condition), __LINE__)

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

void CheckVec(const soft3d::Vec3& actual, const soft3d::Vec3& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z, line);
}

#define CHECK_VEC(actual, ...) CheckVec((actual), soft3d::Vec3 __VA_ARGS__, __LINE__)

struct ControllerFixture {
    ControllerFixture()
        : controls(configuration)
    {
        spot.SetPosition({0x10000, 0x20000, 0x30000});
        controller.SetServices(&camera, nullptr, &controls, &input, &randomState);
        controller.Bind(&spot, true);
    }

    soft3d::Renderer renderer{320, 240};
    soft3d::Camera camera{320, 240, &renderer};
    soft3d::DummySpot spot;
    soft3d::Input input;
    soft3d::GameConfiguration configuration;
    soft3d::ControlMap controls;
    uint32_t randomState = 0;
    game::CameraController controller;
};

void TestFixedAndFollowModes()
{
    ControllerFixture fixture;
    auto& controller = fixture.controller;
    controller.SetMode(game::CameraController::FixedMode, {0x10000, 0x20000, 0x30000},
                       {0, 0, soft3d::fixedOne}, 0, nullptr, 0, false, 0);
    CHECK_VEC(controller.GetPosition(), {0x10000, 0x20000, 0x30000});
    CHECK_VEC(controller.GetTarget(), {0x10000, 0x20000, 0x40000});
    controller.Update();
    CHECK_VEC(fixture.camera.GetPosition(), {0x10000, 0x20000, 0x30000});

    controller.SetMode(game::CameraController::FixedMode, {0x20000, 0, 0},
                       {0, 0, soft3d::fixedOne}, 0, nullptr, 0, true, 0);
    controller.Update();
    CHECK(controller.GetPosition().x == 0x10000 + 6553);
    controller.SetInstant(true);
    controller.SetMode(game::CameraController::FixedMode, {0x30000, 0, 0},
                       {0, 0, soft3d::fixedOne}, 0, nullptr, 0, true, 0);
    CHECK_VEC(controller.GetPosition(), {0x30000, 0, 0});
    controller.SetInstant(false);

    controller.SetMode(game::CameraController::FrontFollow, {}, {0, 0, soft3d::fixedOne},
                       0x40000, nullptr, 0, false, 0);
    CHECK_VEC(controller.GetDesiredPosition(), {0x10000, 0x20000, -0x10000});
    CHECK_VEC(controller.GetDesiredTarget(), {0x10000, 0x40000, 0x30000});
    controller.Update();
    CHECK(controller.GetMode() == game::CameraController::FrontFollow);

    controller.SetMode(game::CameraController::OrbitFollow, {}, {0, 0, soft3d::fixedOne},
                       0x40000, nullptr, 0, false, 0);
    CHECK(controller.GetFollowDistance() == 0x40000);
    controller.Update();
    CHECK(controller.GetMode() == game::CameraController::OrbitFollow);

    controller.SetMode(game::CameraController::OffsetFollow, {1, 2, 3}, {0, 0, soft3d::fixedOne},
                       0x20000, nullptr, 0, false, 0);
    CHECK_VEC(controller.GetDesiredPosition(), {1, 2, 3});
    controller.Update();

    controller.SetMode(game::CameraController::SideFollow, {}, {0, 0, 0},
                       0x40000, nullptr, 0, false, 0);
    CHECK_VEC(controller.GetDesiredPosition(), {0x50000, 0x20000, 0x30000});
    controller.Update();
    CHECK_VEC(controller.GetDesiredPosition(), {-0x30000, 0x40000, 0x30000});

    controller.SetDirectFollow(true, 0x8000);
    CHECK(controller.IsDirectFollow());
    controller.Shake(1, 0x10000);
    controller.Update();
    CHECK(controller.GetShakeFrames() == 0);
}

void TestFirstPersonAndFreeLook()
{
    ControllerFixture fixture;
    auto& controller = fixture.controller;
    controller.SetMode(game::CameraController::FirstPerson, {}, {0, 0, soft3d::fixedOne},
                       0, nullptr, 0, false, 0);
    const soft3d::Vec3 original = controller.GetTarget();
    fixture.input.KeyDown(soft3d::Key::Right);
    controller.Update();
    CHECK(controller.GetTarget().x != original.x || controller.GetTarget().z != original.z);
    CHECK(fixture.spot.GetForward().y == 0);
    fixture.input.ClearKey();

    controller.SetMode(game::CameraController::FreeLook, {}, {}, 0, nullptr, 0, false, 0);
    const soft3d::Vec3 freeOriginal = controller.GetTarget();
    fixture.input.KeyDown(soft3d::Key::Left);
    controller.Update();
    CHECK(controller.GetTarget().x != freeOriginal.x || controller.GetTarget().z != freeOriginal.z);
}

void TestPathsAndShake()
{
    soft3d::CameraPathCollection paths;
    CHECK(paths.Append(soft3d::BezierCurve3(
        std::vector<soft3d::Vec3>{{0, 0, 0}, {0x100000, 0, 0}})));
    ControllerFixture fixture;
    auto& controller = fixture.controller;
    fixture.spot.SetPosition({});
    controller.SetMode(game::CameraController::Path, {}, {}, 0, &paths, 0x1e0000, false, 0);
    controller.Update();
    CHECK(controller.GetPathTime() == 136);
    CHECK(!controller.IsPathCompleted());
    for (int index = 1; index < 482; ++index) {
        controller.Update();
    }
    CHECK(controller.GetPathTime() == 0x10000);
    CHECK(controller.IsPathCompleted());

    controller.SetPathLoop(true);
    controller.SetMode(game::CameraController::Path, {}, {}, 0, &paths, 0x1e0000, false, 0);
    for (int index = 0; index < 482; ++index) {
        controller.Update();
    }
    CHECK(controller.GetPathTime() == 0);
    CHECK(controller.IsPathCompleted());

    controller.SetMode(game::CameraController::Path, {}, {}, 0, &paths, 0, false, 0);
    controller.Update();
    CHECK(controller.GetPathSpeed() == 0);
    CHECK_VEC(controller.GetPosition(), {0, 0, 0});

    ControllerFixture shaken;
    shaken.spot.SetPosition({});
    shaken.controller.SetMode(game::CameraController::FixedMode, {}, {0, 0, soft3d::fixedOne},
                              0, nullptr, 0, false, 0);
    shaken.controller.Shake(1, 0x10000);
    shaken.controller.Update();
    CHECK_VEC(shaken.controller.GetTarget(), {-0x10000, -0x10000, 0x10000});
    CHECK_VEC(shaken.camera.GetTarget(), {-0x10000, -0x10000, 0x10000});
}

void TestTriggerStackAndFreeze()
{
    ControllerFixture fixture;
    soft3d::Creature player;
    player.SetCollisionBounds({{-0x10000, -0x10000, -0x10000},
                               {0x10000, 0x10000, 0x10000}});
    game::CameraTrigger first(&fixture.controller, &player);
    first.SetCollisionBounds({{-0x20000, -0x20000, -0x20000},
                              {0x20000, 0x20000, 0x20000}});
    first.SetMode(game::CameraController::FixedMode, {0x10000, 0, 0},
                  {0, 0, soft3d::fixedOne}, 0, nullptr, 0, false, 0);
    game::CameraTrigger second(&fixture.controller, &player);
    second.SetCollisionBounds(first.GetCollisionBounds());
    second.SetMode(game::CameraController::FrontFollow, {}, {0, 0, soft3d::fixedOne},
                   0x40000, nullptr, 0, false, 0);

    fixture.controller.SetMode(game::CameraController::OrbitFollow, {}, {0, 0, soft3d::fixedOne},
                               0x1a8000, nullptr, 0, false, 0);
    first.Update();
    CHECK(fixture.controller.GetMode() == game::CameraController::FixedMode);
    CHECK(fixture.controller.GetTriggerCount() == 1);
    CHECK(!first.IsOutside());
    second.Update();
    CHECK(fixture.controller.GetMode() == game::CameraController::FrontFollow);
    CHECK(fixture.controller.GetTriggerCount() == 2);

    second.SetCollisionBounds({{0x30000, 0x30000, 0x30000},
                               {0x40000, 0x40000, 0x40000}});
    second.Update();
    CHECK(fixture.controller.GetMode() == game::CameraController::FixedMode);
    CHECK(fixture.controller.GetTriggerCount() == 1);
    CHECK(second.IsOutside());

    fixture.controller.SetFrozen(true);
    first.SetCollisionBounds(second.GetCollisionBounds());
    first.Update();
    CHECK(fixture.controller.GetTriggerCount() == 1);
    fixture.controller.SetFrozen(false);
    CHECK(fixture.controller.GetMode() == game::CameraController::FixedMode);

    first.Update();
    CHECK(fixture.controller.GetTriggerCount() == 0);
    CHECK(fixture.controller.GetMode() == game::CameraController::FixedMode);
    CHECK(first.IsOutside());
}

} // namespace

int main()
{
    try {
        TestFixedAndFollowModes();
        TestFirstPersonAndFreeLook();
        TestPathsAndShake();
        TestTriggerStackAndFreeze();
        std::cout << "game camera tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

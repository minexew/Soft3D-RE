#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/EngineClock.hpp"
#include "soft3d/GameConfig.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/Input.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

class TestClock final : public soft3d::EngineClock {
public:
    uint32_t NowMicroseconds() override { return now; }

    void SleepMicroseconds(uint32_t microseconds) override
    {
        sleeps.push_back(microseconds);
        now += microseconds;
    }

    uint32_t now = 1000;
    std::vector<uint32_t> sleeps;
};

void TestTranslation()
{
    using soft3d::Key;
    constexpr std::array<int, 23> primary = {
        0x2a, 0x7f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x0e, 0x0f, 0x10, 0x11, 0xa4, 0xa5, 0xa7, 0x00, 0x00, 0x00, 0x00,
    };
    for (std::size_t index = 0; index < 19; ++index) {
        CHECK(static_cast<uint8_t>(soft3d::TranslateSymbianKey(primary[index])) == index + 1);
    }
    constexpr std::array<int, 10> alternate = {
        0x4d, 0x52, 0x54, 0x59, 0x46, 0x47, 0x48, 0x56, 0x42, 0x4e,
    };
    for (std::size_t index = 0; index < alternate.size(); ++index) {
        CHECK(static_cast<uint8_t>(soft3d::TranslateSymbianKey(alternate[index])) == index + 3);
    }
    CHECK(soft3d::TranslateSymbianKey(0) == Key::None);
    CHECK(soft3d::TranslateSymbianKey(0xa6) == Key::None);
    CHECK(soft3d::TranslateSymbianKey(0x12) == Key::None);
}

void TestTransitions()
{
    using soft3d::Key;
    soft3d::Input input;
    CHECK(input.IsKeyUp(Key::Left));
    CHECK(!input.IsAnyKeyClick());

    input.KeyDown(Key::Left);
    CHECK(input.GetKeyState(Key::Left) == 3);
    CHECK(input.IsKeyFirstDown(Key::Left));
    CHECK(input.IsKeyDown(Key::Left));
    CHECK(input.IsKeyDownOrClick(Key::Left));
    input.Update();
    CHECK(input.GetKeyState(Key::Left) == 1);
    CHECK(!input.IsKeyFirstDown(Key::Left));

    for (int repeat = 0; repeat < 5; ++repeat) {
        input.KeyDown(Key::Left);
    }
    CHECK(input.GetRepeatCount(Key::Left) == 5);
    CHECK(!input.IsKeyPress(Key::Left));
    input.KeyDown(Key::Left);
    CHECK(input.GetRepeatCount(Key::Left) == 0);
    CHECK(input.GetKeyState(Key::Left) == 5);
    input.KeyDown(Key::Left);
    CHECK(input.GetRepeatCount(Key::Left) == 0);
    input.Update();
    CHECK(input.GetKeyState(Key::Left) == 5);

    input.KeyDown(Key::Right);
    input.KeyDown(Key::Number1);
    CHECK(input.IsKeyDown(Key::Right));
    CHECK(input.IsKeyFirstDown(Key::Number1));
    CHECK(input.IsKeyPress(Key::Left));
    input.KeyUp(Key::Left);
    CHECK(input.GetKeyState(Key::Left) == 8);
    CHECK(input.IsKeyClick(Key::Left));
    CHECK(input.IsKeyDownOrClick(Key::Left));
    CHECK(input.IsKeyDown(Key::Right));
    input.Update();
    CHECK(input.GetKeyState(Key::Left) == 0);
    CHECK(!input.IsKeyClick(Key::Left));
    input.KeyUp(Key::Left);
    CHECK(input.GetKeyState(Key::Left) == 0);

    input.KeyPress(Key::Right);
    CHECK(input.GetKeyState(Key::Right) == 5);
    input.KeyDown(Key::Right);
    CHECK(input.GetKeyState(Key::Right) == 5);
    input.KeyUp(Key::Right);
    input.KeyDown(Key::Right);
    CHECK(input.GetKeyState(Key::Right) == 3);
    CHECK(!input.IsAnyKeyClick());

    input.KeyDown(Key::None);
    input.KeyDown(static_cast<Key>(24));
    CHECK(input.GetKeyState(Key::None) == 0);
    CHECK(input.GetKeyState(static_cast<Key>(24)) == 0);
    input.ClearKey();
    CHECK(input.GetKeyState(Key::Right) == 0);
    CHECK(input.GetRepeatCount(Key::Left) == 0);
}

void TestControlMapRecovery()
{
    using soft3d::Key;
    soft3d::GameConfiguration configuration;
    soft3d::Input input;
    soft3d::ControlMap controls(configuration);

    CHECK(controls.AnyKeyUp(input));
    CHECK(!controls.AnyKeyFirstDown(input) && !controls.AnyKeyDown(input));
    CHECK(!controls.AnyKeyPress(input) && !controls.AnyKeyClick(input));
    CHECK(!controls.AnyKeyDownOrClick(input));
    for (std::size_t slot = 1; slot < soft3d::Input::keySlotCount; ++slot) {
        const Key key = static_cast<Key>(slot);
        input.ClearKey();
        input.KeyDown(key);
        CHECK(controls.AnyKeyFirstDown(input) && controls.AnyKeyDown(input));
        CHECK(controls.AnyKeyDownOrClick(input));
        input.ClearKey();
        input.KeyPress(key);
        CHECK(controls.AnyKeyPress(input));
        input.ClearKey();
        input.KeyDown(key);
        input.KeyUp(key);
        CHECK(controls.AnyKeyClick(input) && controls.AnyKeyDownOrClick(input));
    }

    const Key primary = static_cast<Key>(controls.GetPrimaryBinding(1));
    const Key secondary = static_cast<Key>(controls.GetBinding(1));
    input.ClearKey();
    controls.ActionKeyDown(input, 1);
    CHECK(input.IsKeyFirstDown(primary) && input.IsKeyFirstDown(secondary));
    controls.ActionKeyPress(input, 1);
    CHECK(input.IsKeyPress(primary) && input.IsKeyPress(secondary));
    controls.ActionKeyUp(input, 1);
    CHECK(input.IsKeyClick(primary) && input.IsKeyClick(secondary));

    input.ClearKey();
    controls.SetEnabled(false);
    controls.ActionKeyDown(input, 1);
    CHECK(!input.IsAnyKeyDown());
    controls.SetEnabled(true);
    controls.SetActionEnabled(1, false);
    controls.ActionKeyDown(input, 1);
    CHECK(!input.IsAnyKeyDown());
    controls.ActionKeyPress(input, 1);
    CHECK(!input.IsAnyKeyPress());
    controls.ActionKeyUp(input, 1);
    CHECK(!input.IsAnyKeyClick());

    input.KeyDown(primary);
    CHECK(!controls.ActionFirstDown(input, 1, false, true));
    CHECK(controls.ActionFirstDown(input, 1, false, false));
    CHECK(!controls.ActionDown(input, 1, false, true));
    CHECK(controls.ActionDown(input, 1, false, false));
    CHECK(!controls.ActionDownOrClick(input, 1, false, true));
    CHECK(controls.ActionDownOrClick(input, 1, false, false));
    input.KeyPress(primary);
    CHECK(!controls.ActionPress(input, 1, false, true));
    CHECK(controls.ActionPress(input, 1, false, false));
    input.KeyUp(primary);
    CHECK(!controls.ActionClick(input, 1, false, true));
    CHECK(controls.ActionClick(input, 1, false, false));
    CHECK(!controls.ActionUp(input, 1, false, true));
    CHECK(controls.ActionUp(input, 1, false, false));
    controls.SetActionEnabled(1, true);

    input.ClearKey();
    CHECK(!controls.ActionUp(input, 1, true));
    CHECK(controls.ActionUp(input, 1, false));
    input.KeyDown(secondary);
    CHECK(!controls.ActionUp(input, 1, false));
    CHECK(!controls.ActionUp(input, 1, true));
    input.KeyUp(secondary);
    CHECK(controls.ActionUp(input, 1, false));
    input.KeyDown(primary);
    CHECK(!controls.ActionUp(input, 1, false));
    input.KeyUp(primary);
    CHECK(controls.ActionUp(input, 1, false));

    input.ClearKey();
    input.KeyDown(secondary);
    CHECK(!controls.ActionDown(input, 1, true));
    CHECK(controls.ActionDown(input, 1, false));
    controls.SetEnabled(false);
    CHECK(!controls.ActionDown(input, 1, false, false));
}

void TestTimerLifecycleAndFps()
{
    soft3d::EngineConfig config;
    config.width = 320;
    config.height = 240;
    auto clock = std::make_unique<TestClock>();
    auto* testClock = clock.get();
    config.clock = std::move(clock);
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    config.videoBackend = std::move(backend);
    soft3d::GameEngine engine(std::move(config));
    engine.StartGameEngineTimer(17);
    CHECK(engine.GetFrameIntervalMicroseconds() == 58823);
    CHECK(engine.GetRealFps() == 0);

    engine.GetInput().KeyDown(soft3d::Key::Left);
    engine.Start();
    CHECK(engine.GetInput().IsKeyUp(soft3d::Key::Left));
    CHECK(engine.IsRunning());
    testClock->now += 5823;
    CHECK(engine.RunFrame());
    CHECK(testClock->sleeps.size() == 1);
    CHECK(testClock->sleeps[0] == 53000);
    CHECK(testClock->now == 59823);
    CHECK(dummy->GetPresentedText().empty());

    engine.SetFpsDisplayEnabled(true);
    const std::vector<uint32_t> defaultFrame = dummy->GetPresentedPixels();
    CHECK(engine.RunFrame());
    CHECK(testClock->sleeps.size() == 2);
    CHECK(testClock->sleeps[1] == 58823);
    CHECK(dummy->GetPresentedPixels() == defaultFrame);
    CHECK(dummy->GetPresentedText().size() == 1);
    CHECK(dummy->GetPresentedText()[0].text == u"0");
    CHECK(dummy->GetPresentedText()[0].x == 300);
    CHECK(dummy->GetPresentedText()[0].y == 10);

    engine.SetFpsDisplayEnabled(false);
    CHECK(engine.RunFrame());
    CHECK(dummy->GetPresentedText().empty());
    const int startCount = dummy->GetStartCount();
    engine.Resume();
    CHECK(dummy->GetStartCount() == startCount);
    CHECK(engine.IsRunning());
    engine.Suspend();
    CHECK(!engine.IsRunning());
    engine.Resume();
    CHECK(dummy->GetStartCount() == startCount);
    CHECK(!engine.IsRunning());
}

} // namespace

int main()
{
    try {
        TestTranslation();
        TestTransitions();
        TestControlMapRecovery();
        TestTimerLifecycleAndFps();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "input, timing, lifecycle, and FPS tests passed\n";
    return 0;
}

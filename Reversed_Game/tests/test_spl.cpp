#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/MenuBackdrop.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
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

void CheckVec(const soft3d::Vec3& actual, const soft3d::Vec3& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z, line);
}

#define CHECK_VEC(actual, ...) CheckVec((actual), soft3d::Vec3 __VA_ARGS__, __LINE__)

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void AppendFloat(std::vector<uint8_t>& bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    Append32(bytes, bits);
}

std::vector<uint8_t> MakeLinearRecord()
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 1);
    Append32(bytes, 1);
    Append32(bytes, 2);
    for (unsigned index = 0; index < 6; ++index) {
        AppendFloat(bytes, index == 3 ? 16.0f : 0.0f);
    }
    AppendFloat(bytes, 1.0f);
    AppendFloat(bytes, std::numeric_limits<float>::quiet_NaN());
    AppendFloat(bytes, 1.0f);
    AppendFloat(bytes, 1.0f);
    AppendFloat(bytes, 16.0f);
    AppendFloat(bytes, 0.0f);
    AppendFloat(bytes, 0.0f);
    return bytes;
}

uint32_t HashPixels(const uint32_t* pixels, std::size_t count)
{
    uint32_t hash = 2166136261U;
    for (std::size_t index = 0; index < count; ++index) {
        hash ^= pixels[index];
        hash *= 16777619U;
    }
    return hash;
}

void TestRealSpl()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == soft3d::S3D_OK);
    soft3d::CameraPathCollection paths;
    CHECK(soft3d::LoadSplCameraResource(paths, u".\\common\\1000.spl",
                                       manager.GetFilePak()) == soft3d::S3D_OK);
    CHECK(paths.GetCapacity() == 64 && paths.GetCount() == 11);
    for (std::size_t index = 0; index < paths.GetCount(); ++index) {
        CHECK(paths.GetCurve(index).GetSegmentCount() == 3);
        CHECK(paths.GetCurve(index).GetPointCount() == 4);
    }
    const auto& first = paths.GetCurve(0);
    CHECK_VEC(first.GetPoints()[0], {-4175281, -1919741, -3071833});
    CHECK_VEC(first.GetPoints()[3], {-5269087, -1732000, -1219499});
    CHECK_VEC(paths.GetCurve(10).GetPoints()[3], {-4173191, -1918641, -3071756});
    const std::array<soft3d::Fixed, 16> coefficients = {
        0x10000, 0, 0, 0,
        0x10000, 0x10000, 0, 0,
        0x10000, 0x20000, 0x10000, 0,
        0x10000, 0x30000, 0x30000, 0x10000,
    };
    CHECK(std::equal(coefficients.begin(), coefficients.end(),
                     first.GetCoefficientMatrix().begin()));
    CHECK_VEC(first.Evaluate(0x8000), {-4995310, -1787878, -2372946});
    CHECK_VEC(first.EvaluateDerivative(0x8000), {-987438, 353874, 1607376});
    CHECK_VEC(first.EvaluateSecondDerivative(0x8000), {2184996, -303948, 1818228});

    const std::array<soft3d::Fixed, 11> lengths = {
        2485760, 1180160, 1394688, 1278464, 1314816, 1061888,
        1347840, 1281024, 1128448, 1131776, 1110016,
    };
    const std::array<soft3d::Fixed, 11> ends = {
        11070, 16326, 22537, 28230, 34085, 38814, 44816, 50521, 55546, 60586, 65536,
    };
    CHECK(std::equal(lengths.begin(), lengths.end(), paths.GetPathLengths().begin()));
    CHECK(std::equal(ends.begin(), ends.end(), paths.GetPathEndTimes().begin()));
    CHECK_VEC(paths.EvaluatePosition(0), {-4175281, -1919741, -3071833});
    CHECK_VEC(paths.EvaluateDerivative(0), {-2399049, 7443, 1433130});
    CHECK_VEC(paths.EvaluateSecondDerivative(0), {3461454, 1689684, -1121238});
    CHECK_VEC(paths.EvaluatePosition(0x1000), {-4847407, -1834951, -2568650});
    CHECK_VEC(paths.EvaluatePosition(0x4000), {-5117466, -1838487, -58743});
    CHECK_VEC(paths.EvaluatePosition(0x8000), {-2381450, -1870937, 924540});
    CHECK_VEC(paths.EvaluatePosition(0xc000), {-1098914, -1794845, -2200658});
    CHECK_VEC(paths.EvaluatePosition(0x10000), {-4173191, -1918641, -3071756});
    CHECK_VEC(paths.EvaluateDerivative(0x10000), {-1296741, 594, 779640});
    CHECK_VEC(paths.EvaluateSecondDerivative(0x10000), {-878826, 281616, 1092804});
#endif
}

void TestMalformedSpl()
{
    soft3d::CameraPathCollection empty;
    CHECK_VEC(empty.EvaluatePosition(0x8000), {0, 0, 0});
    CHECK(soft3d::ParseSplCameraPaths(nullptr, 0, empty) == soft3d::S3D_FORMAT);
    const std::vector<uint8_t> truncated = {1, 0, 0, 0, 1};
    CHECK(soft3d::ParseSplCameraPaths(truncated.data(), truncated.size(), empty) ==
          soft3d::S3D_FORMAT);
    std::vector<uint8_t> invalidCount = {0xff, 0xff, 0xff, 0xff};
    CHECK(soft3d::ParseSplCameraPaths(invalidCount.data(), invalidCount.size(), empty) ==
          soft3d::S3D_FORMAT);

    std::vector<uint8_t> bytes = MakeLinearRecord();
    soft3d::CameraPathCollection paths;
    CHECK(soft3d::ParseSplCameraPaths(bytes.data(), bytes.size(), paths) == soft3d::S3D_OK);
    CHECK(paths.GetCount() == 1);
    CHECK_VEC(paths.EvaluatePosition(0x8000), {0x80000, 0, 0});
    CHECK_VEC(paths.EvaluateDerivative(0x8000), {0x100000, 0, 0});
    CHECK(paths.Append(soft3d::BezierCurve3(std::vector<soft3d::Vec3>{{}, {0x10000, 0, 0}})));
    bytes.push_back(0);
    soft3d::CameraPathCollection trailing;
    CHECK(soft3d::ParseSplCameraPaths(bytes.data(), bytes.size(), trailing) == soft3d::S3D_FORMAT);
    std::vector<uint8_t> mismatch = MakeLinearRecord();
    mismatch[4] = 2;
    soft3d::CameraPathCollection invalid;
    CHECK(soft3d::ParseSplCameraPaths(mismatch.data(), mismatch.size(), invalid) ==
          soft3d::S3D_FORMAT);
}

void TestCameraAndSpot()
{
    soft3d::Renderer renderer(320, 240);
    soft3d::Camera camera(320, 240, &renderer);
    CHECK(camera.GetAspectRatio() == 87381);
    CHECK(camera.GetScreenScaleX() == 18183604);
    CHECK(camera.GetScreenScaleY() == 18986777);
    CHECK(camera.GetInverseScreenScaleX() == 236);
    CHECK(camera.GetInverseScreenScaleY() == 226);
    CHECK(camera.GetProjection().values[0] == 118667);
    CHECK(camera.GetProjection().values[5] == 158223);
    CHECK(camera.GetProjection().values[10] == 66007);
    CHECK(camera.GetProjection().values[11] == 0x10000);
    CHECK(camera.GetProjection().values[14] == -66007);
    CHECK(camera.GetProjection().values[15] == 0);
    camera.SetProjection(0x400, 87381, 0x10000, 0x8c0000);
    CHECK(camera.GetScreenScaleX() == 34567141);
    CHECK(camera.GetInverseScreenScaleX() == 124);
    CHECK(camera.GetScreenScaleY() == 18986777);
    CHECK(camera.GetInverseScreenScaleY() == 226);
    CHECK(camera.GetWorldToCamera().values == soft3d::Matrix4::Identity().values);
    camera.SetLookAt({0x10000, 0x20000, 0x30000}, {0x10000, 0x20000, 0x40000},
                     {0, 0x10000, 0});
    CHECK(camera.GetWorldToCamera().values[12] == -0x10000);
    CHECK(camera.GetWorldToCamera().values[13] == -0x20000);
    CHECK(camera.GetWorldToCamera().values[14] == -0x30000);

    soft3d::DummySpot spot;
    CHECK_VEC(spot.GetRight(), {0x10000, 0, 0});
    CHECK_VEC(spot.GetUp(), {0, 0x10000, 0});
    CHECK_VEC(spot.GetForward(), {0, 0, 0x10000});
    spot.SetPosition({1, 2, 3});
    spot.SetRight({4, 5, 6});
    spot.SetUp({7, 8, 9});
    spot.SetForward({10, 11, 12});
    CHECK_VEC(spot.GetPosition(), {1, 2, 3});
    CHECK_VEC(spot.GetRight(), {4, 5, 6});
    CHECK_VEC(spot.GetUp(), {7, 8, 9});
    CHECK_VEC(spot.GetForward(), {10, 11, 12});
}

void TestCameraPathController()
{
    soft3d::CameraPathCollection paths;
    CHECK(paths.Append(soft3d::BezierCurve3(
        std::vector<soft3d::Vec3>{{0, 0, 0}, {0x100000, 0, 0}})));
    soft3d::DummySpot spot;
    soft3d::CameraPathController controller;
    controller.Start(paths, &spot, 0x1e0000, false, false);
    soft3d::Renderer renderer(320, 240);
    soft3d::Camera camera(320, 240, &renderer);
    controller.Update(camera);
    CHECK(controller.GetTime() == 136);
    CHECK_VEC(controller.GetDesiredPosition(), {0, 0, 0});
    CHECK_VEC(controller.GetPosition(), {0, 0, 0});
    CHECK_VEC(controller.GetTarget(), {0xa00000, 0, 0});
    controller.Update(camera);
    CHECK(controller.GetTime() == 272);
    CHECK_VEC(controller.GetDesiredPosition(), {2176, 0, 0});
    CHECK_VEC(controller.GetPosition(), {217, 0, 0});
    CHECK_VEC(controller.GetTarget(), {0xa00000, 0, 0});
    CHECK(!controller.IsFinished());
    for (int index = 2; index < 482; ++index) {
        controller.Update(camera);
    }
    CHECK(controller.GetTime() == 0x10000);
    CHECK(controller.IsFinished());
    controller.Update(camera);
    CHECK_VEC(controller.GetDesiredPosition(), {0x100000, 0, 0});
}

void TestRealMenuBackdrop()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::EngineConfig config;
    auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    auto* dummy = backend.get();
    config.videoBackend = std::move(backend);
    soft3d::GameEngine engine(std::move(config));
    CHECK(soft3d::OpenGameResourceArchive(&engine.GetResourceManager(), SOFT3D_TEST_ARCHIVE) ==
          soft3d::S3D_OK);
    CHECK(engine.LoadMenuBackdrop() == soft3d::S3D_OK);
    CHECK(engine.GetMenuBackdrop()->IsLoaded());
    CHECK(engine.GetMenuBackdrop()->GetPaths().GetCount() == 11);
    engine.GetRenderer().Clear(0);
    engine.Start();
    CHECK(engine.RunFrame());
    const uint32_t* frame = reinterpret_cast<const uint32_t*>(
        engine.GetRenderer().GetCurrentTexture()->GetPixels());
    const uint32_t firstHash = HashPixels(frame, 320 * 240);
    const uint32_t firstPresentedHash = HashPixels(dummy->GetPresentedPixels().data(), 320 * 240);
    CHECK(engine.GetCamera().GetPosition().x == -417490);
    CHECK(engine.GetCamera().GetPosition().y == -191957);
    CHECK(engine.GetCamera().GetPosition().z == -307156);
    CHECK(engine.GetMenuBackdrop()->GetCameraController().GetTime() == 136);
    CHECK(engine.RunFrame());
    const uint32_t secondHash = HashPixels(frame, 320 * 240);
    CHECK(firstHash == 0x57550571);
    CHECK(firstPresentedHash == 0x9c578365);
    CHECK(secondHash == 0x5e0cf7d0);
    CHECK(secondHash != firstHash);
    for (int index = 2; index < 120; ++index) {
        CHECK(engine.RunFrame());
    }
    const uint32_t finalHash = HashPixels(frame, 320 * 240);
    const uint32_t finalPresentedHash = HashPixels(dummy->GetPresentedPixels().data(), 320 * 240);
    CHECK(engine.GetMenuBackdrop()->GetCameraController().GetTime() == 120 * 136);
    CHECK(engine.GetMenuBackdrop()->GetBsp().GetFrameCounter() == 120);
    CHECK(!engine.GetMenuBackdrop()->GetBsp().GetVisibleFaces().empty());
    CHECK(engine.GetRenderQueues().GetCount(3) != 0);
    CHECK(finalHash == 0xbda77efe);
    CHECK(finalPresentedHash == 0x79f3684d);
    CHECK(finalHash != firstHash && finalHash != secondHash);
#endif
}

} // namespace

int main()
{
    try {
        TestRealSpl();
        TestMalformedSpl();
        TestCameraAndSpot();
        TestCameraPathController();
        TestRealMenuBackdrop();
        std::cout << "SPL camera tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/Stream.hpp"
#include "soft3d/Units.hpp"
#include "soft3d/s3dMemory.hpp"
#include "soft3d/s3dResManager.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
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

class TempFile {
public:
    explicit TempFile(const char* name)
        : path(std::filesystem::temp_directory_path() / name)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    ~TempFile()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

std::vector<uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input),
                                std::istreambuf_iterator<char>());
}

void TestMemoryStreams()
{
    soft3d::OutputDataStream output;
    CHECK(output.IsValid());
    CHECK(output.GetSize() == -1);
    CHECK(output.WriteBool(true) == soft3d::S3D_OK);
    CHECK(output.WriteByte(0xab) == soft3d::S3D_OK);
    CHECK(output.WriteInt16(0x1234) == soft3d::S3D_OK);
    CHECK(output.WriteUInt32(0x89abcdef) == soft3d::S3D_OK);
    CHECK(soft3d::WriteFixed(output, -2) == soft3d::S3D_OK);
    const uint8_t raw[] = {0xde, 0xad};
    CHECK(output.Write(raw, 2) == soft3d::S3D_OK);
    CHECK(output.WriteWideString(u"A\u00e9") == soft3d::S3D_OK);
    const std::vector<uint8_t> expected = {
        0x01, 0xab, 0x12, 0x34, 0x89, 0xab, 0xcd, 0xef, 0xff, 0xff,
        0xff, 0xfe, 0xde, 0xad, 0x00, 0x04, 0x41, 0x00, 0xe9, 0x00,
    };
    CHECK(output.GetSize() == static_cast<int32_t>(expected.size()));
    CHECK(output.Tell() == static_cast<int32_t>(expected.size()));
    CHECK(std::vector<uint8_t>(output.GetData(), output.GetData() + expected.size()) == expected);

    soft3d::InputDataStream input;
    input.SetData(expected.data(), static_cast<int32_t>(expected.size()));
    CHECK(input.GetSize() == static_cast<int32_t>(expected.size()));
    bool boolean = false;
    CHECK(input.ReadBool(boolean) == soft3d::S3D_OK && boolean);
    uint8_t byte = 0;
    CHECK(input.ReadByte(byte) == soft3d::S3D_OK && byte == 0xab);
    int16_t shortValue = 0;
    CHECK(input.ReadInt16(shortValue) == soft3d::S3D_OK && shortValue == 0x1234);
    uint32_t word = 0;
    CHECK(input.ReadUInt32(word) == soft3d::S3D_OK && word == 0x89abcdef);
    soft3d::Fixed fixed = 0;
    CHECK(soft3d::ReadFixed(input, fixed) == soft3d::S3D_OK && fixed == -2);
    uint8_t readRaw[2]{};
    CHECK(input.Read(readRaw, 2) == soft3d::S3D_OK);
    CHECK(std::equal(std::begin(readRaw), std::end(readRaw), std::begin(raw)));
    char16_t string[8]{};
    CHECK(input.ReadWideString(string, 8) == soft3d::S3D_OK);
    CHECK(std::u16string(string) == u"A\u00e9");
    CHECK(input.IsEnd());
    byte = 0x55;
    CHECK(input.ReadByte(byte) == soft3d::S3D_ACCESS && byte == 0x55);
    CHECK(input.Seek(-1, SEEK_END) == soft3d::S3D_OK);
    CHECK(input.Tell() == static_cast<int32_t>(expected.size()));
    CHECK(input.Seek(0, SEEK_END) == soft3d::S3D_ACCESS);
    CHECK(input.Seek(0, SEEK_SET) == soft3d::S3D_OK);
    CHECK(input.Seek(1, 99) == soft3d::S3D_PARAM);
    CHECK(input.Seek(-1, SEEK_SET) == soft3d::S3D_ACCESS);

    const uint8_t four[] = {1, 2, 3, 4};
    soft3d::InputDataStream truncated(four, 4);
    CHECK(truncated.GetSize() == 3);
    CHECK(truncated.Seek(2, SEEK_SET) == soft3d::S3D_OK);
    CHECK(truncated.ReadByte(byte) == soft3d::S3D_OK && byte == 3);
    CHECK(truncated.ReadByte(byte) == soft3d::S3D_ACCESS);

    soft3d::OutputDataStream copied(output);
    CHECK(copied.GetSize() == output.GetSize());
    CHECK(copied.WriteByte(0x64) == soft3d::S3D_OK);
    CHECK(output.GetSize() == static_cast<int32_t>(expected.size()));
    copied.Clear();
    CHECK(copied.GetSize() == -1 && copied.Tell() == 0);
}

void TestFileStreamsAndNarrowIo()
{
    TempFile temporary("soft3d-stream-tests.bin");
    const std::u16string widePath = temporary.path.u16string();
    {
        soft3d::OutputFileStream output(widePath.c_str());
        CHECK(output.IsOpen());
        CHECK(output.WriteUInt32(0x12345678) == soft3d::S3D_OK);
        CHECK(output.Flush() == soft3d::S3D_OK);
    }
    const std::vector<uint8_t> expected = {0x12, 0x34, 0x56, 0x78, 0x00};
    CHECK(ReadFile(temporary.path) == expected);
    {
        soft3d::InputFileStream input(widePath.c_str());
        CHECK(input.IsValid() && input.GetSize() == 4);
        uint32_t value = 0;
        CHECK(input.ReadUInt32(value) == soft3d::S3D_OK && value == 0x12345678);
        uint8_t byte = 0;
        CHECK(input.ReadByte(byte) == soft3d::S3D_ACCESS);
    }
    {
        soft3d::OutputFileStream output(widePath.c_str());
        CHECK(output.WriteByte(0xaa) == soft3d::S3D_OK);
        CHECK(output.Flush() == soft3d::S3D_OK);
    }
    CHECK(ReadFile(temporary.path) == std::vector<uint8_t>({0xaa, 0x00}));
    {
        soft3d::InputFileStream missing(u".\\path-that-does-not-exist\\missing.sav");
        CHECK(missing.IsValid() && missing.GetSize() == -1);
        uint8_t byte = 0;
        CHECK(missing.ReadByte(byte) == soft3d::S3D_ACCESS);
    }
    {
        soft3d::OutputFileStream output(u".\\path-that-does-not-exist\\missing.sav");
        CHECK(!output.IsOpen() && output.Flush() == soft3d::S3D_FILE);
    }
    {
        soft3d::s3dIO io;
        const std::string narrowPath = temporary.path.string();
        CHECK(io.Open(narrowPath.c_str(), "wb"));
        const uint8_t bytes[] = {1, 2, 3};
        CHECK(io.Write(bytes, 3, 1) == 0);
        CHECK(io.Reopen(narrowPath.c_str(), "rb"));
        uint8_t read[3]{};
        CHECK(io.Read(read, 3) == 3 && std::equal(std::begin(read), std::end(read), bytes));
        CHECK(io.Reopen(narrowPath.c_str(), "not-a-mode") == false);
        io.Close();
    }
}

void TestMemoryDiagnostics()
{
    const int64_t total = soft3d::MemTotal();
    const int64_t remaining = soft3d::MemRemain();
    CHECK(total > 0);
    CHECK(remaining >= 0 && remaining <= total);
}

void TestGameUnitState()
{
    soft3d::GameUnit source;
    source.SetUnitType(7);
    source.SetUnitId(42);
    source.SetVisible(false);
    source.SetCollisionEnabled(false);
    source.SetPosition({0x10000, -0x20000, 0x30000});
    source.SetLocalBounds({{-1, -2, -3}, {4, 5, 6}});
    source.SetCollisionBounds({{-7, -8, -9}, {10, 11, 12}});
    soft3d::OutputDataStream output;
    source.SaveState(output);
    CHECK(output.GetSize() == 72);
    CHECK(output.GetData()[3] == 7 && output.GetData()[7] == 42);

    soft3d::InputDataStream input;
    input.SetData(output.GetData(), output.GetSize());
    soft3d::GameUnit target;
    target.SetUnitType(99);
    target.SetCollisionEnabled(true);
    int32_t serializedType = 0;
    CHECK(input.ReadInt32(serializedType) == soft3d::S3D_OK);
    target.LoadState(input);
    CHECK(target.GetUnitType() == 99);
    CHECK(target.GetUnitId() == 42);
    CHECK(!target.IsVisible());
    CHECK(target.IsCollisionEnabled());
    CHECK(target.GetPosition().x == 0x10000 && target.GetPosition().y == -0x20000 &&
          target.GetPosition().z == 0x30000);
    CHECK(target.GetLocalBounds().minimum.y == -2 && target.GetLocalBounds().maximum.z == 6);
    CHECK(target.GetCollisionBounds().minimum.x == -7 && target.GetCollisionBounds().maximum.z == 12);

    soft3d::VisUnit visual;
    visual.SetUnitId(9);
    visual.SetVisible(false);
    visual.SetPositionalAudio(true);
    visual.EnableShadow(true);
    soft3d::OutputDataStream visualOutput;
    visual.SaveState(visualOutput);
    CHECK(visualOutput.GetSize() == 92);
    soft3d::InputDataStream visualInput;
    visualInput.SetData(visualOutput.GetData(), visualOutput.GetSize());
    soft3d::VisUnit restored;
    int32_t visualType = 0;
    CHECK(visualInput.ReadInt32(visualType) == soft3d::S3D_OK && visualType == 1);
    restored.LoadState(visualInput);
    CHECK(restored.GetUnitType() == 1 && restored.GetUnitId() == 9);
    CHECK(!restored.IsVisible() && restored.GetObjectCount() == 0);
    CHECK(restored.IsShadowEnabled() && restored.IsPositionalAudio());
}

void TestRealVisUnitState()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    soft3d::s3dResManager manager;
    CHECK(soft3d::OpenGameResourceArchive(&manager, SOFT3D_TEST_ARCHIVE) == soft3d::S3D_OK);
    soft3d::Renderer renderer(320, 240);
    soft3d::VisUnit source(&manager, 0, &renderer, nullptr);
    CHECK(source.LoadObjects(u"cg_bully_tuomao_a", nullptr, 2) == soft3d::S3D_OK);
    CHECK(source.LoadMaterials() == soft3d::S3D_OK);
    CHECK(source.LoadAnimation(u"cg_bully_cc_aa") == soft3d::S3D_OK);
    CHECK(source.LoadAlternateObject(u"cg_bully_tuomao_a") == soft3d::S3D_OK);
    source.SetUnitId(64);
    source.SetDirection({0x10000, 0, 0x10000});
    source.EnableShadow(true);
    source.SetPositionalAudio(true);
    soft3d::Object* object = source.GetObject(0);
    CHECK(object != nullptr);
    object->SetDepthBias(0x1234);
    object->SetObjectIntensity(0x5678);
    object->SetRenderMode(soft3d::ObjectRenderMode::Wireframe);
    object->SetLightingEnabled(false);
    object->SetAmbientLightOnly(true);
    object->SetCullingDisabled(true);
    object->SetExternalTransformEnabled(false);
    object->SetMaterialIntensityState(0, 0x1000, 0x9000, 0x5000, -0x100);
    object->RestoreAnimationState(1, 123, -1, 0, 450);
    source.GetObject(1)->RestoreAnimationState(1, 123, -1, 0, 450);

    soft3d::OutputDataStream output;
    source.SaveState(output);
    soft3d::InputDataStream input;
    input.SetData(output.GetData(), output.GetSize());
    soft3d::VisUnit target(&manager, 3, &renderer, nullptr);
    int32_t serializedType = 0;
    CHECK(input.ReadInt32(serializedType) == soft3d::S3D_OK);
    target.LoadState(input);
    CHECK(target.GetUnitId() == 64 && target.GetObjectCount() == 2);
    CHECK(target.GetAnimation() != nullptr && target.IsShadowEnabled());
    CHECK(target.IsPositionalAudio());
    soft3d::Object* restored = target.GetObject(0);
    CHECK(restored != nullptr);
    CHECK(restored->GetDepthBias() == 0x1234 && restored->GetObjectIntensity() == 0x5678);
    CHECK(restored->GetRenderMode() == soft3d::ObjectRenderMode::Wireframe);
    CHECK(!restored->IsLightingEnabled() && restored->IsAmbientLightOnly());
    CHECK(restored->IsCullingDisabled() && !restored->IsExternalTransformEnabled());
    const std::array<soft3d::Fixed, 4> expectedIntensity = {0x1000, 0x9000, 0x5000, -0x100};
    CHECK(restored->GetMaterialIntensityState(0) == expectedIntensity);
    CHECK(restored->IsAnimationPaused() && restored->GetAnimationTime() == 123);
    CHECK(restored->GetAnimationDirection() == -1 && !restored->IsAnimationLooping());
    CHECK(restored->GetAnimationSpeed() == 450);
    CHECK(restored->GetAxisX().x == object->GetAxisX().x &&
          restored->GetAxisZ().z == object->GetAxisZ().z);
#endif
}

void TestLifecyclePersistence()
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "soft3d-stream-state";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    CHECK(std::filesystem::create_directories(directory));
    const auto makeEngine = [] {
        soft3d::EngineConfig config;
        config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
        return std::make_unique<soft3d::GameEngine>(std::move(config));
    };
    auto engine = makeEngine();
    engine->SetPersistenceDirectory(directory.u16string());
    engine->GetAudioManager().SetGlobalVolume(64);
    engine->GetConfiguration().vibrationEnabled = 1;
    engine->GetConfiguration().controlBindings[2] = 19;
    engine->GetConfiguration().unlocks.models[3] = 42;
    engine->SaveGameState();
    engine->GetConfiguration().masterVolume = 0;
    engine->GetConfiguration().unlocks.models[3] = 0;
    engine->LoadGameState();
    CHECK(engine->GetConfiguration().masterVolume == 64);
    CHECK(engine->GetConfiguration().vibrationEnabled == 1);
    CHECK(engine->GetConfiguration().controlBindings[2] == 19);
    CHECK(engine->GetConfiguration().unlocks.models[3] == 42);
    engine->Start();
    engine->Suspend();
    engine->Resume();
    CHECK(!engine->IsRunning());
    engine->RequestExit();
    CHECK(!engine->IsRunning());

    auto restarted = makeEngine();
    restarted->SetPersistenceDirectory(directory.u16string());
    restarted->LoadGameState();
    CHECK(restarted->GetConfiguration().masterVolume == 64);
    CHECK(restarted->GetConfiguration().controlBindings[2] == 19);
    CHECK(restarted->GetConfiguration().unlocks.models[3] == 42);
    std::filesystem::remove_all(directory, error);
}

} // namespace

int main()
{
    try {
        TestMemoryStreams();
        TestFileStreamsAndNarrowIo();
        TestMemoryDiagnostics();
        TestGameUnitState();
        TestRealVisUnitState();
        TestLifecyclePersistence();
        std::cout << "stream, state, IO, and diagnostic tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

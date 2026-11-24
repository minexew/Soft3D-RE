#include "game/License.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyServices.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "platform/linux/LinuxServices.hpp"
#include "soft3d/MenuDialogs.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

#define CHECK(condition) Check((condition), __LINE__)

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

std::vector<uint8_t> Hex(const char* text)
{
    std::vector<uint8_t> output;
    for (size_t index = 0; text[index] != 0 && text[index + 1] != 0; index += 2) {
        const auto digit = [](char value) -> unsigned {
            if (value >= '0' && value <= '9') return value - '0';
            if (value >= 'a' && value <= 'f') return value - 'a' + 10;
            return value - 'A' + 10;
        };
        output.push_back(static_cast<uint8_t>((digit(text[index]) << 4) | digit(text[index + 1])));
    }
    return output;
}

void Append16(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

void Append32(std::vector<uint8_t>& output, uint32_t value)
{
    output.push_back(static_cast<uint8_t>(value >> 24));
    output.push_back(static_cast<uint8_t>(value >> 16));
    output.push_back(static_cast<uint8_t>(value >> 8));
    output.push_back(static_cast<uint8_t>(value));
}

void TestCryptographyAndResources()
{
    const std::string text = "abc";
    const auto digest = game::License::Md5(text.data(), text.size());
    CHECK(Hex("900150983cd24fb0d6963f7d28e17f72") ==
          std::vector<uint8_t>(digest.begin(), digest.end()));
    game::License license;
    const auto input = Hex("a364748285f5e2549f9af291507a67fb");
    const auto expected = Hex("f393f72a4fd58d27c0cb826d687299f8");
    const auto decrypted = game::License::EncryptBlocks(input.data(), input.size(),
                                                        license.GetInitialKeySchedule(), true);
    std::array<uint8_t, 128> zeroKey{};
    std::array<uint8_t, 8> zeroBlock{};
    const auto zeroResult = game::License::EncryptBlocks(zeroBlock.data(), 8, zeroKey, false);
    CHECK(zeroResult == Hex("c1b123a78ca64de9"));
    CHECK(decrypted == expected);
    CHECK(game::License::EncryptBlocks(decrypted.data(), decrypted.size(),
                                       license.GetInitialKeySchedule(), false) == input);
    std::array<uint8_t, 8> plainBlock{0, 1, 2, 3, 4, 5, 6, 7};
    CHECK(game::License::EncryptBlocks(plainBlock.data(), plainBlock.size(),
                                       license.GetInitialKeySchedule(), false) ==
          Hex("c3bdb5578700247c"));
    CHECK(game::License::EncryptBlocks(input.data(), 7, license.GetInitialKeySchedule(), true).empty());

    soft3d::s3dFilePak pak;
    CHECK(pak.Open(SOFT3D_TEST_ARCHIVE, 1) == soft3d::S3D_OK);
    CHECK(pak.Select(u".\\common\\message.sdt") == soft3d::S3D_OK);
    std::array<uint8_t, 512> message{};
    CHECK(pak.Read(message.data(), message.size()) == soft3d::S3D_OK);
    CHECK(message[0] == 0xb4 && message[0x100] == 0x34);
    CHECK(pak.Select(u".\\common\\domain.sdt") == soft3d::S3D_OK);
    std::array<uint8_t, 36> domain{};
    CHECK(pak.Read(domain.data(), domain.size()) == soft3d::S3D_OK);
    CHECK(std::equal(input.begin(), input.end(), domain.begin()));

    const std::vector<uint8_t> plain = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const auto packet = game::License::EncodePacket(plain, license.GetInitialKeySchedule());
    CHECK(packet.size() == 12 && packet[0] == 0 && packet[1] == 10);
    CHECK(packet[2] == 0 && packet[3] == 1);
    CHECK(game::License::EncodePacket({}, license.GetInitialKeySchedule()).empty());
}

void TestProtocolAndPolicy()
{
    auto identity = std::make_shared<soft3d::platform::DummyDeviceIdentity>();
    auto network = std::make_shared<soft3d::platform::DummyNetworkSession>();
    auto sms = std::make_shared<soft3d::platform::DummySmsSession>();
    identity->identity = "123456789012345";
    game::License license;
    license.SetServices(identity, network, sms);
    license.SetServer("recorded.invalid", 1234);
    network->exchanges.push_back({{}, {}});
    CHECK(license.ConnectSession());
    CHECK(network->connected && network->host == "recorded.invalid" && network->port == 1234);
    CHECK(network->lastPacket.size() == 140);
    CHECK(network->lastPacket[0] == 0 && network->lastPacket[1] == 138);
    CHECK(network->lastPacket[2] == 0 && network->lastPacket[3] == 0);

    std::array<uint8_t, 128> sessionKey{};
    for (size_t index = 0; index < sessionKey.size(); ++index) sessionKey[index] = index;
    std::vector<uint8_t> handshake;
    Append16(handshake, 100);
    const auto encryptedKey = game::License::EncryptBlocks(sessionKey.data(), sessionKey.size(),
                                                           license.GetInitialKeySchedule(), false);
    handshake.insert(handshake.end(), encryptedKey.begin(), encryptedKey.end());
    network->exchanges.push_back({{}, {}});
    CHECK(license.HandleResponse(handshake.data(), handshake.size()));
    CHECK(license.HasSessionKey());
    CHECK(license.GetSessionKeySchedule() == sessionKey);
    CHECK(network->lastPacket.size() == 28 && network->lastPacket[0] == 0 && network->lastPacket[1] == 26);
    CHECK(license.SendVerification(3, "abc"));

    int menuResult = -1;
    license.SetMenuResultHandler([&](int result) { menuResult = result; });
    std::vector<uint8_t> plain;
    Append32(plain, 1);
    Append32(plain, 0);
    std::vector<uint8_t> response;
    Append16(response, 101);
    const auto encrypted = game::License::EncryptBlocks(plain.data(), plain.size(),
                                                        license.GetSessionKeySchedule(), false);
    response.insert(response.end(), encrypted.begin(), encrypted.end());
    CHECK(license.HandleResponse(response.data(), response.size()));
    CHECK(menuResult == 4);
    CHECK(license.GetVerificationWords()[0] == 1);
    CHECK(!license.HandleResponse(response.data(), 3));
    CHECK(!license.HandleResponse(response.data(), 9));

    license.ClearVerification();
    for (int index = 0; index < 9; ++index) license.SetVerification(index);
    CHECK(license.CheckVerification(7) == 1);
    CHECK(license.CheckVerification(0) == 1);
    license.SetCheckPolicy(game::LicenseCheckPolicy::Parameterized);
    CHECK(license.CheckVerification(7) == -9);
    license.SetVerification(9);
    license.ClearVerification();
    CHECK(license.CheckVerification(0) == 1);
    license.SetVerification(0);
    CHECK(license.CheckVerification(0) == -1);

    game::License unsupported;
    CHECK(!unsupported.ConnectSession());
}

void TestSceneAuthorizationAndVibration()
{
    auto sms = std::make_shared<soft3d::platform::DummySmsSession>();
    game::License license;
    license.SetServices(nullptr, nullptr, sms);
    sms->completionStatus = 1;
    CHECK(license.BeginSceneAuthorization() == 2);
    for (int count = 0; count < 8; ++count) CHECK(license.UpdateSceneAuthorization() == 2);
    CHECK(license.UpdateSceneAuthorization() == 4);
    CHECK(license.GetAuthorizationAttempts() == 10);
    CHECK(license.GetVerificationWords()[8] == 1);

    license.ClearVerification();
    license.SetCheckPolicy(game::LicenseCheckPolicy::Parameterized);
    CHECK(license.BeginSceneAuthorization() == 2);
    CHECK(license.UpdateSceneAuthorization() == 4);
    CHECK(license.GetAuthorizationAttempts() == 1);

    game::License failed;
    failed.SetServices(nullptr, nullptr, sms);
    sms->completionStatus = 2;
    failed.BeginSceneAuthorization();
    CHECK(failed.UpdateSceneAuthorization() == 5);
    sms->completionStatus = 0;
    failed.BeginSceneAuthorization();
    for (int count = 0; count < 2881; ++count) failed.UpdateSceneAuthorization();
    CHECK(failed.UpdateSceneAuthorization() == 5);

    soft3d::platform::DummyVibrationBackend vibration;
    CHECK(vibration.IsSupported());
    CHECK(!vibration.Vibrate(20));
    vibration.SetEnabled(true);
    CHECK(vibration.Vibrate(20));
    CHECK(vibration.pulses == std::vector<uint32_t>{20});
    vibration.supported = false;
    CHECK(!vibration.Vibrate(20));
    soft3d::platform::UnsupportedVibrationBackend unsupported;
    CHECK(!unsupported.IsSupported() && !unsupported.Vibrate(20));
}

struct EngineFixture {
    EngineFixture()
    {
        soft3d::EngineConfig config;
        config.width = 320;
        config.height = 240;
        config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
        auto vibrationBackend = std::make_unique<soft3d::platform::DummyVibrationBackend>();
        vibration = vibrationBackend.get();
        config.vibrationBackend = std::move(vibrationBackend);
        engine = std::make_unique<soft3d::GameEngine>(std::move(config));
        engine->SetResourceArchive(SOFT3D_TEST_ARCHIVE);
        CHECK(engine->Init() == soft3d::S3D_OK);
        auto sceneOwner = std::make_unique<game::Scene>(*engine);
        scene = sceneOwner.get();
        CHECK(sceneOwner->Initialize() == soft3d::S3D_OK);
        engine->SetSceneHost(std::move(sceneOwner));
        engine->SetAuthorizationHost(scene->GetLicense());
        soft3d::GameplayHooks hooks;
        hooks.startNewGame = [this](const soft3d::NewGameOptions& options) {
            scene->StartNewGame(options);
        };
        hooks.returnToMenu = [this] { scene->ReturnToMainMenu(); };
        engine->SetGameplayHooks(std::move(hooks));
        engine->Start();
    }

    void Click(soft3d::Key key)
    {
        engine->GetInput().KeyDown(key);
        engine->GetInput().KeyUp(key);
        CHECK(engine->RunFrame());
    }

    void RunFrames(int count)
    {
        for (int index = 0; index < count; ++index) CHECK(engine->RunFrame());
    }

    uint32_t HashFrame() const
    {
        uint32_t hash = 2166136261U;
        const uint32_t* pixels = engine->GetVideo().GetBackBufferPtr();
        for (int index = 0; index < 320 * 240; ++index) {
            hash = (hash ^ pixels[index]) * 16777619U;
        }
        return hash;
    }

    std::unique_ptr<soft3d::GameEngine> engine;
    game::Scene* scene = nullptr;
    soft3d::platform::DummyVibrationBackend* vibration = nullptr;
};

void TestEngineAuthorizationFlow()
{
    {
        EngineFixture fixture;
        CHECK(fixture.scene->GetDoorAuthorizationStatus() == 0);
        fixture.engine->StartNewGame();
        fixture.scene->BeginAuthorization();
        CHECK(fixture.engine->RunFrame());
        CHECK(fixture.scene->GetAuthorizationStage() == 1);
        const uint32_t promptHash = fixture.HashFrame();
        CHECK(promptHash == 0xd893fa55);
        fixture.Click(soft3d::Key::Accept);
        CHECK(fixture.scene->GetAuthorizationStage() == 2);
        const uint32_t progressHash = fixture.HashFrame();
        CHECK(progressHash == 0xd893fa55);
        fixture.RunFrames(9);
        CHECK(fixture.scene->GetAuthorizationStage() == 4);
        CHECK(fixture.scene->GetAuthorizationActive() == 0);
        CHECK(fixture.scene->GetLicense()->GetAuthorizationAttempts() == 10);
    }
    {
        EngineFixture fixture;
        auto sms = std::make_shared<soft3d::platform::UnsupportedSmsSession>();
        fixture.scene->GetLicense()->SetServices(nullptr, nullptr, sms);
        fixture.engine->StartNewGame();
        fixture.scene->BeginAuthorization();
        fixture.Click(soft3d::Key::Accept);
        CHECK(fixture.engine->RunFrame());
        CHECK(fixture.scene->GetAuthorizationStage() == 5);
        const uint32_t failureHash = fixture.HashFrame();
        CHECK(failureHash == 0xbc523725);
        fixture.Click(soft3d::Key::SoftRight);
        CHECK(!fixture.engine->IsInGame());
        CHECK(fixture.scene->GetAuthorizationActive() == 0);
    }
    {
        EngineFixture fixture;
        fixture.engine->StartNewGame();
        CHECK(fixture.scene->LoadDay(2) == soft3d::S3D_OK);
        CHECK(fixture.engine->RunFrame());
        CHECK(fixture.scene->GetAuthorizationStage() == 1);
        CHECK(fixture.scene->GetAuthorizationActive() == 1);
        fixture.scene->GetLicense()->SetCheckPolicy(game::LicenseCheckPolicy::OfflineAuthorized);
        fixture.scene->ReturnToMainMenu();
        fixture.engine->SetInGame(true);
        CHECK(fixture.scene->LoadDay(2) == soft3d::S3D_OK);
        CHECK(fixture.engine->RunFrame());
        CHECK(fixture.scene->GetAuthorizationActive() == 0);
    }
    {
        EngineFixture fixture;
        fixture.engine->GetMainMenu()->ShowAuthorization(0);
        fixture.Click(soft3d::Key::SoftRight);
        CHECK(fixture.engine->GetMainMenu()->GetAuthorizationState() == 2);
        fixture.Click(soft3d::Key::Accept);
        CHECK(fixture.engine->GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);
    }
    {
        EngineFixture fixture;
        fixture.engine->GetRenderer().Clear(0);
        fixture.engine->GetMainMenu()->ShowAuthorization(1);
        CHECK(fixture.engine->RunFrame());
        fixture.Click(soft3d::Key::Accept);
        CHECK(fixture.engine->GetMainMenu()->GetAuthorizationState() == 4);
        const uint32_t menuFailureHash = fixture.HashFrame();
        CHECK(menuFailureHash == 0x4f70a5fd);
        fixture.Click(soft3d::Key::Accept);
        CHECK(fixture.engine->GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);
    }
    {
        EngineFixture fixture;
        CHECK(fixture.vibration->IsSupported());
        CHECK(!fixture.engine->Vibrate(25));
        fixture.engine->SetVibrationEnabled(true);
        CHECK(fixture.engine->Vibrate(25));
        CHECK(fixture.vibration->pulses == std::vector<uint32_t>{25});
        fixture.vibration->supported = false;
        CHECK(!fixture.engine->IsVibrationSupported());
        CHECK(!fixture.engine->Vibrate(25));
    }
}

void TestLinuxIdentityAndCancellation()
{
    soft3d::platform::LinuxDeviceIdentityProvider provider;
    std::string identity;
    CHECK(provider.GetDeviceIdentity(identity));
    CHECK(identity == "001001000000007");
    int sum = 0;
    for (int index = 0; index < 15; ++index) {
        int digit = identity[14 - index] - '0';
        if ((index & 1) == 1) {
            digit *= 2;
            if (digit > 9) digit -= 9;
        }
        sum += digit;
    }
    CHECK(sum % 10 == 0);

    auto network = std::make_shared<soft3d::platform::DummyNetworkSession>();
    game::License license;
    license.SetServices(nullptr, network, nullptr);
    network->connected = true;
    CHECK(license.PollSession());
    license.CancelSession();
    CHECK(!license.PollSession());
    auto identityFailure = std::make_shared<soft3d::platform::DummyDeviceIdentity>();
    identityFailure->available = false;
    license.SetServices(identityFailure, network, nullptr);
    CHECK(!license.ConnectSession());
}

} // namespace

int main()
{
    try {
        TestCryptographyAndResources();
        TestProtocolAndPolicy();
        TestSceneAuthorizationAndVibration();
        TestLinuxIdentityAndCancellation();
        TestEngineAuthorizationFlow();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "License protocol, policy, resources, and vibration tests passed\n";
    return 0;
}

#pragma once

#include "platform/PlatformServices.hpp"
#include "soft3d/GameEngine.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace game {

enum class LicenseCheckPolicy {
    Parameterized,
    CrackedBinary,
    OfflineAuthorized,
};

class License final : public soft3d::AuthorizationHost {
public:
    License();

    void SetServices(std::shared_ptr<soft3d::platform::DeviceIdentityProvider> identity,
                     std::shared_ptr<soft3d::platform::NetworkSession> network,
                     std::shared_ptr<soft3d::platform::SmsSession> sms);
    void SetMessageData(const void* data, size_t size);
    void SetServer(std::string host, uint16_t port);
    void SetCheckPolicy(LicenseCheckPolicy policy) { checkPolicy = policy; }
    void SetMenuRegistered(bool registered) { menuRegistered = registered; }
    void SetRandomSlotSource(std::function<int()> source) { randomSlot = std::move(source); }
    void SetSceneResultHandler(std::function<void(int)> handler) { sceneResult = std::move(handler); }
    void SetMenuResultHandler(std::function<void(int)> handler) override;
    void SetPayloadHandler(std::function<void(const std::vector<uint8_t>&)> handler)
    {
        payloadHandler = std::move(handler);
    }

    void ResetSession() override;
    bool ConnectSession() override;
    void CancelSession();
    bool PollSession() override;
    bool SendVerification(uint16_t operation, const std::string& text = {});
    bool HandleResponse(const void* data, size_t size);

    int CheckVerification(int requestedSlot) const;
    void SetVerification(int slot);
    void ClearVerification();
    const std::array<int32_t, 16>& GetVerificationWords() const { return verification; }
    bool HasSessionKey() const { return sessionKeyReceived; }
    int GetAuthorizationAttempts() const { return authorizationAttempts; }
    int GetAuthorizationElapsed() const { return authorizationElapsed; }
    int BeginSceneAuthorization();
    int UpdateSceneAuthorization();

    const std::array<uint8_t, 128>& GetInitialKeySchedule() const { return initialKeySchedule; }
    const std::array<uint8_t, 128>& GetSessionKeySchedule() const { return sessionKeySchedule; }

    static std::vector<uint8_t> EncryptBlocks(const void* data, size_t size,
                                               const std::array<uint8_t, 128>& schedule,
                                               bool decrypt);
    static std::array<uint8_t, 16> Md5(const void* data, size_t size);
    static std::vector<uint8_t> EncodePacket(const std::vector<uint8_t>& plain,
                                              const std::array<uint8_t, 128>& schedule);

private:
    bool SendPlain(std::vector<uint8_t> plain, bool initialKey);
    std::u16string DecodeMessage(int index) const;

    std::shared_ptr<soft3d::platform::DeviceIdentityProvider> identityProvider;
    std::shared_ptr<soft3d::platform::NetworkSession> network;
    std::shared_ptr<soft3d::platform::SmsSession> sms;
    std::array<uint8_t, 256> identity{};
    std::array<std::array<uint8_t, 256>, 2> messages{};
    std::array<uint8_t, 128> initialKeySchedule{};
    std::array<uint8_t, 128> sessionKeySchedule{};
    std::array<int32_t, 16> verification{};
    LicenseCheckPolicy checkPolicy = LicenseCheckPolicy::CrackedBinary;
    std::string serverHost;
    uint16_t serverPort = 0;
    bool sessionKeyReceived = false;
    bool menuRegistered = false;
    int authorizationAttempts = 0;
    int authorizationElapsed = 0;
    int sceneTimeout = 0;
    std::function<int()> randomSlot = [] { return 0; };
    std::function<void(int)> menuResult;
    std::function<void(int)> sceneResult;
    std::function<void(const std::vector<uint8_t>&)> payloadHandler;
};

} // namespace game

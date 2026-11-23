#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace soft3d::platform {

class DeviceIdentityProvider {
public:
    virtual ~DeviceIdentityProvider() = default;
    virtual bool GetDeviceIdentity(std::string& identity) = 0;
};

class NetworkSession {
public:
    enum class Status {
        Pending,
        Received,
        Failed,
    };

    virtual ~NetworkSession() = default;
    virtual bool Connect(const std::string& host, uint16_t port) = 0;
    virtual bool Send(const std::vector<uint8_t>& packet) = 0;
    virtual Status Poll(std::vector<uint8_t>& response) = 0;
    virtual void Cancel() = 0;
};

class SmsSession {
public:
    virtual ~SmsSession() = default;
    virtual bool Send(const std::u16string& recipient, const std::u16string& body) = 0;
    virtual int PollStatus() const = 0;
    virtual void Cancel() = 0;
};

class VibrationBackend {
public:
    virtual ~VibrationBackend() = default;
    virtual bool IsSupported() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool Vibrate(uint32_t durationMilliseconds) = 0;
};

class UnsupportedDeviceIdentity final : public DeviceIdentityProvider {
public:
    bool GetDeviceIdentity(std::string&) override { return false; }
};

class UnsupportedNetworkSession final : public NetworkSession {
public:
    bool Connect(const std::string&, uint16_t) override { return false; }
    bool Send(const std::vector<uint8_t>&) override { return false; }
    Status Poll(std::vector<uint8_t>&) override { return Status::Failed; }
    void Cancel() override {}
};

class UnsupportedSmsSession final : public SmsSession {
public:
    bool Send(const std::u16string&, const std::u16string&) override { return false; }
    int PollStatus() const override { return 2; }
    void Cancel() override {}
};

class UnsupportedVibrationBackend final : public VibrationBackend {
public:
    bool IsSupported() const override { return false; }
    void SetEnabled(bool) override {}
    bool Vibrate(uint32_t) override { return false; }
};

} // namespace soft3d::platform

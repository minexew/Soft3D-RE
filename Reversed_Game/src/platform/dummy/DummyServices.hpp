#pragma once

#include "platform/PlatformServices.hpp"

#include <deque>
#include <utility>

namespace soft3d::platform {

class DummyDeviceIdentity final : public DeviceIdentityProvider {
public:
    std::string identity;
    bool available = true;

    bool GetDeviceIdentity(std::string& output) override
    {
        output = identity;
        return available && !identity.empty();
    }
};

class DummyNetworkSession final : public NetworkSession {
public:
    struct Exchange {
        std::vector<uint8_t> request;
        std::vector<uint8_t> response;
    };

    bool available = true;
    bool connected = false;
    int pendingPolls = 0;
    std::string host;
    uint16_t port = 0;
    std::vector<uint8_t> lastPacket;
    std::deque<Exchange> exchanges;

    bool Connect(const std::string& value, uint16_t portValue) override
    {
        host = value;
        port = portValue;
        connected = available;
        return connected;
    }

    bool Send(const std::vector<uint8_t>& packet) override
    {
        lastPacket = packet;
        if (!connected || exchanges.empty()) {
            return false;
        }
        return true;
    }

    Status Poll(std::vector<uint8_t>& response) override
    {
        if (!connected || !available) {
            return Status::Failed;
        }
        if (pendingPolls > 0) {
            --pendingPolls;
            return Status::Pending;
        }
        if (exchanges.empty()) {
            return Status::Pending;
        }
        lastPacket = exchanges.front().request.size() == lastPacket.size()
            ? lastPacket : exchanges.front().request;
        response = exchanges.front().response;
        exchanges.pop_front();
        return Status::Received;
    }

    void Cancel() override { connected = false; }
};

class DummySmsSession final : public SmsSession {
public:
    bool available = true;
    int status = 0;
    int completionStatus = 0;
    int sendCount = 0;
    std::u16string recipient;
    std::u16string body;

    bool Send(const std::u16string& to, const std::u16string& text) override
    {
        if (!available) {
            status = 2;
            return false;
        }
        recipient = to;
        body = text;
        ++sendCount;
        status = completionStatus;
        return true;
    }

    int PollStatus() const override { return status; }
    void Cancel() override { status = 2; }
};

class DummyVibrationBackend final : public VibrationBackend {
public:
    bool supported = true;
    bool enabled = false;
    std::vector<uint32_t> pulses;

    bool IsSupported() const override { return supported; }
    void SetEnabled(bool value) override { enabled = value; }
    bool Vibrate(uint32_t durationMilliseconds) override
    {
        if (!supported || !enabled) {
            return false;
        }
        pulses.push_back(durationMilliseconds);
        return true;
    }
};

} // namespace soft3d::platform

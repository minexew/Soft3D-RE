#pragma once

#include "platform/PlatformServices.hpp"

namespace soft3d::platform {

class LinuxDeviceIdentityProvider final : public DeviceIdentityProvider {
public:
    bool GetDeviceIdentity(std::string& identity) override
    {
        identity = "001001000000007";
        return true;
    }
};

class LinuxSmsSession final : public SmsSession {
public:
    bool Send(const std::u16string&, const std::u16string&) override
    {
        status = 1;
        return true;
    }

    int PollStatus() const override { return status; }
    void Cancel() override { status = 2; }

private:
    int status = 0;
};

} // namespace soft3d::platform

#pragma once

#include "soft3d/EngineClock.hpp"

namespace soft3d::platform {

class SystemClock final : public EngineClock {
public:
    uint32_t NowMicroseconds() override;
    void SleepMicroseconds(uint32_t microseconds) override;
};

} // namespace soft3d::platform

#pragma once

#include <cstdint>

namespace soft3d {

class EngineClock {
public:
    virtual ~EngineClock() = default;

    virtual uint32_t NowMicroseconds() = 0;
    virtual void SleepMicroseconds(uint32_t microseconds) = 0;
};

} // namespace soft3d

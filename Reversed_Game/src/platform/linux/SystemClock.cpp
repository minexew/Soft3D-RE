#include "platform/linux/SystemClock.hpp"

#include <chrono>
#include <thread>

namespace soft3d::platform {

uint32_t SystemClock::NowMicroseconds()
{
    const auto elapsed = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}

void SystemClock::SleepMicroseconds(uint32_t microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

} // namespace soft3d::platform

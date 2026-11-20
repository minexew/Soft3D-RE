#include "soft3d/s3dMemory.hpp"

#include <sys/sysinfo.h>

namespace soft3d {
namespace {

int64_t QueryMemory(bool freeMemory)
{
    struct sysinfo information;
    if (sysinfo(&information) != 0) {
        return -1;
    }
    const uint64_t pages = freeMemory ? information.freeram : information.totalram;
    return static_cast<int64_t>(pages * static_cast<uint64_t>(information.mem_unit));
}

} // namespace

int64_t MemTotal()
{
    return QueryMemory(false);
}

int64_t MemRemain()
{
    return QueryMemory(true);
}

} // namespace soft3d

#include "soft3d/s3dLog.hpp"

namespace soft3d {

void s3dLog::SetEngine(Engine* value)
{
    io.SetEngine(value);
    s3dBase::SetEngine(value);
}

} // namespace soft3d

#include "soft3d/s3dLog.hpp"

#ifdef SOFT3D_LOG_BACKEND_STDIO
#include "platform/linux/StdioLog.hpp"
#endif

#include <cstdarg>

namespace soft3d {

void s3dLog::SetEngine(Engine* value)
{
    io.SetEngine(value);
    s3dBase::SetEngine(value);
}

void s3dLog::On()
{
#ifdef SOFT3D_LOG_BACKEND_STDIO
    platform::StdioLog::On(io);
#endif
}

void s3dLog::Output(const char16_t* format, ...)
{
#ifdef SOFT3D_LOG_BACKEND_STDIO
    va_list arguments;
    va_start(arguments, format);
    platform::StdioLog::Output(io, format, arguments);
    va_end(arguments);
#else
    (void)format;
#endif
}

} // namespace soft3d

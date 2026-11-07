#include "platform/linux/StdioLog.hpp"

#include "soft3d/s3dIO.hpp"

namespace soft3d::platform {

void StdioLog::On(s3dIO& io)
{
    if (io.Open(u"log.log", u"w")) {
        io.Printf(u"===== Log On =====\n");
        io.Close();
    }
}

void StdioLog::Output(s3dIO& io, const char16_t* format, va_list arguments)
{
    if (io.Open(u"log.log", u"a")) {
        io.VPrintf(format, arguments);
        io.Close();
    }
}

} // namespace soft3d::platform

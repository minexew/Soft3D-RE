#pragma once

#include <cstdarg>

namespace soft3d {
class s3dIO;
}

namespace soft3d::platform {

class StdioLog {
public:
    static void On(s3dIO& io);
    static void Output(s3dIO& io, const char16_t* format, va_list arguments);
};

} // namespace soft3d::platform

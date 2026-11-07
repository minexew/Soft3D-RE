#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dIO.hpp"

namespace soft3d {

class s3dLog : public s3dBase {
public:
    s3dLog() = default;
    ~s3dLog() override = default;

    void SetEngine(Engine* engine) override;
    void On();
    void Output(const char16_t* format, ...);

private:
    s3dIO io;
};

inline void NullLog(const char16_t*, ...)
{
}

#ifdef SOFT3D_LOG_BACKEND_STDIO
#define S3D_LOG_ON(logger) (logger).On()
#define S3D_LOG(logger, ...) (logger).Output(__VA_ARGS__)
#else
#define S3D_LOG_ON(logger) ((void)0)
#define S3D_LOG(logger, ...) soft3d::NullLog(__VA_ARGS__)
#endif

} // namespace soft3d

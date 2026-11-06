#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dIO.hpp"

namespace soft3d {

class s3dLog : public s3dBase {
public:
    s3dLog() = default;
    ~s3dLog() override = default;

    void SetEngine(Engine* engine) override;

private:
    s3dIO io;
};

} // namespace soft3d

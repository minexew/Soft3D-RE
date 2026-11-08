#pragma once

namespace soft3d {

constexpr int S3D_EOF = -1;

enum result {
    S3D_OK = 0,
    S3D_MATH = 1,
    S3D_PARAM = 2,
    S3D_FILE = 3,
    S3D_MEMORY = 4,
    S3D_ACCESS = 5,
    S3D_FORMAT = 6,
    S3D_STATE = 7,
    S3D_UNKNOWN = 8,
};

} // namespace soft3d

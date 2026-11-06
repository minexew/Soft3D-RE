#include "soft3d/s3dBase.hpp"

namespace soft3d {

s3dBase::s3dBase()
    : engine(nullptr)
{
}

void s3dBase::SetEngine(Engine* value)
{
    engine = value;
}

} // namespace soft3d

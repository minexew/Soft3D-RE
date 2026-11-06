#pragma once

namespace soft3d {

class Engine;

class s3dBase {
public:
    s3dBase();
    virtual ~s3dBase() = default;

    virtual void SetEngine(Engine* engine);

protected:
    Engine* engine;
};

} // namespace soft3d

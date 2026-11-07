#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstdint>

namespace soft3d {

class s3dFilePak;

class Resource : public s3dBase {
public:
    Resource();
    ~Resource() override = default;

    void SetEngine(Engine* engine) override;
    int Initialize(const char16_t* fileName, const char16_t* directory,
                   s3dFilePak* filePak);
    const char16_t* GetPath() const;
    const char16_t* GetFileName() const;
    const char16_t* GetDirectory() const;

    virtual int Load(const char16_t* entryName, s3dFilePak* filePak) = 0;

private:
    s3dIO io;
    s3dLog log;
    char16_t path[64];
    char16_t fileName[32];
    char16_t directory[32];
    uint32_t unknown;
};

} // namespace soft3d

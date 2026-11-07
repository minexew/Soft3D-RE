#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <cstdint>

namespace soft3d {

class Resource;

enum ResourceType : uint32_t {
    ResourceAudio = 0,
    ResourceObject = 1,
    ResourceAnimation = 2,
    ResourceTexture = 3,
};

class s3dResManager : public s3dBase {
public:
    s3dResManager();
    ~s3dResManager() override;

    Resource* GetResource(uint32_t type, int cacheLevel, const char16_t* name,
                          const char16_t* directory = nullptr);
    void ReleaseResource(Resource* resource);
    void ClearCache(int cacheLevel);
    s3dFilePak* GetFilePak();

private:
    s3dFilePak filePak;
    struct Cache;
    Cache* caches;
};

s3dFilePak* GetFilePakFromManager(s3dResManager* manager);
void OpenGameResourceArchive(s3dResManager* manager, const char16_t* fileName);

} // namespace soft3d

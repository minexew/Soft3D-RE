#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstdint>

namespace soft3d {

#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
inline constexpr uint32_t pakPathBytes = 32;
#else
inline constexpr uint32_t pakPathBytes = 64;
#endif
inline constexpr uint32_t pakEntryBytes = pakPathBytes + sizeof(uint32_t);

struct PakEntry {
    char name[pakPathBytes];
    uint32_t dataOffset;
};

static_assert(sizeof(PakEntry) == pakEntryBytes);

class s3dFilePak : public s3dBase {
public:
    s3dFilePak();
    ~s3dFilePak() override;

    result Open(const char16_t* fileName, uint32_t loadIntoMemory);
    void Close();
    result Select(const char16_t* fileName);
    result Seek(int32_t offset, int whence);
    int Tell() const;
    result Read(void* destination, int32_t requestedBytes);
    int GetEntryCount() const;

    void SetEngine(Engine* engine) override;

private:
    s3dIO file;
    uint32_t archiveSize;
    uint8_t* allocationBase;
    int16_t entryCount;
    int16_t selectedEntryIndex;
    PakEntry* entryTable;
    uint8_t* selectedEntryStart;
    uint8_t* selectedEntryEnd;
    uint8_t* currentPosition;
    uint32_t memoryResident;
    s3dLog log;
};

} // namespace soft3d

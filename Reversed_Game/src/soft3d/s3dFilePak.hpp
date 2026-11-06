#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstdint>

namespace soft3d {

struct PakEntry {
    char name[64];
    uint32_t dataOffset;
};

static_assert(sizeof(PakEntry) == 0x44);

class s3dFilePak : public s3dBase {
public:
    s3dFilePak();
    ~s3dFilePak() override;

    int Open(const char16_t* fileName, uint32_t loadIntoMemory);
    void Close();
    int Select(const char16_t* fileName);
    int Seek(int32_t offset, int whence);
    int Tell() const;
    int Read(void* destination, int32_t requestedBytes);
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

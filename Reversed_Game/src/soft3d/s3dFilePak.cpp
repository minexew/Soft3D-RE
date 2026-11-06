#include "soft3d/s3dFilePak.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace soft3d {
namespace {

int CompareNames(const char* left, const char* right)
{
    const auto* a = reinterpret_cast<const uint8_t*>(left);
    const auto* b = reinterpret_cast<const uint8_t*>(right);
    for (;;) {
        int difference = static_cast<int>(*a) - static_cast<int>(*b);
        if (difference != 0) {
            return difference;
        }
        if (*a == 0) {
            return 0;
        }
        ++a;
        ++b;
    }
}

uint32_t EntryOffset(const PakEntry* entry)
{
    uint32_t offset;
    std::memcpy(&offset, reinterpret_cast<const uint8_t*>(entry) + 64, sizeof(offset));
    return offset;
}

} // namespace

s3dFilePak::s3dFilePak()
    : allocationBase(nullptr)
    , entryCount(0)
    , selectedEntryIndex(-1)
{
}

s3dFilePak::~s3dFilePak()
{
    Close();
}

int s3dFilePak::Open(const char16_t* fileName, uint32_t loadIntoMemory)
{
    if (allocationBase != nullptr) {
        std::free(allocationBase);
        allocationBase = nullptr;
    }

    std::array<char16_t, 258> resolvedPath;
    char16_t* output = resolvedPath.data();
    do {
        *output++ = *fileName;
    } while (*fileName++ != u'\0');

    if (!file.Open(resolvedPath.data(), u"rb")) {
        return EFP_OPEN;
    }

    file.Seek(0, SEEK_END);
    archiveSize = file.Tell();
    if (archiveSize == EFP_SEEK) {
        return EFP_IOERR;
    }

    memoryResident = loadIntoMemory;
    file.Seek(0, SEEK_SET);
    if (memoryResident == 0) {
        file.Read(&entryCount, 2, 1);
        int32_t tableBytes = static_cast<int32_t>(entryCount) * 0x44;
        allocationBase = static_cast<uint8_t*>(std::malloc(static_cast<size_t>(tableBytes)));
        file.Read(allocationBase, static_cast<uint32_t>(tableBytes));
        entryTable = reinterpret_cast<PakEntry*>(allocationBase);
    } else {
        allocationBase = static_cast<uint8_t*>(std::malloc(archiveSize));
        if (allocationBase == nullptr) {
            file.Close();
            return EFP_NOMEM;
        }
        file.Read(allocationBase, archiveSize);
        file.Close();
        std::memcpy(&entryCount, allocationBase, sizeof(entryCount));
        entryTable = reinterpret_cast<PakEntry*>(allocationBase + 2);
    }

    selectedEntryIndex = -1;
    return EFP_SUCCESS;
}

void s3dFilePak::Close()
{
    if (allocationBase != nullptr) {
        std::free(allocationBase);
        allocationBase = nullptr;
        file.Close();
    }
    selectedEntryIndex = -1;
    entryCount = 0;
}

int s3dFilePak::Select(const char16_t* fileName)
{
    if (allocationBase == nullptr) {
        return EFP_NOENT;
    }

    selectedEntryIndex = -1;
    int32_t low = 0;
    int32_t high = static_cast<int32_t>(entryCount) - 1;
    std::array<char, 64> name;
    char* output = name.data();
    for (; *fileName != u'\0'; ++fileName, ++output) {
        *output = static_cast<char>(*fileName);
    }
    *output = '\0';

    PakEntry* selected = nullptr;
    while (low <= high) {
        int32_t middle = (low + high) >> 1;
        selected = reinterpret_cast<PakEntry*>(reinterpret_cast<uint8_t*>(entryTable)
                                               + middle * 0x44);
        int comparison = CompareNames(selected->name, name.data());
        if (comparison == 0) {
            selectedEntryIndex = static_cast<int16_t>(middle);
            break;
        }
        if (comparison < 0) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }

    if (selectedEntryIndex < 0) {
        return EFP_NOENT;
    }

    selectedEntryStart = allocationBase + EntryOffset(selected);
    if (selectedEntryIndex == entryCount - 1) {
        selectedEntryEnd = allocationBase + archiveSize;
    } else {
        selectedEntryEnd = allocationBase + EntryOffset(selected + 1);
    }
    currentPosition = selectedEntryStart;
    return EFP_SUCCESS;
}

int s3dFilePak::Seek(int32_t offset, int whence)
{
    if (selectedEntryIndex == -1) {
        return EFP_NOENT;
    }

    if (whence == SEEK_SET) {
        currentPosition = selectedEntryStart + offset;
    }
    if (whence == SEEK_CUR) {
        currentPosition = currentPosition + offset;
    }
    if (whence == SEEK_END) {
        currentPosition = selectedEntryEnd + offset;
    }
    if (currentPosition < selectedEntryStart) {
        currentPosition = selectedEntryStart;
    }
    if (selectedEntryEnd < currentPosition) {
        currentPosition = selectedEntryEnd;
    }
    return EFP_SUCCESS;
}

int s3dFilePak::Tell() const
{
    if (selectedEntryIndex == -1) {
        return EFP_SEEK;
    }
    return static_cast<int>(currentPosition - selectedEntryStart);
}

int s3dFilePak::Read(void* destination, int32_t requestedBytes)
{
    if (selectedEntryIndex == -1) {
        return EFP_NOENT;
    }

    int32_t bytesToRead = requestedBytes;
    // Note the +1 here: it appears to be an original off-by-one overread bug,
    // confirmed by 'add r3,r3,#0x1' instruction at mem 0x000a02b0 in
    // disassembly. We intentionally preserve original logic and observable
    // behavior of engine, though.
    if (selectedEntryEnd < currentPosition + requestedBytes) {
        bytesToRead = static_cast<int32_t>(selectedEntryEnd - currentPosition) + 1;
    }
    if (memoryResident == 0) {
        file.Seek(static_cast<uint32_t>(currentPosition - allocationBase), SEEK_SET);
        file.Read(destination, static_cast<uint32_t>(bytesToRead));
    } else {
        std::memcpy(destination, currentPosition, static_cast<size_t>(bytesToRead));
    }
    currentPosition += bytesToRead;
    return EFP_SUCCESS;
}

int s3dFilePak::GetEntryCount() const
{
    return entryCount;
}

void s3dFilePak::SetEngine(Engine* value)
{
    file.SetEngine(value);
    s3dBase::SetEngine(value);
    log.SetEngine(value);
}

} // namespace soft3d

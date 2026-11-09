#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace soft3d;

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

std::u16string Wide(std::string_view text)
{
    std::u16string result;
    for (unsigned char c : text) {
        result.push_back(c);
    }
    return result;
}

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

struct EntrySpec {
    std::string name;
    std::vector<uint8_t> data;
};

std::vector<uint8_t> MakeArchive(const std::vector<EntrySpec>& entries)
{
    uint32_t nextOffset = static_cast<uint32_t>(2 + entries.size() * pakEntryBytes);
    std::vector<uint8_t> bytes;
    Append16(bytes, static_cast<uint16_t>(entries.size()));
    for (const EntrySpec& entry : entries) {
        CHECK(entry.name.size() < pakPathBytes);
        std::array<char, pakPathBytes> name{};
        std::copy(entry.name.begin(), entry.name.end(), name.begin());
        bytes.insert(bytes.end(), name.begin(), name.end());
        Append32(bytes, nextOffset);
        nextOffset += static_cast<uint32_t>(entry.data.size());
    }
    for (const EntrySpec& entry : entries) {
        bytes.insert(bytes.end(), entry.data.begin(), entry.data.end());
    }
    return bytes;
}

class TempArchive {
public:
    explicit TempArchive(const std::vector<uint8_t>& bytes)
        : path(std::filesystem::temp_directory_path() / "soft3d-filepak-tests.spk")
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.close();
        CHECK(output.good());
    }

    ~TempArchive()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

const std::vector<EntrySpec> standardEntries = {
    {".\\a.txt", {0x10, 0x11, 0x12, 0x13}},
    {".\\b.bin", {0x20, 0x21}},
    {".\\last.dat", {0x30, 0x31, 0x32}},
};

void TestBase()
{
    class InspectableBase : public soft3d::s3dBase {
    public:
        using s3dBase::engine;
    };

    InspectableBase base;
    CHECK(base.engine == nullptr);
    auto* marker = reinterpret_cast<soft3d::Engine*>(0x1234);
    base.SetEngine(marker);
    CHECK(base.engine == marker);

    class InspectableFilePak : public s3dFilePak {
    public:
        using s3dBase::engine;
    };
    InspectableFilePak archive;
    archive.SetEngine(marker);
    CHECK(archive.engine == marker);
}

void TestInitialStateAndOpenFailure()
{
    s3dFilePak archive;
    std::array<uint8_t, 4> output{};
    CHECK(archive.GetEntryCount() == 0);
    CHECK(archive.Tell() == S3D_EOF);
    CHECK(archive.Select(u"") == S3D_ACCESS);
    CHECK(archive.Seek(0, SEEK_SET) == S3D_ACCESS);
    CHECK(archive.Read(output.data(), 1) == S3D_ACCESS);

    const std::u16string missing = Wide(
        (std::filesystem::temp_directory_path() / "soft3d-no-such-archive.spk").string());
    CHECK(archive.Open(missing.c_str(), 0) == S3D_FILE);
    CHECK(archive.GetEntryCount() == 0);
}

void TestStandardArchive(uint32_t memoryResident)
{
    TempArchive file(MakeArchive(standardEntries));
    const std::u16string path = file.path.u16string();
    s3dFilePak archive;
    CHECK(archive.Open(path.c_str(), memoryResident) == S3D_OK);
    CHECK(archive.GetEntryCount() == 3);
    CHECK(archive.Tell() == S3D_EOF);

    CHECK(archive.Select(Wide(".\\a.txt").c_str()) == S3D_OK);
    CHECK(archive.Tell() == 0);
    std::array<uint8_t, 4> output{};
    CHECK(archive.Read(output.data(), output.size()) == S3D_OK);
    CHECK(std::equal(output.begin(), output.end(), standardEntries[0].data.begin()));
    CHECK(archive.Tell() == 4);

    CHECK(archive.Seek(-100, SEEK_SET) == S3D_OK);
    CHECK(archive.Tell() == 0);
    CHECK(archive.Seek(100, SEEK_CUR) == S3D_OK);
    CHECK(archive.Tell() == 4);
    CHECK(archive.Seek(-2, SEEK_END) == S3D_OK);
    CHECK(archive.Tell() == 2);
    CHECK(archive.Seek(0, 99) == S3D_OK);
    CHECK(archive.Tell() == 2);
    std::array<uint8_t, 2> tail{};
    CHECK(archive.Read(tail.data(), tail.size()) == S3D_OK);
    CHECK(tail[0] == 0x12 && tail[1] == 0x13);

    CHECK(archive.Select(Wide(".\\b.bin").c_str()) == S3D_OK);
    std::array<uint8_t, 2> second{};
    CHECK(archive.Read(second.data(), second.size()) == S3D_OK);
    CHECK(std::equal(second.begin(), second.end(), standardEntries[1].data.begin()));

    CHECK(archive.Select(Wide(".\\last.dat").c_str()) == S3D_OK);
    std::array<uint8_t, 3> last{};
    CHECK(archive.Read(last.data(), last.size()) == S3D_OK);
    CHECK(std::equal(last.begin(), last.end(), standardEntries[2].data.begin()));

    CHECK(archive.Select(Wide(".\\A.txt").c_str()) == S3D_ACCESS);
    CHECK(archive.Tell() == S3D_EOF);
    CHECK(archive.Select(Wide(".\\missing").c_str()) == S3D_ACCESS);
    CHECK(archive.Seek(0, SEEK_SET) == S3D_ACCESS);

    archive.Close();
    CHECK(archive.GetEntryCount() == 0);
    CHECK(archive.Tell() == S3D_EOF);
    archive.Close();

    CHECK(archive.Open(path.c_str(), memoryResident) == S3D_OK);
    CHECK(archive.GetEntryCount() == 3);
    CHECK(archive.Select(Wide(".\\a.txt").c_str()) == S3D_OK);
}

void TestReadBoundaryBug(uint32_t memoryResident)
{
    const std::vector<EntrySpec> entries = {
        {".\\a", {0x41, 0x42}},
        {".\\b", {0x51, 0x52}},
    };
    TempArchive file(MakeArchive(entries));
    s3dFilePak archive;
    CHECK(archive.Open(file.path.u16string().c_str(), memoryResident) == S3D_OK);
    CHECK(archive.Select(u".\\a") == S3D_OK);

    CHECK(archive.Seek(0, SEEK_END) == S3D_OK);
    std::array<uint8_t, 2> output{0xcc, 0xcc};
    CHECK(archive.Read(output.data(), 1) == S3D_OK);
    CHECK(output[0] == 0x51);
    CHECK(output[1] == 0xcc);
    CHECK(archive.Tell() == 3);

    CHECK(archive.Seek(-1, SEEK_END) == S3D_OK);
    output = {0xcc, 0xcc};
    CHECK(archive.Read(output.data(), 2) == S3D_OK);
    CHECK(output[0] == 0x42);
    CHECK(output[1] == 0x51);
    CHECK(archive.Tell() == 3);
}

#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
std::vector<uint8_t> ReadFileBytes(const std::filesystem::path& path, uint32_t offset,
                                   size_t size)
{
    std::ifstream input(path, std::ios::binary);
    input.seekg(offset);
    std::vector<uint8_t> bytes(size);
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    CHECK(input.good());
    return bytes;
}
#endif

#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
void CheckRealEntry(s3dFilePak& archive, const std::filesystem::path& path,
                    const std::string& name, uint32_t offset, size_t size)
{
    CHECK(archive.Select(Wide(name).c_str()) == S3D_OK);
    const size_t readSize = std::min<size_t>(size, 256);
    std::vector<uint8_t> actual(readSize);
    CHECK(archive.Read(actual.data(), static_cast<int32_t>(actual.size())) == S3D_OK);
    CHECK(actual == ReadFileBytes(path, offset, readSize));
    CHECK(archive.Tell() == static_cast<int>(readSize));
}
#endif

void TestRealArchive(uint32_t memoryResident)
{
#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
    (void)memoryResident;
    std::cout << "SKIP: real archive uses the Chinese layout\n";
    return;
#else
    const std::filesystem::path path = SOFT3D_TEST_ARCHIVE;
    CHECK(std::filesystem::exists(path));
    std::ifstream input(path, std::ios::binary);
    uint16_t count = 0;
    input.read(reinterpret_cast<char*>(&count), sizeof(count));
    CHECK(count == 3322);

    std::array<char, pakEntryBytes> lastRecord{};
    input.seekg(2 + (static_cast<std::streamoff>(count) - 1) * pakEntryBytes);
    input.read(lastRecord.data(), lastRecord.size());
    CHECK(input.good());
    const std::string lastName(lastRecord.data());
    uint32_t lastOffset;
    std::memcpy(&lastOffset, lastRecord.data() + pakPathBytes, sizeof(lastOffset));
    const uint32_t archiveSize = static_cast<uint32_t>(std::filesystem::file_size(path));

    s3dFilePak archive;
    CHECK(archive.Open(path.u16string().c_str(), memoryResident) == S3D_OK);
    CHECK(archive.GetEntryCount() == count);
    CheckRealEntry(archive, path, ".\\audio\\7DaysPiano03.sau", 0x3726a, 512);
    CheckRealEntry(archive, path, lastName, lastOffset, archiveSize - lastOffset);
#endif
}

void TestLayoutConstants()
{
#ifdef SOFT3D_SPK_ENGLISH_LAYOUT
    CHECK(pakPathBytes == 32);
    CHECK(pakEntryBytes == 36);
#else
    CHECK(pakPathBytes == 64);
    CHECK(pakEntryBytes == 68);
#endif
    std::size_t payloadBytes = 0;
    for (const EntrySpec& entry : standardEntries) {
        payloadBytes += entry.data.size();
    }
    CHECK(MakeArchive(standardEntries).size() ==
          2 + standardEntries.size() * pakEntryBytes + payloadBytes);
}

template <typename Test>
void Run(const char* name, Test test)
{
    test();
    std::cout << "PASS: " << name << '\n';
}

} // namespace

int main()
{
    try {
        Run("s3dBase and engine propagation", TestBase);
        Run("initial state and open failure", TestInitialStateAndOpenFailure);
        Run("selected SPK layout", TestLayoutConstants);
        Run("streaming archive", [] { TestStandardArchive(0); });
        Run("memory-resident archive", [] { TestStandardArchive(1); });
        Run("streaming read boundary bug", [] { TestReadBoundaryBug(0); });
        Run("memory-resident read boundary bug", [] { TestReadBoundaryBug(1); });
        Run("real streaming archive", [] { TestRealArchive(0); });
        Run("real memory-resident archive", [] { TestRealArchive(1); });
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

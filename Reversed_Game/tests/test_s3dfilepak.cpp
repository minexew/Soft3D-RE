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

using soft3d::s3dFilePak;

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
    uint32_t nextOffset = static_cast<uint32_t>(2 + entries.size() * 0x44);
    std::vector<uint8_t> bytes;
    Append16(bytes, static_cast<uint16_t>(entries.size()));
    for (const EntrySpec& entry : entries) {
        CHECK(entry.name.size() < 64);
        std::array<char, 64> name{};
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
    CHECK(archive.Tell() == -1);
    CHECK(archive.Select(u"") == 5);
    CHECK(archive.Seek(0, SEEK_SET) == 5);
    CHECK(archive.Read(output.data(), 1) == 5);

    const std::u16string missing = Wide(
        (std::filesystem::temp_directory_path() / "soft3d-no-such-archive.spk").string());
    CHECK(archive.Open(missing.c_str(), 0) == 3);
    CHECK(archive.GetEntryCount() == 0);
}

void TestStandardArchive(uint32_t memoryResident)
{
    TempArchive file(MakeArchive(standardEntries));
    const std::u16string path = file.path.u16string();
    s3dFilePak archive;
    CHECK(archive.Open(path.c_str(), memoryResident) == 0);
    CHECK(archive.GetEntryCount() == 3);
    CHECK(archive.Tell() == -1);

    CHECK(archive.Select(Wide(".\\a.txt").c_str()) == 0);
    CHECK(archive.Tell() == 0);
    std::array<uint8_t, 4> output{};
    CHECK(archive.Read(output.data(), output.size()) == 0);
    CHECK(std::equal(output.begin(), output.end(), standardEntries[0].data.begin()));
    CHECK(archive.Tell() == 4);

    CHECK(archive.Seek(-100, SEEK_SET) == 0);
    CHECK(archive.Tell() == 0);
    CHECK(archive.Seek(100, SEEK_CUR) == 0);
    CHECK(archive.Tell() == 4);
    CHECK(archive.Seek(-2, SEEK_END) == 0);
    CHECK(archive.Tell() == 2);
    CHECK(archive.Seek(0, 99) == 0);
    CHECK(archive.Tell() == 2);
    std::array<uint8_t, 2> tail{};
    CHECK(archive.Read(tail.data(), tail.size()) == 0);
    CHECK(tail[0] == 0x12 && tail[1] == 0x13);

    CHECK(archive.Select(Wide(".\\b.bin").c_str()) == 0);
    std::array<uint8_t, 2> second{};
    CHECK(archive.Read(second.data(), second.size()) == 0);
    CHECK(std::equal(second.begin(), second.end(), standardEntries[1].data.begin()));

    CHECK(archive.Select(Wide(".\\last.dat").c_str()) == 0);
    std::array<uint8_t, 3> last{};
    CHECK(archive.Read(last.data(), last.size()) == 0);
    CHECK(std::equal(last.begin(), last.end(), standardEntries[2].data.begin()));

    CHECK(archive.Select(Wide(".\\A.txt").c_str()) == 5);
    CHECK(archive.Tell() == -1);
    CHECK(archive.Select(Wide(".\\missing").c_str()) == 5);
    CHECK(archive.Seek(0, SEEK_SET) == 5);

    archive.Close();
    CHECK(archive.GetEntryCount() == 0);
    CHECK(archive.Tell() == -1);
    archive.Close();

    CHECK(archive.Open(path.c_str(), memoryResident) == 0);
    CHECK(archive.GetEntryCount() == 3);
    CHECK(archive.Select(Wide(".\\a.txt").c_str()) == 0);
}

void TestReadBoundaryBug(uint32_t memoryResident)
{
    const std::vector<EntrySpec> entries = {
        {".\\a", {0x41, 0x42}},
        {".\\b", {0x51, 0x52}},
    };
    TempArchive file(MakeArchive(entries));
    s3dFilePak archive;
    CHECK(archive.Open(file.path.u16string().c_str(), memoryResident) == 0);
    CHECK(archive.Select(u".\\a") == 0);

    CHECK(archive.Seek(0, SEEK_END) == 0);
    std::array<uint8_t, 2> output{0xcc, 0xcc};
    CHECK(archive.Read(output.data(), 1) == 0);
    CHECK(output[0] == 0x51);
    CHECK(output[1] == 0xcc);
    CHECK(archive.Tell() == 3);

    CHECK(archive.Seek(-1, SEEK_END) == 0);
    output = {0xcc, 0xcc};
    CHECK(archive.Read(output.data(), 2) == 0);
    CHECK(output[0] == 0x42);
    CHECK(output[1] == 0x51);
    CHECK(archive.Tell() == 3);
}

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

void CheckRealEntry(s3dFilePak& archive, const std::filesystem::path& path,
                    const std::string& name, uint32_t offset, size_t size)
{
    CHECK(archive.Select(Wide(name).c_str()) == 0);
    const size_t readSize = std::min<size_t>(size, 256);
    std::vector<uint8_t> actual(readSize);
    CHECK(archive.Read(actual.data(), static_cast<int32_t>(actual.size())) == 0);
    CHECK(actual == ReadFileBytes(path, offset, readSize));
    CHECK(archive.Tell() == static_cast<int>(readSize));
}

void TestRealArchive(uint32_t memoryResident)
{
    const std::filesystem::path path = SOFT3D_TEST_ARCHIVE;
    CHECK(std::filesystem::exists(path));
    std::ifstream input(path, std::ios::binary);
    uint16_t count = 0;
    input.read(reinterpret_cast<char*>(&count), sizeof(count));
    CHECK(count == 3322);

    std::array<char, 68> lastRecord{};
    input.seekg(2 + (static_cast<std::streamoff>(count) - 1) * 0x44);
    input.read(lastRecord.data(), lastRecord.size());
    CHECK(input.good());
    const std::string lastName(lastRecord.data());
    uint32_t lastOffset;
    std::memcpy(&lastOffset, lastRecord.data() + 64, sizeof(lastOffset));
    const uint32_t archiveSize = static_cast<uint32_t>(std::filesystem::file_size(path));

    s3dFilePak archive;
    CHECK(archive.Open(path.u16string().c_str(), memoryResident) == 0);
    CHECK(archive.GetEntryCount() == count);
    CheckRealEntry(archive, path, ".\\audio\\7DaysPiano03.sau", 0x3726a, 512);
    CheckRealEntry(archive, path, lastName, lastOffset, archiveSize - lastOffset);
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

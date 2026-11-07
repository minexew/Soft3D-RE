#include "soft3d/Texture.hpp"
#include "soft3d/s3dFilePak.hpp"
#include "soft3d/s3dLog.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

class TempLogDirectory {
public:
    TempLogDirectory()
        : previous(std::filesystem::current_path())
        , path(std::filesystem::temp_directory_path() / "soft3d-log-tests")
    {
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
        std::filesystem::current_path(path);
    }

    ~TempLogDirectory()
    {
        std::filesystem::current_path(previous);
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

private:
    std::filesystem::path previous;
    std::filesystem::path path;
};

#ifdef SOFT3D_LOG_BACKEND_STDIO
std::u16string ReadLog()
{
    std::ifstream input("log.log", std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
    CHECK(bytes.size() % 2 == 0);
    std::u16string text;
    for (size_t i = 0; i < bytes.size(); i += 2) {
        text += static_cast<char16_t>(bytes[i] | (static_cast<uint16_t>(bytes[i + 1]) << 8));
    }
    return text;
}
#endif

} // namespace

int main()
{
    try {
        TempLogDirectory directory;
        soft3d::s3dLog log;
        soft3d::s3dFilePak filePak;
        soft3d::Texture texture;

#ifdef SOFT3D_LOG_BACKEND_STDIO
        S3D_LOG_ON(log);
        S3D_LOG(log, u"value=%d text=%s narrow=%S hex=%05x\n",
                -7, u"wide", "ascii", 0x2a);
        S3D_LOG(log, u"append=%05u|%4s|%-4S|%c\n", 42, u"x", "yz", u'!');
        CHECK(texture.LoadStx(u"missing.stx", &filePak) == 3);

        const std::u16string expected =
            u"===== Log On =====\n"
            u"value=-7 text=wide narrow=ascii hex=   2A\n"
            u"append=42|   x|yz|!\n"
            u"Texture::CreateFromPak missing.stx: can't find!\n";
        const std::u16string actual = ReadLog();
        if (actual != expected) {
            std::cerr << "actual log: " << std::string(actual.begin(), actual.end()) << '\n';
        }
        CHECK(actual == expected);
#else
        S3D_LOG_ON(log);
        S3D_LOG(log, u"ignored %d %s\n", 7, u"message");
        soft3d::NullLog(u"also ignored\n");
        CHECK(texture.LoadStx(u"missing.stx", &filePak) == 3);
        CHECK(!std::filesystem::exists("log.log"));
#endif
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

#include "soft3d/s3dIO.hpp"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
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

class TempFile {
public:
    explicit TempFile(const char* name)
        : path(std::filesystem::temp_directory_path() / name)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    ~TempFile()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::u16string WidePath() const
    {
        std::string narrow = path.string();
        return std::u16string(narrow.begin(), narrow.end());
    }

    std::filesystem::path path;
};

std::vector<uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)),
                                std::istreambuf_iterator<char>());
}

std::u16string ReadWide(const std::filesystem::path& path)
{
    std::vector<uint8_t> bytes = ReadBytes(path);
    CHECK(bytes.size() % 2 == 0);
    std::u16string text;
    for (size_t i = 0; i < bytes.size(); i += 2) {
        text += static_cast<char16_t>(bytes[i] | (static_cast<uint16_t>(bytes[i + 1]) << 8));
    }
    return text;
}

int WriteV(soft3d::s3dIO& io, const char16_t* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = io.VPrintf(format, arguments);
    va_end(arguments);
    return result;
}

void CheckFormat(const std::u16string& expected, const char16_t* format, ...)
{
    char16_t output[258];
    va_list arguments;
    va_start(arguments, format);
    int length = soft3d::FormatWideString(output, format, arguments);
    va_end(arguments);
    CHECK(output == expected);
    CHECK(length == static_cast<int>(expected.size()));
}

} // namespace

int main()
{
    try {
        TempFile file("soft3d-io-tests.bin");
        TempFile reopenedFile("soft3d-io-reopened-tests.bin");
        soft3d::s3dIO io;

        CHECK(io.Open(file.WidePath().c_str(), u"w"));
        const char bytes[] = {'A', 'B', 'C', 'D'};
        CHECK(io.Write(bytes, 1, 4) == 0);
        io.Close();
        CHECK(ReadBytes(file.path) == std::vector<uint8_t>({'A', 'B', 'C', 'D'}));

        CHECK(io.Open(file.WidePath().c_str(), u"w"));
        CHECK(io.Printf(u"%s|%S|%d|%i|%u|%x|%X|%o|%c",
                        u"wide", "narrow", -12, 34, 56, 0x2a, 0x2a, 8, u'!') == 0);
        io.Close();
        CHECK(ReadWide(file.path) == u"wide|narrow|-12|34|56|2A|2A|10|!");

        CHECK(io.Open(file.WidePath().c_str(), u"a"));
        CHECK(WriteV(io, u"|%05d|%#x|%#X|%#o", 12, 42, 42, 8) == 0);
        CHECK(WriteV(io, u"") == -1);
        io.Close();

        CHECK(io.Open(file.WidePath().c_str(), u"r"));
        io.Seek(0, SEEK_END);
        CHECK(io.Tell() == 2 * 52);
        io.Seek(0, SEEK_SET);
        char16_t first[5] = {};
        CHECK(io.Read(first, 2, 4) == 8);
        CHECK(first[0] == u'w' && first[3] == u'e');
        CHECK(io.Reopen(reopenedFile.WidePath().c_str(), u"w"));
        CHECK(io.Write(u"ok", 2, 2) == 0);
        io.Close();
        CHECK(ReadWide(reopenedFile.path) == u"ok");

        CheckFormat(u"00012", u"%05d", 12);
        CheckFormat(u"+12", u"%+d", 12);
        CheckFormat(u"0x2A|0X2A|010", u"%#x|%#X|%#o", 42, 42, 8);
        CheckFormat(u"", u"%x", 0);
        CheckFormat(u"   x", u"%4s", u"x");
        CheckFormat(u"wi", u"%2s", u"wide");
        CheckFormat(u"  x", u"%.3s", u"x");
        CheckFormat(u"yz", u"%-4S", "yz");
        CheckFormat(u"7", u"%4u", 7);
        CheckFormat(u"%", u"%%");
        CheckFormat(u"", u"%q");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

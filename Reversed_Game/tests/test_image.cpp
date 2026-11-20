#include "soft3d/s3dImageLoader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

#define CHECK(condition) Check((condition), __LINE__)

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

std::vector<uint8_t> MakeTga(uint8_t type, uint8_t depth, uint16_t width, uint16_t height,
                             const std::vector<uint8_t>& payload, uint8_t idLength = 0)
{
    std::vector<uint8_t> bytes(18, 0);
    bytes[0] = idLength;
    bytes[2] = type;
    bytes[12] = static_cast<uint8_t>(width);
    bytes[13] = static_cast<uint8_t>(width >> 8);
    bytes[14] = static_cast<uint8_t>(height);
    bytes[15] = static_cast<uint8_t>(height >> 8);
    bytes[16] = depth;
    bytes.insert(bytes.end(), static_cast<size_t>(idLength), 0x55);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

void TestUncompressed()
{
    soft3d::s3dImageLoader loader;
    soft3d::ImageInfo image;
    const std::vector<uint8_t> bytes = MakeTga(2, 24, 2, 2,
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, 2);
    CHECK(loader.ParseTga(bytes.data(), bytes.size(), image) == soft3d::S3D_OK);
    CHECK(image.palette == nullptr && image.bitDepth == 3);
    CHECK(image.width == 2 && image.height == 2);
    const std::array<uint8_t, 12> expected = {9, 8, 7, 12, 11, 10, 3, 2, 1, 6, 5, 4};
    CHECK(std::equal(expected.begin(), expected.end(), image.data));
}

void TestRleAcrossRows()
{
    soft3d::s3dImageLoader loader;
    soft3d::ImageInfo image;
    const std::vector<uint8_t> bytes = MakeTga(10, 32, 2, 2,
        {0x83, 1, 2, 3, 4});
    CHECK(loader.ParseTga(bytes.data(), bytes.size(), image) == soft3d::S3D_OK);
    const std::array<uint8_t, 16> expected = {
        3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4,
    };
    CHECK(std::equal(expected.begin(), expected.end(), image.data));
    CHECK(image.bitDepth == 4);

    const std::vector<uint8_t> mixedBytes = MakeTga(10, 32, 3, 1,
        {0x01, 1, 2, 3, 4, 5, 6, 7, 8, 0x80, 9, 10, 11, 12});
    CHECK(loader.ParseTga(mixedBytes.data(), mixedBytes.size(), image) == soft3d::S3D_OK);
    const std::array<uint8_t, 12> mixedExpected = {
        3, 2, 1, 4, 7, 6, 5, 8, 11, 10, 9, 12,
    };
    CHECK(std::equal(mixedExpected.begin(), mixedExpected.end(), image.data));
}

void TestFileAndErrors()
{
    const std::filesystem::path path = std::filesystem::temp_directory_path() /
        "soft3d-image-test.tga";
    const std::vector<uint8_t> bytes = MakeTga(2, 24, 1, 1, {0x11, 0x22, 0x33});
    {
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    soft3d::s3dImageLoader loader;
    soft3d::ImageInfo image;
    CHECK(loader.LoadTGA(path.u16string().c_str(), image) == soft3d::S3D_OK);
    CHECK(image.data[0] == 0x33 && image.data[1] == 0x22 && image.data[2] == 0x11);
    image.Free();
    CHECK(image.data == nullptr && image.width == 0);
    CHECK(loader.LoadTGA(u".\\file-that-does-not-exist.tga", image) == soft3d::S3D_FILE);
    std::filesystem::remove(path);

    const std::vector<uint8_t> unsupported = MakeTga(1, 24, 1, 1, {0, 0, 0});
    CHECK(loader.ParseTga(unsupported.data(), unsupported.size(), image) == soft3d::S3D_FORMAT);
    CHECK(loader.ParseTga(nullptr, 0, image) == soft3d::S3D_FORMAT);
    const std::vector<uint8_t> truncated = MakeTga(10, 24, 2, 1, {0x81, 1, 2});
    CHECK(loader.ParseTga(truncated.data(), truncated.size(), image) == soft3d::S3D_FORMAT);
    const std::vector<uint8_t> overflow = MakeTga(10, 24, 1, 1, {0x81, 1, 2, 3});
    CHECK(loader.ParseTga(overflow.data(), overflow.size(), image) == soft3d::S3D_FORMAT);
}

} // namespace

int main()
{
    try {
        TestUncompressed();
        TestRleAcrossRows();
        TestFileAndErrors();
        std::cout << "Image loader tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

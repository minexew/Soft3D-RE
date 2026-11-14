#include "soft3d/Font.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
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

void TestRealFonts(soft3d::s3dFilePak& archive)
{
    soft3d::StringManager base;
    CHECK(base.LoadBaseFont(2, &archive) == soft3d::S3D_OK);
    CHECK(base.GetFont()->GetMaxHeight() == 15);
    CHECK(base.GetFont()->GetBaseCharCnt() == 64);
    CHECK(base.GetFont()->GetLocalCharCnt() == 0);
    CHECK(base.GetFont()->GetCharWidth(4) == 7);
    CHECK(base.GetFont()->GetCharHeight(4) == 15);
    CHECK(base.GetFont()->GetCharWidth(3) == 7);
    CHECK(base.GetFont()->GetCharWidth(30) == 7);

    soft3d::StringManager localized;
    CHECK(localized.LoadBaseFont(2, &archive) == soft3d::S3D_OK);
    CHECK(localized.LoadSbnFont(u".\\ui\\system.sbn", &archive) == soft3d::S3D_OK);
    CHECK(localized.GetFont()->GetLocalCharCnt() == 18);
    CHECK(localized.GetCount() == 581);
    const soft3d::VisString* english = localized.GetString(1);
    CHECK(english != nullptr && english->GetLength() == 7);
    const std::array<uint32_t, 7> englishCodes = {32, 18, 17, 9, 12, 21, 16};
    CHECK(std::equal(englishCodes.begin(), englishCodes.end(), english->GetBuffer()));
    CHECK(localized.GetString(0) == nullptr);
    CHECK(localized.GetString(30) != nullptr);
    CHECK(localized.GetString(30)->GetLength() == 0);
    CHECK(localized.GetString(582) == nullptr);
}

void TestTextAndDrawing(soft3d::StringManager& manager)
{
    soft3d::VisString text(u"aZ09 -?");
    const std::array<uint32_t, 7> expected = {4, 55, 56, 65, 3, 66, 67};
    CHECK(text.GetLength() == expected.size());
    CHECK(std::equal(expected.begin(), expected.end(), text.GetBuffer()));
    CHECK(text.GetNextBlankIndex(0) == 4);
    CHECK(text.GetNextBlankIndex(4) == 1);
    text.Append(-12);
    CHECK(text.GetBuffer()[7] == 67);
    CHECK(text.GetBuffer()[8] == 57);
    CHECK(text.GetBuffer()[9] == 58);
    text.Clear();
    CHECK(text.GetLength() == 0);

    std::unique_ptr<soft3d::VisString> glyph(manager.NewString(u"a"));
    const uint32_t width = manager.GetFont()->GetCharWidth(4);
    const uint32_t height = manager.GetFont()->GetCharHeight(4);
    CHECK(manager.CountTextWidth(glyph.get()) == width);
    CHECK(manager.GetTextHeight() == 15);
    std::vector<uint32_t> target(32 * 32, 0);
    manager.SetRenderProperties(target.data(), 32, 32);
    manager.DrawString(glyph.get(), 0xf800, 0, soft3d::Point{2, 3});
    const uint8_t* bitmap = manager.GetFont()->GetFontRes(4);
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint32_t bit = y * width + x;
            const uint32_t expectedPixel = ((bitmap[bit >> 3] >> (bit & 7)) & 1) != 0
                ? 0x7fff0000U | 0xf800U
                : 0;
            CHECK(target[(y + 3) * 32 + x + 2] == expectedPixel);
        }
    }

    std::fill(target.begin(), target.end(), 0);
    manager.DrawString(glyph.get(), 0x07e0, 2, soft3d::Point{10, 10});
    CHECK(std::any_of(target.begin(), target.end(), [](uint32_t pixel) { return pixel != 0; }));
    for (int y = 0; y < 32; ++y) {
        for (int x = 10; x < 32; ++x) {
            CHECK(target[y * 32 + x] == 0);
        }
    }

    const auto checkGlyph = [&manager](const std::vector<uint32_t>& pixels, uint32_t code,
                                       uint16_t color, int32_t left, int32_t top) {
        const uint32_t glyphWidth = manager.GetFont()->GetCharWidth(code);
        const uint32_t glyphHeight = manager.GetFont()->GetCharHeight(code);
        const uint8_t* glyphData = manager.GetFont()->GetFontRes(code);
        for (uint32_t y = 0; y < glyphHeight; ++y) {
            for (uint32_t x = 0; x < glyphWidth; ++x) {
                const uint32_t bit = y * glyphWidth + x;
                const uint32_t expectedPixel = ((glyphData[bit >> 3] >> (bit & 7)) & 1) != 0
                    ? 0x7fff0000U | color
                    : 0;
                CHECK(pixels[(top + static_cast<int32_t>(y)) * 32 + left + x] == expectedPixel);
            }
        }
    };

    soft3d::VisString wrapped(u"a Z");
    CHECK(manager.CountTextLines(&wrapped, 0, 14) == 2);
    const soft3d::Rect rect{0, 0, 14, 40};
    std::fill(target.begin(), target.end(), 0);
    manager.DrawString(&wrapped, 0xf800, rect);
    checkGlyph(target, 4, 0xf800, 0, 0);
    checkGlyph(target, 55, 0xf800, 0, 15);

    std::fill(target.begin(), target.end(), 0);
    manager.DrawString(&wrapped, 0x07e0, 2, rect);
    checkGlyph(target, 4, 0x07e0, 0, 0);
    checkGlyph(target, 55, 0x07e0, 7, 15);

    std::fill(target.begin(), target.end(), 0);
    manager.DrawString(&wrapped, 0x001f, 4, rect);
    checkGlyph(target, 55, 0x001f, 4, 15);

    const soft3d::Rect lineRect{0, 0, 70, 40};
    std::vector<uint32_t> ranged(32 * 32, 0);
    manager.SetRenderProperties(ranged.data(), 32, 32);
    manager.DrawStringRange(1, 0xf800, 0, 1, lineRect);
    std::fill(target.begin(), target.end(), 0);
    manager.SetRenderProperties(target.data(), 32, 32);
    manager.DrawString(manager.GetString(1), 0xf800, lineRect);
    CHECK(ranged == target);

    std::fill(ranged.begin(), ranged.end(), 0);
    manager.SetRenderProperties(ranged.data(), 32, 32);
    manager.DrawStringRange(1, 0xf800, 0, 0, lineRect);
    CHECK(std::all_of(ranged.begin(), ranged.end(), [](uint32_t pixel) { return pixel == 0; }));
}

} // namespace

int main()
{
    try {
        soft3d::s3dFilePak archive;
        CHECK(archive.Open(SOFT3D_TEST_ARCHIVE, 0) == soft3d::S3D_OK);
        TestRealFonts(archive);
        soft3d::StringManager manager;
        CHECK(manager.LoadBaseFont(2, &archive) == soft3d::S3D_OK);
        CHECK(manager.LoadSbnFont(u".\\ui\\system.sbn", &archive) == soft3d::S3D_OK);
        TestTextAndDrawing(manager);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "font and string tests passed\n";
    return 0;
}

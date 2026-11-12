#pragma once

#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dIO.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace soft3d {

class s3dFilePak;

class VisString {
public:
    VisString() = default;
    explicit VisString(uint32_t length);
    explicit VisString(const char16_t* text);

    void Clear();
    uint32_t GetLength() const;
    const uint32_t* GetBuffer() const;
    void Append(const char16_t* text);
    void Append(int32_t number);
    int32_t GetNextBlankIndex(int32_t start) const;

private:
    void SetString(const uint32_t* codes, uint32_t length);

    std::vector<uint32_t> codes;

    friend class StringManager;
};

class Font : public s3dBase {
public:
    explicit Font(uint32_t language = 0);

    result ParseBaseFont(const void* data, size_t size);
    result ParseLocalFont(const void* data, size_t size);
    result ParseLocalFont(const uint8_t*& input, size_t& remaining);
    void ClearLocal();

    uint32_t GetMaxHeight() const;
    uint16_t GetBaseCharCnt() const;
    uint16_t GetLocalCharCnt() const;
    uint32_t GetCharHeight(uint32_t code) const;
    uint32_t GetCharWidth(uint32_t code) const;
    const uint8_t* GetFontRes(uint32_t code) const;

private:
    struct Glyph {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> data;
    };

    result ParseGlyphRecords(const uint8_t*& input, size_t& remaining,
                             uint32_t count, bool base);
    const Glyph* GetGlyph(uint32_t code) const;

    s3dIO io;
    uint32_t maxHeight = 0;
    uint16_t baseCount = 0;
    std::vector<Glyph> baseGlyphs;
    uint16_t localCount = 0;
    std::vector<Glyph> localGlyphs;
    uint32_t language;
};

struct Point {
    int32_t x = 0;
    int32_t y = 0;
};

struct Rect {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;
};

class StringManager : public s3dBase {
public:
    StringManager() = default;

    result LoadBaseFont(uint32_t language, s3dFilePak* filePak);
    result LoadSbnFont(const char16_t* entryName, s3dFilePak* filePak);
    void Clear();

    void SetRenderProperties(uint32_t* target, int32_t width, int32_t height);
    uint32_t GetCount() const;
    VisString* GetString(uint32_t index) const;
    VisString* NewString(const char16_t* text) const;
    VisString* NewString(uint32_t length) const;
    uint32_t CountTextWidth(const VisString* text, uint32_t start = 0) const;
    uint32_t GetTextHeight() const;
    void DrawString(uint32_t index, uint16_t color, uint32_t anchor, Point point) const;
    void DrawString(const VisString* text, uint16_t color, uint32_t anchor, Point point) const;
    Font* GetFont() const;

private:
    void DrawChar(uint32_t code, uint16_t color, Point point) const;
    void ApplyAnchor(uint32_t anchor, const VisString* text, Point& point) const;

    s3dIO io;
    std::vector<std::unique_ptr<VisString>> strings;
    std::unique_ptr<Font> font;
    uint32_t* target = nullptr;
    int32_t targetWidth = 0;
    int32_t targetHeight = 0;
    uint32_t language = 0;
};

void SetFontRenderProperties(StringManager* manager, uint32_t* target,
                             int32_t width, int32_t height);

} // namespace soft3d

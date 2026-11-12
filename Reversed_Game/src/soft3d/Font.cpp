#include "soft3d/Font.hpp"

#include "soft3d/s3dFilePak.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace soft3d {
namespace {

bool Read32(const uint8_t*& input, size_t& remaining, uint32_t& value)
{
    if (remaining < 4) {
        return false;
    }
    std::memcpy(&value, input, sizeof(value));
    input += 4;
    remaining -= 4;
    return true;
}

uint16_t PackColor(uint32_t rgb)
{
    return static_cast<uint16_t>(((rgb >> 16) & 0xf8) << 8) |
           static_cast<uint16_t>(((rgb >> 8) & 0xfc) << 3) |
           static_cast<uint16_t>((rgb & 0xff) >> 3);
}

result ReadArchiveEntry(s3dFilePak* filePak, const char16_t* entryName,
                        std::vector<uint8_t>& data)
{
    if (filePak == nullptr || filePak->Select(entryName) != S3D_OK) {
        return S3D_FILE;
    }
    filePak->Seek(0, SEEK_END);
    const int bytes = filePak->Tell();
    if (bytes == S3D_EOF) {
        return S3D_FILE;
    }
    data.resize(static_cast<size_t>(bytes));
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data.data(), bytes);
    return S3D_OK;
}

} // namespace

VisString::VisString(uint32_t length)
    : codes(length, 0)
{
}

VisString::VisString(const char16_t* text)
{
    Append(text);
}

void VisString::Clear()
{
    codes.clear();
}

uint32_t VisString::GetLength() const
{
    return static_cast<uint32_t>(codes.size());
}

const uint32_t* VisString::GetBuffer() const
{
    return codes.data();
}

void VisString::Append(const char16_t* text)
{
    if (text == nullptr) {
        return;
    }
    while (*text != u'\0') {
        const uint16_t character = static_cast<uint16_t>(*text++);
        uint32_t code = 3;
        if (character >= u'a' && character <= u'z') {
            code = character - 0x5d;
        } else if (character >= u'A' && character <= u'Z') {
            code = character - 0x23;
        } else if (character >= u'0' && character <= u'9') {
            code = character + 8;
        } else if (character == u'-') {
            code = 0x42;
        } else if (character >= u'.') {
            code = 0x43;
        }
        codes.push_back(code);
    }
}

void VisString::Append(int32_t number)
{
    uint32_t magnitude = number < 0 ? static_cast<uint32_t>(-static_cast<int64_t>(number))
                                    : static_cast<uint32_t>(number);
    if (number < 0) {
        codes.push_back(0x43);
    }
    std::string digits = std::to_string(magnitude);
    for (char digit : digits) {
        codes.push_back(static_cast<uint32_t>(digit - '0') + 0x38);
    }
}

int32_t VisString::GetNextBlankIndex(int32_t start) const
{
    if (start < 0 || static_cast<size_t>(start) >= codes.size()) {
        return 0;
    }
    for (size_t index = static_cast<size_t>(start); index < codes.size(); ++index) {
        if (codes[index] == 3 || codes[index] == 0) {
            return index == static_cast<size_t>(start)
                ? 1
                : static_cast<int32_t>(index - static_cast<size_t>(start));
        }
    }
    return static_cast<int32_t>(codes.size() - static_cast<size_t>(start));
}

void VisString::SetString(const uint32_t* input, uint32_t length)
{
    codes.assign(input, input + length);
}

Font::Font(uint32_t value)
    : language(value)
{
}

result Font::ParseGlyphRecords(const uint8_t*& input, size_t& remaining,
                               uint32_t count, bool base)
{
    if (count > 0xffff) {
        return S3D_FORMAT;
    }
    std::vector<Glyph> glyphs(count);
    for (uint32_t index = 0; index < count; ++index) {
        uint32_t dataSize;
        if (!Read32(input, remaining, glyphs[index].width) ||
            !Read32(input, remaining, glyphs[index].height) ||
            !Read32(input, remaining, dataSize) || remaining < dataSize) {
            return S3D_FORMAT;
        }
        glyphs[index].data.assign(input, input + dataSize);
        input += dataSize;
        remaining -= dataSize;
        const size_t originalAdvance = dataSize & ~size_t(3);
        if (originalAdvance < dataSize) {
            if (remaining < dataSize - originalAdvance) {
                return S3D_FORMAT;
            }
            input -= dataSize - originalAdvance;
            remaining += dataSize - originalAdvance;
        }
    }
    if (base) {
        baseCount = static_cast<uint16_t>(count);
        baseGlyphs = std::move(glyphs);
    } else {
        localCount = static_cast<uint16_t>(count);
        localGlyphs = std::move(glyphs);
    }
    return S3D_OK;
}

result Font::ParseBaseFont(const void* data, size_t size)
{
    const uint8_t* input = static_cast<const uint8_t*>(data);
    size_t remaining = size;
    uint32_t count;
    if (!Read32(input, remaining, maxHeight) || !Read32(input, remaining, count)) {
        return S3D_FORMAT;
    }
    if (language != 2) {
        maxHeight += 2;
    }
    return ParseGlyphRecords(input, remaining, count, true);
}

result Font::ParseLocalFont(const void* data, size_t size)
{
    const uint8_t* input = static_cast<const uint8_t*>(data);
    size_t remaining = size;
    return ParseLocalFont(input, remaining);
}

result Font::ParseLocalFont(const uint8_t*& input, size_t& remaining)
{
    uint32_t count;
    if (!Read32(input, remaining, count)) {
        return S3D_FORMAT;
    }
    return ParseGlyphRecords(input, remaining, count, false);
}

void Font::ClearLocal()
{
    localGlyphs.clear();
    localCount = 0;
}

uint32_t Font::GetMaxHeight() const { return maxHeight; }
uint16_t Font::GetBaseCharCnt() const { return baseCount; }
uint16_t Font::GetLocalCharCnt() const { return localCount; }

const Font::Glyph* Font::GetGlyph(uint32_t code) const
{
    if (code < 4) {
        return nullptr;
    }
    const uint32_t index = code - 4;
    if (index < baseCount) {
        return &baseGlyphs[index];
    }
    const uint32_t localIndex = index - baseCount;
    return localIndex < localCount ? &localGlyphs[localIndex] : nullptr;
}

uint32_t Font::GetCharHeight(uint32_t code) const
{
    if (code == 3) {
        return baseGlyphs.empty() ? 0 : baseGlyphs.front().height;
    }
    const Glyph* glyph = GetGlyph(code);
    return glyph != nullptr ? glyph->height : 0;
}

uint32_t Font::GetCharWidth(uint32_t code) const
{
    if (code == 3) {
        return baseGlyphs.size() > 3 ? baseGlyphs[3].width : 0;
    }
    const Glyph* glyph = GetGlyph(code);
    return glyph != nullptr ? glyph->width : 0;
}

const uint8_t* Font::GetFontRes(uint32_t code) const
{
    const Glyph* glyph = GetGlyph(code);
    return glyph != nullptr ? glyph->data.data() : nullptr;
}

result StringManager::LoadBaseFont(uint32_t value, s3dFilePak* filePak)
{
    language = value;
    font = std::make_unique<Font>(language);
    font->SetEngine(engine);
    std::vector<uint8_t> data;
    const result status = ReadArchiveEntry(
        filePak, language == 2 ? u".\\ui\\basefont.sbn" : u".\\uien\\basefont.sbn", data);
    return status == S3D_OK ? font->ParseBaseFont(data.data(), data.size()) : status;
}

result StringManager::LoadSbnFont(const char16_t* entryName, s3dFilePak* filePak)
{
    if (font == nullptr) {
        return S3D_STATE;
    }
    std::vector<uint8_t> data;
    result status = ReadArchiveEntry(filePak, entryName, data);
    if (status != S3D_OK) {
        return status;
    }
    const uint8_t* input = data.data();
    size_t remaining = data.size();
    status = font->ParseLocalFont(input, remaining);
    if (status != S3D_OK) {
        return status;
    }
    uint32_t count;
    if (!Read32(input, remaining, count)) {
        return S3D_FORMAT;
    }
    strings.clear();
    strings.reserve(count);
    for (uint32_t index = 0; index < count; ++index) {
        uint32_t length;
        if (!Read32(input, remaining, length) || remaining / 4 < length) {
            return S3D_FORMAT;
        }
        auto text = std::make_unique<VisString>();
        text->SetString(reinterpret_cast<const uint32_t*>(input), length);
        strings.push_back(std::move(text));
        input += static_cast<size_t>(length) * 4;
        remaining -= static_cast<size_t>(length) * 4;
    }
    return S3D_OK;
}

void StringManager::Clear()
{
    strings.clear();
    if (font != nullptr) {
        font->ClearLocal();
    }
}

void StringManager::SetRenderProperties(uint32_t* renderTarget, int32_t width, int32_t height)
{
    target = renderTarget;
    targetWidth = width;
    targetHeight = height;
}

uint32_t StringManager::GetCount() const
{
    return static_cast<uint32_t>(strings.size());
}

VisString* StringManager::GetString(uint32_t index) const
{
    if (index == 0 || index > strings.size()) {
        return nullptr;
    }
    return strings[index - 1].get();
}

VisString* StringManager::NewString(const char16_t* text) const
{
    return new VisString(text);
}

VisString* StringManager::NewString(uint32_t length) const
{
    return new VisString(length);
}

uint32_t StringManager::CountTextWidth(const VisString* text, uint32_t start) const
{
    if (font == nullptr || text == nullptr || start >= text->GetLength()) {
        return 0;
    }
    const uint32_t* codes = text->GetBuffer();
    uint32_t width = 0;
    for (uint32_t index = start; index < text->GetLength(); ++index) {
        if (codes[index] == 1) {
            ++index;
        } else {
            width += font->GetCharWidth(codes[index]);
        }
    }
    return width;
}

uint32_t StringManager::GetTextHeight() const
{
    return font != nullptr ? font->GetMaxHeight() : 0;
}

void StringManager::ApplyAnchor(uint32_t anchor, const VisString* text, Point& point) const
{
    if (text == nullptr) {
        return;
    }
    uint32_t start = 0;
    const uint32_t* codes = text->GetBuffer();
    while (start < text->GetLength() && codes[start] == 3) {
        ++start;
    }
    if ((anchor & 4) != 0) {
        point.x -= static_cast<int32_t>(CountTextWidth(text, start) >> 1);
    } else if ((anchor & 2) != 0) {
        point.x -= static_cast<int32_t>(CountTextWidth(text, start));
    }
    if ((anchor & 0x40) != 0) {
        point.y -= static_cast<int32_t>(GetTextHeight() >> 1);
    } else if ((anchor & 0x20) != 0) {
        point.y -= static_cast<int32_t>(GetTextHeight());
    }
}

void StringManager::DrawString(uint32_t index, uint16_t color, uint32_t anchor, Point point) const
{
    DrawString(GetString(index), color, anchor, point);
}

void StringManager::DrawString(const VisString* text, uint16_t color, uint32_t anchor,
                               Point point) const
{
    if (font == nullptr || text == nullptr || text->GetLength() == 0) {
        return;
    }
    ApplyAnchor(anchor, text, point);
    const uint16_t originalColor = color;
    const uint32_t* codes = text->GetBuffer();
    for (uint32_t index = 0; index < text->GetLength(); ++index) {
        const uint32_t code = codes[index];
        if (code == 1 && index + 1 < text->GetLength()) {
            color = PackColor(codes[++index]);
        } else if (code == 2) {
            color = originalColor;
        } else {
            DrawChar(code, color, point);
            point.x += static_cast<int32_t>(font->GetCharWidth(code));
            if (point.x > targetWidth) {
                return;
            }
        }
    }
}

void StringManager::DrawChar(uint32_t code, uint16_t color, Point point) const
{
    if (code <= 3 || font == nullptr || target == nullptr) {
        return;
    }
    const uint8_t* bitmap = font->GetFontRes(code);
    if (bitmap == nullptr) {
        return;
    }
    const int32_t width = static_cast<int32_t>(font->GetCharWidth(code));
    const int32_t height = static_cast<int32_t>(font->GetCharHeight(code));
    for (int32_t y = 0; y < height; ++y) {
        const int32_t targetY = point.y + y;
        if (targetY < 0 || targetY >= targetHeight) {
            continue;
        }
        for (int32_t x = 0; x < width; ++x) {
            const int32_t targetX = point.x + x;
            if (targetX < 0 || targetX >= targetWidth) {
                continue;
            }
            const uint32_t bit = static_cast<uint32_t>(y * width + x);
            if (((bitmap[bit >> 3] >> (bit & 7)) & 1) != 0) {
                target[static_cast<size_t>(targetY) * targetWidth + targetX] =
                    static_cast<uint32_t>(color) | 0x7fff0000;
            }
        }
    }
}

Font* StringManager::GetFont() const
{
    return font.get();
}

void SetFontRenderProperties(StringManager* manager, uint32_t* target,
                             int32_t width, int32_t height)
{
    manager->SetRenderProperties(target, width, height);
}

} // namespace soft3d

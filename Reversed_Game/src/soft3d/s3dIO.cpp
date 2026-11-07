#include "soft3d/s3dIO.hpp"

#include "platform/linux/File.hpp"

#include <new>

namespace soft3d {
namespace {

struct WideFormatSpec {
    char16_t flag;
    int width;
    int precision;
    char16_t conversion;
};

int WideStringLength(const char16_t* text)
{
    const char16_t* end = text;
    while (*end != u'\0') {
        ++end;
    }
    return static_cast<int>(end - text);
}

void CopyWideString(char16_t* destination, const char16_t* source)
{
    do {
        *destination++ = *source;
    } while (*source++ != u'\0');
}

void WidenString(char16_t* destination, const char* source)
{
    do {
        *destination++ = static_cast<unsigned char>(*source);
    } while (*source++ != '\0');
}

int ParseWideFormatSpec(const char16_t* format, WideFormatSpec& spec)
{
    const char16_t* cursor = format;
    if (*cursor == u'#' || *cursor == u'+' || *cursor == u'-' ||
        *cursor == u'0' || *cursor == u' ') {
        spec.flag = *cursor++;
    } else {
        spec.flag = 0xffff;
    }

    if (*cursor < u'0' || *cursor > u'9') {
        spec.width = -1;
    } else {
        spec.width = 0;
        while (*cursor >= u'0' && *cursor <= u'9') {
            spec.width = spec.width * 10 + (*cursor++ - u'0');
        }
    }

    if (*cursor == u'.') {
        ++cursor;
        if (*cursor < u'0' || *cursor > u'9') {
            spec.precision = -1;
        } else {
            spec.precision = 0;
            while (*cursor >= u'0' && *cursor <= u'9') {
                spec.precision = spec.precision * 10 + (*cursor++ - u'0');
            }
        }
    } else {
        spec.precision = -1;
    }

    spec.conversion = *cursor;
    return static_cast<int>(cursor - format) + 1;
}

void AppendPadding(char16_t* output, int& outputLength, int count, char16_t conversion)
{
    char16_t padding;
    switch (conversion) {
    case u'X':
    case u'c':
    case u'o':
    case u's':
    case u'x':
        padding = u' ';
        break;
    case u'd':
        padding = u'0';
        break;
    default:
        return;
    }
    while (count-- > 0) {
        output[outputLength++] = padding;
    }
}

int ApplyWideFieldFormat(char16_t* output, const WideFormatSpec& spec)
{
    int inputLength = WideStringLength(output);
    int outputLength = 0;
    char16_t formatted[258];

    if (spec.width != -1) {
        int fieldContentLength = spec.precision == -1 ? inputLength : spec.precision;
        AppendPadding(formatted, outputLength, spec.width - fieldContentLength,
                      spec.conversion);
    }

    if (spec.flag == u'#') {
        if (spec.conversion == u'o') {
            formatted[outputLength++] = u'0';
        } else if (spec.conversion == u'X' || spec.conversion == u'x') {
            formatted[outputLength++] = u'0';
            formatted[outputLength++] = spec.conversion;
        }
    } else if (spec.flag == u'+' && spec.conversion == u'd' && output[0] != u'-') {
        formatted[outputLength++] = u'+';
    }

    if (spec.precision != -1 && inputLength < spec.precision) {
        AppendPadding(formatted, outputLength, spec.precision - inputLength,
                      spec.conversion);
    }
    CopyWideString(formatted + outputLength, output);

    if (spec.width == -1) {
        if (spec.precision == -1) {
            outputLength += inputLength;
        } else {
            outputLength += inputLength < spec.precision ? inputLength : spec.precision;
        }
    } else if (spec.precision == -1) {
        outputLength += inputLength < spec.width ? inputLength : spec.width;
    } else {
        outputLength += inputLength < spec.precision ? inputLength : spec.precision;
    }

    for (int i = 0; i < outputLength; ++i) {
        output[i] = formatted[i];
    }
    output[outputLength] = u'\0';
    return outputLength;
}

void UnsignedToWide(uint32_t value, char16_t* output, uint32_t base, bool emitZero)
{
    char16_t reversed[34];
    int count = 0;
    if (value != 0 || emitZero) {
        do {
            uint32_t digit = value % base;
            value /= base;
            reversed[count++] = static_cast<char16_t>(digit < 10 ? u'0' + digit
                                                                 : u'A' + digit - 10);
        } while (value != 0);
    }
    for (int i = 0; i < count; ++i) {
        output[i] = reversed[count - i - 1];
    }
    output[count] = u'\0';
}

void SignedToWide(int32_t value, char16_t* output)
{
    if (value < 0) {
        output[0] = u'-';
        UnsignedToWide(0u - static_cast<uint32_t>(value), output + 1, 10, true);
    } else {
        UnsignedToWide(static_cast<uint32_t>(value), output, 10, true);
    }
}

} // namespace

s3dIO::s3dIO() = default;

bool s3dIO::Open(const char16_t* path, const char16_t* mode)
{
    platform::File* opened = new (std::nothrow) platform::File;
    file = opened;
    if (opened == nullptr) {
        return false;
    }
    if (!opened->Open(path, mode)) {
        delete opened;
        file = nullptr;
        return false;
    }
    return true;
}

void s3dIO::Close()
{
    delete file;
    file = nullptr;
}

void s3dIO::Seek(uint32_t offset, int whence)
{
    file->Seek(offset, whence);
}

uint32_t s3dIO::Tell() const
{
    return file->Tell();
}

uint32_t s3dIO::Read(void* destination, uint32_t bytes)
{
    return file->Read(destination, bytes);
}

uint32_t s3dIO::Read(void* destination, uint32_t size, uint32_t count)
{
    return file->Read(destination, size, count);
}

int s3dIO::Write(const void* source, int size, int count)
{
    return file->Write(source, static_cast<uint32_t>(size * count));
}

int s3dIO::Printf(const char16_t* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = VPrintf(format, arguments);
    va_end(arguments);
    return result;
}

int s3dIO::VPrintf(const char16_t* format, va_list arguments)
{
    char16_t formatted[258];
    int formattedLength = FormatWideString(formatted, format, arguments);
    if (formattedLength < 1) {
        return -1;
    }
    return Write(formatted, formattedLength * 2, 1);
}

bool s3dIO::Reopen(const char16_t* path, const char16_t* mode)
{
    Close();
    return Open(path, mode);
}

int FormatWideString(char16_t* output, const char16_t* format, va_list arguments)
{
    int outputLength = 0;
    char16_t converted[258];

    while (*format != u'\0') {
        if (*format == u'%' && format[1] != u'%') {
            ++format;
            WideFormatSpec spec;
            format += ParseWideFormatSpec(format, spec);
            switch (spec.conversion) {
            case u'S':
                WidenString(converted, va_arg(arguments, const char*));
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u'X':
            case u'x':
                UnsignedToWide(va_arg(arguments, uint32_t), converted, 16, false);
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u'c':
                output[outputLength] = static_cast<char16_t>(va_arg(arguments, int));
                output[outputLength + 1] = u'\0';
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u'd':
            case u'i':
                SignedToWide(va_arg(arguments, int32_t), converted);
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u'o':
                UnsignedToWide(va_arg(arguments, uint32_t), converted, 8, false);
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u's':
                CopyWideString(converted, va_arg(arguments, const char16_t*));
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case u'u':
                UnsignedToWide(va_arg(arguments, uint32_t), converted, 10, true);
                CopyWideString(output + outputLength, converted);
                outputLength += ApplyWideFieldFormat(output + outputLength, spec);
                break;
            case 0:
                output[outputLength] = u'\0';
                return outputLength;
            default:
                break;
            }
        } else {
            output[outputLength++] = *format++;
        }
    }
    output[outputLength] = u'\0';
    return outputLength;
}

int WideSprintf(char16_t* output, const char16_t* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int result = FormatWideString(output, format, arguments);
    va_end(arguments);
    return result;
}

} // namespace soft3d

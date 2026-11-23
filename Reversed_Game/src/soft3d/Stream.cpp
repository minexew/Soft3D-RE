#include "soft3d/Stream.hpp"

#include "soft3d/Object.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

namespace soft3d {
namespace {

uint8_t* Allocate(int32_t bytes)
{
    return bytes > 0 ? new (std::nothrow) uint8_t[static_cast<size_t>(bytes)]() : nullptr;
}

} // namespace

Stream::Stream() = default;

Stream::Stream(int32_t byteSize)
    : size(byteSize)
{
    if (size > 0) {
        buffer = Allocate(size);
    }
}

Stream::Stream(const void* data, int32_t byteSize)
    : size(byteSize)
{
    if (size > 0) {
        buffer = Allocate(size);
        if (buffer != nullptr && data != nullptr) {
            std::memcpy(buffer, data, static_cast<size_t>(size));
        }
    }
}

Stream::Stream(const Stream& other)
    : size(other.size)
    , position(other.position)
{
    if (size > 0) {
        buffer = Allocate(size);
        if (buffer != nullptr && other.buffer != nullptr) {
            std::memcpy(buffer, other.buffer, static_cast<size_t>(size));
        }
    }
}

Stream::~Stream()
{
    delete[] buffer;
}

uint8_t* Stream::GetData() { return buffer; }
const uint8_t* Stream::GetData() const { return buffer; }
int32_t Stream::GetSize() const { return size - 1; }
int32_t Stream::Tell() const { return position; }
bool Stream::IsValid() const { return buffer != nullptr || size == 0; }
bool Stream::IsEnd() const { return size - 1 <= position; }

result Stream::Seek(int32_t offset, int whence)
{
    int64_t destination = offset;
    if (whence == SEEK_CUR) {
        destination += position;
    } else if (whence == SEEK_END) {
        destination += size;
    } else if (whence != SEEK_SET) {
        return S3D_PARAM;
    }
    if (destination < 0 || destination >= size) {
        return S3D_ACCESS;
    }
    position = static_cast<int32_t>(destination);
    return S3D_OK;
}

InputStream::InputStream() = default;
InputStream::InputStream(const void* data, int32_t byteSize)
    : Stream(data, byteSize)
{
}

void InputStream::SetData(const void* data, int32_t byteSize)
{
    delete[] buffer;
    buffer = nullptr;
    size = byteSize + 1;
    position = 0;
    error = false;
    if (size > 0) {
        buffer = Allocate(size);
        if (buffer != nullptr && data != nullptr && byteSize > 0) {
            std::memcpy(buffer, data, static_cast<size_t>(byteSize));
        }
    }
}

result InputStream::Read(void* destination, int32_t bytes)
{
    if (bytes < 0 || (destination == nullptr && bytes != 0)) {
        error = true;
        return S3D_PARAM;
    }
    if (static_cast<int64_t>(position) + bytes >= size) {
        error = true;
        return S3D_ACCESS;
    }
    if (bytes != 0) {
        std::memcpy(destination, buffer + position, static_cast<size_t>(bytes));
    }
    position += bytes;
    return S3D_OK;
}

result InputStream::ReadBool(bool& value)
{
    uint8_t byte = 0;
    const result status = ReadByte(byte);
    if (status == S3D_OK) {
        value = byte != 0;
    }
    return status;
}

result InputStream::ReadByte(uint8_t& value)
{
    return Read(&value, 1);
}

result InputStream::ReadUInt16(uint16_t& value)
{
    uint8_t bytes[2];
    const result status = Read(bytes, 2);
    if (status == S3D_OK) {
        value = static_cast<uint16_t>(bytes[0]) << 8 | bytes[1];
    }
    return status;
}

result InputStream::ReadInt16(int16_t& value)
{
    uint16_t bits = 0;
    const result status = ReadUInt16(bits);
    if (status == S3D_OK) {
        value = static_cast<int16_t>(bits);
    }
    return status;
}

result InputStream::ReadUInt32(uint32_t& value)
{
    uint8_t bytes[4];
    const result status = Read(bytes, 4);
    if (status == S3D_OK) {
        value = static_cast<uint32_t>(bytes[0]) << 24 |
                static_cast<uint32_t>(bytes[1]) << 16 |
                static_cast<uint32_t>(bytes[2]) << 8 | bytes[3];
    }
    return status;
}

result InputStream::ReadInt32(int32_t& value)
{
    uint32_t bits = 0;
    const result status = ReadUInt32(bits);
    if (status == S3D_OK) {
        value = static_cast<int32_t>(bits);
    }
    return status;
}

result InputStream::ReadWideString(char16_t* destination, int32_t characterCapacity)
{
    int16_t bytes = 0;
    result status = ReadInt16(bytes);
    if (status != S3D_OK) {
        return status;
    }
    if (destination == nullptr || characterCapacity < 0) {
        error = true;
        return S3D_PARAM;
    }
    if (characterCapacity < bytes || bytes < 0) {
        error = true;
        return S3D_ACCESS;
    }
    std::memset(destination, 0, static_cast<size_t>(characterCapacity) * sizeof(char16_t));
    if (bytes == 0) {
        return S3D_OK;
    }
    if ((bytes & 1) != 0 || static_cast<int64_t>(position) + bytes >= size) {
        error = true;
        return S3D_ACCESS;
    }
    std::memcpy(destination, buffer + position, static_cast<size_t>(bytes));
    position += bytes;
    return S3D_OK;
}

InputDataStream::InputDataStream() = default;
InputDataStream::InputDataStream(const void* data, int32_t byteSize)
    : InputStream(data, byteSize)
{
}

InputFileStream::InputFileStream(const char16_t* path, Engine* engine)
{
    s3dIO file;
    file.SetEngine(engine);
    if (!file.Open(path, u"rb")) {
        return;
    }
    file.Seek(0, SEEK_END);
    const int32_t fileSize = static_cast<int32_t>(file.Tell());
    if (fileSize == -1) {
        file.Close();
        return;
    }
    file.Seek(0, SEEK_SET);
    size = fileSize;
    if (fileSize > 0) {
        buffer = Allocate(fileSize);
        if (buffer != nullptr && file.Read(buffer, static_cast<uint32_t>(fileSize)) !=
                                   static_cast<uint32_t>(fileSize)) {
            error = true;
        }
    }
    file.Close();
}

OutputStream::OutputStream() = default;
OutputStream::OutputStream(const OutputStream& other)
    : Stream()
    , capacity(other.capacity)
{
    size = other.size;
    position = other.position;
    if (capacity > 0) {
        buffer = Allocate(capacity);
        if (buffer != nullptr && other.buffer != nullptr) {
            std::memcpy(buffer, other.buffer, static_cast<size_t>(capacity));
        }
    }
}

void OutputStream::Clear()
{
    size = 0;
    position = 0;
}

result OutputStream::Reserve(int32_t bytes)
{
    if (bytes < 0) {
        return S3D_PARAM;
    }
    const int64_t requested = static_cast<int64_t>(capacity) + static_cast<int64_t>(bytes) * 2;
    if (requested > INT32_MAX) {
        return S3D_MEMORY;
    }
    int32_t newCapacity = static_cast<int32_t>(requested);
    if (newCapacity == capacity) {
        ++newCapacity;
    }
    uint8_t* replacement = Allocate(newCapacity);
    if (replacement == nullptr) {
        return S3D_MEMORY;
    }
    if (buffer != nullptr && capacity > 0) {
        std::memcpy(replacement, buffer, static_cast<size_t>(capacity));
    }
    delete[] buffer;
    buffer = replacement;
    capacity = newCapacity;
    return S3D_OK;
}

result OutputStream::Write(const void* source, int32_t bytes)
{
    if (bytes < 0 || (source == nullptr && bytes != 0)) {
        return S3D_PARAM;
    }
    result status = S3D_OK;
    while (static_cast<int64_t>(position) + bytes >= capacity) {
        status = Reserve(bytes);
        if (status != S3D_OK) {
            return status;
        }
    }
    if (bytes != 0) {
        std::memcpy(buffer + position, source, static_cast<size_t>(bytes));
    }
    position += bytes;
    if (size <= position) {
        size = position + 1;
    }
    return S3D_OK;
}

result OutputStream::Assign(const OutputStream& other)
{
    Clear();
    return Write(other.buffer, other.size);
}

result OutputStream::Append(const OutputStream& other)
{
    position = size > 0 ? size - 1 : 0;
    return Write(other.buffer, other.size);
}

result OutputStream::WriteBool(bool value)
{
    return WriteByte(value ? 1 : 0);
}

result OutputStream::WriteByte(uint8_t value)
{
    return Write(&value, 1);
}

result OutputStream::WriteUInt16(uint16_t value)
{
    const uint8_t bytes[] = {
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value),
    };
    return Write(bytes, 2);
}

result OutputStream::WriteInt16(int16_t value)
{
    return WriteUInt16(static_cast<uint16_t>(value));
}

result OutputStream::WriteUInt32(uint32_t value)
{
    const uint8_t bytes[] = {
        static_cast<uint8_t>(value >> 24),
        static_cast<uint8_t>(value >> 16),
        static_cast<uint8_t>(value >> 8),
        static_cast<uint8_t>(value),
    };
    return Write(bytes, 4);
}

result OutputStream::WriteInt32(int32_t value)
{
    return WriteUInt32(static_cast<uint32_t>(value));
}

result OutputStream::WriteWideString(const char16_t* value)
{
    if (value == nullptr) {
        return S3D_PARAM;
    }
    int32_t length = 0;
    while (value[length] != u'\0') {
        ++length;
    }
    const int32_t bytes = length * 2;
    if (bytes > INT16_MAX) {
        return S3D_PARAM;
    }
    const result status = WriteInt16(static_cast<int16_t>(bytes));
    return status == S3D_OK ? Write(value, bytes) : status;
}

OutputDataStream::OutputDataStream() = default;
OutputDataStream::OutputDataStream(const OutputDataStream& other)
    : OutputStream(other)
{
}

OutputFileStream::OutputFileStream(const char16_t* path, Engine* engine)
{
    file.SetEngine(engine);
    open = file.Open(path, u"wb");
}

OutputFileStream::~OutputFileStream()
{
    file.Close();
}

result OutputFileStream::Flush()
{
    if (!open) {
        return S3D_FILE;
    }
    if (size > 0) {
        file.Write(buffer, size, 1);
    }
    return S3D_OK;
}

bool OutputFileStream::IsOpen() const { return open; }

result ReadFixed(InputStream& stream, Fixed& value)
{
    return stream.ReadInt32(value);
}

result ReadVec2(InputStream& stream, Vec2& value)
{
    result status = ReadFixed(stream, value.x);
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.y);
    }
    return status;
}

result ReadVec3(InputStream& stream, Vec3& value)
{
    result status = ReadFixed(stream, value.x);
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.y);
    }
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.z);
    }
    return status;
}

result ReadVec4(InputStream& stream, Vec4& value)
{
    result status = ReadFixed(stream, value.x);
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.y);
    }
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.z);
    }
    if (status == S3D_OK) {
        status = ReadFixed(stream, value.w);
    }
    return status;
}

result ReadBounds(InputStream& stream, ObjectBounds& value)
{
    result status = ReadVec3(stream, value.minimum);
    if (status == S3D_OK) {
        status = ReadVec3(stream, value.maximum);
    }
    return status;
}

result WriteFixed(OutputStream& stream, Fixed value)
{
    return stream.WriteInt32(value);
}

result WriteVec2(OutputStream& stream, const Vec2& value)
{
    result status = WriteFixed(stream, value.x);
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.y);
    }
    return status;
}

result WriteVec3(OutputStream& stream, const Vec3& value)
{
    result status = WriteFixed(stream, value.x);
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.y);
    }
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.z);
    }
    return status;
}

result WriteVec4(OutputStream& stream, const Vec4& value)
{
    result status = WriteFixed(stream, value.x);
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.y);
    }
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.z);
    }
    if (status == S3D_OK) {
        status = WriteFixed(stream, value.w);
    }
    return status;
}

result WriteBounds(OutputStream& stream, const ObjectBounds& value)
{
    result status = WriteVec3(stream, value.minimum);
    if (status == S3D_OK) {
        status = WriteVec3(stream, value.maximum);
    }
    return status;
}

} // namespace soft3d

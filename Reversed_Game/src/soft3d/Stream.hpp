#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dIO.hpp"

#include <cstddef>
#include <cstdint>

namespace soft3d {

struct ObjectBounds;

class Stream {
public:
    virtual ~Stream();

    Stream& operator=(const Stream&) = delete;

    uint8_t* GetData();
    const uint8_t* GetData() const;
    int32_t GetSize() const;
    int32_t Tell() const;
    bool IsValid() const;
    bool IsEnd() const;
    result Seek(int32_t offset, int whence);

protected:
    Stream();
    explicit Stream(int32_t size);
    Stream(const void* data, int32_t size);
    Stream(const Stream& other);

    uint8_t* buffer = nullptr;
    int32_t size = 0;
    int32_t position = 0;
};

class InputStream : public Stream {
public:
    InputStream();
    InputStream(const void* data, int32_t size);
    ~InputStream() override = default;

    void SetData(const void* data, int32_t size);
    result Read(void* destination, int32_t bytes);
    result ReadBool(bool& value);
    result ReadByte(uint8_t& value);
    result ReadInt16(int16_t& value);
    result ReadUInt16(uint16_t& value);
    result ReadInt32(int32_t& value);
    result ReadUInt32(uint32_t& value);
    result ReadWideString(char16_t* destination, int32_t capacity);
    bool HasError() const { return error; }

protected:
    bool error = false;
};

class InputDataStream : public InputStream {
public:
    InputDataStream();
    InputDataStream(const void* data, int32_t size);
    ~InputDataStream() override = default;
};

class InputFileStream : public InputDataStream {
public:
    explicit InputFileStream(const char16_t* path, Engine* engine = nullptr);
    ~InputFileStream() override = default;
};

class OutputStream : public Stream {
public:
    OutputStream();
    OutputStream(const OutputStream& other);
    ~OutputStream() override = default;

    void Clear();
    result Assign(const OutputStream& other);
    result Append(const OutputStream& other);
    result Write(const void* source, int32_t bytes);
    result WriteBool(bool value);
    result WriteByte(uint8_t value);
    result WriteInt16(int16_t value);
    result WriteUInt16(uint16_t value);
    result WriteInt32(int32_t value);
    result WriteUInt32(uint32_t value);
    result WriteWideString(const char16_t* value);

protected:
    result Reserve(int32_t bytes);

    int32_t capacity = 0;
};

class OutputDataStream : public OutputStream {
public:
    OutputDataStream();
    OutputDataStream(const OutputDataStream& other);
    ~OutputDataStream() override = default;
};

class OutputFileStream : public OutputDataStream {
public:
    explicit OutputFileStream(const char16_t* path, Engine* engine = nullptr);
    ~OutputFileStream() override;

    result Flush();
    bool IsOpen() const;

private:
    s3dIO file;
    bool open = false;
};

result ReadFixed(InputStream& stream, Fixed& value);
result ReadVec2(InputStream& stream, Vec2& value);
result ReadVec3(InputStream& stream, Vec3& value);
result ReadVec4(InputStream& stream, Vec4& value);
result ReadBounds(InputStream& stream, ObjectBounds& value);

result WriteFixed(OutputStream& stream, Fixed value);
result WriteVec2(OutputStream& stream, const Vec2& value);
result WriteVec3(OutputStream& stream, const Vec3& value);
result WriteVec4(OutputStream& stream, const Vec4& value);
result WriteBounds(OutputStream& stream, const ObjectBounds& value);

} // namespace soft3d

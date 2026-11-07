#include "soft3d/Animation.hpp"

#include "soft3d/s3dFilePak.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace soft3d {
namespace {

uint16_t Read16(const uint8_t*& input)
{
    uint16_t value;
    std::memcpy(&value, input, sizeof(value));
    input += sizeof(value);
    return value;
}

uint32_t Read32(const uint8_t*& input)
{
    uint32_t value;
    std::memcpy(&value, input, sizeof(value));
    input += sizeof(value);
    return value;
}

int32_t ReadSignedValue(const uint8_t*& input, uint16_t signs, unsigned bit)
{
    int32_t value = Read16(input);
    if (((signs >> bit) & 1) != 0) {
        value = -value;
    }
    return value;
}

void ReadTransform(std::array<int32_t, 6>& transform, const uint8_t*& input)
{
    for (int32_t& value : transform) {
        value = static_cast<int32_t>(Read32(input));
    }
}

} // namespace

int Animation::Load(const char16_t* entryName, s3dFilePak* filePak)
{
    int status = filePak->Select(entryName);
    if (status != 0) {
        return status;
    }
    filePak->Seek(0, SEEK_END);
    int32_t bytes = filePak->Tell();
    if (bytes == -1) {
        return 8;
    }
    void* data = std::malloc(static_cast<size_t>(bytes));
    if (data == nullptr) {
        return 4;
    }
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data, bytes);
    status = ParseSai(data);
    std::free(data);
    return status;
}

int Animation::ParseSai(const void* data)
{
    tracks.clear();
    morphs.clear();
    const uint8_t* input = static_cast<const uint8_t*>(data);
    header0 = Read32(input);
    header1 = Read32(input);
    uint32_t flags = Read32(input);
    transformCount = flags & 0xff;
    for (uint32_t i = 0; i < transformCount; ++i) {
        if (i == 0) {
            ReadTransform(firstTransform, input);
        } else {
            ReadTransform(secondTransform, input);
        }
    }
    if (transformCount == 1) {
        secondTransform = firstTransform;
    }

    trackCount = static_cast<int16_t>(Read16(input));
    Read16(input);
    tracks.resize(static_cast<uint16_t>(trackCount));
    for (int32_t i = 0; i < trackCount; ++i) {
        SaiTrack& track = tracks[static_cast<size_t>(i)];
        track.keyframeCount = static_cast<int32_t>(Read32(input));
        track.keyframes.resize(static_cast<uint32_t>(track.keyframeCount));
        for (int32_t j = 0; j < track.keyframeCount; ++j) {
            SaiKeyframe& keyframe = track.keyframes[static_cast<size_t>(j)];
            keyframe.time = static_cast<int32_t>(Read32(input));
            uint16_t signs = Read16(input);
            keyframe.value0 = ReadSignedValue(input, signs, 0);
            keyframe.value1 = ReadSignedValue(input, signs, 1);
            keyframe.value2 = ReadSignedValue(input, signs, 2);
            keyframe.value3 = ReadSignedValue(input, signs, 3);
            keyframe.value4 = ReadSignedValue(input, signs, 4);
            keyframe.value5 = ReadSignedValue(input, signs, 5);
            keyframe.value6 = ReadSignedValue(input, signs, 6);
            keyframe.value7 = ReadSignedValue(input, signs, 7);
            keyframe.value8 = ReadSignedValue(input, signs, 8);
            for (int32_t& value : keyframe.translation) {
                value = static_cast<int32_t>(Read32(input));
            }
            keyframe.zero10 = 0;
            keyframe.zero20 = 0;
            keyframe.zero30 = 0;
            keyframe.scale = 0x10000;
        }
    }

    if (((flags >> 8) & 1) != 0) {
        morphFrameCount = static_cast<int16_t>(Read16(input));
        if (morphFrameCount == 2) {
            morphCount = static_cast<int16_t>(Read16(input));
            morphs.resize(static_cast<uint32_t>(morphCount));
            for (int32_t i = 0; i < morphCount; ++i) {
                SaiMorph& morph = morphs[static_cast<size_t>(i)];
                morph.unknown00 = static_cast<int32_t>(Read32(input));
                morph.frames.resize(static_cast<uint32_t>(morphFrameCount));
                for (auto& frame : morph.frames) {
                    for (int32_t& value : frame) {
                        value = static_cast<int32_t>(Read32(input));
                    }
                }
            }
        } else {
            morphFrameCount = 0;
        }
    }
    return 0;
}

uint32_t Animation::GetHeader0() const { return header0; }
uint32_t Animation::GetHeader1() const { return header1; }
uint32_t Animation::GetTransformCount() const { return transformCount; }
int16_t Animation::GetTrackCount() const { return trackCount; }
int16_t Animation::GetMorphFrameCount() const { return morphFrameCount; }
const std::array<int32_t, 6>& Animation::GetFirstTransform() const { return firstTransform; }
const std::array<int32_t, 6>& Animation::GetSecondTransform() const { return secondTransform; }
const std::vector<SaiTrack>& Animation::GetTracks() const { return tracks; }
const std::vector<SaiMorph>& Animation::GetMorphs() const { return morphs; }

} // namespace soft3d

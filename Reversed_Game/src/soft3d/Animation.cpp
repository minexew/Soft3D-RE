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

void ReadBounds(std::array<int32_t, 6>& bounds, const uint8_t*& input)
{
    for (int32_t& value : bounds) {
        value = static_cast<int32_t>(Read32(input));
    }
}

Fixed InterpolationFactor(int32_t time, int32_t previous, int32_t next)
{
    const uint16_t interval = static_cast<uint16_t>(next - previous);
    const Fixed reciprocal = interval == 0 ? 0x7fffffff :
        static_cast<Fixed>((0x40000000U / interval) >> 14);
    return FixedMultiply(static_cast<Fixed>(static_cast<uint32_t>(time - previous) << 16),
                         reciprocal);
}

Fixed Interpolate(Fixed previous, Fixed next, Fixed factor)
{
    return static_cast<Fixed>(static_cast<uint32_t>(previous) +
        static_cast<uint32_t>(FixedMultiply(next - previous, factor)));
}

} // namespace

result Animation::Load(const char16_t* entryName, s3dFilePak* filePak)
{
    result status = filePak->Select(entryName);
    if (status != S3D_OK) {
        S3D_LOG(log, u"Load Animation %s: can't find!\n", entryName);
        return status;
    }
    filePak->Seek(0, SEEK_END);
    int32_t bytes = filePak->Tell();
    if (bytes == S3D_EOF) {
        S3D_LOG(log, u"Load Animation %s: ftell return EOF!\n", entryName);
        return S3D_UNKNOWN;
    }
    void* data = std::malloc(static_cast<size_t>(bytes));
    if (data == nullptr) {
        S3D_LOG(log, u"Load Animation %s: memory out!\n", entryName);
        return S3D_MEMORY;
    }
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data, bytes);
    status = ParseSai(data);
    std::free(data);
    return status;
}

result Animation::ParseSai(const void* data)
{
    tracks.clear();
    morphs.clear();
    const uint8_t* input = static_cast<const uint8_t*>(data);
    startTime = static_cast<int32_t>(Read32(input));
    endTime = static_cast<int32_t>(Read32(input));
    uint32_t flags = Read32(input);
    boundingVolumeCount = flags & 0xff;
    for (uint32_t i = 0; i < boundingVolumeCount; ++i) {
        if (i == 0) {
            ReadBounds(firstBounds, input);
        } else {
            ReadBounds(secondBounds, input);
        }
    }
    if (boundingVolumeCount == 1) {
        secondBounds = firstBounds;
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
            constexpr std::array<unsigned, 9> matrixIndices = {0, 1, 2, 4, 5, 6, 8, 9, 10};
            for (unsigned index = 0; index < matrixIndices.size(); ++index) {
                keyframe.transform.values[matrixIndices[index]] =
                    ReadSignedValue(input, signs, index);
            }
            for (unsigned index = 12; index < 15; ++index) {
                keyframe.transform.values[index] = static_cast<int32_t>(Read32(input));
            }
            keyframe.transform.values[3] = 0;
            keyframe.transform.values[7] = 0;
            keyframe.transform.values[11] = 0;
            keyframe.transform.values[15] = fixedOne;
        }
    }

    if (((flags >> 8) & 1) != 0) {
        morphTargetCount = static_cast<int16_t>(Read16(input));
        if (morphTargetCount == 2) {
            morphCount = static_cast<int16_t>(Read16(input));
            morphs.resize(static_cast<uint32_t>(morphCount));
            for (int32_t i = 0; i < morphCount; ++i) {
                SaiMorph& morph = morphs[static_cast<size_t>(i)];
                morph.time = static_cast<int32_t>(Read32(input));
                morph.targets.resize(static_cast<uint32_t>(morphTargetCount));
                for (auto& target : morph.targets) {
                    for (int32_t& value : target) {
                        value = static_cast<int32_t>(Read32(input));
                    }
                }
            }
        } else {
            morphTargetCount = 0;
        }
    }
    return S3D_OK;
}

int32_t Animation::GetStartTime() const { return startTime; }
int32_t Animation::GetEndTime() const { return endTime; }
uint32_t Animation::GetBoundingVolumeCount() const { return boundingVolumeCount; }
int16_t Animation::GetTrackCount() const { return trackCount; }
int16_t Animation::GetMorphTargetCount() const { return morphTargetCount; }
const std::array<int32_t, 6>& Animation::GetFirstBounds() const { return firstBounds; }
const std::array<int32_t, 6>& Animation::GetSecondBounds() const { return secondBounds; }
const std::vector<SaiTrack>& Animation::GetTracks() const { return tracks; }
const std::vector<SaiMorph>& Animation::GetMorphs() const { return morphs; }

Matrix4 Animation::SampleTransform(uint32_t trackIndex, int32_t time) const
{
    const SaiTrack& track = tracks.at(trackIndex);
    if (track.keyframeCount == 1) {
        return track.keyframes[0].transform;
    }
    const SaiKeyframe* previous = nullptr;
    const SaiKeyframe* next = nullptr;
    int32_t intervalEnd = endTime;
    for (size_t index = 0; index < track.keyframes.size(); ++index) {
        const SaiKeyframe& keyframe = track.keyframes[index];
        if (keyframe.time == time) {
            return keyframe.transform;
        }
        if (time < keyframe.time) {
            previous = &track.keyframes[index - 1];
            next = &keyframe;
            intervalEnd = next->time;
            break;
        }
    }
    if (previous == nullptr) {
        previous = &track.keyframes.back();
        next = &track.keyframes.front();
    }
    const Fixed factor = InterpolationFactor(time, previous->time, intervalEnd);
    Matrix4 result = previous->transform;
    for (unsigned index = 0; index < result.values.size(); ++index) {
        if (index != 3 && index != 7 && index != 11) {
            result.values[index] = Interpolate(previous->transform.values[index],
                                               next->transform.values[index], factor);
        }
    }
    return result;
}

Vec3 Animation::SampleMorph(uint32_t target, int32_t time) const
{
    if (target >= static_cast<uint32_t>(morphTargetCount)) {
        return {};
    }
    const SaiMorph* previous = nullptr;
    const SaiMorph* next = nullptr;
    int32_t intervalEnd = endTime;
    for (size_t index = 0; index < morphs.size(); ++index) {
        const SaiMorph& morph = morphs[index];
        if (morph.time == time) {
            const auto& value = morph.targets[target];
            return {value[0], value[1], value[2]};
        }
        if (time < morph.time) {
            previous = &morphs[index - 1];
            next = &morph;
            intervalEnd = next->time;
            break;
        }
    }
    if (previous == nullptr) {
        previous = &morphs.back();
        next = &morphs.front();
    }
    const Fixed factor = InterpolationFactor(time, previous->time, intervalEnd);
    Vec3 result;
    result.x = Interpolate(previous->targets[target][0], next->targets[target][0], factor);
    result.y = Interpolate(previous->targets[target][1], next->targets[target][1], factor);
    result.z = Interpolate(previous->targets[target][2], next->targets[target][2], factor);
    return result;
}

} // namespace soft3d

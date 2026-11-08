#pragma once

#include "soft3d/Fixed.hpp"
#include "soft3d/Resource.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace soft3d {

struct SaiKeyframe {
    int32_t time = 0;
    Matrix4 transform = Matrix4::Identity();
};

struct SaiTrack {
    int32_t keyframeCount = 0;
    std::vector<SaiKeyframe> keyframes;
};

struct SaiMorph {
    int32_t time = 0;
    std::vector<std::array<int32_t, 3>> targets;
};

class Animation : public Resource {
public:
    Animation() = default;

    result Load(const char16_t* entryName, s3dFilePak* filePak) override;
    result ParseSai(const void* data);

    int32_t GetStartTime() const;
    int32_t GetEndTime() const;
    uint32_t GetBoundingVolumeCount() const;
    int16_t GetTrackCount() const;
    int16_t GetMorphTargetCount() const;
    const std::array<int32_t, 6>& GetFirstBounds() const;
    const std::array<int32_t, 6>& GetSecondBounds() const;
    const std::vector<SaiTrack>& GetTracks() const;
    const std::vector<SaiMorph>& GetMorphs() const;

    Matrix4 SampleTransform(uint32_t track, int32_t time) const;
    Vec3 SampleMorph(uint32_t target, int32_t time) const;

private:
    std::array<int32_t, 6> firstBounds{};
    std::array<int32_t, 6> secondBounds{};
    uint32_t boundingVolumeCount = 0;
    int32_t startTime = 0;
    int32_t endTime = 0;
    int32_t morphCount = 0;
    int16_t trackCount = 0;
    int16_t morphTargetCount = 0;
    std::vector<SaiTrack> tracks;
    std::vector<SaiMorph> morphs;
};

} // namespace soft3d

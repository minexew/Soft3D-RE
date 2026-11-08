#pragma once

#include "soft3d/Resource.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace soft3d {

struct SaiKeyframe {
    int32_t time = 0;
    int32_t value0 = 0;
    int32_t value1 = 0;
    int32_t value2 = 0;
    int32_t zero10 = 0;
    int32_t value3 = 0;
    int32_t value4 = 0;
    int32_t value5 = 0;
    int32_t zero20 = 0;
    int32_t value6 = 0;
    int32_t value7 = 0;
    int32_t value8 = 0;
    int32_t zero30 = 0;
    std::array<int32_t, 3> translation{};
    int32_t scale = 0;
};

struct SaiTrack {
    int32_t keyframeCount = 0;
    std::vector<SaiKeyframe> keyframes;
};

struct SaiMorph {
    int32_t unknown00 = 0;
    std::vector<std::array<int32_t, 3>> frames;
};

class Animation : public Resource {
public:
    Animation() = default;

    result Load(const char16_t* entryName, s3dFilePak* filePak) override;
    result ParseSai(const void* data);

    uint32_t GetHeader0() const;
    uint32_t GetHeader1() const;
    uint32_t GetTransformCount() const;
    int16_t GetTrackCount() const;
    int16_t GetMorphFrameCount() const;
    const std::array<int32_t, 6>& GetFirstTransform() const;
    const std::array<int32_t, 6>& GetSecondTransform() const;
    const std::vector<SaiTrack>& GetTracks() const;
    const std::vector<SaiMorph>& GetMorphs() const;

private:
    std::array<int32_t, 6> firstTransform{};
    std::array<int32_t, 6> secondTransform{};
    uint32_t transformCount = 0;
    uint32_t header0 = 0;
    uint32_t header1 = 0;
    int32_t morphCount = 0;
    int16_t trackCount = 0;
    int16_t morphFrameCount = 0;
    std::vector<SaiTrack> tracks;
    std::vector<SaiMorph> morphs;
};

} // namespace soft3d

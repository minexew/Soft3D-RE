#pragma once

#include "soft3d/Resource.hpp"

#include <cstdint>

namespace soft3d {

class AudioData : public Resource {
public:
    AudioData();
    ~AudioData() override;

    result Load(const char16_t* entryName, s3dFilePak* filePak) override;
    uint32_t GetPcmSampleCount() const;
    int16_t* GetPcmBuffer() const;

private:
    uint32_t pcmSampleCount;
    int16_t* pcmBuffer;
};

void DecodeSauAdpcm(const void* compressed, int32_t compressedBytes, int16_t* pcm);

} // namespace soft3d

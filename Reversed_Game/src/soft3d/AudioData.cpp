#include "soft3d/AudioData.hpp"

#include "soft3d/s3dFilePak.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace soft3d {
namespace {

constexpr std::array<int32_t, 89> stepTable = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28,
    31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408,
    449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282,
    1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
    4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086,
    29794, 32767
};

constexpr std::array<int32_t, 8> indexTable = {-1, -1, -1, -1, 2, 4, 6, 8};

} // namespace

AudioData::AudioData()
    : pcmSampleCount(0)
    , pcmBuffer(nullptr)
{
}

AudioData::~AudioData()
{
    std::free(pcmBuffer);
}

int AudioData::Load(const char16_t* entryName, s3dFilePak* filePak)
{
    int status = filePak->Select(entryName);
    if (status != 0) {
        return status;
    }

    filePak->Seek(0, SEEK_END);
    int32_t compressedBytes = filePak->Tell();
    pcmSampleCount = static_cast<uint32_t>(compressedBytes) << 1;
    filePak->Seek(0, SEEK_SET);
    void* compressed = std::malloc(static_cast<size_t>(compressedBytes));
    if (compressed == nullptr) {
        return 4;
    }

    filePak->Read(compressed, compressedBytes);
    pcmBuffer = static_cast<int16_t*>(std::malloc(static_cast<size_t>(pcmSampleCount) << 1));
    if (pcmBuffer == nullptr) {
        std::free(compressed);
        return 4;
    }

    std::memset(pcmBuffer, 0, static_cast<size_t>(pcmSampleCount) << 1);
    DecodeSauAdpcm(compressed, compressedBytes, pcmBuffer);
    std::free(compressed);
    return 0;
}

uint32_t AudioData::GetPcmSampleCount() const
{
    return pcmSampleCount;
}

int16_t* AudioData::GetPcmBuffer() const
{
    return pcmBuffer;
}

void DecodeSauAdpcm(const void* compressed, int32_t compressedBytes, int16_t* pcm)
{
    int32_t predictor = 0;
    int32_t stepIndex = 0;
    uint32_t bitOffset = 32;
    uint32_t word = 0;
    const uint8_t* input = static_cast<const uint8_t*>(compressed);
    const uint8_t* end = input + compressedBytes;
    int16_t* output = pcm;

    while (input < end) {
        if (bitOffset < 28) {
            bitOffset += 4;
        } else {
            std::memcpy(&word, input, sizeof(word));
            bitOffset = 0;
        }
        if (((bitOffset >> 2) & 1) != 0) {
            ++input;
        }

        uint32_t code = (word >> bitOffset) & 0xf;
        uint32_t magnitude = code & 7;
        int32_t step = stepTable[static_cast<size_t>(stepIndex)];
        int32_t difference = static_cast<int32_t>(magnitude * static_cast<uint32_t>(step) >> 2)
                             + (step >> 3);
        stepIndex += indexTable[magnitude];
        if (stepIndex < 0) {
            stepIndex = 0;
        } else if (stepIndex > 88) {
            stepIndex = 88;
        }
        if ((code >> 3) != 0) {
            difference = -difference;
        }
        predictor += difference;
        if (predictor >= 32768) {
            predictor = 32767;
        } else if (predictor < -32768) {
            predictor = -32768;
        }
        *output++ = static_cast<int16_t>(predictor);
    }
}

} // namespace soft3d

#include "soft3d/SoundInstance.hpp"

#include "soft3d/AudioManager.hpp"

#include <algorithm>

namespace soft3d {
namespace {

int32_t WrapSubtract(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) - static_cast<uint32_t>(right));
}

int32_t WrapAdd(int32_t left, int32_t right)
{
    return static_cast<int32_t>(static_cast<uint32_t>(left) + static_cast<uint32_t>(right));
}

int32_t Absolute(int32_t value)
{
    return value < 0 ? static_cast<int32_t>(0U - static_cast<uint32_t>(value)) : value;
}

int32_t FixedSquareRoot(int32_t value)
{
    uint32_t remainder = static_cast<uint32_t>(value);
    uint32_t root = 0;
    if (value >= 0x40000000) {
        remainder -= 0x40000000;
        root = 0x40000000;
    }
    for (uint32_t bit = 0x10000000; bit != 0; bit >>= 2) {
        const uint32_t trial = root + bit;
        root >>= 1;
        if (trial <= remainder) {
            remainder -= trial;
            root |= bit;
        }
    }
    if (root < remainder) {
        ++root;
    }
    return static_cast<int32_t>(root << 8);
}

} // namespace

Fixed CalculateFixedPointDistance(const Vec3& first, const Vec3& second)
{
    int32_t x = Absolute(WrapSubtract(first.x, second.x));
    int32_t y = Absolute(WrapSubtract(first.y, second.y));
    int32_t z = Absolute(WrapSubtract(first.z, second.z));
    const int32_t sum = WrapAdd(WrapAdd(x, y), z);
    if (sum == 0) {
        return 0;
    }

    int32_t threshold = 0xb50000;
    uint32_t normalization = 0;
    while (threshold < sum) {
        threshold = static_cast<int32_t>(static_cast<uint32_t>(threshold) << 1);
        normalization += 2;
    }
    x >>= normalization;
    y >>= normalization;
    z >>= normalization;
    const Fixed squared = WrapAdd(
        WrapAdd(FixedMultiply(x, x), FixedMultiply(y, y)), FixedMultiply(z, z));
    return static_cast<Fixed>(static_cast<uint32_t>(FixedSquareRoot(squared)) << normalization);
}

SoundInstance::SoundInstance() = default;

int SoundInstance::Initialize(AudioManager* manager, AudioData* soundData, bool repeat, bool isOwned)
{
    owner = manager;
    data = soundData;
    looping = repeat;
    distance = 0;
    volume = 0;
    owned = isOwned;
    return 0;
}

void SoundInstance::UpdateVolume()
{
    CalculateSpatialVolume();
    if (voice != -1) {
        owner->SetVoiceVolume(voice, volume);
    }
}

void SoundInstance::Activate()
{
    if (data == nullptr || voice != -1) {
        if (voice != -1) {
            owner->SetVoiceVolume(voice, volume);
        }
        return;
    }
    voice = owner->PlaySound(data, looping != 0);
    if (voice != -1) {
        owner->SetVoiceVolume(voice, volume);
    }
}

void SoundInstance::Stop()
{
    if (voice != -1) {
        owner->StopVoice(voice);
        voice = -1;
    }
    listIndex = -1;
}

void SoundInstance::CalculateSpatialVolume()
{
    distance = CalculateFixedPointDistance(listenerPosition, position);
    volume = 285 - (FixedMultiply(distance, 0x38000) >> 16);
    volume = std::clamp(volume, 0, 255);
}

void SoundInstance::SetPosition(const Vec3& value)
{
    position = value;
}

void SoundInstance::SetListenerPosition(const Vec3& value)
{
    listenerPosition = value;
}

bool SoundInstance::IsActive() const
{
    return voice != -1;
}

AudioData* SoundInstance::GetData() const
{
    return data;
}

int SoundInstance::GetVoice() const
{
    return voice;
}

int SoundInstance::GetVolume() const
{
    return volume;
}

Fixed SoundInstance::GetDistance() const
{
    return distance;
}

} // namespace soft3d

#pragma once

#include "soft3d/Fixed.hpp"

#include <cstdint>

namespace soft3d {

class AudioData;
class AudioManager;

class SoundInstance {
public:
    SoundInstance();

    int Initialize(AudioManager* manager, AudioData* data, bool looping, bool owned);
    void UpdateVolume();
    void Activate();
    void Stop();
    void CalculateSpatialVolume();
    void SetPosition(const Vec3& position);
    void SetListenerPosition(const Vec3& position);
    bool IsActive() const;

    AudioData* GetData() const;
    int GetVoice() const;
    int GetVolume() const;
    Fixed GetDistance() const;

private:
    AudioManager* owner = nullptr;
    Vec3 position;
    int volume = 0;
    int voice = -1;
    Vec3 listenerPosition;
    Fixed distance = 0;
    uint32_t looping = 0;
    AudioData* data = nullptr;
    uint32_t owned = 0;
    int listIndex = -1;

    friend class AudioManager;
};

Fixed CalculateFixedPointDistance(const Vec3& first, const Vec3& second);

} // namespace soft3d

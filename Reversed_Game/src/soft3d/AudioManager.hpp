#pragma once

#include "soft3d/AudioDevice.hpp"
#include "soft3d/Fixed.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace soft3d {

class AudioData;
class s3dResManager;
class SoundInstance;

class AudioManager {
public:
    AudioManager(s3dResManager* resources, std::unique_ptr<AudioBackend> backend);
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void Update();
    void Pause();
    void Resume();
    void Shutdown();

    void PlayMusic(const char16_t* name, bool looping, int fadeFrames = 0);
    void PlayMusic(int nameIndex, bool looping, int fadeFrames = 0);
    int PlaySound(const char16_t* name, bool looping = false);
    int PlaySound(int nameIndex, bool looping = false);
    int PlaySound(AudioData* data, bool looping = false);
    void StopMusic(int fadeFrames = 0);
    void StopAllSounds();
    void StopVoice(int voice);
    bool IsVoiceFinished(int voice);

    void SetGlobalVolume(int volume);
    void SetMusicVolume(int volume);
    void SetEffectsVolume(int volume);
    void SetVoiceVolume(int voice, int volume);
    void StartMusicFade(int fadeFrames, bool fadeIn);

    int AddSoundInstance(SoundInstance* instance);
    int CreateSoundInstance(AudioData* data, const Vec3& position, bool looping);
    int LoadSoundInstance(SoundInstance* instance, int nameIndex);
    void RemoveSoundInstance(int index);
    void ClearSoundInstances();
    void ScheduleSoundInstance(SoundInstance* instance);
    void SetListenerPosition(const Vec3& position);
    void SetSoundNameTable(const char16_t* names);

    s3dAudioDevice& GetDevice();
    bool IsRunning() const;

private:
    const char16_t* soundNameTable = nullptr;
    s3dAudioDevice device;
    int musicVolume = 255;
    int effectsVolume = 255;
    Fixed fadeCurrent = 0xff0000;
    Fixed fadeStep = 0;
    int fadeRemaining = 0;
    int fadeDuration = 0;
    int fadeIn = 0;
    s3dResManager* resourceManager;
    AudioData* musicData = nullptr;
    std::vector<SoundInstance*> soundInstances;
    std::array<SoundInstance*, 16> scheduledInstances{};
};

} // namespace soft3d

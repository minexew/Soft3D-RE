#include "soft3d/AudioManager.hpp"

#include "soft3d/AudioData.hpp"
#include "soft3d/SoundInstance.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <new>

namespace soft3d {

AudioManager::AudioManager(s3dResManager* resources, std::unique_ptr<AudioBackend> backend)
    : device(std::move(backend))
    , resourceManager(resources)
{
    device.SetMasterVolume(255);
    SetMusicVolume(255);
    SetEffectsVolume(255);
}

AudioManager::~AudioManager()
{
    Shutdown();
    ClearSoundInstances();
}

void AudioManager::Update()
{
    device.Update();
    for (int index = 0; index < static_cast<int>(soundInstances.size()); ++index) {
        SoundInstance* instance = soundInstances[index];
        instance->UpdateVolume();
        if (instance->voice != -1 && IsVoiceFinished(instance->voice)) {
            RemoveSoundInstance(index);
            --index;
        }
    }

    scheduledInstances.fill(nullptr);
    for (SoundInstance* instance : soundInstances) {
        ScheduleSoundInstance(instance);
    }

    if (device.IsVoiceFinished(0) || fadeDuration == 0) {
        return;
    }
    if (fadeRemaining == 0 && !device.GetVoiceLooping(0) && fadeIn != 0) {
        const int32_t fadeStart = static_cast<int32_t>(musicData->GetPcmSampleCount() -
            static_cast<uint32_t>(fadeDuration * s3dAudioDevice::sampleRate));
        if (fadeStart < 1) {
            return;
        }
        if (static_cast<int32_t>(device.GetVoicePosition(0)) >= fadeStart) {
            StopMusic(fadeDuration);
            musicData = nullptr;
        }
    }
    fadeCurrent = static_cast<Fixed>(static_cast<uint32_t>(fadeCurrent) +
                                     static_cast<uint32_t>(fadeStep));
    device.SetVoiceVolume(0, fadeCurrent >> 16);
    if (fadeRemaining != 0) {
        --fadeRemaining;
    }
}

void AudioManager::Pause()
{
    device.Pause();
}

void AudioManager::Resume()
{
    device.Resume();
}

void AudioManager::Shutdown()
{
    StopAllSounds();
    device.Stop();
}

void AudioManager::PlayMusic(const char16_t* name, bool looping, int fadeFrames)
{
    if (musicVolume == 0) {
        return;
    }
    musicData = static_cast<AudioData*>(resourceManager->GetResource(
        ResourceAudio, 3, name, u".\\audio\\"));
    if (musicData == nullptr) {
        return;
    }
    if (fadeFrames == 0) {
        device.StartVoice(musicData->GetPcmBuffer(), musicData->GetPcmSampleCount(),
                          0, musicVolume, looping);
    } else {
        StartMusicFade(fadeFrames, true);
        device.StartVoice(musicData->GetPcmBuffer(), musicData->GetPcmSampleCount(),
                          0, 0, looping);
    }
}

void AudioManager::PlayMusic(int nameIndex, bool looping, int fadeFrames)
{
    PlayMusic(soundNameTable + nameIndex * 64, looping, fadeFrames);
}

int AudioManager::PlaySound(const char16_t* name, bool looping)
{
    if (effectsVolume == 0) {
        return -1;
    }
    AudioData* data = static_cast<AudioData*>(resourceManager->GetResource(
        ResourceAudio, 3, name, u".\\audio\\"));
    return PlaySound(data, looping);
}

int AudioManager::PlaySound(int nameIndex, bool looping)
{
    return PlaySound(soundNameTable + nameIndex * 64, looping);
}

int AudioManager::PlaySound(AudioData* data, bool looping)
{
    if (effectsVolume == 0 || data == nullptr) {
        return -1;
    }
    for (int voice = 1; voice < 16; ++voice) {
        if (device.IsVoiceFinished(voice)) {
            device.StartVoice(data->GetPcmBuffer(), data->GetPcmSampleCount(),
                              voice, effectsVolume, looping);
            return voice;
        }
    }
    return -1;
}

void AudioManager::StopMusic(int fadeFrames)
{
    if (fadeFrames == 0) {
        device.StopVoice(0);
        musicData = nullptr;
    } else {
        StartMusicFade(fadeFrames, false);
    }
}

void AudioManager::StopAllSounds()
{
    device.StopAllVoices();
    ClearSoundInstances();
    scheduledInstances.fill(nullptr);
    musicData = nullptr;
}

void AudioManager::StopVoice(int voice)
{
    device.StopVoice(voice);
}

bool AudioManager::IsVoiceFinished(int voice)
{
    return device.IsVoiceFinished(voice);
}

void AudioManager::SetGlobalVolume(int volume)
{
    int master = volume;
    if (volume < 2) {
        musicVolume = 0;
        effectsVolume = 0;
        master = 0;
    } else {
        musicVolume = 255;
        effectsVolume = 255;
    }
    device.SetMasterVolume(std::clamp(master, 0, 255));
}

void AudioManager::SetMusicVolume(int volume)
{
    if (device.GetMasterVolume() > 1) {
        musicVolume = std::clamp(volume, 0, 255);
        device.SetVoiceVolume(0, musicVolume);
    }
}

void AudioManager::SetEffectsVolume(int volume)
{
    if (device.GetMasterVolume() > 1) {
        effectsVolume = std::clamp(volume, 0, 255);
        for (int voice = 1; voice < 16; ++voice) {
            device.SetVoiceVolume(voice, effectsVolume);
        }
    }
}

void AudioManager::SetVoiceVolume(int voice, int volume)
{
    if (device.GetMasterVolume() > 1) {
        device.SetVoiceVolume(voice, std::clamp(volume, 0, 255));
    }
}

void AudioManager::StartMusicFade(int fadeFrames, bool direction)
{
    fadeDuration = fadeFrames;
    fadeRemaining = fadeFrames;
    if (fadeDuration == 0) {
        return;
    }
    fadeIn = direction;
    if (!direction) {
        device.SetVoiceVolume(0, 255);
        fadeCurrent = 0xff0000;
        fadeStep = static_cast<Fixed>((int64_t{0xff010000} << 16) /
                                      (static_cast<int64_t>(fadeRemaining) << 16));
    } else {
        device.SetVoiceVolume(0, 0);
        fadeCurrent = 0;
        fadeStep = static_cast<Fixed>((int64_t{0xff0000} << 16) /
                                      (static_cast<int64_t>(fadeRemaining) << 16));
    }
}

int AudioManager::AddSoundInstance(SoundInstance* instance)
{
    if (effectsVolume == 0 || instance == nullptr) {
        return -1;
    }
    instance->listIndex = static_cast<int>(soundInstances.size());
    soundInstances.push_back(instance);
    return instance->listIndex;
}

int AudioManager::CreateSoundInstance(AudioData* data, const Vec3& position, bool looping)
{
    if (effectsVolume == 0) {
        return -1;
    }
    SoundInstance* instance = new (std::nothrow) SoundInstance();
    if (instance == nullptr) {
        return -1;
    }
    instance->Initialize(this, data, looping, true);
    instance->SetPosition(position);
    return AddSoundInstance(instance);
}

int AudioManager::LoadSoundInstance(SoundInstance* instance, int nameIndex)
{
    if (effectsVolume == 0 || instance == nullptr) {
        return -1;
    }
    instance->data = static_cast<AudioData*>(resourceManager->GetResource(
        ResourceAudio, 3, soundNameTable + nameIndex * 64, u".\\audio\\"));
    if (instance->data == nullptr) {
        return -1;
    }
    return AddSoundInstance(instance);
}

void AudioManager::RemoveSoundInstance(int index)
{
    if (index < 0 || index >= static_cast<int>(soundInstances.size())) {
        return;
    }
    SoundInstance* instance = soundInstances[index];
    instance->Stop();
    if (instance->owned != 0) {
        delete instance;
    }
    soundInstances.erase(soundInstances.begin() + index);
    for (int remaining = index; remaining < static_cast<int>(soundInstances.size()); ++remaining) {
        soundInstances[remaining]->listIndex = remaining;
    }
}

void AudioManager::ClearSoundInstances()
{
    for (SoundInstance* instance : soundInstances) {
        instance->Stop();
        if (instance->owned != 0) {
            delete instance;
        }
    }
    soundInstances.clear();
}

void AudioManager::ScheduleSoundInstance(SoundInstance* instance)
{
    for (int slot = 0; slot < 16; ++slot) {
        SoundInstance* scheduled = scheduledInstances[slot];
        if (scheduled == instance) {
            return;
        }
        if (scheduled == nullptr) {
            if (instance->volume != 0) {
                scheduledInstances[slot] = instance;
                instance->Activate();
                return;
            }
        } else if (instance->distance < scheduled->distance) {
            scheduledInstances[slot] = instance;
            instance->Activate();
            if (scheduled->IsActive()) {
                ScheduleSoundInstance(scheduled);
            }
            return;
        }
    }
    instance->Stop();
}

void AudioManager::SetListenerPosition(const Vec3& position)
{
    for (SoundInstance* instance : soundInstances) {
        instance->SetListenerPosition(position);
    }
}

void AudioManager::SetSoundNameTable(const char16_t* names)
{
    soundNameTable = names;
}

s3dAudioDevice& AudioManager::GetDevice()
{
    return device;
}

bool AudioManager::IsRunning() const
{
    return device.IsRunning();
}

} // namespace soft3d

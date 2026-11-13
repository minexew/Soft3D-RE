#include "soft3d/AudioDevice.hpp"

#include <algorithm>
#include <chrono>

namespace soft3d {

s3dAudioDevice::s3dAudioDevice(std::unique_ptr<AudioBackend> audioBackend)
    : backend(std::move(audioBackend))
{
    finished.fill(1);
    if (backend != nullptr) {
        Start();
    }
}

s3dAudioDevice::~s3dAudioDevice()
{
    Stop();
}

bool s3dAudioDevice::Start()
{
    if (worker.joinable()) {
        return streamOpen;
    }
    if (backend == nullptr) {
        return false;
    }
    if (!backend->Start(sampleRate, 1)) {
        return false;
    }
    command = 0;
    active = true;
    streamOpen = true;
    worker = std::thread(&s3dAudioDevice::Run, this);
    return true;
}

void s3dAudioDevice::Stop()
{
    {
        std::lock_guard<std::mutex> lock(commandMutex);
        command = 2;
    }
    commandChanged.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
    if (backend != nullptr) {
        backend->Stop();
    }
    streamOpen = false;
    active = false;
}

void s3dAudioDevice::Pause()
{
    if (active.exchange(false)) {
        {
            std::lock_guard<std::mutex> lock(commandMutex);
            command = 1;
        }
        commandChanged.notify_all();
    }
}

void s3dAudioDevice::Resume()
{
    if (!active.exchange(true)) {
        {
            std::lock_guard<std::mutex> lock(commandMutex);
            command = 0;
        }
        commandChanged.notify_all();
    }
}

void s3dAudioDevice::Update()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (streamError != 0 && recoveryCountdown > 0) {
        --recoveryCountdown;
        if (recoveryCountdown == 0) {
            Resume();
        }
        if (recoveryCountdown == 45) {
            Pause();
        }
    }
}

int s3dAudioDevice::VolumeToShift(int volume)
{
    ++volume;
    int shift = 8;
    while ((((volume >> shift) ^ 1) & 1) != 0 || shift < 0) {
        --shift;
    }
    return shift;
}

void s3dAudioDevice::StartVoice(const int16_t* source, uint32_t sampleCount, int voice,
                                int volume, bool repeat)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    pcm[voice] = source;
    pcmSampleCounts[voice] = sampleCount;
    positions[voice] = 0;
    looping[voice] = repeat;
    finished[voice] = 0;
    volumes[voice] = directVolume[voice] == 0 ? VolumeToShift(volume) : volume;
}

void s3dAudioDevice::StopVoice(int voice)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    pcm[voice] = nullptr;
    pcmSampleCounts[voice] = 0;
    positions[voice] = 0;
    finished[voice] = 1;
    directVolume[voice] = 0;
}

void s3dAudioDevice::StopAllVoices()
{
    for (int voice = 0; voice < voiceCount; ++voice) {
        StopVoice(voice);
    }
}

bool s3dAudioDevice::IsVoiceFinished(int voice) const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return finished[voice] != 0;
}

void s3dAudioDevice::SetVoiceLooping(int voice, bool repeat)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    looping[voice] = repeat;
}

bool s3dAudioDevice::GetVoiceLooping(int voice) const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return looping[voice] != 0;
}

void s3dAudioDevice::SetVoiceVolume(int voice, int volume)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    volumes[voice] = directVolume[voice] == 0 ? VolumeToShift(volume) : volume;
}

int s3dAudioDevice::GetVoiceVolume(int voice) const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return directVolume[voice] == 0 ? volumes[voice] << 5 : volumes[voice];
}

void s3dAudioDevice::SetVoiceVolumeMode(int voice, bool direct)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    directVolume[voice] = direct;
}

void s3dAudioDevice::SetMasterVolume(int volume)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    masterShift = VolumeToShift(volume);
}

int s3dAudioDevice::GetMasterVolume() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return 1 << masterShift;
}

uint32_t s3dAudioDevice::GetVoicePosition(int voice) const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return positions[voice];
}

const std::array<int16_t, s3dAudioDevice::mixSampleCount>& s3dAudioDevice::MixBlock()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    const int outputShift = masterShift;
    accumulation.fill(0);

    for (int voice = 0; voice < voiceCount; ++voice) {
        if (finished[voice] != 0 || pcm[voice] == nullptr || volumes[voice] == 0) {
            continue;
        }
        const int16_t* source = pcm[voice];
        uint32_t position = positions[voice];
        const uint32_t sampleCount = pcmSampleCounts[voice];
        int volume = volumes[voice];
        const bool direct = directVolume[voice] == 1;

        if (direct) {
            volume = std::clamp(volume, 0, 255);
        } else {
            volume = std::clamp(volume, 0, 8);
        }

        for (int outputIndex = 0; outputIndex < mixSampleCount; ++outputIndex) {
            const int32_t sample = source[position++];
            const int32_t scaled = direct
                ? sample * volume
                : static_cast<int32_t>(static_cast<uint32_t>(sample) *
                                       (uint32_t{1} << static_cast<unsigned>(volume)));
            accumulation[outputIndex] = static_cast<int32_t>(
                static_cast<uint32_t>(accumulation[outputIndex]) + static_cast<uint32_t>(scaled));

            if (position >= sampleCount) {
                if (looping[voice] != 0) {
                    position = 0;
                } else {
                    pcm[voice] = nullptr;
                    pcmSampleCounts[voice] = 0;
                    finished[voice] = 1;
                    directVolume[voice] = 0;
                    position = 0;
                    break;
                }
            }
        }
        positions[voice] = position;
    }

    const int clampedOutputShift = std::min(outputShift, 8);
    for (int index = 0; index < mixSampleCount; ++index) {
        int32_t sample = static_cast<int32_t>(
            static_cast<uint32_t>(accumulation[index] >> 8) <<
            static_cast<unsigned>(clampedOutputShift));
        sample >>= 8;
        sample = std::clamp(sample, -32768, 32767);
        output[index] = static_cast<int16_t>(sample);
    }
    return output;
}

bool s3dAudioDevice::IsRunning() const
{
    return streamOpen;
}

void s3dAudioDevice::Run()
{
    std::unique_lock<std::mutex> lock(commandMutex);
    while (command != 2) {
        if (command == 1) {
            if (streamOpen.exchange(false)) {
                backend->Stop();
            }
            commandChanged.wait(lock);
            continue;
        }
        if (!streamOpen) {
            lock.unlock();
            const bool started = backend->Start(sampleRate, 1);
            streamOpen = started;
            lock.lock();
            if (!started) {
                commandChanged.wait(lock);
                continue;
            }
        }

        lock.unlock();
        SubmitBlock();
        lock.lock();
        if (command == 0) {
            commandChanged.wait_for(lock, std::chrono::milliseconds(25));
        }
    }
}

void s3dAudioDevice::SubmitBlock()
{
    const auto& samples = MixBlock();
    if (streamOpen && !backend->Submit(samples.data(), samples.size())) {
        RecordStreamError();
    }
}

void s3dAudioDevice::RecordStreamError()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    streamError = 1;
    if (recoveryCountdown == 0) {
        recoveryCountdown = 90;
    }
}

} // namespace soft3d

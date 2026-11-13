#include "platform/dummy/DummyAudioBackend.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/AudioDevice.hpp"
#include "soft3d/AudioManager.hpp"
#include "soft3d/SoundInstance.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void TestDirectMixingAndCompletion()
{
    soft3d::s3dAudioDevice device(nullptr);
    device.SetMasterVolume(255);
    device.SetVoiceVolumeMode(0, true);
    const std::array<int16_t, 4> pcm = {1000, -1000, 32767, -32768};
    device.StartVoice(pcm.data(), pcm.size(), 0, 128, false);
    CHECK(!device.IsVoiceFinished(0));
    CHECK(device.GetVoiceVolume(0) == 128);

    const auto& mixed = device.MixBlock();
    CHECK(mixed[0] == 500);
    CHECK(mixed[1] == -500);
    CHECK(mixed[2] == 16383);
    CHECK(mixed[3] == -16384);
    CHECK(std::all_of(mixed.begin() + 4, mixed.end(), [](int16_t sample) {
        return sample == 0;
    }));
    CHECK(device.IsVoiceFinished(0));
    CHECK(device.GetVoicePosition(0) == 0);
    CHECK(device.GetVoiceVolume(0) == 4096);

    device.StartVoice(pcm.data(), pcm.size(), 0, 255, false);
    CHECK(device.GetVoiceVolume(0) == 256);
    CHECK(device.MixBlock()[0] == 1000);
}

void TestShiftMixingLoopingAndSaturation()
{
    soft3d::s3dAudioDevice device(nullptr);
    device.SetMasterVolume(255);
    const std::array<int16_t, 4> pcm = {1, 2, 3, 4};
    device.StartVoice(pcm.data(), pcm.size(), 0, 127, false);
    const auto& shifted = device.MixBlock();
    CHECK(shifted[0] == 0);
    CHECK(shifted[1] == 1);
    CHECK(shifted[2] == 1);
    CHECK(shifted[3] == 2);

    const std::array<int16_t, 2> loopingPcm = {5, -5};
    device.StartVoice(loopingPcm.data(), loopingPcm.size(), 0, 255, true);
    const auto& looping = device.MixBlock();
    for (int index = 0; index < 6; ++index) {
        CHECK(looping[index] == (index % 2 == 0 ? 5 : -5));
    }
    CHECK(!device.IsVoiceFinished(0));
    CHECK(device.GetVoicePosition(0) == 0);
    CHECK(device.GetVoiceLooping(0));
    device.SetVoiceLooping(0, false);
    device.StopVoice(0);
    CHECK(device.IsVoiceFinished(0));
    CHECK(!device.GetVoiceLooping(0));

    std::array<int16_t, 400> loud{};
    loud.fill(32767);
    for (int voice = 0; voice < 16; ++voice) {
        device.SetVoiceVolumeMode(voice, true);
        device.StartVoice(loud.data(), loud.size(), voice, 255, false);
    }
    const auto& saturated = device.MixBlock();
    CHECK(std::all_of(saturated.begin(), saturated.end(), [](int16_t sample) {
        return sample == 32767;
    }));

    loud.fill(-32768);
    for (int voice = 0; voice < 16; ++voice) {
        device.SetVoiceVolumeMode(voice, true);
        device.StartVoice(loud.data(), loud.size(), voice, 255, false);
    }
    const auto& negative = device.MixBlock();
    CHECK(std::all_of(negative.begin(), negative.end(), [](int16_t sample) {
        return sample == -32768;
    }));

    device.StartVoice(pcm.data(), pcm.size(), 0, 0, false);
    device.MixBlock();
    CHECK(!device.IsVoiceFinished(0));
    CHECK(device.GetVoicePosition(0) == 0);
    device.SetMasterVolume(127);
    CHECK(device.GetMasterVolume() == 128);
    device.StartVoice(pcm.data(), pcm.size(), 0, 255, false);
    CHECK(device.MixBlock()[1] == 1);
}

void TestFixedPointDistance()
{
    const soft3d::Vec3 origin{};
    CHECK(soft3d::CalculateFixedPointDistance(origin, origin) == 0);
    CHECK(soft3d::CalculateFixedPointDistance(origin, {3 << 16, 4 << 16, 0}) == 5 << 16);
    CHECK(soft3d::CalculateFixedPointDistance(origin, {10 << 16, 0, 0}) == 10 << 16);
    CHECK(soft3d::CalculateFixedPointDistance(origin, {1000 << 16, 0, 0}) == 1000 << 16);
}

void TestManagerWithRealAudio()
{
    soft3d::s3dResManager resources;
    const std::filesystem::path archive = SOFT3D_TEST_ARCHIVE;
    CHECK(soft3d::OpenGameResourceArchive(&resources, archive.u16string().c_str()) == soft3d::S3D_OK);
    soft3d::AudioManager manager(&resources, nullptr);

    manager.PlayMusic(u"7DaysPiano03", false);
    CHECK(!manager.IsVoiceFinished(0));
    bool audible = false;
    for (int block = 0; block < 10 && !audible; ++block) {
        const auto& mixed = manager.GetDevice().MixBlock();
        audible = std::any_of(mixed.begin(), mixed.end(), [](int16_t sample) {
            return sample != 0;
        });
    }
    CHECK(audible);

    auto* data = static_cast<soft3d::AudioData*>(resources.GetResource(
        soft3d::ResourceAudio, 3, u"7DaysPiano03", u".\\audio\\"));
    CHECK(data != nullptr);
    for (int voice = 1; voice < 16; ++voice) {
        CHECK(manager.PlaySound(data) == voice);
    }
    CHECK(manager.PlaySound(data) == -1);
    manager.StopVoice(7);
    CHECK(manager.PlaySound(data) == 7);
    manager.StopAllSounds();
    for (int voice = 0; voice < 16; ++voice) {
        CHECK(manager.IsVoiceFinished(voice));
    }

    manager.PlayMusic(u"7DaysPiano03", false, 4);
    const std::array<int, 5> fadeVolumes = {0, 192, 224, 224, 256};
    CHECK(manager.GetDevice().GetVoiceVolume(0) == fadeVolumes[0]);
    for (std::size_t index = 1; index < fadeVolumes.size(); ++index) {
        manager.Update();
        CHECK(manager.GetDevice().GetVoiceVolume(0) == fadeVolumes[index]);
    }
    manager.StopMusic(2);
    for (int update = 0; update < 4; ++update) {
        manager.Update();
    }
    CHECK(!manager.IsVoiceFinished(0));
    manager.StopMusic();
    CHECK(manager.IsVoiceFinished(0));

    manager.SetGlobalVolume(0);
    manager.PlayMusic(u"7DaysPiano03", false);
    CHECK(manager.IsVoiceFinished(0));
    CHECK(manager.PlaySound(data) == -1);
    manager.SetGlobalVolume(255);
    CHECK(manager.GetDevice().GetMasterVolume() == 256);
}

void TestPositionalSoundWithRealAudio()
{
    soft3d::s3dResManager resources;
    const std::filesystem::path archive = SOFT3D_TEST_ARCHIVE;
    CHECK(soft3d::OpenGameResourceArchive(&resources, archive.u16string().c_str()) == soft3d::S3D_OK);
    auto* data = static_cast<soft3d::AudioData*>(resources.GetResource(
        soft3d::ResourceAudio, 3, u"7DaysPiano03", u".\\audio\\"));
    CHECK(data != nullptr);

    soft3d::AudioManager manager(&resources, nullptr);
    const int instanceIndex = manager.CreateSoundInstance(data, {}, false);
    CHECK(instanceIndex == 0);
    manager.Update();
    CHECK(!manager.IsVoiceFinished(1));

    manager.SetListenerPosition({10 << 16, 0, 0});
    manager.Update();
    CHECK(!manager.IsVoiceFinished(1));
    manager.SetListenerPosition({100 << 16, 0, 0});
    manager.Update();
    CHECK(manager.IsVoiceFinished(1));

    soft3d::SoundInstance external;
    external.Initialize(&manager, data, false, false);
    external.SetPosition({3 << 16, 4 << 16, 0});
    external.CalculateSpatialVolume();
    CHECK(external.GetDistance() == 5 << 16);
    CHECK(external.GetVolume() == 255);
    CHECK(manager.AddSoundInstance(&external) == 1);
    manager.Update();
    CHECK(external.IsActive());
    manager.ClearSoundInstances();
    CHECK(!external.IsActive());
}

bool WaitFor(const std::function<bool()>& predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

void TestThreadedBackendLifecycle()
{
    auto backend = std::make_unique<soft3d::platform::DummyAudioBackend>();
    auto* dummy = backend.get();
    soft3d::s3dAudioDevice device(std::move(backend));
    CHECK(device.IsRunning());
    CHECK(dummy->GetStartCount() == 1);

    std::array<int16_t, 400> pcm{};
    pcm.fill(123);
    device.SetMasterVolume(255);
    device.StartVoice(pcm.data(), pcm.size(), 0, 255, true);
    CHECK(WaitFor([&] {
        const auto block = dummy->GetLastBlock();
        return dummy->GetSubmittedSampleCount() >= 800 && !block.empty() && block[0] == 123;
    }));

    device.Pause();
    CHECK(WaitFor([&] { return !dummy->IsRunning(); }));
    CHECK(dummy->GetStopCount() == 1);
    device.Pause();
    CHECK(dummy->GetStopCount() == 1);
    device.Resume();
    CHECK(WaitFor([&] { return dummy->IsRunning(); }));
    CHECK(dummy->GetStartCount() == 2);
    device.Stop();
    CHECK(!device.IsRunning());
    CHECK(!dummy->IsRunning());
}

void TestBackendStartFailure()
{
    auto backend = std::make_unique<soft3d::platform::DummyAudioBackend>();
    auto* dummy = backend.get();
    dummy->SetStartResult(false);
    soft3d::s3dAudioDevice device(std::move(backend));
    CHECK(!device.IsRunning());
    CHECK(dummy->GetStartCount() == 1);
    dummy->SetStartResult(true);
    CHECK(device.Start());
    CHECK(dummy->GetStartCount() == 2);
}

} // namespace

int main()
{
    try {
        TestDirectMixingAndCompletion();
        TestShiftMixingLoopingAndSaturation();
        TestFixedPointDistance();
        TestManagerWithRealAudio();
        TestPositionalSoundWithRealAudio();
        TestThreadedBackendLifecycle();
        TestBackendStartFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

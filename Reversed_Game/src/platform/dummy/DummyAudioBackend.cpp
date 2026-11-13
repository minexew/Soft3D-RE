#include "platform/dummy/DummyAudioBackend.hpp"

namespace soft3d::platform {

bool DummyAudioBackend::Start(int, int)
{
    std::lock_guard<std::mutex> lock(mutex);
    ++startCount;
    running = startResult;
    return running;
}

void DummyAudioBackend::Stop()
{
    std::lock_guard<std::mutex> lock(mutex);
    ++stopCount;
    running = false;
}

bool DummyAudioBackend::Submit(const int16_t* samples, std::size_t sampleCount)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!running || !submitResult) {
        return false;
    }
    lastBlock.assign(samples, samples + sampleCount);
    submittedSamples += sampleCount;
    return true;
}

void DummyAudioBackend::SetStartResult(bool result)
{
    std::lock_guard<std::mutex> lock(mutex);
    startResult = result;
}

void DummyAudioBackend::SetSubmitResult(bool result)
{
    std::lock_guard<std::mutex> lock(mutex);
    submitResult = result;
}

bool DummyAudioBackend::IsRunning() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

int DummyAudioBackend::GetStartCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return startCount;
}

int DummyAudioBackend::GetStopCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return stopCount;
}

uint64_t DummyAudioBackend::GetSubmittedSampleCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return submittedSamples;
}

std::vector<int16_t> DummyAudioBackend::GetLastBlock() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return lastBlock;
}

} // namespace soft3d::platform

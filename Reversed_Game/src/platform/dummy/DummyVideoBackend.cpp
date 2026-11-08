#include "platform/dummy/DummyVideoBackend.hpp"

#include <cstddef>

namespace soft3d::platform {

bool DummyVideoBackend::Start(int, int)
{
    ++startCount;
    started = startResult;
    return started;
}

void DummyVideoBackend::Stop()
{
    ++stopCount;
    started = false;
}

int DummyVideoBackend::BeginUpdate(VideoBufferInfo&)
{
    ++beginUpdateCount;
    return beginUpdateError;
}

bool DummyVideoBackend::EndUpdate(const VideoBufferInfo& buffer)
{
    ++updateCount;
    if (!updateResult) {
        return false;
    }
    presentedPixels.resize(static_cast<std::size_t>(buffer.width) *
                           static_cast<std::size_t>(buffer.height));
    for (int y = 0; y < buffer.height; ++y) {
        const auto* source = reinterpret_cast<const uint8_t*>(buffer.address) + y * buffer.pitch;
        for (int x = 0; x < buffer.width; ++x) {
            presentedPixels[static_cast<std::size_t>(y) * buffer.width + x] =
                reinterpret_cast<const uint32_t*>(source)[x];
        }
    }
    return true;
}

const char* DummyVideoBackend::GetName() const
{
    return "dummy";
}

void DummyVideoBackend::SetStartResult(bool result)
{
    startResult = result;
}

void DummyVideoBackend::SetBeginUpdateError(int error)
{
    beginUpdateError = error;
}

void DummyVideoBackend::SetUpdateResult(bool result)
{
    updateResult = result;
}

bool DummyVideoBackend::IsStarted() const
{
    return started;
}

int DummyVideoBackend::GetStartCount() const
{
    return startCount;
}

int DummyVideoBackend::GetStopCount() const
{
    return stopCount;
}

int DummyVideoBackend::GetBeginUpdateCount() const
{
    return beginUpdateCount;
}

int DummyVideoBackend::GetUpdateCount() const
{
    return updateCount;
}

const std::vector<uint32_t>& DummyVideoBackend::GetPresentedPixels() const
{
    return presentedPixels;
}

} // namespace soft3d::platform

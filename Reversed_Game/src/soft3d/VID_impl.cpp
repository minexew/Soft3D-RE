#include "soft3d/VID_impl.hpp"

#include "soft3d/s3dLog.hpp"

#include <cstddef>
#include <utility>

namespace soft3d {

VID_impl::VID_impl(int width, int height, std::unique_ptr<VideoBackend> videoBackend)
    : backBuffer(static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
    , backend(std::move(videoBackend))
{
    bufferInfo.address = backBuffer.data();
    bufferInfo.width = width;
    bufferInfo.height = height;
    bufferInfo.pitch = width * static_cast<int>(sizeof(uint32_t));
    backend->Start(bufferInfo.width, bufferInfo.height);
}

bool VID_impl::Start()
{
    if (!started) {
        started = backend->Start(bufferInfo.width, bufferInfo.height);
    }
    return started;
}

void VID_impl::Stop()
{
    if (started) {
        backend->Stop();
        started = false;
    }
}

uint32_t* VID_impl::GetBackBufferPtr()
{
    if (!bufferReady) {
        const int error = backend->BeginUpdate(bufferInfo);
        if (error != 0) {
            NullLog(u"GetBackBufferPtr Error Code: %d", error);
        }
    }
    backBufferPtr = bufferInfo.address;
    return backBufferPtr;
}

bool VID_impl::Update()
{
    const bool updated = backend->EndUpdate(bufferInfo);
    bufferReady = false;
    return updated;
}

bool VID_impl::IsStarted() const
{
    return started;
}

const VideoBufferInfo& VID_impl::GetBufferInfo() const
{
    return bufferInfo;
}

const char* VID_impl::GetBackendName() const
{
    return backend->GetName();
}

} // namespace soft3d

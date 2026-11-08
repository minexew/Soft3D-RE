#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace soft3d {

struct VideoBufferInfo {
    uint32_t* address = nullptr;
    int width = 0;
    int height = 0;
    int pitch = 0;
    int pixelShift = 2;
};

class VideoBackend {
public:
    virtual ~VideoBackend() = default;

    virtual bool Start(int width, int height) = 0;
    virtual void Stop() = 0;
    virtual int BeginUpdate(VideoBufferInfo& buffer) = 0;
    virtual bool EndUpdate(const VideoBufferInfo& buffer) = 0;
    virtual const char* GetName() const = 0;
};

class VID_impl {
public:
    VID_impl(int width, int height, std::unique_ptr<VideoBackend> backend);
    ~VID_impl() = default;

    VID_impl(const VID_impl&) = delete;
    VID_impl& operator=(const VID_impl&) = delete;

    bool Start();
    void Stop();
    uint32_t* GetBackBufferPtr();
    bool Update();

    bool IsStarted() const;
    const VideoBufferInfo& GetBufferInfo() const;
    const char* GetBackendName() const;

private:
    bool started = false;
    bool bufferReady = false;
    uint32_t* backBufferPtr = nullptr;
    std::vector<uint32_t> backBuffer;
    VideoBufferInfo bufferInfo;
    std::unique_ptr<VideoBackend> backend;
};

} // namespace soft3d

#pragma once

#include "soft3d/VID_impl.hpp"

#include <cstdint>
#include <vector>

namespace soft3d::platform {

class DummyVideoBackend final : public VideoBackend {
public:
    bool Start(int width, int height) override;
    void Stop() override;
    int BeginUpdate(VideoBufferInfo& buffer) override;
    bool EndUpdate(const VideoBufferInfo& buffer) override;
    const char* GetName() const override;

    void SetStartResult(bool result);
    void SetBeginUpdateError(int error);
    void SetUpdateResult(bool result);

    bool IsStarted() const;
    int GetStartCount() const;
    int GetStopCount() const;
    int GetBeginUpdateCount() const;
    int GetUpdateCount() const;
    const std::vector<uint32_t>& GetPresentedPixels() const;

private:
    bool started = false;
    bool startResult = true;
    bool updateResult = true;
    int beginUpdateError = 0;
    int startCount = 0;
    int stopCount = 0;
    int beginUpdateCount = 0;
    int updateCount = 0;
    std::vector<uint32_t> presentedPixels;
};

} // namespace soft3d::platform

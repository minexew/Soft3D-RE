#pragma once

#include "soft3d/VID_impl.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace soft3d::platform {

struct DummyDrawnText {
    std::u16string text;
    int x;
    int y;
};

class DummyVideoBackend final : public VideoBackend {
public:
    bool Start(int width, int height) override;
    void Stop() override;
    int BeginUpdate(VideoBufferInfo& buffer) override;
    bool EndUpdate(const VideoBufferInfo& buffer) override;
    void DrawText(std::u16string_view text, int x, int y) override;
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
    const std::vector<DummyDrawnText>& GetPresentedText() const;

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
    std::vector<DummyDrawnText> pendingText;
    std::vector<DummyDrawnText> presentedText;
};

} // namespace soft3d::platform

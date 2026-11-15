#pragma once

#include "soft3d/VID_impl.hpp"

#include <SDL3/SDL.h>

#include <functional>
#include <string>
#include <vector>

namespace soft3d::platform {

struct SdlDrawTextRequest {
    std::string text;
    float x;
    float y;
};

void RestoreLandscapeFrame(const uint32_t* source, int width, int height,
                           int controlOrientation, uint32_t* destination);

class SdlVideoBackend final : public VideoBackend {
public:
    using ControlOrientationSource = std::function<int()>;
    using LandscapeSource = std::function<bool()>;

    SdlVideoBackend(int scale, std::string renderer, bool landscape = false);
    ~SdlVideoBackend() override;

    void SetControlOrientationSource(ControlOrientationSource source);
    void SetLandscapeSource(LandscapeSource source);
    bool Start(int width, int height) override;
    void Stop() override;
    int BeginUpdate(VideoBufferInfo& buffer) override;
    bool EndUpdate(const VideoBufferInfo& buffer) override;
    void DrawText(std::u16string_view text, int x, int y) override;
    const char* GetName() const override;

private:
    bool ConfigureDisplay(bool landscapeDisplay);

    int scale;
    int presentationWidth = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    std::string requestedRenderer;
    bool landscape;
    ControlOrientationSource controlOrientationSource;
    LandscapeSource landscapeSource;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    std::vector<SdlDrawTextRequest> pendingText;
    std::vector<uint32_t> landscapeFrame;
};

} // namespace soft3d::platform

#pragma once

#include "soft3d/VID_impl.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace soft3d::platform {

struct SdlDrawTextRequest {
    std::string text;
    float x;
    float y;
};

class SdlVideoBackend final : public VideoBackend {
public:
    SdlVideoBackend(int scale, std::string renderer);
    ~SdlVideoBackend() override;

    bool Start(int width, int height) override;
    void Stop() override;
    int BeginUpdate(VideoBufferInfo& buffer) override;
    bool EndUpdate(const VideoBufferInfo& buffer) override;
    void DrawText(std::u16string_view text, int x, int y) override;
    const char* GetName() const override;

private:
    int scale;
    int presentationWidth = 0;
    int sourceWidth = 0;
    std::string requestedRenderer;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    std::vector<SdlDrawTextRequest> pendingText;
};

} // namespace soft3d::platform

#pragma once

#include "soft3d/VID_impl.hpp"

#include <SDL3/SDL.h>

#include <string>

namespace soft3d::platform {

class SdlVideoBackend final : public VideoBackend {
public:
    SdlVideoBackend(int scale, std::string renderer);
    ~SdlVideoBackend() override;

    bool Start(int width, int height) override;
    void Stop() override;
    int BeginUpdate(VideoBufferInfo& buffer) override;
    bool EndUpdate(const VideoBufferInfo& buffer) override;
    const char* GetName() const override;

private:
    int scale;
    int presentationWidth = 0;
    std::string requestedRenderer;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
};

} // namespace soft3d::platform

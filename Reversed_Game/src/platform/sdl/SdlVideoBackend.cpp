#include "platform/sdl/SdlVideoBackend.hpp"

#include <iostream>
#include <utility>

namespace soft3d::platform {

SdlVideoBackend::SdlVideoBackend(int windowScale, std::string rendererName)
    : scale(windowScale)
    , requestedRenderer(std::move(rendererName))
{
}

SdlVideoBackend::~SdlVideoBackend()
{
    Stop();
}

bool SdlVideoBackend::Start(int width, int height)
{
    if (frameTexture != nullptr) {
        return true;
    }
    window = SDL_CreateWindow("7 Days: Salvation - Soft3D reconstruction",
                              width * scale, height * scale, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        return false;
    }

    const char* requested = requestedRenderer == "auto" ? nullptr : requestedRenderer.c_str();
    renderer = SDL_CreateRenderer(window, requested);
    if (renderer == nullptr && requestedRenderer != "software") {
        std::cerr << "Requested renderer unavailable; trying SDL software renderer: "
                  << SDL_GetError() << '\n';
        renderer = SDL_CreateRenderer(window, "software");
    }
    if (renderer == nullptr) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        Stop();
        return false;
    }

    frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (frameTexture == nullptr) {
        std::cerr << "Framebuffer texture creation failed: " << SDL_GetError() << '\n';
        Stop();
        return false;
    }
    SDL_SetTextureScaleMode(frameTexture, SDL_SCALEMODE_NEAREST);
    if (!SDL_SetRenderLogicalPresentation(renderer, width, height,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        Stop();
        return false;
    }
    return true;
}

void SdlVideoBackend::Stop()
{
    if (frameTexture != nullptr) {
        SDL_DestroyTexture(frameTexture);
        frameTexture = nullptr;
    }
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

int SdlVideoBackend::BeginUpdate(VideoBufferInfo&)
{
    return frameTexture != nullptr ? 0 : -1;
}

bool SdlVideoBackend::EndUpdate(const VideoBufferInfo& buffer)
{
    return SDL_UpdateTexture(frameTexture, nullptr, buffer.address, buffer.pitch) &&
           SDL_RenderClear(renderer) &&
           SDL_RenderTexture(renderer, frameTexture, nullptr, nullptr) &&
           SDL_RenderPresent(renderer);
}

const char* SdlVideoBackend::GetName() const
{
    return renderer != nullptr ? SDL_GetRendererName(renderer) : "not started";
}

} // namespace soft3d::platform

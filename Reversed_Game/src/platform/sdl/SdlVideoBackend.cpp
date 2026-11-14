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
    presentationWidth = height;
    sourceWidth = width;
    window = SDL_CreateWindow("7 Days: Salvation - Soft3D reconstruction",
                              height * scale, width * scale, SDL_WINDOW_RESIZABLE);
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
                                     SDL_TEXTUREACCESS_STREAMING, height, width);
    if (frameTexture == nullptr) {
        std::cerr << "Framebuffer texture creation failed: " << SDL_GetError() << '\n';
        Stop();
        return false;
    }
    SDL_SetTextureScaleMode(frameTexture, SDL_SCALEMODE_NEAREST);
    if (!SDL_SetRenderLogicalPresentation(renderer, height, width,
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
    bool updated = SDL_UpdateTexture(frameTexture, nullptr, buffer.address,
                                     presentationWidth * static_cast<int>(sizeof(uint32_t))) &&
                   SDL_RenderClear(renderer) &&
                   SDL_RenderTexture(renderer, frameTexture, nullptr, nullptr);
    if (!updated) {
        pendingText.clear();
        return false;
    }
    for (const SdlDrawTextRequest& text : pendingText) {
        updated = SDL_RenderDebugText(renderer, text.x, text.y, text.text.c_str()) && updated;
    }
    pendingText.clear();
    return SDL_RenderPresent(renderer) && updated;
}

void SdlVideoBackend::DrawText(std::u16string_view text, int x, int y)
{
    std::string narrow;
    narrow.reserve(text.size());
    for (char16_t character : text) {
        narrow.push_back(character <= 0x7f ? static_cast<char>(character) : '?');
    }
    pendingText.push_back({std::move(narrow), static_cast<float>(sourceWidth - 1 - x),
                           static_cast<float>(y)});
}

const char* SdlVideoBackend::GetName() const
{
    return renderer != nullptr ? SDL_GetRendererName(renderer) : "not started";
}

} // namespace soft3d::platform

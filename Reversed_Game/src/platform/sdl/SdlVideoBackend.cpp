#include "platform/sdl/SdlVideoBackend.hpp"

#include <iostream>
#include <utility>

namespace soft3d::platform {

void RestoreLandscapeFrame(const uint32_t* source, int width, int height,
                           int controlOrientation, uint32_t* destination)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int sourceIndex = controlOrientation == 2
                ? height * x + (height - 1 - y)
                : height * (width - 1 - x) + y;
            destination[y * width + x] = source[sourceIndex];
        }
    }
}

SdlVideoBackend::SdlVideoBackend(int windowScale, std::string rendererName, bool landscapeDisplay)
    : scale(windowScale)
    , requestedRenderer(std::move(rendererName))
    , landscape(landscapeDisplay)
{
}

SdlVideoBackend::~SdlVideoBackend()
{
    Stop();
}

void SdlVideoBackend::SetControlOrientationSource(ControlOrientationSource source)
{
    controlOrientationSource = std::move(source);
}

void SdlVideoBackend::SetLandscapeSource(LandscapeSource source)
{
    landscapeSource = std::move(source);
}

bool SdlVideoBackend::Start(int width, int height)
{
    if (frameTexture != nullptr) {
        return true;
    }
    presentationWidth = height;
    sourceWidth = width;
    sourceHeight = height;
    const int displayWidth = landscape ? width : height;
    const int displayHeight = landscape ? height : width;
    window = SDL_CreateWindow("7 Days: Salvation - Soft3D reconstruction",
                              displayWidth * scale, displayHeight * scale, SDL_WINDOW_RESIZABLE);
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
    if (!ConfigureDisplay(landscape)) {
        Stop();
        return false;
    }
    return true;
}

bool SdlVideoBackend::ConfigureDisplay(bool landscapeDisplay)
{
    const int displayWidth = landscapeDisplay ? sourceWidth : sourceHeight;
    const int displayHeight = landscapeDisplay ? sourceHeight : sourceWidth;
    SDL_Texture* newTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                displayWidth, displayHeight);
    if (newTexture == nullptr) {
        std::cerr << "Framebuffer texture creation failed: " << SDL_GetError() << '\n';
        return false;
    }
    if (frameTexture != nullptr) {
        SDL_DestroyTexture(frameTexture);
    }
    frameTexture = newTexture;
    landscape = landscapeDisplay;
    SDL_SetTextureScaleMode(frameTexture, SDL_SCALEMODE_NEAREST);
    SDL_SetWindowSize(window, displayWidth * scale, displayHeight * scale);
    landscapeFrame.resize(landscape ? static_cast<std::size_t>(sourceWidth) * sourceHeight : 0);
    return SDL_SetRenderLogicalPresentation(renderer, displayWidth, displayHeight,
                                            SDL_LOGICAL_PRESENTATION_LETTERBOX);
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
    if (landscapeSource && landscapeSource() != landscape && !ConfigureDisplay(!landscape)) {
        pendingText.clear();
        return false;
    }
    const uint32_t* pixels = buffer.address;
    int uploadWidth = presentationWidth;
    if (landscape) {
        const int controlOrientation = controlOrientationSource ? controlOrientationSource() : 1;
        RestoreLandscapeFrame(buffer.address, sourceWidth, sourceHeight,
                              controlOrientation, landscapeFrame.data());
        pixels = landscapeFrame.data();
        uploadWidth = sourceWidth;
    }
    bool updated = SDL_UpdateTexture(frameTexture, nullptr, pixels,
                                     uploadWidth * static_cast<int>(sizeof(uint32_t))) &&
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
    pendingText.push_back({std::move(narrow),
                           static_cast<float>(landscape ? x : sourceWidth - 1 - x),
                           static_cast<float>(y)});
}

const char* SdlVideoBackend::GetName() const
{
    return renderer != nullptr ? SDL_GetRendererName(renderer) : "not started";
}

} // namespace soft3d::platform

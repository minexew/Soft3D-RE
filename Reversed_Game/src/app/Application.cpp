#include "app/Application.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace app {
namespace {

constexpr int frameWidth = 320;
constexpr int frameHeight = 240;
constexpr uint64_t frameIntervalNs = 1000000000ULL / 60ULL;

uint32_t Rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
{
    return (static_cast<uint32_t>(red) << 24) | (static_cast<uint32_t>(green) << 16) |
           (static_cast<uint32_t>(blue) << 8) | alpha;
}

void FillRectangle(SoftwareFrame& frame, int left, int top, int width, int height, uint32_t color)
{
    const int right = std::min(left + width, frame.Width());
    const int bottom = std::min(top + height, frame.Height());
    left = std::max(left, 0);
    top = std::max(top, 0);
    for (int y = top; y < bottom; ++y) {
        std::fill_n(frame.Pixels() + y * frame.Width() + left, right - left, color);
    }
}

} // namespace

Application::Application(ApplicationConfig applicationConfig)
    : config(std::move(applicationConfig))
{
}

Application::~Application()
{
    Shutdown();
}

int Application::Run()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }
    if (!InitializeVideo()) {
        Shutdown();
        return 1;
    }
    if (config.enableAudio && !audio.Start()) {
        std::cerr << "Audio unavailable; continuing silently: " << SDL_GetError() << '\n';
        audio.Stop();
    }

    std::cout << "Renderer: " << SDL_GetRendererName(renderer) << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight
              << " (CPU), audio: " << (audio.IsRunning() ? "ready" : "disabled") << '\n';

    frame.Resize(frameWidth, frameHeight);
    timer.Reset();
    bool running = true;
    uint64_t frameNumber = 0;
    while (running && (config.maximumFrames == 0 || frameNumber <
                       static_cast<uint64_t>(config.maximumFrames))) {
        const uint64_t frameStart = timer.NowNanoseconds();
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
            controls.ProcessEvent(event);
        }

        const double deltaSeconds = timer.NextDeltaSeconds();
        RenderPlaceholder(frameNumber, deltaSeconds);
        if (!SDL_UpdateTexture(frameTexture, nullptr, frame.Pixels(), frame.Pitch()) ||
            !SDL_RenderClear(renderer) ||
            !SDL_RenderTexture(renderer, frameTexture, nullptr, nullptr) ||
            !SDL_RenderPresent(renderer)) {
            std::cerr << "Frame presentation failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        ++frameNumber;

        const uint64_t remaining = frameIntervalNs -
            std::min(frameIntervalNs, timer.NowNanoseconds() - frameStart);
        if (remaining >= 1000000ULL) {
            SDL_Delay(static_cast<uint32_t>(remaining / 1000000ULL));
        }
    }

    Shutdown();
    return 0;
}

bool Application::InitializeVideo()
{
    window = SDL_CreateWindow("7 Days: Salvation - Soft3D reconstruction",
                              frameWidth * config.scale, frameHeight * config.scale,
                              SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        return false;
    }

    const char* requestedRenderer = config.renderer == "auto" ? nullptr : config.renderer.c_str();
    renderer = SDL_CreateRenderer(window, requestedRenderer);
    if (renderer == nullptr && config.renderer != "software") {
        std::cerr << "Requested renderer unavailable; trying SDL software renderer: "
                  << SDL_GetError() << '\n';
        renderer = SDL_CreateRenderer(window, "software");
    }
    if (renderer == nullptr) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        return false;
    }

    frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING, frameWidth, frameHeight);
    if (frameTexture == nullptr) {
        std::cerr << "Framebuffer texture creation failed: " << SDL_GetError() << '\n';
        return false;
    }
    SDL_SetTextureScaleMode(frameTexture, SDL_SCALEMODE_NEAREST);
    return SDL_SetRenderLogicalPresentation(renderer, frameWidth, frameHeight,
                                            SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void Application::RenderPlaceholder(uint64_t frameNumber, double deltaSeconds)
{
    const ControlState& input = controls.State();
    const float distance = static_cast<float>(90.0 * deltaSeconds);
    markerX += (static_cast<float>(input.right) - static_cast<float>(input.left)) * distance;
    markerY += (static_cast<float>(input.down) - static_cast<float>(input.up)) * distance;
    markerX = std::clamp(markerX, 8.0F, static_cast<float>(frameWidth - 32));
    markerY = std::clamp(markerY, 32.0F, static_cast<float>(frameHeight - 32));

    const uint8_t pulse = static_cast<uint8_t>(32 + (frameNumber % 48));
    for (int y = 0; y < frame.Height(); ++y) {
        for (int x = 0; x < frame.Width(); ++x) {
            frame.Pixels()[y * frame.Width() + x] =
                Rgba(static_cast<uint8_t>(8 + x / 16),
                     static_cast<uint8_t>(12 + y / 12), pulse);
        }
    }

    for (int y = 40; y < frameHeight; y += 40) {
        FillRectangle(frame, 0, y, frameWidth, 1, Rgba(35, 50, 65));
    }
    for (int x = 40; x < frameWidth; x += 40) {
        FillRectangle(frame, x, 0, 1, frameHeight, Rgba(35, 50, 65));
    }
    FillRectangle(frame, 0, 0, frameWidth, 24, Rgba(12, 18, 28));
    FillRectangle(frame, 8, 8, 72, 8, Rgba(40, 170, 210));
    FillRectangle(frame, 88, 8, 48, 8,
                  audio.IsRunning() ? Rgba(70, 190, 110) : Rgba(160, 70, 70));
    FillRectangle(frame, 144, 8, 168, 8, Rgba(45, 55, 75));

    const int markerLeft = static_cast<int>(std::lround(markerX));
    const int markerTop = static_cast<int>(std::lround(markerY));
    FillRectangle(frame, markerLeft - 3, markerTop - 3, 30, 30, Rgba(10, 12, 18));
    FillRectangle(frame, markerLeft, markerTop, 24, 24,
                  input.action ? Rgba(250, 190, 60) : Rgba(210, 225, 235));
    FillRectangle(frame, markerLeft + 5, markerTop + 5, 14, 14, Rgba(180, 65, 55));
}

void Application::Shutdown()
{
    audio.Stop();
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
    SDL_Quit();
}

} // namespace app

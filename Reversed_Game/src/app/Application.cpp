#include "app/Application.hpp"

#include "platform/sdl/SdlVideoBackend.hpp"

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

void FillRectangle(const soft3d::VideoBufferInfo& frame, int left, int top, int width, int height,
                   uint32_t color)
{
    const int right = std::min(left + width, frame.width);
    const int bottom = std::min(top + height, frame.height);
    left = std::max(left, 0);
    top = std::max(top, 0);
    for (int y = top; y < bottom; ++y) {
        std::fill_n(frame.address + y * frame.width + left, right - left, color);
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

    std::cout << "Renderer: " << video->GetBackendName() << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight
              << " (CPU), audio: " << (audio.IsRunning() ? "ready" : "disabled") << '\n';

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
        if (video->GetBackBufferPtr() == nullptr) {
            std::cerr << "Framebuffer acquisition failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        RenderPlaceholder(frameNumber, deltaSeconds);
        if (!video->Update()) {
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
    auto backend = std::make_unique<soft3d::platform::SdlVideoBackend>(
        config.scale, config.renderer);
    video = std::make_unique<soft3d::VID_impl>(frameWidth, frameHeight, std::move(backend));
    return video->Start();
}

void Application::RenderPlaceholder(uint64_t frameNumber, double deltaSeconds)
{
    const ControlState& input = controls.State();
    const soft3d::VideoBufferInfo& frame = video->GetBufferInfo();
    const float distance = static_cast<float>(90.0 * deltaSeconds);
    markerX += (static_cast<float>(input.right) - static_cast<float>(input.left)) * distance;
    markerY += (static_cast<float>(input.down) - static_cast<float>(input.up)) * distance;
    markerX = std::clamp(markerX, 8.0F, static_cast<float>(frameWidth - 32));
    markerY = std::clamp(markerY, 32.0F, static_cast<float>(frameHeight - 32));

    const uint8_t pulse = static_cast<uint8_t>(32 + (frameNumber % 48));
    for (int y = 0; y < frame.height; ++y) {
        for (int x = 0; x < frame.width; ++x) {
            frame.address[y * frame.width + x] =
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
    if (video != nullptr) {
        video->Stop();
        video.reset();
    }
    SDL_Quit();
}

} // namespace app

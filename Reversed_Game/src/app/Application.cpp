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

uint16_t Rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>((red >> 3) << 11) |
           static_cast<uint16_t>((green >> 2) << 5) |
           static_cast<uint16_t>(blue >> 3);
}

void FillRectangle(soft3d::Texture* frame, int left, int top, int width, int height,
                   uint16_t color)
{
    const int right = std::min(left + width, static_cast<int>(frame->GetWidth()));
    const int bottom = std::min(top + height, static_cast<int>(frame->GetHeight()));
    left = std::max(left, 0);
    top = std::max(top, 0);
    uint16_t* pixels = reinterpret_cast<uint16_t*>(frame->GetPixels());
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * frame->GetWidth() + x) * 2;
            pixels[offset] = color;
            pixels[offset + 1] = 0;
        }
    }
}

} // namespace

class Application::HostEngine final : public soft3d::GameEngine {
public:
    HostEngine(soft3d::EngineConfig config, Application& application)
        : GameEngine(std::move(config))
        , app(application)
    {
    }

    void SubmitRenderables() override
    {
        app.RenderPlaceholder(app.currentFrame, app.currentDeltaSeconds);
        GameEngine::SubmitRenderables();
    }

private:
    Application& app;
};

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

    std::cout << "Renderer: " << engine->GetVideo().GetBackendName() << '\n'
              << "Framebuffer: " << frameWidth << 'x' << frameHeight
              << " (CPU), audio: " << (audio.IsRunning() ? "ready" : "disabled") << '\n';

    timer.Reset();
    bool running = true;
    while (running && (config.maximumFrames == 0 ||
                       currentFrame < static_cast<uint64_t>(config.maximumFrames))) {
        const uint64_t frameStart = timer.NowNanoseconds();
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
            controls.ProcessEvent(event);
        }

        currentDeltaSeconds = timer.NextDeltaSeconds();
        if (!engine->RunFrame()) {
            std::cerr << "Frame presentation failed: " << SDL_GetError() << '\n';
            Shutdown();
            return 1;
        }
        ++currentFrame;

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
    soft3d::EngineConfig engineConfig;
    engineConfig.width = frameWidth;
    engineConfig.height = frameHeight;
    engineConfig.videoBackend = std::make_unique<soft3d::platform::SdlVideoBackend>(
        config.scale, config.renderer);
    engine = std::make_unique<HostEngine>(std::move(engineConfig), *this);
    if (engine->Init() != 0) {
        return false;
    }
    engine->Start();
    return engine->IsRunning();
}

void Application::RenderPlaceholder(uint64_t frameNumber, double deltaSeconds)
{
    const ControlState& input = controls.State();
    soft3d::Texture* frame = engine->GetRenderer().GetCurrentTexture();
    const float distance = static_cast<float>(90.0 * deltaSeconds);
    markerX += (static_cast<float>(input.right) - static_cast<float>(input.left)) * distance;
    markerY += (static_cast<float>(input.down) - static_cast<float>(input.up)) * distance;
    markerX = std::clamp(markerX, 8.0F, static_cast<float>(frameWidth - 32));
    markerY = std::clamp(markerY, 32.0F, static_cast<float>(frameHeight - 32));

    const uint8_t pulse = static_cast<uint8_t>(32 + (frameNumber % 48));
    uint16_t* pixels = reinterpret_cast<uint16_t*>(frame->GetPixels());
    for (int y = 0; y < static_cast<int>(frame->GetHeight()); ++y) {
        for (int x = 0; x < static_cast<int>(frame->GetWidth()); ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * frame->GetWidth() + x) * 2;
            pixels[offset] = Rgb565(static_cast<uint8_t>(8 + x / 16),
                                    static_cast<uint8_t>(12 + y / 12), pulse);
            pixels[offset + 1] = 0;
        }
    }

    for (int y = 40; y < frameHeight; y += 40) {
        FillRectangle(frame, 0, y, frameWidth, 1, Rgb565(35, 50, 65));
    }
    for (int x = 40; x < frameWidth; x += 40) {
        FillRectangle(frame, x, 0, 1, frameHeight, Rgb565(35, 50, 65));
    }
    FillRectangle(frame, 0, 0, frameWidth, 24, Rgb565(12, 18, 28));
    FillRectangle(frame, 8, 8, 72, 8, Rgb565(40, 170, 210));
    FillRectangle(frame, 88, 8, 48, 8,
                  audio.IsRunning() ? Rgb565(70, 190, 110) : Rgb565(160, 70, 70));
    FillRectangle(frame, 144, 8, 168, 8, Rgb565(45, 55, 75));

    const int markerLeft = static_cast<int>(std::lround(markerX));
    const int markerTop = static_cast<int>(std::lround(markerY));
    FillRectangle(frame, markerLeft - 3, markerTop - 3, 30, 30, Rgb565(10, 12, 18));
    FillRectangle(frame, markerLeft, markerTop, 24, 24,
                  input.action ? Rgb565(250, 190, 60) : Rgb565(210, 225, 235));
    FillRectangle(frame, markerLeft + 5, markerTop + 5, 14, 14, Rgb565(180, 65, 55));
}

void Application::Shutdown()
{
    audio.Stop();
    if (engine != nullptr) {
        engine->GetVideo().Stop();
        engine->Shutdown();
        engine.reset();
    }
    SDL_Quit();
}

} // namespace app

#include "platform/sdl/SdlVideoBackend.hpp"
#include "soft3d/BasicShader.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

} // namespace

int main()
{
    try {
        const std::array<uint32_t, 6> landscape = {
            0xf800, 0x07e0, 0x001f, 0xffff, 0x0000, 0x1234,
        };
        std::array<uint32_t, 6> expanded{};
        for (std::size_t index = 0; index < landscape.size(); ++index) {
            const uint32_t pixel = landscape[index];
            expanded[index] = ((pixel & 0xf800) << 8) |
                              ((pixel & 0x07e0) << 5) | ((pixel & 0x001f) << 3);
        }
        std::array<uint32_t, 6> portrait{};
        std::array<uint32_t, 6> restored{};
        soft3d::BasicShader shader(3, 2);

        shader.Render(landscape.data(), portrait.data());
        soft3d::platform::RestoreLandscapeFrame(portrait.data(), 3, 2, 1, restored.data());
        CHECK(restored == expanded);

        shader.SetEnabled(2);
        shader.Render(landscape.data(), portrait.data());
        soft3d::platform::RestoreLandscapeFrame(portrait.data(), 3, 2, 2, restored.data());
        CHECK(restored == expanded);

        CHECK(SDL_Init(SDL_INIT_VIDEO));
        {
            bool showLandscape = false;
            soft3d::platform::SdlVideoBackend backend(1, "software");
            backend.SetLandscapeSource([&showLandscape] { return showLandscape; });
            CHECK(backend.Start(3, 2));
            soft3d::VideoBufferInfo buffer;
            buffer.address = portrait.data();
            buffer.width = 2;
            buffer.height = 3;
            buffer.pitch = 2 * sizeof(uint32_t);
            CHECK(backend.EndUpdate(buffer));
            showLandscape = true;
            CHECK(backend.EndUpdate(buffer));
            showLandscape = false;
            CHECK(backend.EndUpdate(buffer));
            backend.Stop();
        }
        SDL_Quit();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "SDL video orientation tests passed\n";
    return 0;
}

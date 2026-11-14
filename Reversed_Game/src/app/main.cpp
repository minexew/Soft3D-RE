#include "app/Application.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

void PrintUsage(const char* executable)
{
    std::cout << "Usage: " << executable
              << " [--renderer auto|software|NAME] [--scale 1..4]"
                 " [--frames N] [--resources PATH] [--no-audio] [--show-fps]\n"
                 "Controls: arrows/WASD select, Space/Enter action, Escape quits.\n";
}

bool ParseInteger(std::string_view text, int& value)
{
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

} // namespace

int main(int argc, char** argv)
{
    app::ApplicationConfig config;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }
        if (argument == "--no-audio") {
            config.enableAudio = false;
            continue;
        }
        if (argument == "--show-fps") {
            config.showFps = true;
            continue;
        }
        if (argument == "--renderer" || argument == "--scale" || argument == "--frames" ||
            argument == "--resources") {
            if (index + 1 >= argc) {
                std::cerr << "Missing value for " << argument << '\n';
                return 2;
            }
            const std::string_view value = argv[++index];
            if (argument == "--renderer") {
                config.renderer = std::string(value);
            } else if (argument == "--resources") {
                config.resourceArchive = std::string(value);
            } else if (!ParseInteger(value, argument == "--scale" ? config.scale
                                                                  : config.maximumFrames)) {
                std::cerr << "Invalid value for " << argument << ": " << value << '\n';
                return 2;
            }
            continue;
        }
        std::cerr << "Unknown option: " << argument << '\n';
        PrintUsage(argv[0]);
        return 2;
    }
    if (config.scale < 1 || config.scale > 4 || config.maximumFrames < 0) {
        std::cerr << "Scale must be 1..4 and frame count must be nonnegative.\n";
        return 2;
    }

    try {
        app::Application application(config);
        return application.Run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}

#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/TextureRes.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
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

uint64_t HashFrame(const soft3d::Texture& texture)
{
    uint64_t hash = 1469598103934665603ULL;
    const uint8_t* pixels = texture.GetPixels();
    for (size_t index = 0; index < texture.GetPixelSize(); ++index) {
        hash = (hash ^ pixels[index]) * 1099511628211ULL;
    }
    return hash;
}

uint16_t PaletteColor(const soft3d::Texture& texture, uint32_t offset)
{
    const uint8_t index = texture.GetPixels()[offset];
    uint16_t color;
    std::memcpy(&color, texture.GetPalette() + static_cast<size_t>(index) * 2, 2);
    return color;
}

void CheckVisibleImage(const soft3d::Texture& frame, const soft3d::Texture& image,
                       int32_t x, int32_t y)
{
    bool found = false;
    for (uint32_t sourceY = 0; sourceY < image.GetHeight() && !found; ++sourceY) {
        for (uint32_t sourceX = 0; sourceX < image.GetWidth(); ++sourceX) {
            const uint32_t offset = sourceY * image.GetWidth() + sourceX;
            if (image.HasTransparentColor() &&
                image.GetPixels()[offset] == image.GetTransparentColor()) {
                continue;
            }
            const uint32_t* pixels = reinterpret_cast<const uint32_t*>(frame.GetPixels());
            CHECK(pixels[(y + static_cast<int32_t>(sourceY)) * frame.GetWidth() + x + sourceX] ==
                  PaletteColor(image, offset));
            found = true;
            break;
        }
    }
    CHECK(found);
}

} // namespace

int main()
{
    try {
        soft3d::EngineConfig config;
        config.width = 320;
        config.height = 240;
        auto backend = std::make_unique<soft3d::platform::DummyVideoBackend>();
        auto* dummy = backend.get();
        config.videoBackend = std::move(backend);
        soft3d::GameEngine engine(std::move(config));
        engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
        CHECK(engine.Init() == soft3d::S3D_OK);
        CHECK(engine.GetBaseFont()->GetFont()->GetBaseCharCnt() == 64);
        CHECK(engine.GetSecondaryFont()->GetCount() == 581);

        soft3d::UiManager* ui = engine.GetUiManager();
        CHECK(ui->GetDialogCount() == 7);
        CHECK(ui->GetResourceNames().size() == 136);
        const soft3d::SdtDialog* dialog = ui->GetDialog(0);
        CHECK(dialog != nullptr && dialog->states.size() == 10);
        const soft3d::SdtState& handedness = dialog->states[7];
        CHECK(handedness.id == 2 && handedness.elements.size() == 5);
        CHECK(dialog->visuals[45].resourceIndex == 87);
        CHECK(dialog->visuals[46].resourceIndex == 90);
        CHECK(dialog->visuals[47].resourceIndex == 88);
        CHECK(dialog->visuals[48].resourceIndex == 89);

        engine.Start();
        CHECK(engine.RunFrame());
        const soft3d::Texture* frame = engine.GetRenderer().GetCurrentTexture();
        CHECK(HashFrame(*frame) != 0);
        const uint32_t* pixels = reinterpret_cast<const uint32_t*>(frame->GetPixels());
        CHECK(pixels[0] == 0 && pixels[320 * 240 - 1] == 0);
        CheckVisibleImage(*frame, *dialog->visuals[45].resource->GetTexture(), 106, 63);
        CheckVisibleImage(*frame, *dialog->visuals[47].resource->GetTexture(), 219, 54);
        const uint64_t leftFrame = HashFrame(*frame);
        CHECK(dummy->GetPresentedPixels().size() == 320 * 240);

        engine.SetMenuInput(false, true, false);
        CHECK(engine.RunFrame());
        const uint64_t rightFrame = HashFrame(*frame);
        CHECK(rightFrame != leftFrame);
        CheckVisibleImage(*frame, *dialog->visuals[46].resource->GetTexture(), 106, 63);
        CheckVisibleImage(*frame, *dialog->visuals[48].resource->GetTexture(), 219, 54);

        engine.SetMenuInput(true, false, false);
        CHECK(engine.RunFrame());
        CHECK(HashFrame(*frame) == leftFrame);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "SDT and initial-screen tests passed\n";
    return 0;
}

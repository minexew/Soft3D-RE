#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/TextureRes.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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
                  (0x7fff0000U | PaletteColor(image, offset)));
            found = true;
            break;
        }
    }
    CHECK(found);
}

void Append32(std::vector<uint8_t>& data, int32_t value)
{
    const uint32_t bits = static_cast<uint32_t>(value);
    data.push_back(static_cast<uint8_t>(bits));
    data.push_back(static_cast<uint8_t>(bits >> 8));
    data.push_back(static_cast<uint8_t>(bits >> 16));
    data.push_back(static_cast<uint8_t>(bits >> 24));
}

void Append16(std::vector<uint8_t>& data, uint16_t value)
{
    data.push_back(static_cast<uint8_t>(value));
    data.push_back(static_cast<uint8_t>(value >> 8));
}

std::unique_ptr<soft3d::Texture> MakeSource(const std::vector<uint8_t>& indices,
                                             uint32_t width, uint32_t height,
                                             int16_t transparent = -1)
{
    const std::array<uint16_t, 4> palette = {0x0000, 0xf800, 0x07e0, 0x001f};
    std::vector<uint8_t> data;
    Append32(data, static_cast<int32_t>(palette.size() | (0x90U << 8) |
                                        (static_cast<uint32_t>(static_cast<uint16_t>(transparent)) << 16)));
    Append32(data, static_cast<int32_t>(width));
    Append32(data, static_cast<int32_t>(height));
    for (uint16_t color : palette) {
        Append16(data, color);
    }
    data.insert(data.end(), indices.begin(), indices.end());
    auto texture = std::make_unique<soft3d::Texture>();
    CHECK(texture->ParseStx(data.data()) == soft3d::S3D_OK);
    return texture;
}

std::unique_ptr<soft3d::Texture> MakeAlpha(const std::vector<uint8_t>& alpha,
                                            uint32_t width, uint32_t height)
{
    std::vector<uint8_t> data;
    Append32(data, 0x10U << 8);
    Append32(data, static_cast<int32_t>(width));
    Append32(data, static_cast<int32_t>(height));
    data.insert(data.end(), alpha.begin(), alpha.end());
    auto texture = std::make_unique<soft3d::Texture>();
    CHECK(texture->ParseStx(data.data()) == soft3d::S3D_OK);
    return texture;
}

std::unique_ptr<soft3d::Texture> MakeTarget(uint32_t width, uint32_t height, uint32_t initial)
{
    auto texture = std::make_unique<soft3d::Texture>();
    CHECK(texture->Resize(width, height, 2) == soft3d::S3D_OK);
    uint32_t* pixels = reinterpret_cast<uint32_t*>(texture->GetPixels());
    std::fill(pixels, pixels + width * height, initial);
    return texture;
}

uint32_t ExpectedMode(uint32_t destination, uint16_t source, uint32_t mode)
{
    if (mode == 0) {
        return 0x7fff0000U | source;
    }
    destination |= 0x7fff0000U;
    const uint16_t old = static_cast<uint16_t>(destination);
    uint16_t color = old;
    if (mode == 1) {
        const uint32_t blue = std::min<uint32_t>(31, (old & 31) + (source & 31));
        const uint32_t green = std::min<uint32_t>(0x7e0, (old & 0x7e0) + (source & 0x7e0));
        const uint32_t red = std::min<uint32_t>(0xf800, (old & 0xf800) + (source & 0xf800));
        color = static_cast<uint16_t>(red | green | blue);
    } else if (mode == 2) {
        const uint32_t blue = (old & 31) >= (source & 31) ? (old & 31) - (source & 31) : 0;
        const uint32_t green = (old & 0x7e0) >= (source & 0x7e0) ? (old & 0x7e0) - (source & 0x7e0) : 0;
        const uint32_t red = (old & 0xf800) >= (source & 0xf800) ? (old & 0xf800) - (source & 0xf800) : 0;
        color = static_cast<uint16_t>(red | green | blue);
    } else if (mode == 3) {
        color = static_cast<uint16_t>(((old & 0xf7de) >> 1) + ((old & 0xe79c) >> 2) +
                                      ((source & 0xe79c) >> 2));
    } else if (mode == 4) {
        color = static_cast<uint16_t>(((old & 0xf7de) >> 1) + ((source & 0xf7de) >> 1));
    } else if (mode == 5) {
        color = static_cast<uint16_t>(((old & 0xe79c) >> 2) + ((source & 0xf7de) >> 1) +
                                      ((source & 0xe79c) >> 2));
    }
    return (destination & 0xffff0000U) | color;
}

uint32_t ExpectedSecondary(uint32_t destination, uint16_t source, uint8_t alpha, bool scaled)
{
    if (scaled) {
        destination |= 0x7fff0000U;
    } else if (alpha <= 1) {
        return destination;
    } else {
        destination |= 0x7fff0000U;
    }
    if (alpha <= 1) {
        return destination;
    }
    if (alpha >= 31) {
        return (destination & 0xffff0000U) | source;
    }
    const uint16_t old = static_cast<uint16_t>(destination);
    const uint32_t oldColor = ((old & 0xf800) << 10) | ((old & 0x7e0) << 5) | (old & 31);
    const uint32_t sourceColor = ((source & 0xf800) << 10) | ((source & 0x7e0) << 5) | (source & 31);
    const uint32_t value = oldColor * 32 + (sourceColor - oldColor) * alpha;
    const uint16_t color = static_cast<uint16_t>(((value & 0x7c000000) >> 23) |
        ((value & 0x1f8000) >> 10) | ((value & 0x3e0) >> 5));
    return (destination & 0xffff0000U) | color;
}

void TestBlitterModes()
{
    constexpr uint32_t initial = 0x12340000U | 0x4200;
    for (uint32_t transparent = 0; transparent < 2; ++transparent) {
        for (uint32_t mode = 0; mode < 6; ++mode) {
            for (uint32_t scaled = 0; scaled < 2; ++scaled) {
                auto source = MakeSource({1, 2}, 2, 1, transparent != 0 ? 1 : -1);
                auto target = MakeTarget(2, 1, initial);
                soft3d::ScreenBlitter blitter;
                blitter.SetTexture(target.get());
                if (scaled != 0) {
                    blitter.Blit(source.get(), {0, 0, 2, 1}, {0, 0, 2, 1}, mode);
                } else {
                    blitter.BlitUnscaled(source.get(), 0, 0, {0, 0, 2, 1}, mode);
                }
                const uint32_t* pixels = reinterpret_cast<const uint32_t*>(target->GetPixels());
                CHECK(pixels[0] == (transparent != 0 ? initial : ExpectedMode(initial, 0xf800, mode)));
                CHECK(pixels[1] == ExpectedMode(initial, 0x07e0, mode));
            }
        }
    }
}

void TestBlitterClippingAndSecondary()
{
    soft3d::ScreenBlitter blitter;
    auto source = MakeSource({1, 2, 1, 3}, 2, 2);
    auto target = MakeTarget(2, 2, 0x11223344);
    blitter.SetTexture(target.get());
    blitter.Blit(source.get(), {-1, -1, 1, 1}, {0, 0, 2, 2});
    uint32_t* pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    CHECK(pixels[0] == (0x7fff0000U | 0x001f));
    CHECK(pixels[1] == 0x11223344 && pixels[2] == 0x11223344 && pixels[3] == 0x11223344);

    target = MakeTarget(4, 1, 0);
    auto row = MakeSource({1, 2}, 2, 1);
    blitter.SetTexture(target.get());
    blitter.Blit(row.get(), {0, 0, 4, 1}, {0, 0, 2, 1});
    pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    CHECK(pixels[0] == (0x7fff0000U | 0xf800));
    CHECK(pixels[1] == (0x7fff0000U | 0xf800));
    CHECK(pixels[2] == (0x7fff0000U | 0x07e0));
    CHECK(pixels[3] == (0x7fff0000U | 0x07e0));

    target = MakeTarget(2, 2, 0xaaaaaaaa);
    blitter.SetTexture(target.get());
    blitter.Clear(255, 255, 255);
    pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    CHECK(pixels[0] == 0xaaaaaaaa);
    CHECK(pixels[1] == 0xffff && pixels[2] == 0xffff && pixels[3] == 0xffff);

    auto alpha = MakeAlpha({0, 16}, 2, 1);
    target = MakeTarget(2, 1, 0x12340000U | 0x4200);
    blitter.SetTexture(target.get());
    blitter.BlitUnscaled(row.get(), 0, 0, {0, 0, 2, 1}, 0, alpha.get());
    pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    CHECK(pixels[0] == (0x12340000U | 0x4200));
    CHECK(pixels[1] == ExpectedSecondary(0x12340000U | 0x4200, 0x07e0, 16, false));

    auto keyed = MakeSource({1, 2}, 2, 1, 1);
    auto opaque = MakeAlpha({31, 31}, 2, 1);
    target = MakeTarget(2, 1, 0x12340000U | 0x4200);
    blitter.SetTexture(target.get());
    blitter.Blit(keyed.get(), {0, 0, 2, 1}, {0, 0, 2, 1}, 0, opaque.get());
    pixels = reinterpret_cast<uint32_t*>(target->GetPixels());
    CHECK(pixels[0] == ExpectedSecondary(0x12340000U | 0x4200, 0xf800, 31, true));
    CHECK(pixels[1] == (0x12340000U | 0x4200));
}

void AppendKey(std::vector<uint8_t>& data, const soft3d::SdtAnimationKey& key)
{
    const int32_t* fields = reinterpret_cast<const int32_t*>(&key);
    for (size_t index = 0; index < 13; ++index) {
        Append32(data, fields[index]);
    }
}

void TestInterpolation()
{
    soft3d::SdtAnimationKey first;
    first.time = 0;
    first.x = 0;
    first.y = 0;
    first.width = 10;
    first.height = 20;
    first.mode = 3;
    first.sourceX = 0;
    first.sourceY = 0;
    first.sourceWidth = 2;
    first.sourceHeight = 4;
    first.widthDivisor = 2;
    first.heightDivisor = 4;
    first.extra = 77;
    soft3d::SdtAnimationKey second;
    second.time = 10;
    second.x = 10;
    second.y = 20;
    second.width = 30;
    second.height = 40;
    second.mode = 5;
    second.sourceX = 4;
    second.sourceY = 6;
    second.sourceWidth = 8;
    second.sourceHeight = 10;
    second.widthDivisor = 1;
    second.heightDivisor = 1;
    second.extra = 88;

    std::vector<uint8_t> data;
    Append32(data, 1);
    Append32(data, 1);
    Append32(data, 1);
    Append32(data, 0);
    Append32(data, 0);
    Append32(data, 0);
    Append32(data, 1);
    Append32(data, 9);
    Append32(data, 2);
    Append32(data, 0);
    Append32(data, 2);
    Append32(data, 0);
    Append32(data, 0);
    AppendKey(data, first);
    AppendKey(data, second);
    Append32(data, 0);
    Append32(data, -1);
    Append32(data, 0);
    Append32(data, 0);

    soft3d::UiManager manager;
    CHECK(manager.ParseStateSdt(data.data(), data.size()) == soft3d::S3D_OK);
    CHECK(manager.GetDeclaredVisualTotal() == 1);
    soft3d::Dialog* dialog = manager.GetDialogRuntime(0);
    CHECK(dialog != nullptr);
    soft3d::SdtVisual* visual = dialog->GetVisual(0);
    CHECK(dialog->SetElementTime(0, 0, -1) == 0 && visual->hidden);
    CHECK(dialog->RenderElementAtTime(0, 0, -1) == 1 && visual->hidden);
    CHECK(dialog->SetElementTime(0, 0, 0) == 1 && !visual->hidden);
    CHECK(visual->extra == 77 && visual->blitMode == 3);
    CHECK(dialog->SetElementTime(0, 0, 5) == 1 && !visual->hidden);
    CHECK(visual->x == 5 && visual->y == 10);
    CHECK(visual->width == 20 && visual->height == 30);
    CHECK(visual->blitMode == 3);
    CHECK(visual->sourceX == 2 && visual->sourceY == 3);
    CHECK(visual->sourceWidth == 5 && visual->sourceHeight == 7);
    CHECK(visual->widthDivisor == 2 && visual->heightDivisor == 4);
    CHECK(visual->extra == -1);
    CHECK(dialog->SetElementTime(0, 0, 10) == 1 && !visual->hidden);
    CHECK(visual->x == 10 && visual->y == 20 && visual->extra == 88);
    CHECK(visual->blitMode == 5 && visual->widthDivisor == 1 && visual->heightDivisor == 1);
    CHECK(dialog->SetElementTime(0, 0, 11) == 1 && visual->hidden);
    CHECK(dialog->SetElementTime(0, 1, 0) == 1);
    CHECK(visual->hidden);
}

void CheckExactKey(const soft3d::SdtVisual& visual, const soft3d::SdtAnimationKey& key)
{
    CHECK(visual.x == static_cast<int16_t>(key.x));
    CHECK(visual.y == static_cast<int16_t>(key.y));
    CHECK(visual.width == static_cast<int16_t>(key.width));
    CHECK(visual.height == static_cast<int16_t>(key.height));
    CHECK(visual.blitMode == static_cast<int16_t>(key.mode));
    CHECK(visual.sourceX == static_cast<int16_t>(key.sourceX));
    CHECK(visual.sourceY == static_cast<int16_t>(key.sourceY));
    CHECK(visual.sourceWidth == static_cast<int16_t>(key.sourceWidth));
    CHECK(visual.sourceHeight == static_cast<int16_t>(key.sourceHeight));
    CHECK(visual.widthDivisor == static_cast<int16_t>(key.widthDivisor));
    CHECK(visual.heightDivisor == static_cast<int16_t>(key.heightDivisor));
    CHECK(visual.extra == static_cast<int16_t>(key.extra));
    CHECK(!visual.hidden);
}

void TestRealTables(soft3d::UiManager& ui)
{
    CHECK(ui.GetDialogCount() == 7);
    CHECK(ui.GetResourceNames().size() == 138);
    CHECK(ui.GetSoundNames().size() == 3);
    CHECK(ui.GetStrReserved0() == 5);
    CHECK(ui.GetDeclaredVisualTotal() == 390);
    size_t visualTotal = 0;
    size_t elementTotal = 0;
    size_t keyTotal = 0;
    size_t behaviorZero = 0;
    size_t behaviorOne = 0;
    size_t secondaryTotal = 0;
    for (size_t dialogIndex = 0; dialogIndex < ui.GetDialogCount(); ++dialogIndex) {
        soft3d::Dialog* dialog = ui.GetDialogRuntime(dialogIndex);
        CHECK(dialog != nullptr && dialog->GetIndex() == dialogIndex);
        CHECK(dialog->IsActive() == (dialogIndex == 0));
        const soft3d::SdtDialog& definition = dialog->GetData();
        visualTotal += definition.visuals.size();
        for (const soft3d::SdtState& state : definition.states) {
            elementTotal += state.elements.size();
            int32_t renderTime = 0;
            bool haveRenderTime = false;
            for (size_t elementIndex = 0; elementIndex < state.elements.size(); ++elementIndex) {
                const soft3d::SdtElement& element = state.elements[elementIndex];
                behaviorZero += element.behavior == 0;
                behaviorOne += element.behavior == 1;
                CHECK(element.behavior <= 1);
                if (element.secondaryTextureIndex != 0) {
                    ++secondaryTotal;
                    CHECK(element.secondaryTextureIndex < ui.GetResourceNames().size());
                    const uint32_t primaryIndex = definition.visuals[element.visualIndex].resourceIndex;
                    CHECK(ui.GetResourceNames()[element.secondaryTextureIndex] ==
                          ui.GetResourceNames()[primaryIndex] + u"_alp");
                    const soft3d::TextureRes* secondary = ui.GetResourceByIndex(element.secondaryTextureIndex);
                    CHECK(secondary != nullptr && secondary->GetTexture() != nullptr);
                }
                keyTotal += element.keys.size();
                if (element.behavior == 0 && !element.keys.empty() && !haveRenderTime) {
                    renderTime = element.keys.front().time;
                    haveRenderTime = true;
                }
                for (const soft3d::SdtAnimationKey& key : element.keys) {
                    CHECK(dialog->SetElementTime(static_cast<int32_t>(&state - definition.states.data()),
                                                 static_cast<int32_t>(elementIndex), key.time) == 1);
                    CheckExactKey(*dialog->GetVisual(element.visualIndex), key);
                }
            }
            CHECK(dialog->SetStateTime(static_cast<int32_t>(&state - definition.states.data()),
                                       renderTime) == 1);
        }
    }
    CHECK(visualTotal == 390 && elementTotal == 390 && keyTotal == 923);
    CHECK(behaviorZero == 218 && behaviorOne == 172);
    CHECK(secondaryTotal == 5);
    CHECK(ui.GetResourceNames()[136] == u"fengmian_kuang_alp");
    CHECK(ui.GetResourceNames()[137] == u"tongjishuzi_alp");
}

void TestInitialScreen(soft3d::GameEngine& engine,
                       soft3d::platform::DummyVideoBackend* dummy)
{
    soft3d::UiManager* ui = engine.GetUiManager();
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
    const uint32_t* pixels = reinterpret_cast<const uint32_t*>(frame->GetPixels());
    CHECK(pixels[0] == 0x7fff0000U && pixels[320 * 240 - 1] == 0x7fff0000U);
    CheckVisibleImage(*frame, *dialog->visuals[45].resource->GetTexture(), 106, 63);
    CheckVisibleImage(*frame, *dialog->visuals[47].resource->GetTexture(), 219, 54);
    const uint64_t leftFrame = HashFrame(*frame);
    CHECK(dummy->GetPresentedPixels().size() == 320 * 240);

    engine.GetInput().KeyDown(soft3d::Key::Right);
    CHECK(engine.RunFrame());
    const uint64_t rightFrame = HashFrame(*frame);
    CHECK(rightFrame != leftFrame);
    CheckVisibleImage(*frame, *dialog->visuals[46].resource->GetTexture(), 106, 63);
    CheckVisibleImage(*frame, *dialog->visuals[48].resource->GetTexture(), 219, 54);

    engine.GetInput().KeyUp(soft3d::Key::Right);
    engine.GetInput().KeyDown(soft3d::Key::Left);
    CHECK(engine.RunFrame());
    CHECK(HashFrame(*frame) == leftFrame);
}

} // namespace

int main()
{
    try {
        TestBlitterModes();
        TestBlitterClippingAndSecondary();
        TestInterpolation();

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
        TestInitialScreen(engine, dummy);
        TestRealTables(*engine.GetUiManager());
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "SDT, dialog, blitter, and initial-screen tests passed\n";
    return 0;
}

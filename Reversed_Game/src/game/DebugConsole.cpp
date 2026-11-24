#include "game/DebugConsole.hpp"

#include "game/Scene.hpp"
#include "soft3d/Bsp.hpp"
#include "soft3d/Color.hpp"
#include "soft3d/s3dIO.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cstring>

namespace game {

DebugConsole::DebugConsole(Scene& owner)
    : scene(owner)
{
    choices.fill(0);
    choices[0] = u' ';
    for (int32_t index = 0; index < 10; ++index) {
        choices[static_cast<std::size_t>(index + 1)] = static_cast<char16_t>(u'0' + index);
    }
    entered.fill(0);
    textY = (scene.GetEngine().GetHeight() - scene.GetEngine().GetBaseFont()->GetTextHeight()) >> 1;
}

void DebugConsole::Reset()
{
    selection = 0;
    entered.fill(0);
    enteredCount = 0;
    frameCounter = 0;
    display.Clear();
}

bool DebugConsole::IsDigitSequence(int32_t start) const
{
    for (int32_t index = start; index < enteredCount; ++index) {
        const char16_t value = entered[static_cast<std::size_t>(index)];
        if (value < u'0' || value > u'9') return false;
    }
    return true;
}

int32_t DebugConsole::ParseUnsigned(int32_t start) const
{
    int32_t result = 0;
    for (int32_t index = start; index < enteredCount; ++index) {
        result = result * 10 + entered[static_cast<std::size_t>(index)] - u'0';
    }
    return result;
}

int32_t DebugConsole::Update()
{
    soft3d::Input& input = scene.GetEngine().GetInput();
    Player* player = scene.GetPlayer();
    if (input.IsKeyClick(soft3d::Key::Hash)) {
        active = !active;
        if (active) Reset();
    }
    if (!active) return 0;

    if (input.IsKeyClick(soft3d::Key::Up)) {
        if (--selection < 0) selection = 10;
    }
    if (input.IsKeyClick(soft3d::Key::Down)) {
        if (++selection > 10) selection = 0;
    }
    if (input.IsKeyClick(soft3d::Key::Left) && enteredCount > 0) --enteredCount;
    if (input.IsKeyClick(soft3d::Key::Right)) {
        if (enteredCount < 7) {
            entered[static_cast<std::size_t>(enteredCount++)] =
                choices[static_cast<std::size_t>(selection)];
        }
        selection = 0;
    }
    if (input.IsKeyClick(soft3d::Key::Accept) || input.IsKeyClick(soft3d::Key::Number7)) {
        if (enteredCount > 2 && entered[0] == u' ' && entered[1] == u' ' &&
            IsDigitSequence(2)) {
            const int32_t day = ParseUnsigned(2);
            if (day > 0 && day < 8) {
                if (scene.GetDay() == day) {
                    const bool preserve1000 = scene.GetEventFlag(1000);
                    const bool preserve1001 = scene.GetEventFlag(1001);
                    const bool preserve1002 = scene.GetEventFlag(1002);
                    scene.ClearEventFlags();
                    if (preserve1000) scene.SetEventFlag(1000);
                    if (preserve1001) scene.SetEventFlag(1001);
                    if (preserve1002) scene.SetEventFlag(1002);
                }
                scene.UnloadLevel();
                scene.LoadDay(day);
            }
        }
        if (enteredCount == 4 && IsDigitSequence(0)) {
            const int32_t mapId = ParseUnsigned(0);
            if (mapId > 100 && mapId < 7000 && LoadMapSpawn(mapId)) {
                scene.ResetTransitionForDebugJump();
                scene.SetLoadMapParams(0, spawnPosition, spawnDirection, 0);
                scene.LoadNextLevel(mapId, mapId);
            }
        }
        if (entered[0] == u' ' && IsDigitSequence(1) && player != nullptr) {
            player->AddItem(ParseUnsigned(1), true);
        }
        if (enteredCount == 1 && entered[0] == u' ' && player != nullptr) {
            player->SetHealth(player->GetMaxHealth());
        }
        if (enteredCount == 2 && entered[0] == u' ' && entered[1] == u' ' && player != nullptr) {
            player->SetHealth(1);
        }
        if (enteredCount == 2 && entered[0] == u'0' && IsDigitSequence(1) && player != nullptr) {
            player->SetWeapon(ParseUnsigned(1));
        }
        if (enteredCount == 3 && entered[0] == u' ' && entered[1] == u' ' &&
            entered[2] == u' ' && player != nullptr) {
            player->ToggleUsedItemHealing();
        }
        input.ClearKey();
        active = false;
        return 0;
    }
    entered[static_cast<std::size_t>(enteredCount)] = 0;
    return 1;
}

bool DebugConsole::LoadMapSpawn(int32_t mapId)
{
    char16_t mapName[32];
    if (mapId < 1000) {
        soft3d::WideSprintf(mapName, u"%03d.sbp", mapId);
    } else {
        soft3d::WideSprintf(mapName, u"%d.sbp", mapId);
    }
    char16_t path[96];
    soft3d::WideSprintf(path, u".\\day%d\\%s", scene.GetDay(), mapName);
    soft3d::s3dFilePak* archive = scene.GetResources()->GetFilePak();
    if (archive == nullptr) return false;
    if (archive->Select(path) != soft3d::S3D_OK) {
        soft3d::WideSprintf(path, u".\\day1\\%s", mapName);
        if (archive->Select(path) != soft3d::S3D_OK) return false;
    }
    if (archive->Seek(0, SEEK_SET) != soft3d::S3D_OK) return false;
    soft3d::SbpHeader header;
    if (archive->Read(&header, sizeof(header)) != soft3d::S3D_OK || header.version != 7.6f) {
        return false;
    }
    for (std::size_t index = 0; index < 3; ++index) {
        (&spawnPosition.x)[index] = static_cast<soft3d::Fixed>(header.values[index] * 65536.0f);
        (&spawnDirection.x)[index] = static_cast<soft3d::Fixed>(header.values[index + 3] * 65536.0f);
    }
    return true;
}

void DebugConsole::Render()
{
    if (!active) return;
    ++frameCounter;
    char16_t text[32]{};
    std::memcpy(text, entered.data(), entered.size() * sizeof(char16_t));
    text[enteredCount] = choices[static_cast<std::size_t>(selection)];
    text[enteredCount + 1] = 0;
    display.Clear();
    display.Append(text);
    soft3d::StringManager* font = scene.GetEngine().GetBaseFont();
    const int32_t textHeight = font->GetTextHeight();
    soft3d::ScreenBlitter& blitter = scene.GetEngine().GetScreenBlitter();
    blitter.FillRect(0, {textX, textY, scene.GetEngine().GetWidth() - textX, textY + textHeight});
    if ((frameCounter & 1) != 0) {
        const int32_t end = textX + static_cast<int32_t>(font->CountTextWidth(&display, 1));
        blitter.FillRect(0xffff, {end, textY + textHeight - 2, end + 5,
                                  textY + textHeight});
    }
    font->DrawString(&display, soft3d::PackRgb565(0xff, 0, 0), 0,
                     soft3d::Point{textX, textY});
}

} // namespace game

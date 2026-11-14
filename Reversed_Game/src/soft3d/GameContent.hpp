#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace soft3d {

struct AboutModelRecord {
    uint32_t nameString = 0;
    uint32_t descriptionString = 0;
    std::u16string modelName;
};

struct GameTextRecord {
    uint32_t titleString = 0;
    uint32_t bodyString = 0;
};

struct GameItemRecord {
    uint32_t nameString = 0;
    uint32_t descriptionString = 0;
};

struct HelpStatistics {
    uint32_t first = 0;
    uint32_t second = 0;
    uint32_t elapsedSeconds = 0;
    uint32_t fourth = 0;
    uint32_t filesFound = 0;
    uint32_t secretsFound = 0;
};

struct NewGameOptions {
    int32_t weapon = 0;
    int32_t outfit = 0;
};

struct GameContent {
    GameContent()
    {
        inventory.fill(-1);
        noteIds.fill(-1);
    }

    std::array<AboutModelRecord, 100> models;
    std::array<GameTextRecord, 100> documents;
    std::array<GameTextRecord, 100> photos;
    std::array<GameItemRecord, 100> items;
    std::array<int32_t, 9> inventory;
    std::array<int32_t, 128> noteIds;
    HelpStatistics statistics;
    int32_t mapId = 0;
};

} // namespace soft3d

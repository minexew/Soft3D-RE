#pragma once

#include "soft3d/GameContent.hpp"
#include "soft3d/Object.hpp"
#include "soft3d/s3dCommon.hpp"
#include "soft3d/s3dFilePak.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace game {

inline constexpr std::size_t animationResourceNameCount = 260;
inline constexpr std::size_t soundResourceNameCount = 143;
inline constexpr std::size_t doorResourceCount = 22;
inline constexpr std::size_t resourceNameCapacity = 64;
inline constexpr std::size_t enemyInfoGroupCapacity = 100;
inline constexpr std::size_t enemySpawnCapacity = 32;

struct GameItemSpawnRecord {
    int32_t itemId = 0;
    int32_t pickupTextId = 0;
    std::array<uint8_t, 16> packedModelName{};
    soft3d::Vec3 position;
    soft3d::Vec3 axisX;
    soft3d::Vec3 axisY;
    soft3d::Vec3 axisZ;
    soft3d::ObjectBounds pickupBounds;
};

struct GameItemGroup {
    int16_t mapId = 0;
    std::vector<GameItemSpawnRecord> records;
};

struct EnemyWaypoint {
    soft3d::Vec3 position;
    int32_t waitTime = 0;
    soft3d::Vec3 direction;
};

struct EnemySpawnRecord {
    soft3d::Vec3 position;
    soft3d::Vec3 direction;
    int16_t active = -1;
    int16_t enemyType = 0;
    int16_t unitId = -1;
    std::vector<EnemyWaypoint> waypoints;
};

struct EnemyInfoGroup {
    int16_t mapId = 0;
    std::vector<EnemySpawnRecord> records;
};

struct DoorResourceRecord {
    std::array<char16_t, resourceNameCapacity> modelName{};
    int32_t openSoundIndex = 0;
    int32_t closeSoundIndex = 0;
};

class GameData {
public:
    GameData();

    soft3d::result LoadItemData(soft3d::s3dFilePak& filePak, soft3d::GameContent& content);
    soft3d::result LoadDayContent(int day, soft3d::s3dFilePak& filePak);

    static soft3d::result ParseItemData(const void* data, std::size_t size,
                                        soft3d::GameContent& content);
    static soft3d::result ParseGameItemData(const void* data, std::size_t size,
                                            std::vector<GameItemGroup>& groups);
    static soft3d::result ParseEnemyInfoData(const void* data, std::size_t size,
                                             std::vector<EnemyInfoGroup>& groups);

    const char16_t* GetSoundNameTable() const;
    const char16_t* GetAnimationName(std::size_t index) const;
    const char16_t* GetSoundName(std::size_t index) const;
    const DoorResourceRecord& GetDoorResource(std::size_t index) const;
    GameItemGroup* FindGameItemGroup(int32_t mapId);
    const GameItemGroup* FindGameItemGroup(int32_t mapId) const;
    EnemyInfoGroup* FindEnemyInfoGroup(int32_t mapId);
    const EnemyInfoGroup* FindEnemyInfoGroup(int32_t mapId) const;
    EnemySpawnRecord* AddEnemySpawnRecord(int32_t mapId, int32_t enemyType, int32_t unitId);
    const std::vector<GameItemGroup>& GetGameItemGroups() const;
    const std::vector<EnemyInfoGroup>& GetEnemyInfoGroups() const;

private:
    std::array<char16_t, animationResourceNameCount * resourceNameCapacity> animationNames{};
    std::array<char16_t, soundResourceNameCount * resourceNameCapacity> soundNames{};
    std::array<DoorResourceRecord, doorResourceCount> doorResources{};
    std::vector<GameItemGroup> gameItemGroups;
    std::vector<EnemyInfoGroup> enemyInfoGroups;
};

} // namespace game

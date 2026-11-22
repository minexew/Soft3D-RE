#include "game/GameData.hpp"

#include "soft3d/s3dIO.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

namespace game {
namespace {

struct Reader {
    const uint8_t* data;
    std::size_t size;
    std::size_t offset = 0;

    bool Read32(int32_t& value)
    {
        if (size - offset < sizeof(value)) {
            return false;
        }
        std::memcpy(&value, data + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    }

    bool ReadBytes(void* destination, std::size_t count)
    {
        if (size - offset < count) {
            return false;
        }
        std::memcpy(destination, data + offset, count);
        offset += count;
        return true;
    }

    std::size_t Remaining() const { return size - offset; }
};

soft3d::result ReadArchiveEntry(soft3d::s3dFilePak& filePak, const char16_t* name,
                                std::vector<uint8_t>& data)
{
    if (filePak.Select(name) != soft3d::S3D_OK || filePak.Seek(0, SEEK_END) != soft3d::S3D_OK) {
        return soft3d::S3D_FILE;
    }
    const int bytes = filePak.Tell();
    if (bytes == soft3d::S3D_EOF || filePak.Seek(0, SEEK_SET) != soft3d::S3D_OK) {
        return soft3d::S3D_FILE;
    }
    data.resize(static_cast<std::size_t>(bytes));
    if (bytes != 0 && filePak.Read(data.data(), bytes) != soft3d::S3D_OK) {
        return soft3d::S3D_FILE;
    }
    return soft3d::S3D_OK;
}

bool ReadVec3(Reader& reader, soft3d::Vec3& value)
{
    return reader.Read32(value.x) && reader.Read32(value.y) && reader.Read32(value.z);
}

bool ReadBounds(Reader& reader, soft3d::ObjectBounds& value)
{
    return ReadVec3(reader, value.minimum) && ReadVec3(reader, value.maximum);
}

bool FitsInt16(int32_t value)
{
    return value >= 0 && value <= std::numeric_limits<int16_t>::max();
}

} // namespace

soft3d::result GameData::ParseItemData(const void* data, std::size_t size,
                                       soft3d::GameContent& content)
{
    if (data == nullptr && size != 0) {
        return soft3d::S3D_PARAM;
    }
    Reader reader{static_cast<const uint8_t*>(data), size};
    int32_t itemCount;
    if (!reader.Read32(itemCount) || itemCount < 0 ||
        static_cast<std::size_t>(itemCount) > reader.Remaining() / 52) {
        return soft3d::S3D_FORMAT;
    }

    soft3d::GameContent parsed;
    for (int32_t index = 0; index < itemCount; ++index) {
        int32_t itemId;
        int32_t nameString;
        char modelName[32];
        int32_t combineWith;
        int32_t combined;
        int32_t descriptionString;
        if (!reader.Read32(itemId) || itemId < 0 ||
            itemId >= static_cast<int32_t>(parsed.items.size()) ||
            !reader.Read32(nameString) || !reader.ReadBytes(modelName, sizeof(modelName)) ||
            !reader.Read32(combineWith) || !reader.Read32(combined) ||
            !reader.Read32(descriptionString)) {
            return soft3d::S3D_FORMAT;
        }
        const char* terminator = static_cast<const char*>(std::memchr(modelName, '\0', sizeof(modelName)));
        if (terminator == nullptr) {
            return soft3d::S3D_FORMAT;
        }

        soft3d::GameItemRecord& item = parsed.items[static_cast<std::size_t>(itemId)];
        item.nameString = static_cast<uint32_t>(nameString);
        item.descriptionString = static_cast<uint32_t>(descriptionString);
        item.combineWithItemId = combineWith;
        item.combinedItemId = combined;
        item.runtimeState = 0;
        item.modelName.clear();
        for (const char* current = modelName; current != terminator; ++current) {
            item.modelName.push_back(static_cast<unsigned char>(*current));
        }
    }

    int32_t textCount;
    if (!reader.Read32(textCount) || textCount < 0 ||
        static_cast<std::size_t>(textCount) > reader.Remaining() / 12) {
        return soft3d::S3D_FORMAT;
    }
    for (int32_t index = 0; index < textCount; ++index) {
        int32_t textId;
        int32_t titleString;
        int32_t bodyString;
        if (!reader.Read32(textId) || !reader.Read32(titleString) || !reader.Read32(bodyString)) {
            return soft3d::S3D_FORMAT;
        }
        if (textId >= 1000 && textId < 2000) {
            soft3d::GameTextRecord& text = parsed.documents[static_cast<std::size_t>(textId - 1000)];
            text.titleString = static_cast<uint32_t>(titleString);
            text.bodyString = static_cast<uint32_t>(bodyString);
        } else if (textId >= 2000 && textId < 2100) {
            soft3d::GameTextRecord& text = parsed.photos[static_cast<std::size_t>(textId - 2000)];
            text.titleString = static_cast<uint32_t>(titleString);
            text.bodyString = static_cast<uint32_t>(bodyString);
        } else {
            return soft3d::S3D_FORMAT;
        }
    }
    if (reader.Remaining() != 0) {
        return soft3d::S3D_FORMAT;
    }

    for (std::size_t index = 0; index < parsed.items.size(); ++index) {
        parsed.models[index].nameString = parsed.items[index].nameString;
        parsed.models[index].descriptionString = parsed.items[index].descriptionString;
        parsed.models[index].modelName = parsed.items[index].modelName;
    }
    content.items = std::move(parsed.items);
    content.models = std::move(parsed.models);
    content.documents = std::move(parsed.documents);
    content.photos = std::move(parsed.photos);
    return soft3d::S3D_OK;
}

soft3d::result GameData::ParseGameItemData(const void* data, std::size_t size,
                                           std::vector<GameItemGroup>& groups)
{
    if (data == nullptr && size != 0) {
        return soft3d::S3D_PARAM;
    }
    Reader reader{static_cast<const uint8_t*>(data), size};
    int32_t groupCount;
    if (!reader.Read32(groupCount) || groupCount < 0 ||
        static_cast<std::size_t>(groupCount) > reader.Remaining() / 8) {
        return soft3d::S3D_FORMAT;
    }

    std::vector<GameItemGroup> parsed;
    parsed.reserve(static_cast<std::size_t>(groupCount));
    for (int32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
        int32_t mapId;
        int32_t recordCount;
        if (!reader.Read32(mapId) || !reader.Read32(recordCount) || !FitsInt16(mapId) ||
            recordCount < 0 || static_cast<std::size_t>(recordCount) > reader.Remaining() / 0x60) {
            return soft3d::S3D_FORMAT;
        }
        GameItemGroup group;
        group.mapId = static_cast<int16_t>(mapId);
        group.records.resize(static_cast<std::size_t>(recordCount));
        for (GameItemSpawnRecord& record : group.records) {
            if (!reader.Read32(record.itemId) || record.itemId < 0 || record.itemId >= 2100 ||
                !reader.Read32(record.pickupTextId) || record.pickupTextId < 0 ||
                !reader.ReadBytes(record.packedModelName.data(), record.packedModelName.size()) ||
                !ReadVec3(reader, record.position) || !ReadVec3(reader, record.axisX) ||
                !ReadVec3(reader, record.axisY) || !ReadVec3(reader, record.axisZ) ||
                !ReadBounds(reader, record.pickupBounds)) {
                return soft3d::S3D_FORMAT;
            }
        }
        parsed.push_back(std::move(group));
    }
    if (reader.Remaining() != 0) {
        return soft3d::S3D_FORMAT;
    }
    groups = std::move(parsed);
    return soft3d::S3D_OK;
}

soft3d::result GameData::ParseEnemyInfoData(const void* data, std::size_t size,
                                            std::vector<EnemyInfoGroup>& groups)
{
    if (data == nullptr && size != 0) {
        return soft3d::S3D_PARAM;
    }
    Reader reader{static_cast<const uint8_t*>(data), size};
    int32_t groupCount;
    if (!reader.Read32(groupCount) || groupCount < 0 || groupCount > 100 ||
        static_cast<std::size_t>(groupCount) > reader.Remaining() / 8) {
        return soft3d::S3D_FORMAT;
    }

    std::vector<EnemyInfoGroup> parsed;
    parsed.reserve(static_cast<std::size_t>(groupCount));
    for (int32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
        int32_t mapId;
        int32_t recordCount;
        if (!reader.Read32(mapId) || !reader.Read32(recordCount) || !FitsInt16(mapId) ||
            recordCount < 0 || recordCount > static_cast<int32_t>(enemySpawnCapacity)) {
            return soft3d::S3D_FORMAT;
        }
        EnemyInfoGroup group;
        group.mapId = static_cast<int16_t>(mapId);
        group.records.resize(static_cast<std::size_t>(recordCount));
        for (EnemySpawnRecord& record : group.records) {
            int32_t enemyType;
            int32_t waypointCount;
            if (!reader.Read32(enemyType) || !FitsInt16(enemyType) ||
                !ReadVec3(reader, record.position) || !ReadVec3(reader, record.direction) ||
                !reader.Read32(waypointCount) || waypointCount < 0 ||
                static_cast<std::size_t>(waypointCount) > reader.Remaining() / 0x1c ||
                waypointCount > std::numeric_limits<int16_t>::max()) {
                return soft3d::S3D_FORMAT;
            }
            record.enemyType = static_cast<int16_t>(enemyType);
            record.waypoints.resize(static_cast<std::size_t>(waypointCount));
            for (EnemyWaypoint& waypoint : record.waypoints) {
                if (!ReadVec3(reader, waypoint.position) || !reader.Read32(waypoint.waitTime) ||
                    !ReadVec3(reader, waypoint.direction)) {
                    return soft3d::S3D_FORMAT;
                }
            }
        }
        parsed.push_back(std::move(group));
    }
    if (reader.Remaining() != 0) {
        return soft3d::S3D_FORMAT;
    }
    groups = std::move(parsed);
    return soft3d::S3D_OK;
}

soft3d::result GameData::LoadItemData(soft3d::s3dFilePak& filePak, soft3d::GameContent& content)
{
    std::vector<uint8_t> data;
    const soft3d::result status = ReadArchiveEntry(filePak, u".\\common\\itemdata.sdt", data);
    return status == soft3d::S3D_OK ? ParseItemData(data.data(), data.size(), content) : status;
}

soft3d::result GameData::LoadDayContent(int day, soft3d::s3dFilePak& filePak)
{
    if (day < 1 || day > 7) {
        return soft3d::S3D_PARAM;
    }

    char16_t path[64];
    soft3d::WideSprintf(path, u".\\common\\einfo%d.sdt", day);
    std::vector<uint8_t> data;
    soft3d::result status = ReadArchiveEntry(filePak, path, data);
    if (status == soft3d::S3D_FILE && day >= 6) {
        enemyInfoGroups.clear();
    } else if (status == soft3d::S3D_OK) {
        status = ParseEnemyInfoData(data.data(), data.size(), enemyInfoGroups);
    }
    if (status != soft3d::S3D_OK && !(status == soft3d::S3D_FILE && day >= 6)) {
        return status;
    }

    soft3d::WideSprintf(path, u".\\common\\gameitem%d.sdt", day);
    status = ReadArchiveEntry(filePak, path, data);
    if (status != soft3d::S3D_OK) {
        return status;
    }
    return ParseGameItemData(data.data(), data.size(), gameItemGroups);
}

GameItemGroup* GameData::FindGameItemGroup(int32_t mapId)
{
    const auto found = std::find_if(gameItemGroups.begin(), gameItemGroups.end(),
                                    [mapId](const GameItemGroup& group) {
                                        return group.mapId == mapId;
                                    });
    return found == gameItemGroups.end() ? nullptr : &*found;
}

const GameItemGroup* GameData::FindGameItemGroup(int32_t mapId) const
{
    const auto found = std::find_if(gameItemGroups.begin(), gameItemGroups.end(),
                                    [mapId](const GameItemGroup& group) {
                                        return group.mapId == mapId;
                                    });
    return found == gameItemGroups.end() ? nullptr : &*found;
}

const EnemyInfoGroup* GameData::FindEnemyInfoGroup(int32_t mapId) const
{
    const auto found = std::find_if(enemyInfoGroups.begin(), enemyInfoGroups.end(),
                                    [mapId](const EnemyInfoGroup& group) {
                                        return group.mapId == mapId;
                                    });
    return found == enemyInfoGroups.end() ? nullptr : &*found;
}

const std::vector<GameItemGroup>& GameData::GetGameItemGroups() const
{
    return gameItemGroups;
}

const std::vector<EnemyInfoGroup>& GameData::GetEnemyInfoGroups() const
{
    return enemyInfoGroups;
}

} // namespace game

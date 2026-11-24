#include "game/Scene.hpp"

#include "soft3d/AudioData.hpp"
#include "soft3d/Endian.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/Stream.hpp"

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <limits>
#include <utility>
#include <vector>

namespace game {
namespace {

constexpr std::size_t saveHeaderSize = 44;

bool ReadLe32(soft3d::InputStream& stream, int32_t& value)
{
    uint8_t bytes[4]{};
    if (stream.Read(bytes, 4) != soft3d::S3D_OK) return false;
    value = static_cast<int32_t>(static_cast<uint32_t>(bytes[0]) |
                                 static_cast<uint32_t>(bytes[1]) << 8 |
                                 static_cast<uint32_t>(bytes[2]) << 16 |
                                 static_cast<uint32_t>(bytes[3]) << 24);
    return true;
}

bool WriteLe32(soft3d::OutputStream& stream, int32_t value)
{
    const uint32_t bits = static_cast<uint32_t>(value);
    const uint8_t bytes[] = {
        static_cast<uint8_t>(bits), static_cast<uint8_t>(bits >> 8),
        static_cast<uint8_t>(bits >> 16), static_cast<uint8_t>(bits >> 24),
    };
    return stream.Write(bytes, 4) == soft3d::S3D_OK;
}

bool ReadLeVec3(soft3d::InputStream& stream, soft3d::Vec3& value)
{
    return ReadLe32(stream, value.x) && ReadLe32(stream, value.y) && ReadLe32(stream, value.z);
}

bool WriteLeVec3(soft3d::OutputStream& stream, const soft3d::Vec3& value)
{
    return WriteLe32(stream, value.x) && WriteLe32(stream, value.y) && WriteLe32(stream, value.z);
}

bool ReadLeBounds(soft3d::InputStream& stream, soft3d::ObjectBounds& value)
{
    return ReadLeVec3(stream, value.minimum) && ReadLeVec3(stream, value.maximum);
}

bool WriteLeBounds(soft3d::OutputStream& stream, const soft3d::ObjectBounds& value)
{
    return WriteLeVec3(stream, value.minimum) && WriteLeVec3(stream, value.maximum);
}

bool ReadWaypoint(soft3d::InputStream& stream, EnemyWaypoint& waypoint)
{
    return ReadLeVec3(stream, waypoint.position) && ReadLe32(stream, waypoint.waitTime) &&
           ReadLeVec3(stream, waypoint.direction);
}

bool WriteWaypoint(soft3d::OutputStream& stream, const EnemyWaypoint& waypoint)
{
    return WriteLeVec3(stream, waypoint.position) && WriteLe32(stream, waypoint.waitTime) &&
           WriteLeVec3(stream, waypoint.direction);
}

bool ReadGameItem(soft3d::InputStream& stream, GameItemSpawnRecord& record)
{
    return ReadLe32(stream, record.itemId) && ReadLe32(stream, record.pickupTextId) &&
           stream.Read(record.packedModelName.data(), record.packedModelName.size()) == soft3d::S3D_OK &&
           ReadLeVec3(stream, record.position) && ReadLeVec3(stream, record.axisX) &&
           ReadLeVec3(stream, record.axisY) && ReadLeVec3(stream, record.axisZ) &&
           ReadLeBounds(stream, record.pickupBounds);
}

bool WriteGameItem(soft3d::OutputStream& stream, const GameItemSpawnRecord& record)
{
    return WriteLe32(stream, record.itemId) && WriteLe32(stream, record.pickupTextId) &&
           stream.Write(record.packedModelName.data(), record.packedModelName.size()) == soft3d::S3D_OK &&
           WriteLeVec3(stream, record.position) && WriteLeVec3(stream, record.axisX) &&
           WriteLeVec3(stream, record.axisY) && WriteLeVec3(stream, record.axisZ) &&
           WriteLeBounds(stream, record.pickupBounds);
}

std::size_t Remaining(const soft3d::InputStream& stream)
{
    return static_cast<std::size_t>(std::max(0, stream.GetSize() - stream.Tell()));
}

uint32_t DecodeLe32(const uint8_t* bytes)
{
    return soft3d::Read32LE(bytes);
}

bool ReadHeader(soft3d::GameEngine& engine, int slot, soft3d::SaveSlotInfo& header)
{
    char16_t name[32];
    soft3d::WideSprintf(name, u"slot%d.sav", slot);
    const std::u16string path = engine.PersistencePath(name);
    soft3d::s3dIO file;
    if (!file.Open(path.c_str(), u"rb")) return false;
    std::array<uint8_t, saveHeaderSize> bytes{};
    const uint32_t read = file.Read(bytes.data(), bytes.size());
    file.Close();
    if (read != bytes.size()) return false;
    for (std::size_t index = 0; index < header.words.size(); ++index) {
        header.words[index] = DecodeLe32(bytes.data() + index * 4);
    }
    header.present = true;
    return true;
}

bool FileExists(const std::u16string& path)
{
    soft3d::s3dIO file;
    if (!file.Open(path.c_str(), u"rb")) return false;
    file.Close();
    return true;
}

std::u16string SidecarPath(soft3d::GameEngine& engine, int slot)
{
    char16_t name[32];
    soft3d::WideSprintf(name, u"spt\\slot%d.sav", slot);
    return engine.PersistencePath(name);
}

void SetTimestamp(soft3d::SaveSlotInfo& header)
{
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    std::array<uint16_t, 6> fields{};
    if (local != nullptr) {
        fields = {
            static_cast<uint16_t>(local->tm_year + 1900), static_cast<uint16_t>(local->tm_mon),
            static_cast<uint16_t>(local->tm_mday), static_cast<uint16_t>(local->tm_hour),
            static_cast<uint16_t>(local->tm_min), static_cast<uint16_t>(local->tm_sec),
        };
    }
    for (std::size_t index = 0; index < 3; ++index) {
        header.words[index + 2] = fields[index * 2] |
                                  static_cast<uint32_t>(fields[index * 2 + 1]) << 16;
    }
}

soft3d::GameEngine* ManagerEngine(const soft3d::UnitManagerContext& context)
{
    return static_cast<soft3d::GameEngine*>(context.gameContext);
}

} // namespace

soft3d::result MyGameUnitMgr::SaveState(soft3d::OutputStream& stream) const
{
    for (const auto& record : scriptTimers) {
        for (int32_t value : record) {
            if (!WriteLe32(stream, value)) return soft3d::S3D_MEMORY;
        }
    }
    for (int32_t value : doorStates) {
        if (!WriteLe32(stream, value)) return soft3d::S3D_MEMORY;
    }

    const auto& enemyGroups = data.GetEnemyInfoGroups();
    if (stream.WriteInt32(static_cast<int32_t>(enemyGroups.size())) != soft3d::S3D_OK) {
        return soft3d::S3D_MEMORY;
    }
    for (const EnemyInfoGroup& group : enemyGroups) {
        if (group.records.size() > enemySpawnCapacity) return soft3d::S3D_FORMAT;
        stream.WriteInt16(group.mapId);
        stream.WriteInt16(static_cast<int16_t>(group.records.size()));
        for (const EnemySpawnRecord& record : group.records) {
            if (record.waypoints.size() > std::numeric_limits<int16_t>::max()) {
                return soft3d::S3D_FORMAT;
            }
            soft3d::WriteVec3(stream, record.position);
            soft3d::WriteVec3(stream, record.direction);
            stream.WriteInt16(record.active);
            stream.WriteInt16(record.enemyType);
            stream.WriteInt16(record.unitId);
            stream.WriteInt16(static_cast<int16_t>(record.waypoints.size()));
            for (const EnemyWaypoint& waypoint : record.waypoints) {
                if (!WriteWaypoint(stream, waypoint)) return soft3d::S3D_MEMORY;
            }
        }
    }

    const auto& itemGroups = data.GetGameItemGroups();
    if (stream.WriteInt32(static_cast<int32_t>(itemGroups.size())) != soft3d::S3D_OK) {
        return soft3d::S3D_MEMORY;
    }
    for (const GameItemGroup& group : itemGroups) {
        if (group.records.size() > std::numeric_limits<int16_t>::max()) return soft3d::S3D_FORMAT;
        stream.WriteInt16(group.mapId);
        stream.WriteInt16(static_cast<int16_t>(group.records.size()));
        for (const GameItemSpawnRecord& record : group.records) {
            if (!WriteGameItem(stream, record)) return soft3d::S3D_MEMORY;
        }
    }

    soft3d::GameEngine* engine = ManagerEngine(GetContext());
    if (engine == nullptr) return soft3d::S3D_STATE;
    for (const soft3d::GameItemRecord& item : engine->GetContent().items) {
        stream.WriteInt32(item.runtimeState);
    }

    for (int bucket = 0; bucket < 8; ++bucket) {
        if (bucket == 6 || bucket == 7) continue;
        for (soft3d::GameUnit* unit = GetFirstUnit(bucket); unit != nullptr;
             unit = unit->GetNextUnit()) {
            if (unit->GetUnitId() != -1) unit->SaveState(stream);
        }
    }
    for (int32_t index = 0; index < stayBoxCount; ++index) {
        const StayBox& box = stayBoxes[static_cast<std::size_t>(index)];
        if (box.GetUnitId() != -1) box.SaveState(stream);
    }
    return soft3d::S3D_OK;
}

soft3d::result MyGameUnitMgr::LoadUnitState(soft3d::InputStream& stream)
{
    Reset();
    for (auto& pool : enemyPools) {
        for (auto& enemy : pool) {
            if (enemy != nullptr) enemy->Reset();
        }
    }
    for (auto& record : scriptTimers) {
        for (int32_t& value : record) {
            if (!ReadLe32(stream, value)) return soft3d::S3D_FORMAT;
        }
    }
    for (int32_t& value : doorStates) {
        if (!ReadLe32(stream, value)) return soft3d::S3D_FORMAT;
    }

    int32_t enemyGroupCount = 0;
    if (stream.ReadInt32(enemyGroupCount) != soft3d::S3D_OK || enemyGroupCount < 0 ||
        enemyGroupCount > static_cast<int32_t>(enemyInfoGroupCapacity)) {
        return soft3d::S3D_FORMAT;
    }
    std::vector<EnemyInfoGroup> enemyGroups;
    enemyGroups.reserve(static_cast<std::size_t>(enemyGroupCount));
    for (int32_t groupIndex = 0; groupIndex < enemyGroupCount; ++groupIndex) {
        EnemyInfoGroup group;
        int16_t recordCount = 0;
        if (stream.ReadInt16(group.mapId) != soft3d::S3D_OK ||
            stream.ReadInt16(recordCount) != soft3d::S3D_OK || recordCount < 0 ||
            recordCount > static_cast<int16_t>(enemySpawnCapacity)) {
            return soft3d::S3D_FORMAT;
        }
        group.records.reserve(static_cast<std::size_t>(recordCount));
        for (int16_t recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
            EnemySpawnRecord record;
            int16_t waypointCount = 0;
            if (soft3d::ReadVec3(stream, record.position) != soft3d::S3D_OK ||
                soft3d::ReadVec3(stream, record.direction) != soft3d::S3D_OK ||
                stream.ReadInt16(record.active) != soft3d::S3D_OK ||
                stream.ReadInt16(record.enemyType) != soft3d::S3D_OK ||
                stream.ReadInt16(record.unitId) != soft3d::S3D_OK ||
                stream.ReadInt16(waypointCount) != soft3d::S3D_OK || waypointCount < 0 ||
                static_cast<std::size_t>(waypointCount) * 0x1c > Remaining(stream)) {
                return soft3d::S3D_FORMAT;
            }
            record.waypoints.reserve(static_cast<std::size_t>(waypointCount));
            for (int16_t waypointIndex = 0; waypointIndex < waypointCount; ++waypointIndex) {
                EnemyWaypoint waypoint;
                if (!ReadWaypoint(stream, waypoint)) return soft3d::S3D_FORMAT;
                record.waypoints.push_back(waypoint);
            }
            group.records.push_back(std::move(record));
        }
        enemyGroups.push_back(std::move(group));
    }

    int32_t itemGroupCount = 0;
    if (stream.ReadInt32(itemGroupCount) != soft3d::S3D_OK || itemGroupCount < 0 ||
        static_cast<std::size_t>(itemGroupCount) > Remaining(stream) / 8) {
        return soft3d::S3D_FORMAT;
    }
    std::vector<GameItemGroup> itemGroups;
    itemGroups.reserve(static_cast<std::size_t>(itemGroupCount));
    for (int32_t groupIndex = 0; groupIndex < itemGroupCount; ++groupIndex) {
        GameItemGroup group;
        int16_t recordCount = 0;
        if (stream.ReadInt16(group.mapId) != soft3d::S3D_OK ||
            stream.ReadInt16(recordCount) != soft3d::S3D_OK || recordCount < 0 ||
            static_cast<std::size_t>(recordCount) * 0x60 > Remaining(stream)) {
            return soft3d::S3D_FORMAT;
        }
        group.records.reserve(static_cast<std::size_t>(recordCount));
        for (int16_t recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
            GameItemSpawnRecord record;
            if (!ReadGameItem(stream, record)) return soft3d::S3D_FORMAT;
            group.records.push_back(record);
        }
        itemGroups.push_back(std::move(group));
    }

    soft3d::GameEngine* engine = ManagerEngine(GetContext());
    if (engine == nullptr) return soft3d::S3D_STATE;
    for (soft3d::GameItemRecord& item : engine->GetContent().items) {
        if (stream.ReadInt32(item.runtimeState) != soft3d::S3D_OK) return soft3d::S3D_FORMAT;
    }
    data.RestoreEnemyInfoGroups(std::move(enemyGroups));
    data.RestoreGameItemGroups(std::move(itemGroups));
    stayBoxCount = 0;

    auto* concretePlayer = dynamic_cast<Player*>(player);
    auto* scene = static_cast<Scene*>(GetContext().sceneContext);
    soft3d::ControlMap* controls = engine != nullptr ? &engine->GetControlMap() : nullptr;
    std::vector<std::pair<std::unique_ptr<soft3d::GameUnit>, int>> pendingUnits;
    std::vector<Enemy*> pendingEnemies;
    while (!stream.IsEnd()) {
        int32_t type = 0;
        if (stream.ReadInt32(type) != soft3d::S3D_OK) return soft3d::S3D_FORMAT;
        std::unique_ptr<soft3d::GameUnit> unit;
        int bucket = -1;
        switch (type) {
        case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
        case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a: {
            soft3d::InputDataStream savedEnemy;
            savedEnemy.SetData(stream.GetData() + stream.Tell(), Remaining(stream));
            Enemy probe(GetContext().resources, GetContext().renderer, GetContext().audio,
                        GetContext().bsp);
            probe.LoadState(savedEnemy);
            if (savedEnemy.HasError() || probe.GetEnemyType() < 0 ||
                probe.GetEnemyType() >= static_cast<int32_t>(enemyTypeCount)) {
                return soft3d::S3D_FORMAT;
            }
            Enemy* enemy = nullptr;
            for (std::size_t slot = 0; slot < enemyPoolStride && enemy == nullptr; ++slot) {
                Enemy* candidate = GetPooledEnemy(probe.GetEnemyType(), slot);
                if (candidate != nullptr && candidate->GetState() == EnemyRemoved) enemy = candidate;
            }
            if (enemy == nullptr) return soft3d::S3D_FORMAT;
            enemy->SetManager(this);
            enemy->SetPlayer(concretePlayer);
            enemy->LoadState(stream);
            if (stream.HasError()) return soft3d::S3D_FORMAT;
            for (EnemyInfoGroup& group : data.GetEnemyInfoGroups()) {
                for (EnemySpawnRecord& record : group.records) {
                    if (record.unitId == enemy->GetUnitId()) {
                        enemy->SetSpawnRecord(&record);
                        enemy->SetStayBoxReturnPosition(record.position);
                        for (const EnemyWaypoint& waypoint : record.waypoints) {
                            enemy->AddWaypoint(waypoint.position, waypoint.waitTime, waypoint.direction);
                        }
                        break;
                    }
                }
            }
            pendingEnemies.push_back(enemy);
            continue;
        }
        case 0:
            unit = std::make_unique<soft3d::GameUnit>();
            bucket = 1;
            break;
        case 4:
            unit = std::make_unique<soft3d::Things>(GetContext().resources, 3,
                GetContext().renderer, GetContext().audio, GetContext().bsp);
            bucket = 2;
            break;
        case 0x1b: {
            auto trigger = std::make_unique<EventTrigger>(concretePlayer, interactionUi);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        case 0x1c:
            unit = std::make_unique<Breakable>(GetContext().resources, 3, GetContext().renderer,
                                               GetContext().audio, GetContext().bsp);
            bucket = 2;
            break;
        case 0x1d:
            unit = std::make_unique<Pushable>(GetContext().resources, 3, GetContext().renderer,
                                              GetContext().audio, GetContext().bsp);
            bucket = 2;
            break;
        case 0x1e:
            unit = std::make_unique<Pickable>(GetContext().resources, 3, GetContext().renderer,
                                              GetContext().audio, GetContext().bsp, this);
            bucket = 2;
            break;
        case 0x1f:
            unit = std::make_unique<Trap>(concretePlayer);
            bucket = 5;
            break;
        case 0x21: {
            auto trigger = std::make_unique<ActTrigger>(cameraController, scene, concretePlayer,
                                                        controls, interactionUi);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        case 0x22: {
            auto trigger = std::make_unique<DoorTrigger>(scene, this, concretePlayer,
                                                         controls, interactionUi);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        case 0x23: {
            auto trigger = std::make_unique<HideTrigger>(concretePlayer, controls, interactionUi);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        case 0x25: {
            auto trigger = std::make_unique<PickTrigger>(this, concretePlayer, controls, interactionUi);
            trigger->LoadState(stream);
            int32_t linkedThing = 0;
            if (stream.ReadInt32(linkedThing) != soft3d::S3D_OK) return soft3d::S3D_FORMAT;
            Pickable* pickable = nullptr;
            if (linkedThing != 0) {
                for (const auto& pending : pendingUnits) {
                    if (pending.second == 2 && pending.first->GetUnitId() == linkedThing) {
                        pickable = dynamic_cast<Pickable*>(pending.first.get());
                        break;
                    }
                }
                if (pickable == nullptr) return soft3d::S3D_FORMAT;
            }
            trigger->Configure(pickable, this, concretePlayer, controls, interactionUi);
            if (stream.HasError()) return soft3d::S3D_FORMAT;
            pendingUnits.emplace_back(std::move(trigger), 4);
            continue;
        }
        case 0x26: {
            StayBox* box = RestoreStayBox();
            if (box == nullptr) return soft3d::S3D_FORMAT;
            box->LoadState(stream);
            if (stream.HasError()) return soft3d::S3D_FORMAT;
            continue;
        }
        case 0x27: {
            auto trigger = std::make_unique<ItemTrigger>(concretePlayer);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        case 0x28: {
            auto trigger = std::make_unique<TalkTrigger>(scene, this, concretePlayer,
                                                         controls, interactionUi);
            unit = std::move(trigger);
            bucket = 4;
            break;
        }
        default:
            return soft3d::S3D_FORMAT;
        }
        unit->LoadState(stream);
        if (stream.HasError()) return soft3d::S3D_FORMAT;
        pendingUnits.emplace_back(std::move(unit), bucket);
    }
    if (stream.HasError()) return soft3d::S3D_FORMAT;
    for (auto current = pendingEnemies.rbegin(); current != pendingEnemies.rend(); ++current) {
        AddUnit(*current, 0);
    }
    for (auto current = pendingUnits.rbegin(); current != pendingUnits.rend(); ++current) {
        AddUnit(current->first.release(), current->second);
    }
    return soft3d::S3D_OK;
}

bool Scene::SaveSaveSlot(int slot)
{
    if (!initialized || player == nullptr || unitManager == nullptr || slot < 1 || slot > 3) {
        return false;
    }
    player->SnapshotHeaderStatistics();
    soft3d::SaveSlotInfo header;
    header.present = true;
    header.words[0] = static_cast<uint32_t>(day);
    header.words[1] = static_cast<uint32_t>(mapId);
    SetTimestamp(header);
    uint32_t saveNumber = 0;
    for (const soft3d::SaveSlotInfo& current : engine.GetSaveSlots()) {
        if (current.present) saveNumber = std::max(saveNumber, current.GetSaveNumber());
    }
    header.words[5] = saveNumber + 1;
    header.words[6] = 1;
    header.words[7] = static_cast<uint32_t>(player->GetItemUseCount());
    header.words[8] = static_cast<uint32_t>(player->GetKillCount());
    header.words[9] = static_cast<uint32_t>(player->GetPlayTime());
    header.words[10] = static_cast<uint32_t>(player->GetDeathCount());

    char16_t name[32];
    soft3d::WideSprintf(name, u"slot%d.sav", slot);
    const std::u16string path = engine.PersistencePath(name);
    soft3d::OutputFileStream stream(path.c_str(), &engine);
    if (!stream.IsOpen()) return false;
    for (uint32_t word : header.words) {
        if (!WriteLe32(stream, static_cast<int32_t>(word))) return false;
    }
    stream.WriteInt32(day);
    stream.WriteInt32(mapId);
    const char16_t* music = engine.GetAudioManager().GetMusicName();
    if (stream.WriteWideString(music != nullptr ? music : u"") != soft3d::S3D_OK) return false;
    for (const EnvironmentChannel& channel : environment) {
        stream.WriteInt32(channel.baseDelay);
        stream.WriteInt32(channel.randomDelay);
        stream.WriteInt32(channel.resourceCount);
        for (int32_t index = 0; index < channel.resourceCount; ++index) {
            const soft3d::AudioData* resource = channel.resources[static_cast<std::size_t>(index)];
            stream.WriteWideString(resource != nullptr ? resource->GetFileName() : u"");
        }
    }
    if (stream.Write(eventFlags.data(), eventFlags.size()) != soft3d::S3D_OK ||
        stream.Write(dayStartSnapshot.data(), dayStartSnapshot.size()) != soft3d::S3D_OK) {
        return false;
    }
    stream.WriteInt32(engine.GetGameUi()->IsGunHudEnabled());
    player->SaveState(stream);
    if (unitManager->SaveState(stream) != soft3d::S3D_OK || stream.Flush() != soft3d::S3D_OK) {
        return false;
    }

    const std::u16string sidecar = SidecarPath(engine, slot);
    std::u16string sidecarDirectory = engine.PersistencePath(u"spt");
    std::replace(sidecarDirectory.begin(), sidecarDirectory.end(), u'\\', u'/');
    std::error_code error;
    std::filesystem::create_directories(std::filesystem::path(sidecarDirectory), error);
    if (error || engine.GetScriptEngine().SaveArchive(sidecar.c_str()) != soft3d::S3D_OK) return false;
    engine.GetSaveSlots()[static_cast<std::size_t>(slot - 1)] = header;
    return true;
}

bool Scene::LoadSaveSlot(int slot)
{
    if (!initialized || player == nullptr || unitManager == nullptr || slot < 1 || slot > 3) {
        return false;
    }
    soft3d::SaveSlotInfo header;
    if (!ReadHeader(engine, slot, header) || !FileExists(SidecarPath(engine, slot))) return false;

    player->ResetAll(unitManager->GetData());
    player->RestoreHeaderStatistics(static_cast<int32_t>(header.words[7]),
                                    static_cast<int32_t>(header.words[8]),
                                    static_cast<int32_t>(header.words[9]),
                                    static_cast<int32_t>(header.words[10]));
    engine.GetScriptEngine().Reset();
    cameraController.Reset();
    restoreSavedState = 1;
    saveSlotPending = 0;
    nextLevelPending = 0;
    dayIntroPending = 0;
    showDayTitle = 0;
    transitionStage = 0;
    transitionTimer = 0;
    SetLoadMapParams(0, {}, {0, 0, soft3d::fixedOne}, 0);
    UnloadLevel();
    mapProgress = 0;

    char16_t name[32];
    soft3d::WideSprintf(name, u"slot%d.sav", slot);
    soft3d::InputFileStream stream(engine.PersistencePath(name).c_str(), &engine);
    if (stream.GetSize() < static_cast<int32_t>(saveHeaderSize) ||
        stream.Seek(saveHeaderSize, SEEK_SET) != soft3d::S3D_OK) {
        return false;
    }
    if (stream.ReadInt32(day) != soft3d::S3D_OK || day < 1 || day > 7 ||
        stream.ReadInt32(mapId) != soft3d::S3D_OK) {
        return false;
    }
    engine.GetContent().mapId = mapId;
    engine.GetGameUi()->LoadDayResources(day);
    soft3d::StringManager* font = engine.GetBaseFont();
    font->Clear();
    char16_t fontPath[32];
    soft3d::WideSprintf(fontPath, engine.GetLanguage() == 2 ? u".\\uien\\day%d.sbn"
                                                             : u".\\ui\\day%d.sbn", day);
    font->LoadSbnFont(fontPath, engine.GetResourceManager().GetFilePak());

    char16_t music[256]{};
    if (stream.ReadWideString(music, 256) != soft3d::S3D_OK) return false;
    if (music[0] != u'\0') engine.GetAudioManager().PlayMusic(music, true, 0);
    for (EnvironmentChannel& channel : environment) {
        if (stream.ReadInt32(channel.baseDelay) != soft3d::S3D_OK ||
            stream.ReadInt32(channel.randomDelay) != soft3d::S3D_OK ||
            stream.ReadInt32(channel.resourceCount) != soft3d::S3D_OK ||
            channel.resourceCount < 0 || channel.resourceCount > 4) {
            return false;
        }
        for (int32_t index = 0; index < channel.resourceCount; ++index) {
            char16_t resourceName[256]{};
            if (stream.ReadWideString(resourceName, 256) != soft3d::S3D_OK) return false;
            channel.resources[static_cast<std::size_t>(index)] = static_cast<soft3d::AudioData*>(
                engine.GetResourceManager().GetResource(soft3d::ResourceAudio, 3, resourceName,
                                                        u".\\audio\\"));
        }
        channel.elapsed = 0;
        channel.nextDelay = -1;
        channel.voice = 0;
    }
    if (stream.Read(eventFlags.data(), eventFlags.size()) != soft3d::S3D_OK ||
        stream.Read(dayStartSnapshot.data(), dayStartSnapshot.size()) != soft3d::S3D_OK) {
        return false;
    }
    unitManager->PrepareDayEnemies(day);
    int32_t gunHud = 0;
    if (stream.ReadInt32(gunHud) != soft3d::S3D_OK) return false;
    engine.GetGameUi()->SetGunHudEnabled(gunHud);
    player->LoadState(stream);
    if (stream.HasError() || unitManager->LoadUnitState(stream) != soft3d::S3D_OK) return false;
    unitManager->RestoreDayAndMap(day, mapId);
    if (engine.GetScriptEngine().LoadArchive(SidecarPath(engine, slot).c_str(),
            engine.GetResourceManager().GetFilePak()) != soft3d::S3D_OK) {
        return false;
    }

    BeginMapLoad();
    transitionStage = 1;
    SetLoadMapParams(0, player->GetPosition(), player->GetForward(), 0);
    engine.GetSaveSlots()[static_cast<std::size_t>(slot - 1)] = header;
    return true;
}

} // namespace game

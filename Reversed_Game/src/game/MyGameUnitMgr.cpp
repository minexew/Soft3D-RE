#include "game/MyGameUnitMgr.hpp"

#include "game/CameraController.hpp"
#include "game/MaidBully.hpp"
#include "game/NamedEnemies.hpp"
#include "game/Scene.hpp"
#include "game/SpecialEnemies.hpp"
#include "game/Triggers.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/ObjectRes.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace game {
namespace {

std::string EntityText(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp, int index)
{
    if (index < 0 || index >= record.pairCount) {
        return {};
    }
    const std::size_t pairIndex = static_cast<std::size_t>(record.firstPair + index);
    if (pairIndex >= bsp.GetEntityPairs().size()) {
        return {};
    }
    const auto& pair = bsp.GetEntityPairs()[pairIndex];
    char text[33];
    std::memcpy(text, pair.key.data(), 16);
    std::memcpy(text + 16, pair.value.data(), 16);
    text[32] = '\0';
    return text;
}

soft3d::Fixed ParseFixed(const std::string& text)
{
    const char* begin = text.c_str();
    while (std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }
    if (std::strchr(begin, '.') == nullptr && std::strchr(begin, 'e') == nullptr) {
        const char* digits = begin;
        if (*digits == '-' || *digits == '+') {
            ++digits;
        }
        return soft3d::ShiftLeftFixed(static_cast<int32_t>(std::strtol(digits, nullptr, 10)), 16);
    }
    return static_cast<soft3d::Fixed>(std::strtod(begin, nullptr) * 65536.0);
}

soft3d::Vec3 ParseVector(const std::string& text)
{
    soft3d::Vec3 result;
    const std::size_t first = text.find(',');
    const std::size_t second = first == std::string::npos ? first : text.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        return result;
    }
    const std::size_t begin = text.find('(') == std::string::npos ? 0 : text.find('(') + 1;
    result.x = ParseFixed(text.substr(begin, first - begin));
    result.y = ParseFixed(text.substr(first + 1, second - first - 1));
    result.z = ParseFixed(text.substr(second + 1));
    return result;
}

} // namespace

MyGameUnitMgr::MyGameUnitMgr(const soft3d::UnitManagerContext& context)
    : GameUnitManager(context)
    , interactionState(this)
{
    for (auto& record : serialized19c) {
        record.fill(-1);
    }
    auto* engine = static_cast<soft3d::GameEngine*>(context.gameContext);
    for (auto& lensflare : lensflares) {
        lensflare = std::make_unique<soft3d::Lensflare>(context.renderer,
            engine != nullptr ? &engine->GetScreenBlitter() : nullptr, context.resources);
    }
}

soft3d::result MyGameUnitMgr::Initialize(soft3d::GameEngine& engine)
{
    engine.GetAudioManager().SetSoundNameTable(data.GetSoundNameTable());
    const soft3d::result status = data.LoadItemData(*engine.GetResourceManager().GetFilePak(),
                                                     engine.GetContent());
    if (status != soft3d::S3D_OK) {
        return status;
    }
    for (int32_t type = 0; type < static_cast<int32_t>(enemyTypeCount); ++type) {
        const std::size_t count = IsSingletonEnemyType(type) ? 1 : enemyPoolStride;
        for (std::size_t slot = 0; slot < count; ++slot) {
            std::unique_ptr<Enemy> enemy = CreateMaidBullyPoolEnemy(GetContext(), data, type);
            if (enemy == nullptr) {
                enemy = CreateSpecialPoolEnemy(GetContext(), data, type);
            }
            if (enemy == nullptr) {
                enemy = CreateNamedPoolEnemy(GetContext(), data, type);
            }
            if (enemy == nullptr) {
                throw std::runtime_error("No concrete enemy class for pool type");
            }
            enemyPools[static_cast<std::size_t>(type)][slot] = std::move(enemy);
        }
    }
    return soft3d::S3D_OK;
}

soft3d::result MyGameUnitMgr::LoadDayContent(int value, soft3d::GameEngine& engine)
{
    const soft3d::result status = data.LoadDayContent(value, *engine.GetResourceManager().GetFilePak());
    doorStates.fill(0);
    if (status == soft3d::S3D_OK) {
        day = value;
    }
    return status;
}

void MyGameUnitMgr::SetMapId(int32_t value)
{
    UpdateEnemySpawnRecords();
    mapId = value;
    for (auto& pool : enemyPools) {
        for (auto& enemy : pool) {
            if (enemy != nullptr) {
                enemy->Reset();
            }
        }
    }
}

void MyGameUnitMgr::Reset()
{
    for (int bucket : {1, 2, 4, 5, 6, 7}) {
        ClearBucket(bucket, true);
    }
    ResetBuckets();
    lensflareCount = 0;
    mapReady = false;
    paused = false;
    for (auto& record : serialized19c) {
        record.fill(-1);
    }
    pushTarget = nullptr;
    interactionState.Reset();
}

void MyGameUnitMgr::FinishMapLoad(bool restoreSavedState)
{
    if (!restoreSavedState) {
        SpawnMapItems();
        SpawnMapEnemies();
    }
    stayBoxCount = 0;
    mapReady = true;
    paused = false;
}

void MyGameUnitMgr::ConfigureScene(CameraController* newCameraController,
                                   soft3d::Creature* newPlayer)
{
    cameraController = newCameraController;
    player = newPlayer;
    for (auto& pool : enemyPools) {
        for (auto& enemy : pool) {
            if (enemy != nullptr) {
                enemy->SetPlayer(dynamic_cast<Player*>(player));
                enemy->SetManager(this);
            }
        }
    }
}

Enemy* MyGameUnitMgr::GetPooledEnemy(int32_t enemyType, std::size_t slot) const
{
    if (enemyType < 0 || static_cast<std::size_t>(enemyType) >= enemyTypeCount ||
        slot >= enemyPoolStride) {
        return nullptr;
    }
    return enemyPools[static_cast<std::size_t>(enemyType)][slot].get();
}

Enemy* MyGameUnitMgr::FindEnemy(int32_t unitId) const
{
    return dynamic_cast<Enemy*>(FindUnit(0, unitId));
}

bool MyGameUnitMgr::AddEnemy(int32_t unitId, int32_t enemyType, int32_t spawnState,
                             soft3d::Vec3& requestedPosition, const soft3d::Vec3& direction,
                             bool fromEnemyInfo)
{
    if (spawnState == 0 || enemyType < 0 ||
        static_cast<std::size_t>(enemyType) >= enemyTypeCount) {
        return false;
    }
    if (!fromEnemyInfo) {
        const EnemyInfoGroup* group = data.FindEnemyInfoGroup(mapId);
        if (group != nullptr) {
            for (const EnemySpawnRecord& record : group->records) {
                if (record.unitId == unitId) {
                    return false;
                }
            }
        }
    }
    Enemy* enemy = nullptr;
    for (std::size_t slot = 0; slot < enemyPoolStride; ++slot) {
        Enemy* candidate = GetPooledEnemy(enemyType, slot);
        if (candidate != nullptr && candidate->GetState() == EnemyRemoved) {
            enemy = candidate;
            break;
        }
    }
    if (enemy == nullptr) {
        return false;
    }

    enemy->SetPosition(requestedPosition);
    enemy->CheckGroundForSpawn();
    soft3d::ObjectBounds bounds = enemy->GetLocalBounds();
    bounds.minimum = soft3d::Add(bounds.minimum, enemy->GetPosition());
    bounds.maximum = soft3d::Add(bounds.maximum, enemy->GetPosition());
    enemy->SetCollisionBounds(bounds);
    enemy->QueryUnitCollision();
    bool valid = !enemy->HasUnitCollision();
    if (!valid) {
        for (soft3d::Fixed x = -0x40000; x < 0x40000 && !valid; x += 0x20000) {
            for (soft3d::Fixed z = -0x40000; z < 0x40000; z += 0x20000) {
                if (x == 0 && z == 0) {
                    continue;
                }
                soft3d::Vec3 candidate = requestedPosition;
                candidate.x = soft3d::AddFixed(candidate.x, x);
                candidate.z = soft3d::AddFixed(candidate.z, z);
                enemy->SetPosition(candidate);
                enemy->CheckGroundForSpawn();
                bounds = enemy->GetLocalBounds();
                bounds.minimum = soft3d::Add(bounds.minimum, candidate);
                bounds.maximum = soft3d::Add(bounds.maximum, candidate);
                enemy->SetCollisionBounds(bounds);
                enemy->QueryUnitCollision();
                const bool solid = GetContext().bsp != nullptr &&
                    GetContext().bsp->Trace(candidate, candidate, enemy->GetLocalBounds()).startSolid;
                if (!enemy->HasUnitCollision() && !solid) {
                    requestedPosition = candidate;
                    valid = true;
                    break;
                }
            }
        }
    }
    if (!valid) {
        return false;
    }

    enemy->SetUnitId(unitId);
    enemy->SetPlayer(dynamic_cast<Player*>(player));
    enemy->SetManager(this);
    enemy->Spawn(requestedPosition);
    enemy->CheckGroundForSpawn();
    enemy->SetHealth(spawnState < 0 ? enemy->GetMaxHealth() : spawnState);
    AddUnit(enemy, 0);
    enemy->SetNavigationEnabled(paused);
    if (direction.x != 0 || direction.y != 0 || direction.z != 0) {
        enemy->SetDirection(direction);
    }
    return true;
}

void MyGameUnitMgr::RemoveEnemy(Enemy* enemy)
{
    if (enemy == nullptr) {
        return;
    }
    enemy->Reset();
    RemoveUnit(enemy, 0, false);
}

void MyGameUnitMgr::RemoveEnemiesByType(int32_t enemyType)
{
    soft3d::GameUnit* unit = GetFirstUnit(0);
    while (unit != nullptr) {
        soft3d::GameUnit* next = unit->GetNextUnit();
        auto* enemy = dynamic_cast<Enemy*>(unit);
        if (enemy != nullptr && enemy->GetEnemyType() == enemyType) {
            RemoveEnemy(enemy);
        }
        unit = next;
    }
}

void MyGameUnitMgr::RemoveAllEnemies()
{
    soft3d::GameUnit* unit = GetFirstUnit(0);
    while (unit != nullptr) {
        soft3d::GameUnit* next = unit->GetNextUnit();
        RemoveEnemy(dynamic_cast<Enemy*>(unit));
        unit = next;
    }
}

void MyGameUnitMgr::PrepareDayEnemies(int32_t value)
{
    auto prepare = [this](int32_t type, const char16_t* model) {
        const std::size_t count = IsSingletonEnemyType(type) ? 1 : enemyPoolStride;
        for (std::size_t slot = 0; slot < count; ++slot) {
            Enemy* enemy = GetPooledEnemy(type, slot);
            if (enemy != nullptr) {
                enemy->LoadResources(model, nullptr, nullptr);
            }
        }
    };
    auto* concretePlayer = dynamic_cast<Player*>(player);
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    const bool extraWeapon = engine != nullptr &&
        engine->GetConfiguration().unlocks.extraWeapon1 != 0;
    auto prepareWeapon = [&](int32_t type) {
        if (concretePlayer != nullptr) {
            concretePlayer->PrepareWeapon(data, type);
        }
    };
    switch (value) {
    case 2:
        prepare(1, u"r_bully_a");
        prepare(0, u"r_nvyong_a");
        prepare(5, u"r_weed_a");
        prepareWeapon(extraWeapon ? 4 : 1);
        break;
    case 3:
        prepare(3, u"");
        prepare(0, u"r_nvyong_a");
        prepare(4, u"r_lamper_a");
        for (std::size_t slot = 0; slot < enemyPoolStride; ++slot) {
            Enemy* enemy = GetPooledEnemy(4, slot);
            if (enemy != nullptr) {
                enemy->EnableShadow(false);
            }
        }
        prepare(5, u"r_weed_a");
        prepareWeapon(extraWeapon ? 4 : 2);
        break;
    case 4:
        prepare(0, u"r_nvyong_a");
        prepare(5, u"r_weed_a");
        prepare(6, u"r_pangnvyong_a");
        prepare(7, u"r_nvyong_b");
        break;
    case 5:
        prepare(12, u"r_victor_a");
        prepare(15, u"r_sam_b");
        prepare(9, u"r_bingnvyong_a");
        prepare(10, u"r_bug_a");
        prepare(11, u"r_qiutu_a");
        prepareWeapon(extraWeapon ? 4 : 3);
        prepareWeapon(1);
        break;
    case 6: {
        prepare(1, u"r_bully_a");
        prepare(16, u"r_kate_a");
        auto* kate = dynamic_cast<Kate*>(GetPooledEnemy(16, 0));
        auto* bully = dynamic_cast<Bully*>(GetPooledEnemy(1, 0));
        if (kate != nullptr) {
            kate->SetLinkedBully(bully);
        }
        break;
    }
    case 7:
        if (concretePlayer != nullptr) {
            concretePlayer->RemoveItem(1);
        }
        prepare(0, u"r_nvyong_a");
        prepare(3, u"");
        prepareWeapon(3);
        break;
    default:
        break;
    }
}

EnemySpawnRecord* MyGameUnitMgr::CreateEnemySpawnRecord(Enemy* enemy)
{
    return enemy != nullptr
        ? data.AddEnemySpawnRecord(mapId, enemy->GetEnemyType(), enemy->GetUnitId()) : nullptr;
}

void MyGameUnitMgr::HandleMapEntity(const soft3d::BspEntityRecord& record,
                                    const soft3d::Bsp& bsp)
{
    if (record.type == 10) {
        auto trigger = std::make_unique<CameraTrigger>(cameraController, player);
        const soft3d::result status = trigger->LoadEntity(record, bsp, *GetContext().resources);
        if (status != soft3d::S3D_OK) {
            throw std::runtime_error("CameraTrigger_LoadEntity failed with Soft3D result " +
                                     std::to_string(status));
        }
        AddUnit(trigger.release(), 6);
    } else if (record.type == 11) {
        auto trigger = std::make_unique<LightTrigger>(player);
        const soft3d::result status = trigger->LoadEntity(record, bsp);
        if (status != soft3d::S3D_OK) {
            throw std::runtime_error("LightTrigger_LoadEntity failed with Soft3D result " +
                                     std::to_string(status));
        }
        AddUnit(trigger.release(), 7);
    } else if (record.type == 102) {
        if (record.brushIndex < 0 ||
            static_cast<std::size_t>(record.brushIndex) >= bsp.GetEntityBrushes().size()) {
            throw std::runtime_error("Map header entity has no brush record");
        }
        const auto& values = bsp.GetEntityBrushes()[static_cast<std::size_t>(record.brushIndex)].values;
        for (std::size_t index = 0; index < mapHeaderValues.size(); ++index) {
            mapHeaderValues[index] = soft3d::ShiftLeftFixed(values[index], 16);
        }
    } else if (record.type == 202 && lensflareCount < lensflares.size()) {
        soft3d::Lensflare& lensflare = *lensflares[lensflareCount++];
        lensflare.LoadElement(0, u"guangyun_a", 0);
        lensflare.LoadElement(1, u"guangyun_b", 0x4ccc);
        lensflare.LoadElement(2, u"guangyun_c", 0x9999);
        lensflare.SetSource(ParseVector(EntityText(record, bsp, 0)));
    }
}

soft3d::Things* MyGameUnitMgr::AddThing(int32_t unitId, int8_t thingType, int8_t variant,
                                        bool collisionEnabled, const char16_t* modelName,
                                        const soft3d::Vec3& position, const soft3d::Vec3& axisX,
                                        const soft3d::Vec3& axisY, const soft3d::Vec3& axisZ)
{
    const soft3d::UnitManagerContext& services = GetContext();
    std::unique_ptr<soft3d::Things> thing;
    switch (thingType) {
    case 0:
        thing = std::make_unique<soft3d::Things>(services.resources, 3, services.renderer,
                                                 services.audio, services.bsp);
        break;
    case 1: {
        auto breakable = std::make_unique<Breakable>(services.resources, 3, services.renderer,
                                                     services.audio, services.bsp);
        breakable->Configure(dynamic_cast<Player*>(player), variant);
        thing = std::move(breakable);
        break;
    }
    case 2:
        thing = std::make_unique<Pushable>(services.resources, 3, services.renderer,
                                           services.audio, services.bsp);
        break;
    case 3:
        thing = std::make_unique<Pickable>(services.resources, 3, services.renderer,
                                           services.audio, services.bsp, this);
        break;
    default:
        throw std::invalid_argument("Unknown Things variant " + std::to_string(thingType));
    }
    const soft3d::result status = thing->LoadObjects(modelName, nullptr, 1);
    if (status != soft3d::S3D_OK) {
        throw std::runtime_error("Things model load failed with Soft3D result " +
                                 std::to_string(status));
    }
    thing->LoadMaterials();

    const Scene* scene = static_cast<const Scene*>(services.sceneContext);
    if (scene != nullptr) {
        const soft3d::NewGameOptions& options = scene->GetNewGameOptions();
        const char16_t* replacement = nullptr;
        if (options.weapon != 0) {
            if (std::u16string(modelName) == u"r_ken_a") replacement = u"dj_ken_ab";
            if (std::u16string(modelName) == u"r_kenh_a") replacement = u"dj_ken_bb";
        } else if (options.outfit != 0) {
            if (std::u16string(modelName) == u"r_ken_a") replacement = u"dj_ken_ac";
            if (std::u16string(modelName) == u"r_kenh_a") replacement = u"dj_ken_bc";
        }
        if (replacement != nullptr) {
            for (std::size_t index = 0; index < thing->GetObjectCount(); ++index) {
                thing->GetObject(index)->LoadTextureOverride(0, replacement, 3);
            }
        }
    }

    thing->SetUnitId(unitId);
    thing->SetPosition(position);
    thing->SetAxisX(axisX);
    thing->SetAxisY(axisY);
    thing->SetAxisZ(axisZ);
    if (thing->GetObject(0) != nullptr) {
        thing->GetObject(0)->UpdateTransform();
    }
    thing->SetCollisionEnabled(collisionEnabled);
    thing->SetUnitManager(this);
    thing->UpdateCurrentLeaf();
    soft3d::Things* result = thing.get();
    AddUnit(thing.release(), 2);
    if (thingType == 3) {
        result->SetMaterialIntensityBounce(0, 0xe666, 10);
        result->SetCollisionEnabled(false);
    }
    return result;
}

Trap* MyGameUnitMgr::AddTrap(int32_t unitId, const soft3d::ObjectBounds& bounds,
                             const soft3d::Vec3& impulse)
{
    auto trap = std::make_unique<Trap>(dynamic_cast<Player*>(player));
    trap->SetUnitId(unitId);
    trap->SetCollisionBounds(bounds);
    trap->SetImpulse(impulse);
    Trap* result = trap.get();
    AddUnit(trap.release(), 5);
    return result;
}

EventTrigger* MyGameUnitMgr::AddEventTrigger(int32_t unitId, int16_t count, int32_t textId,
                                             const soft3d::ObjectBounds& bounds)
{
    auto trigger = std::make_unique<EventTrigger>(dynamic_cast<Player*>(player), interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetRemainingCount(count);
    trigger->SetTextId(textId);
    trigger->SetCollisionBounds(bounds);
    EventTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

PickTrigger* MyGameUnitMgr::AddPickTrigger(int32_t unitId, int32_t thingId, int32_t textId,
                                           const soft3d::ObjectBounds& bounds)
{
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    auto* pickable = dynamic_cast<Pickable*>(FindUnit(2, thingId));
    auto trigger = std::make_unique<PickTrigger>(this, dynamic_cast<Player*>(player),
        engine != nullptr ? &engine->GetControlMap() : nullptr, interactionUi);
    trigger->Configure(pickable, this, dynamic_cast<Player*>(player),
        engine != nullptr ? &engine->GetControlMap() : nullptr, interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetDialogTitleId(textId);
    trigger->SetCollisionBounds(bounds);
    PickTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

DoorTrigger* MyGameUnitMgr::AddDoorTrigger(int32_t unitId, int32_t requiredItem,
        int32_t lockedText, int32_t doorResourceIndex, int32_t nextMap, int32_t nextEpisode,
        const soft3d::Vec3& spawnPosition, const soft3d::Vec3& spawnDirection,
        const soft3d::ObjectBounds& bounds)
{
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    auto trigger = std::make_unique<DoorTrigger>(static_cast<Scene*>(GetContext().sceneContext),
        this, dynamic_cast<Player*>(player), engine != nullptr ? &engine->GetControlMap() : nullptr,
        interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetRequiredItem(requiredItem);
    trigger->SetLockedText(lockedText);
    trigger->SetDoorResourceIndex(doorResourceIndex);
    trigger->SetNextMap(nextMap);
    trigger->SetNextEpisode(nextEpisode);
    trigger->SetSpawnPosition(spawnPosition);
    trigger->SetSpawnDirection(spawnDirection);
    trigger->SetCollisionBounds(bounds);
    DoorTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

ActTrigger* MyGameUnitMgr::AddActTrigger(int32_t unitId, const soft3d::ObjectBounds& bounds)
{
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    auto trigger = std::make_unique<ActTrigger>(cameraController,
        static_cast<Scene*>(GetContext().sceneContext), dynamic_cast<Player*>(player),
        engine != nullptr ? &engine->GetControlMap() : nullptr, interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetCollisionBounds(bounds);
    ActTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

HideTrigger* MyGameUnitMgr::AddHideTrigger(int32_t unitId, int32_t maskIndex,
        const soft3d::Vec3& hidePosition, const soft3d::Vec3& cameraDirection,
        const soft3d::ObjectBounds& bounds)
{
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    auto trigger = std::make_unique<HideTrigger>(dynamic_cast<Player*>(player),
        engine != nullptr ? &engine->GetControlMap() : nullptr, interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetMaskIndex(maskIndex - 1);
    trigger->SetHidePosition(hidePosition);
    trigger->SetCameraDirection(cameraDirection);
    trigger->SetCollisionBounds(bounds);
    HideTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

ItemTrigger* MyGameUnitMgr::AddItemTrigger(int32_t unitId, int32_t itemId,
                                           const soft3d::ObjectBounds& bounds)
{
    auto trigger = std::make_unique<ItemTrigger>(dynamic_cast<Player*>(player));
    trigger->SetUnitId(unitId);
    trigger->SetCollisionBounds(bounds);
    trigger->AddItem(itemId);
    ItemTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

TalkTrigger* MyGameUnitMgr::AddTalkTrigger(int32_t unitId, const soft3d::ObjectBounds& bounds,
                                           bool fadeAtEnd)
{
    auto* engine = static_cast<soft3d::GameEngine*>(GetContext().gameContext);
    auto* scene = static_cast<Scene*>(GetContext().sceneContext);
    auto trigger = std::make_unique<TalkTrigger>(scene, this, dynamic_cast<Player*>(player),
        engine != nullptr ? &engine->GetControlMap() : nullptr, interactionUi);
    trigger->SetUnitId(unitId);
    trigger->SetCollisionBounds(bounds);
    trigger->SetCameraPosition(cameraController != nullptr ? cameraController->GetPosition()
                                                           : soft3d::Vec3{});
    trigger->SetFadeAtEnd(fadeAtEnd);
    TalkTrigger* result = trigger.get();
    AddUnit(trigger.release(), 4);
    return result;
}

StayBox* MyGameUnitMgr::AddStayBox(int32_t unitId, const soft3d::ObjectBounds& bounds)
{
    StayBox* box = RestoreStayBox();
    if (box != nullptr) {
        box->SetUnitId(unitId);
        box->SetLocalBounds(bounds);
    }
    return box;
}

StayBox* MyGameUnitMgr::RestoreStayBox()
{
    if (stayBoxCount >= static_cast<int32_t>(stayBoxes.size())) {
        return nullptr;
    }
    StayBox& box = stayBoxes[static_cast<std::size_t>(stayBoxCount++)];
    box.Configure(player, stayBoxHost);
    return &box;
}

int MyGameUnitMgr::SpawnStayBoxEnemy(uint8_t unitId, uint8_t type, uint8_t active,
                                     const soft3d::Vec3& position, const soft3d::Vec3& direction)
{
    soft3d::Vec3 spawnPosition = position;
    return AddEnemy(unitId, type, active, spawnPosition, direction, false) ? 1 : 0;
}

soft3d::GameUnit* MyGameUnitMgr::FindStayBoxEnemy(uint8_t unitId)
{
    return FindEnemy(unitId);
}

const soft3d::Vec3& MyGameUnitMgr::GetStayBoxEnemySpawnPosition(
    const soft3d::GameUnit* enemy) const
{
    static constexpr soft3d::Vec3 zero{};
    const auto* concrete = dynamic_cast<const Enemy*>(enemy);
    return concrete != nullptr ? concrete->GetStayBoxReturnPosition() : zero;
}

int MyGameUnitMgr::GetStayBoxEnemyState(const soft3d::GameUnit* enemy) const
{
    const auto* concrete = dynamic_cast<const Enemy*>(enemy);
    return concrete != nullptr ? concrete->GetState() : EnemyRemoved;
}

uint8_t MyGameUnitMgr::GetStayBoxEnemyType(const soft3d::GameUnit* enemy) const
{
    const auto* concrete = dynamic_cast<const Enemy*>(enemy);
    return concrete != nullptr ? static_cast<uint8_t>(concrete->GetEnemyType()) : 0;
}

uint8_t MyGameUnitMgr::GetStayBoxEnemyActive(const soft3d::GameUnit* enemy) const
{
    const auto* concrete = dynamic_cast<const Enemy*>(enemy);
    return concrete != nullptr ? static_cast<uint8_t>(concrete->GetHealth()) : 0;
}

uint8_t MyGameUnitMgr::GetStayBoxEnemySpawnState(const soft3d::GameUnit* enemy) const
{
    const auto* concrete = dynamic_cast<const Enemy*>(enemy);
    return concrete != nullptr ? static_cast<uint8_t>(concrete->GetSpawnState()) : 0;
}

void MyGameUnitMgr::SetStayBoxEnemySpawnState(soft3d::GameUnit* enemy, uint8_t value)
{
    auto* concrete = dynamic_cast<Enemy*>(enemy);
    if (concrete != nullptr) {
        concrete->SetSpawnState(value);
    }
}

void MyGameUnitMgr::BindStayBoxEnemy(soft3d::GameUnit* enemy, StayBox* stayBox,
                                     const soft3d::Vec3& returnPosition)
{
    auto* concrete = dynamic_cast<Enemy*>(enemy);
    if (concrete != nullptr) {
        concrete->AttachStayBox(stayBox);
        concrete->SetStayBoxReturnPosition(returnPosition);
    }
}

void MyGameUnitMgr::AttachStayBoxEnemy(soft3d::GameUnit* enemy, StayBox* stayBox)
{
    auto* concrete = dynamic_cast<Enemy*>(enemy);
    if (concrete != nullptr) {
        concrete->AttachStayBox(stayBox);
    }
}

void MyGameUnitMgr::AttachStayBoxEnemy(soft3d::GameUnit& enemy, StayBox& box)
{
    if (stayBoxHost != nullptr) {
        stayBoxHost->AttachStayBoxEnemy(&enemy, &box);
    }
}

void MyGameUnitMgr::RemoveStayBoxEnemy(soft3d::GameUnit* enemy)
{
    RemoveEnemy(dynamic_cast<Enemy*>(enemy));
}

soft3d::GameUnit* MyGameUnitMgr::AddCollisionUnit(int32_t unitId,
                                                  const soft3d::ObjectBounds& bounds)
{
    auto unit = std::make_unique<soft3d::GameUnit>();
    unit->SetUnitId(unitId);
    unit->SetCollisionBounds(bounds);
    soft3d::GameUnit* result = unit.get();
    AddUnit(unit.release(), 1);
    return result;
}

StayBox* MyGameUnitMgr::FindStayBox(int32_t unitId)
{
    for (int32_t index = 0; index < stayBoxCount; ++index) {
        StayBox& box = stayBoxes[static_cast<std::size_t>(index)];
        if (box.GetUnitId() == unitId) {
            return &box;
        }
    }
    return nullptr;
}

void MyGameUnitMgr::SpawnMapItems()
{
    GameItemGroup* group = data.FindGameItemGroup(mapId);
    if (group == nullptr) {
        return;
    }
    int32_t nextUnitId = 9999;
    for (GameItemSpawnRecord& record : group->records) {
        if (record.itemId == 0) {
            continue;
        }
        std::array<int32_t, 4> packedName{};
        std::memcpy(packedName.data(), record.packedModelName.data(), record.packedModelName.size());
        const std::u16string modelName = soft3d::DecodeSojTextureName(packedName);
        auto* pickable = dynamic_cast<Pickable*>(AddThing(nextUnitId, 3, -1, false,
            modelName.c_str(), record.position, record.axisX, record.axisY, record.axisZ));
        if (pickable == nullptr) {
            throw std::runtime_error("Map item did not create a Pickable");
        }
        pickable->SetSpawnRecord(&record.itemId);
        pickable->SetItemId(record.itemId);
        AddPickTrigger(nextUnitId, nextUnitId, record.pickupTextId, record.pickupBounds);
        --nextUnitId;
    }
}

void MyGameUnitMgr::SpawnMapEnemies()
{
    EnemyInfoGroup* group = data.FindEnemyInfoGroup(mapId);
    if (group == nullptr) {
        return;
    }
    int32_t nextUnitId = 9999;
    for (EnemySpawnRecord& record : group->records) {
        if (record.active == 0) {
            continue;
        }
        if (record.unitId != -1) {
            nextUnitId = record.unitId;
        }
        soft3d::Vec3 position = record.position;
        if (AddEnemy(nextUnitId, record.enemyType, record.active, position,
                     record.direction, true)) {
            Enemy* enemy = FindEnemy(nextUnitId);
            if (enemy != nullptr) {
                for (const EnemyWaypoint& waypoint : record.waypoints) {
                    enemy->AddWaypoint(waypoint.position, waypoint.waitTime, waypoint.direction);
                }
                enemy->SetSpawnRecord(&record);
                if (record.direction.x != 0 || record.direction.y != 0 || record.direction.z != 0) {
                    enemy->SetDirection(record.direction);
                }
                --nextUnitId;
            }
        }
    }
}

void MyGameUnitMgr::UpdateEnemySpawnRecords()
{
    if (mapId == -1) {
        return;
    }
    for (soft3d::GameUnit* unit = GetFirstUnit(0); unit != nullptr; unit = unit->GetNextUnit()) {
        auto* enemy = dynamic_cast<Enemy*>(unit);
        if (enemy == nullptr || enemy->GetSpawnRecord() == nullptr) {
            continue;
        }
        EnemySpawnRecord& record = *enemy->GetSpawnRecord();
        record.active = static_cast<int16_t>(enemy->GetHealth());
        if (record.active < 0) {
            record.active = 0;
        }
        record.position = enemy->GetPosition();
        record.direction = enemy->GetObject(0) != nullptr ? enemy->GetObject(0)->GetAxisZ()
                                                          : soft3d::Vec3{};
        record.unitId = static_cast<int16_t>(enemy->GetUnitId());
    }
}

int32_t MyGameUnitMgr::GetDoorState(int32_t doorId) const
{
    return doorId >= 10000 && doorId < 11024 ? doorStates[static_cast<std::size_t>(doorId - 10000)] : 0;
}

void MyGameUnitMgr::SetDoorState(int32_t doorId, int32_t value)
{
    if (doorId >= 10000 && doorId < 11024) {
        doorStates[static_cast<std::size_t>(doorId - 10000)] = value;
    }
}

void MyGameUnitMgr::Update()
{
    for (soft3d::GameUnit* unit = GetFirstUnit(4); unit != nullptr; unit = unit->GetNextUnit()) {
        if (unit->GetUnitType() == 0x28) {
            unit->Update();
        }
    }
    if (!paused) {
        for (int32_t index = 0; index < stayBoxCount; ++index) {
            stayBoxes[static_cast<std::size_t>(index)].Update();
        }
        UpdateUnits();
    } else {
        for (soft3d::GameUnit* unit = GetFirstUnit(1); unit != nullptr; unit = unit->GetNextUnit()) {
            unit->Update();
        }
        for (soft3d::GameUnit* unit = GetFirstUnit(4); unit != nullptr; unit = unit->GetNextUnit()) {
            if (unit->GetUnitType() == 0x27) {
                unit->Update();
            }
        }
    }
}

void MyGameUnitMgr::SubmitRenderables(soft3d::RenderQueues& queues)
{
    SubmitUnits(queues);
    for (std::size_t index = 0; index < lensflareCount; ++index) {
        queues.Submit(lensflares[index].get(), 6);
    }
}

soft3d::Lensflare* MyGameUnitMgr::GetLensflare(std::size_t index) const
{
    return index < lensflareCount ? lensflares[index].get() : nullptr;
}

uint32_t MyGameUnitMgr::NextRandom()
{
    randomState = randomState * 0x343fdU + 0x269ec3U;
    return (randomState >> 16) & 0x7fffU;
}

GameData& MyGameUnitMgr::GetData()
{
    return data;
}

const GameData& MyGameUnitMgr::GetData() const
{
    return data;
}

} // namespace game

#include "game/MapUnits.hpp"

#include "soft3d/AudioData.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>

namespace game {
namespace {

soft3d::AudioData* LoadAudio(soft3d::s3dResManager* resources, const char16_t* name)
{
    if (resources == nullptr || name == nullptr) {
        return nullptr;
    }
    return dynamic_cast<soft3d::AudioData*>(resources->GetResource(
        soft3d::ResourceAudio, 3, name, u".\\audio\\"));
}

void WriteAudioName(soft3d::OutputStream& stream, const soft3d::AudioData* audio)
{
    stream.WriteWideString(audio != nullptr ? audio->GetFileName() : u"");
}

void ReadAudioName(soft3d::InputStream& stream, soft3d::s3dResManager* resources,
                   soft3d::AudioData*& audio)
{
    char16_t name[256]{};
    stream.ReadWideString(name, 256);
    audio = LoadAudio(resources, name);
}

} // namespace

Breakable::Breakable(soft3d::s3dResManager* resources, int cacheLevel,
                     soft3d::Renderer* renderer, soft3d::AudioManager* audio, soft3d::Bsp* bsp)
    : Things(resources, cacheLevel, renderer, audio, bsp)
{
    unitType = 0x1c;
}

void Breakable::Configure(Player* newPlayer, int32_t weapon)
{
    player = newPlayer;
    requiredWeapon = weapon;
}

void Breakable::SetBreakSound(const char16_t* name)
{
    breakSound = LoadAudio(resourceManager, name);
}

bool Breakable::ConsumeBreakEvent()
{
    const bool result = breakEvent != 0;
    if (result) {
        breakEvent = 0;
    }
    return result;
}

void Breakable::Update()
{
    if (player != nullptr) {
        Sword* sword = player->GetSword();
        if (player->GetAttackPhase() == 0) {
            hitLatched = 0;
            breakEvent = 0;
        } else if (hitLatched == 0 && sword != nullptr && QueryBounds(sword->GetHitBounds()) != nullptr) {
            hitLatched = 1;
            breakEvent = 2;
        }
        if (requiredWeapon >= 0 && breakEvent == 2 && player->GetWeapon() >= requiredWeapon) {
            if (breakSound != nullptr) {
                PlaySound(breakSound, false);
            }
            if (GetAlternateObject() != nullptr) {
                GetAlternateObject()->SetAnimationSpeed(300);
            }
            breakEvent = 1;
            StartDestruction(32);
        }
    }
    Things::Update();
}

void Breakable::SaveState(soft3d::OutputStream& stream) const
{
    Things::SaveState(stream);
    stream.WriteInt32(requiredWeapon);
    WriteAudioName(stream, breakSound);
}

void Breakable::LoadState(soft3d::InputStream& stream)
{
    Things::LoadState(stream);
    stream.ReadInt32(requiredWeapon);
    ReadAudioName(stream, resourceManager, breakSound);
}

Pushable::Pushable(soft3d::s3dResManager* resources, int cacheLevel,
                   soft3d::Renderer* renderer, soft3d::AudioManager* audio, soft3d::Bsp* bsp)
    : Things(resources, cacheLevel, renderer, audio, bsp)
{
    unitType = 0x1d;
    physicsFlags = soft3d::PhysicKeepDirection;
}

void Pushable::SetPushSound(const char16_t* name)
{
    pushSound = LoadAudio(resourceManager, name);
}

void Pushable::Update()
{
    if (pushEnabled) {
        if (pusher != nullptr) {
            velocity = soft3d::Subtract(pusher->GetPosition(), pusher->GetPreviousPosition());
        }
        if (soft3d::SquaredLength(velocity) < 0x28f) {
            velocity = {};
        }
    }
    Things::Update();
    SetCurrentLeaf(-1);
}

void Pushable::SaveState(soft3d::OutputStream& stream) const
{
    Things::SaveState(stream);
    WriteAudioName(stream, pushSound);
}

void Pushable::LoadState(soft3d::InputStream& stream)
{
    Things::LoadState(stream);
    ReadAudioName(stream, resourceManager, pushSound);
}

Pickable::Pickable(soft3d::s3dResManager* resources, int cacheLevel,
                   soft3d::Renderer* renderer, soft3d::AudioManager* audio, soft3d::Bsp* bsp,
                   soft3d::GameUnitManager* manager)
    : Things(resources, cacheLevel, renderer, audio, bsp)
{
    unitType = 0x1e;
    physicsFlags = 0x1e0;
    SetUnitManager(manager);
}

void Pickable::SetPickupSound(const char16_t* name)
{
    pickupSound = LoadAudio(resourceManager, name);
}

void Pickable::Update()
{
    Things::Update();
    if (primaryObject != nullptr) {
        primaryObject->SetObjectIntensity(0);
    }
}

void Pickable::PickUp(Player* player)
{
    if (player != nullptr) {
        player->AddItem(itemId, true);
    }
    if (spawnItemId != nullptr) {
        *spawnItemId = 0;
    }
}

void Pickable::SaveState(soft3d::OutputStream& stream) const
{
    Things::SaveState(stream);
    stream.WriteInt32(itemId);
    WriteAudioName(stream, pickupSound);
}

void Pickable::LoadState(soft3d::InputStream& stream)
{
    Things::LoadState(stream);
    stream.ReadInt32(itemId);
    ReadAudioName(stream, resourceManager, pickupSound);
}

BspUnit::BspUnit()
{
    unitType = 3;
}

void BspUnit::LoadEntity(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp)
{
    unitId = record.type;
    if (record.brushIndex >= 0 &&
        static_cast<std::size_t>(record.brushIndex) < bsp.GetEntityBrushes().size()) {
        const auto& values = bsp.GetEntityBrushes()[static_cast<std::size_t>(record.brushIndex)].values;
        localBounds = {{soft3d::ShiftLeftFixed(values[0], 16),
                        soft3d::ShiftLeftFixed(values[1], 16),
                        soft3d::ShiftLeftFixed(values[2], 16)},
                       {soft3d::ShiftLeftFixed(values[3], 16),
                        soft3d::ShiftLeftFixed(values[4], 16),
                        soft3d::ShiftLeftFixed(values[5], 16)}};
    }
    if (record.modelIndex >= 0 &&
        static_cast<std::size_t>(record.modelIndex) < bsp.GetModels().size()) {
        const soft3d::BspModel& model = bsp.GetModels()[static_cast<std::size_t>(record.modelIndex)];
        SetObject(model.object.get());
        mapAnimation = model.animation;
        secondaryVertices = model.vertexLights.dataByteSize != 0 ? &model.vertexLights : nullptr;
    }
}

void BspUnit::Submit(soft3d::RenderQueues& queues)
{
    soft3d::Object* object = GetObject();
    if (object == nullptr) {
        return;
    }
    if (object->IsExternalTransformEnabled()) {
        object->UpdateTransform();
    }
    if (!object->IsExternalTransformEnabled()) {
        object->UpdateFrustumVisibility();
    }
    if (object->IsFrustumCulled()) {
        return;
    }
    if (mapAnimation != nullptr) {
        object->AdvanceAnimation();
    }
    if (secondaryVertices != nullptr) {
        object->SetSecondaryVertices(secondaryVertices);
    }
    const soft3d::Fixed depth = object->GetCameraDepth();
    object->sortKey = object->HasTransparency() ? depth : -depth;
    queues.Submit(object, object->HasTransparency() ? 4 : 3);
}

ElectricSaw::ElectricSaw(soft3d::s3dResManager* resources, int cacheLevel,
                         soft3d::Renderer* renderer, soft3d::AudioManager* audio)
    : VisUnit(resources, cacheLevel, renderer, audio)
{
}

void ElectricSaw::Update()
{
    VisUnit::Update();
    collisionBounds = {soft3d::Add(localBounds.minimum, position),
                       soft3d::Add(localBounds.maximum, position)};
    collisionBounds.minimum.x = soft3d::SubtractFixed(collisionBounds.minimum.x, 0x20000);
    collisionBounds.maximum.x = soft3d::AddFixed(collisionBounds.maximum.x, 0x20000);
    collisionBounds.minimum.z = soft3d::SubtractFixed(collisionBounds.minimum.z, 0x20000);
    collisionBounds.maximum.z = soft3d::AddFixed(collisionBounds.maximum.z, 0x20000);
    collisionBounds.minimum.y = soft3d::SubtractFixed(collisionBounds.minimum.y, 0xa0000);
    collisionBounds.maximum.y = soft3d::AddFixed(collisionBounds.maximum.y, 0xa0000);
}

Trap::Trap(Player* newPlayer) : player(newPlayer)
{
    unitType = 0x1f;
}

void Trap::Update()
{
    if (player == nullptr) {
        return;
    }
    if (QueryBounds(player->GetCollisionBounds()) != nullptr) {
        player->SetExternalVelocity(impulse);
    } else if (player->GetExternalVelocity().x == impulse.x &&
               player->GetExternalVelocity().y == impulse.y &&
               player->GetExternalVelocity().z == impulse.z) {
        player->SetExternalVelocity({});
    }
}

void Trap::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    soft3d::WriteVec3(stream, impulse);
}

void Trap::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    soft3d::ReadVec3(stream, impulse);
}

StayBox::StayBox()
    : enemyOutside(0)
    , spawned(1)
{
    unitType = 0x26;
}

void StayBox::Configure(soft3d::GameUnit* newEnemy, StayBoxHost* newHost)
{
    enemy = newEnemy;
    host = newHost;
}

void StayBox::AddAllowedRegion(const soft3d::ObjectBounds& bounds)
{
    allowedRegions.insert(allowedRegions.begin(), bounds);
}

void StayBox::AddDeniedRegion(const soft3d::ObjectBounds& bounds)
{
    deniedRegions.insert(deniedRegions.begin(), bounds);
}

void StayBox::AddSpawnRecord(const soft3d::GameUnit& unit)
{
    SpawnRecord record;
    record.unitId = static_cast<uint8_t>(unit.GetUnitId());
    spawnRecords.insert(spawnRecords.begin(), record);
}

void StayBox::AddSpawnRecord(const SpawnRecord& record)
{
    spawnRecords.insert(spawnRecords.begin(), record);
}

bool StayBox::OverlapsAny(const std::vector<soft3d::ObjectBounds>& regions) const
{
    if (enemy == nullptr) {
        return false;
    }
    for (const soft3d::ObjectBounds& bounds : regions) {
        if (enemy->QueryBounds(bounds) != nullptr) {
            return true;
        }
    }
    return false;
}

void StayBox::SpawnEnemies()
{
    if (host != nullptr) {
        for (const SpawnRecord& record : spawnRecords) {
            if (record.active == 0) {
                continue;
            }
            host->SpawnStayBoxEnemy(record.unitId, record.type, record.active,
                                    record.position, {});
            soft3d::GameUnit* unit = host->FindStayBoxEnemy(record.unitId);
            if (unit != nullptr) {
                host->SetStayBoxEnemySpawnState(unit, record.spawnState);
                host->BindStayBoxEnemy(unit, const_cast<StayBox*>(this), record.position);
            }
        }
    }
    spawned = 1;
}

void StayBox::DespawnEnemies()
{
    if (host != nullptr) {
        for (SpawnRecord& record : spawnRecords) {
            soft3d::GameUnit* unit = host->FindStayBoxEnemy(record.unitId);
            if (unit == nullptr || host->GetStayBoxEnemyState(unit) > 14) {
                record.active = 0;
            } else {
                record.position = host->GetStayBoxEnemySpawnPosition(unit);
                record.type = host->GetStayBoxEnemyType(unit);
                record.active = host->GetStayBoxEnemyActive(unit);
                record.spawnState = host->GetStayBoxEnemySpawnState(unit);
                host->RemoveStayBoxEnemy(unit);
            }
        }
    }
    spawned = 0;
}

void StayBox::Update()
{
    enemyOutside = enemy != nullptr && enemy->QueryBounds(localBounds) == nullptr ? 1 : 0;
    if (OverlapsAny(allowedRegions) && spawned == 0) {
        SpawnEnemies();
    }
    if (OverlapsAny(deniedRegions) && spawned != 0) {
        DespawnEnemies();
    }
}

void StayBox::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(enemyOutside);
    stream.WriteInt32(spawned);
}

void StayBox::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(enemyOutside);
    stream.ReadInt32(spawned);
}

} // namespace game

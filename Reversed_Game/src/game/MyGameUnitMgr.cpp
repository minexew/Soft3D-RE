#include "game/MyGameUnitMgr.hpp"

#include "game/CameraController.hpp"
#include "game/Scene.hpp"
#include "game/Triggers.hpp"
#include "soft3d/GameEngine.hpp"

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
{
    auto* engine = static_cast<soft3d::GameEngine*>(context.gameContext);
    for (auto& lensflare : lensflares) {
        lensflare = std::make_unique<soft3d::Lensflare>(context.renderer,
            engine != nullptr ? &engine->GetScreenBlitter() : nullptr, context.resources);
    }
}

soft3d::result MyGameUnitMgr::Initialize(soft3d::GameEngine& engine)
{
    engine.GetAudioManager().SetSoundNameTable(data.GetSoundNameTable());
    return data.LoadItemData(*engine.GetResourceManager().GetFilePak(), engine.GetContent());
}

soft3d::result MyGameUnitMgr::LoadDayContent(int value, soft3d::GameEngine& engine)
{
    const soft3d::result status = data.LoadDayContent(value, *engine.GetResourceManager().GetFilePak());
    if (status == soft3d::S3D_OK) {
        day = value;
    }
    return status;
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
    healthPackCount = 0;
    pushTarget = nullptr;
}

void MyGameUnitMgr::FinishMapLoad(bool restoreSavedState)
{
    (void)restoreSavedState;
    stayBoxCount = 0;
    mapReady = true;
    paused = false;
}

void MyGameUnitMgr::ConfigureScene(CameraController* newCameraController,
                                   soft3d::Creature* newPlayer)
{
    cameraController = newCameraController;
    player = newPlayer;
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

void MyGameUnitMgr::Update()
{
    for (int32_t index = 0; index < stayBoxCount; ++index) {
        stayBoxes[static_cast<std::size_t>(index)].Update();
    }
    if (!paused) {
        UpdateUnits();
    } else {
        for (soft3d::GameUnit* unit = GetFirstUnit(1); unit != nullptr; unit = unit->GetNextUnit()) {
            unit->Update();
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

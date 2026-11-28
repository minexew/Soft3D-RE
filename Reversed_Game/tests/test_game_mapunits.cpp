#include "game/GameSI.hpp"
#include "game/MapUnits.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

#define CHECK(condition) Check((condition), __LINE__)

void Check(bool condition, int line)
{
    if (!condition) throw std::runtime_error("check failed at line " + std::to_string(line));
}

class TestClock final : public soft3d::EngineClock {
public:
    uint32_t NowMicroseconds() override { return 0; }
    void SleepMicroseconds(uint32_t) override {}
};

struct Argument {
    soft3d::ScriptValueType type = soft3d::ScriptValueType::Int;
    int32_t value = 0;
    std::string text;
    static Argument Int(int32_t value) { return {soft3d::ScriptValueType::Int, value, {}}; }
    static Argument Short(int16_t value) { return {soft3d::ScriptValueType::Short, value, {}}; }
    static Argument Fixed(int32_t value) { return {soft3d::ScriptValueType::Fixed, value, {}}; }
    static Argument Bool(bool value) { return {soft3d::ScriptValueType::Bool, value, {}}; }
    static Argument String(const char* value) { return {soft3d::ScriptValueType::String, 0, value}; }
};

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    Append16(bytes, static_cast<uint16_t>(value));
    Append16(bytes, static_cast<uint16_t>(value >> 16));
}

std::vector<uint8_t> MakeHostScript(uint16_t id, const std::vector<Argument>& arguments, bool output)
{
    std::vector<uint8_t> bytes;
    Append16(bytes, output ? 1 : 0);
    if (output) Append16(bytes, static_cast<uint16_t>(soft3d::ScriptValueType::Int));
    Append16(bytes, 0);
    Append16(bytes, 1);
    Append16(bytes, 0);
    Append16(bytes, 0);
    Append16(bytes, 1);
    Append16(bytes, id);
    Append16(bytes, 1);
    Append16(bytes, static_cast<uint16_t>(arguments.size()));
    for (const Argument& argument : arguments) {
        Append16(bytes, static_cast<uint16_t>(argument.type));
        if (argument.type == soft3d::ScriptValueType::String) {
            Append16(bytes, static_cast<uint16_t>((argument.text.size() + 2) & ~std::size_t(1)));
            for (char value : argument.text) bytes.push_back(static_cast<uint8_t>(value));
            bytes.push_back(0);
            while ((bytes.size() & 1) != 0) bytes.push_back(0);
        } else {
            Append16(bytes, 4);
            Append32(bytes, static_cast<uint32_t>(argument.value));
        }
    }
    Append16(bytes, output ? 6 : 0);
    if (output) {
        Append16(bytes, 4);
        Append16(bytes, 0);
        Append16(bytes, 1);
    }
    Append16(bytes, 0);
    Append16(bytes, 0);
    return bytes;
}

struct GameFixture {
    GameFixture() : engine(MakeConfig())
    {
        engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
        CHECK(engine.Init() == soft3d::S3D_OK);
        auto sceneOwner = std::make_unique<game::Scene>(engine);
        scene = sceneOwner.get();
        CHECK(sceneOwner->Initialize() == soft3d::S3D_OK);
        engine.SetSceneHost(std::move(sceneOwner));
        gameInterface = std::make_unique<game::GameSI>();
        gameInterface->Bind({scene, scene->GetUnitManager(), scene->GetCameraController(),
                             scene->GetPlayer(), scene->GetGameUi(), scene->GetAudio(),
                             scene->GetResources(), scene->GetBsp(), scene->GetControlMap()});
        engine.SetScriptInterface(gameInterface.get());
        soft3d::GameplayHooks hooks;
        hooks.startNewGame = [this](const soft3d::NewGameOptions& options) {
            scene->StartNewGame(options);
        };
        hooks.loadGame = [this](int slot) { return scene->LoadSaveSlot(slot); };
        hooks.returnToMenu = [this] { scene->ReturnToMainMenu(); };
        engine.SetGameplayHooks(std::move(hooks));
        engine.Start();
    }
    ~GameFixture() { engine.SetScriptInterface(nullptr); }

    static soft3d::EngineConfig MakeConfig()
    {
        soft3d::EngineConfig config;
        config.width = 320;
        config.height = 240;
        config.clock = std::make_unique<TestClock>();
        config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
        return config;
    }

    int32_t Execute(uint16_t id, const std::vector<Argument>& arguments = {}, bool output = false)
    {
        const auto bytes = MakeHostScript(id, arguments, output);
        soft3d::SimpleScript script;
        CHECK(script.Load(bytes.data(), bytes.size()) == soft3d::S3D_OK);
        script.SetScriptInterface(gameInterface.get());
        script.Start();
        script.SetState(soft3d::ScriptState::Running);
        script.EnableTask(0);
        script.Update();
        return output ? script.GetGlobalVariable(0)->GetInt() : 0;
    }

    void RunFrames(int count)
    {
        for (int index = 0; index < count; ++index) CHECK(engine.RunFrame());
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
};

std::vector<Argument> AddArguments(int id, int type, int variant, const char* model = "ow_dianju_a")
{
    return {Argument::Short(static_cast<int16_t>(id)), Argument::Int(type), Argument::Int(variant),
        Argument::String(model), Argument::Fixed(0x10000), Argument::Fixed(0x20000),
        Argument::Fixed(0x30000), Argument::Fixed(0x10000), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0x10000), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0x10000), Argument::Bool(true)};
}

void TestAddVariantsAndBasicCommands()
{
    GameFixture fixture;
    fixture.Execute(221, AddArguments(1, 0, -1));
    fixture.Execute(221, AddArguments(2, 1, 0));
    fixture.Execute(221, AddArguments(3, 2, -1));
    fixture.Execute(221, AddArguments(4, 3, -1, "oi_yaoping_a"));
    auto* manager = fixture.scene->GetUnitManager();
    CHECK(manager->GetBucketCount(2) == 4);
    CHECK(manager->FindUnit(2, 1)->GetUnitType() == 4);
    CHECK(dynamic_cast<game::Breakable*>(manager->FindUnit(2, 2)) != nullptr);
    CHECK(dynamic_cast<game::Pushable*>(manager->FindUnit(2, 3)) != nullptr);
    auto* pickable = dynamic_cast<game::Pickable*>(manager->FindUnit(2, 4));
    CHECK(pickable != nullptr && !pickable->IsCollisionEnabled());
    CHECK(pickable->GetObject(0)->GetMaterialIntensityState(0)[1] == 0xe666);

    soft3d::Things& thing = dynamic_cast<soft3d::Things&>(*manager->FindUnit(2, 1));
    CHECK(thing.GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::Normal);
    const soft3d::Vec3 initialPosition = thing.GetPosition();
    thing.Update();
    CHECK(thing.GetPosition().x == initialPosition.x && thing.GetPosition().y == initialPosition.y &&
          thing.GetPosition().z == initialPosition.z);
    CHECK(thing.GetCollisionBounds().minimum.x == thing.GetObject(0)->GetWorldBounds().minimum.x &&
          thing.GetCollisionBounds().maximum.z == thing.GetObject(0)->GetWorldBounds().maximum.z);
    std::vector<Argument> secondary = AddArguments(14, 0, -1);
    secondary.back() = Argument::Bool(false);
    fixture.Execute(221, secondary);
    CHECK(manager->FindUnit(2, 14)->GetUnitType() == 4);
    auto& secondaryThing = dynamic_cast<soft3d::Things&>(*manager->FindUnit(2, 14));
    CHECK(secondaryThing.GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::SecondaryStreams);
    secondaryThing.SetCurrentLeaf(0);
    soft3d::RenderQueues secondaryQueues;
    secondaryThing.SubmitRenderables(secondaryQueues);
    CHECK(secondaryQueues.GetCount(3) + secondaryQueues.GetCount(4) == 1);
    fixture.Execute(246, {Argument::Short(14)});
    fixture.Execute(222, {Argument::Short(1), Argument::Fixed(0x10000), Argument::Fixed(0),
                          Argument::Fixed(0x10000)});
    CHECK(thing.GetObject(0)->GetAxisZ().y == 0);
    fixture.Execute(223, {Argument::Short(4), Argument::Short(1001)});
    CHECK(pickable->GetItemId() == 1001);
    fixture.Execute(224, {Argument::Short(1), Argument::Bool(false)});
    CHECK(!thing.IsCollisionEnabled());
    fixture.Execute(225, {Argument::Short(1), Argument::Fixed(0x1000), Argument::Fixed(0x8000),
                          Argument::Short(10)});
    CHECK(thing.GetObject(0)->GetMaterialIntensityState(0)[1] == 0x8000);
    fixture.Execute(226, {Argument::Short(1), Argument::Bool(false)});
    CHECK(fixture.Execute(227, {Argument::Short(1)}, true) == 0);
    fixture.Execute(226, {Argument::Short(1), Argument::Bool(true)});
    CHECK(fixture.Execute(227, {Argument::Short(1)}, true) == 1);

    fixture.Execute(228, {Argument::Short(1), Argument::String("ow_dianju_walk_a"),
                          Argument::Int(300), Argument::Int(1), Argument::Int(0)});
    CHECK(thing.GetAnimation() != nullptr);
    CHECK(thing.GetObject(0)->GetAnimationSpeed() == 300);
    CHECK(!thing.GetObject(0)->IsAnimationLooping());
    fixture.Execute(229, {Argument::Short(1), Argument::Bool(true)});
    CHECK(fixture.Execute(230, {Argument::Short(1)}, true) == 1);
    fixture.Execute(231, {Argument::Short(1), Argument::Int(5)});
    CHECK(fixture.Execute(232, {Argument::Short(1)}, true) == 5);
    fixture.Execute(229, {Argument::Short(1), Argument::Bool(false)});
    fixture.Execute(231, {Argument::Short(1), Argument::Int(-1)});
    thing.PostUpdate();
    CHECK(fixture.Execute(233, {Argument::Short(1)}, true) == 1);

    fixture.Execute(234, {Argument::Short(2), Argument::String("ow_dianju_a"),
                          Argument::String("ow_dianju_walk_a")});
    auto& breakable = dynamic_cast<game::Breakable&>(*manager->FindUnit(2, 2));
    CHECK(breakable.GetAlternateObject() != nullptr);
    CHECK(breakable.GetAlternateAnimation() != nullptr);
    fixture.Execute(235, {Argument::Short(2), Argument::String("ow_dianju_daiji_a")});
    fixture.Execute(236, {Argument::Short(2), Argument::String("t_swordbreak")});
    fixture.Execute(237, {Argument::Short(3), Argument::String("t_saw")});

    fixture.Execute(238, {Argument::Short(1), Argument::Short(90)});
    fixture.Execute(239, {Argument::Short(1), Argument::Short(90)});
    fixture.Execute(240, {Argument::Short(1), Argument::Short(90)});
    CHECK(thing.GetObject(0)->GetAxisX().x != 0x10000 ||
          thing.GetObject(0)->GetAxisY().y != 0x10000);
    CHECK(fixture.Execute(254, {Argument::Short(1)}, true) == 0);

    const soft3d::ObjectBounds originalBounds = thing.GetCollisionBounds();
    fixture.Execute(241, {Argument::Short(1), Argument::Fixed(0x50000), Argument::Fixed(0x60000),
                          Argument::Fixed(0x70000)});
    CHECK(thing.GetPosition().x == 0x50000);
    CHECK(thing.GetCollisionBounds().minimum.x == originalBounds.minimum.x);
    fixture.Execute(242, {Argument::Short(1), Argument::Fixed(0x10000), Argument::Fixed(0x20000),
                          Argument::Fixed(0x30000)});
    CHECK(thing.GetPosition().x == 0x60000);
    CHECK(thing.GetCollisionBounds().minimum.x ==
          soft3d::AddFixed(thing.GetLocalBounds().minimum.x, 0x60000));
    CHECK(fixture.Execute(243, {Argument::Short(1), Argument::Fixed(0x60000),
        Argument::Fixed(0x80000), Argument::Fixed(0xa0000), Argument::Fixed(0x10000)}, true) == 1);
    CHECK(fixture.Execute(243, {Argument::Short(1), Argument::Fixed(0x100000),
        Argument::Fixed(0x80000), Argument::Fixed(0xa0000), Argument::Fixed(0x10000)}, true) == 0);

    fixture.Execute(245, {Argument::Short(1), Argument::Fixed(0x12345)});
    CHECK(thing.GetObject(0)->GetDepthBias() == 0x12345);
    fixture.Execute(224, {Argument::Short(1), Argument::Bool(true)});
    CHECK(fixture.Execute(249, {Argument::Short(1), Argument::Fixed(-0x100000),
        Argument::Fixed(-0x100000), Argument::Fixed(-0x100000), Argument::Fixed(0x100000),
        Argument::Fixed(0x100000), Argument::Fixed(0x100000)}, true) == 1);
    CHECK(fixture.Execute(250, {Argument::Short(1), Argument::Fixed(-0x100000),
        Argument::Fixed(-0x100000), Argument::Fixed(-0x100000), Argument::Fixed(0x100000),
        Argument::Fixed(0x100000), Argument::Fixed(0x100000)}, true) == 0);
    fixture.Execute(251, {Argument::Short(1), Argument::Bool(false)});
    CHECK(!thing.GetObject(0)->IsLightingEnabled());
    fixture.Execute(252, {Argument::Short(3), Argument::Bool(false)});
    CHECK(!dynamic_cast<game::Pushable&>(*manager->FindUnit(2, 3)).CanBePushed());
    fixture.Execute(253, {Argument::Short(1), Argument::Short(0), Argument::String("zj_dianju_a")});

    CHECK(fixture.Execute(247, {Argument::Short(1)}, true) == 1);
    fixture.Execute(246, {Argument::Short(1)});
    CHECK(fixture.Execute(247, {Argument::Short(1)}, true) == 0);
    fixture.Execute(244, {Argument::Short(2), Argument::Short(3)});
    CHECK(breakable.IsDestroying());
    manager->Update();
    CHECK(breakable.GetDestructionTimer() == 1);
    manager->Update();
    manager->Update();
    CHECK(fixture.Execute(247, {Argument::Short(2)}, true) == 0);

    int32_t spawnItem = 1;
    pickable->SetSpawnRecord(&spawnItem);
    pickable->PickUp(fixture.scene->GetPlayer());
    CHECK(spawnItem == 0);
    CHECK(fixture.scene->GetPlayer()->HasItem(1001));

    const std::vector<Argument> trap = {Argument::Short(20), Argument::Fixed(-0x10000),
        Argument::Fixed(-0x10000), Argument::Fixed(-0x10000), Argument::Fixed(0x10000),
        Argument::Fixed(0x10000), Argument::Fixed(0x10000), Argument::Fixed(0x1000),
        Argument::Fixed(0x2000), Argument::Fixed(0x3000)};
    fixture.Execute(219, trap);
    CHECK(manager->GetBucketCount(5) == 1);
    fixture.scene->GetPlayer()->SetCollisionBounds({{-1, -1, -1}, {1, 1, 1}});
    manager->Update();
    CHECK(fixture.scene->GetPlayer()->GetExternalVelocity().z == 0x3000);
    fixture.scene->GetPlayer()->SetCollisionBounds({{0x20000, 0, 0}, {0x30000, 1, 1}});
    manager->Update();
    CHECK(fixture.scene->GetPlayer()->GetExternalVelocity().x == 0);
    fixture.Execute(220, {Argument::Short(20)});
    CHECK(manager->GetBucketCount(5) == 0);
}

void TestBreakPushAndOrientation()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    fixture.Execute(221, AddArguments(10, 1, -1));
    auto& breakable = dynamic_cast<game::Breakable&>(*manager->FindUnit(2, 10));
    fixture.scene->GetPlayer()->PrepareWeapon(manager->GetData(), 1);
    fixture.scene->GetPlayer()->SetWeapon(1);
    fixture.scene->GetPlayer()->EnterState(game::PlayerAttackA2);
    fixture.scene->GetPlayer()->GetSword()->Update();
    breakable.SetCollisionBounds(fixture.scene->GetPlayer()->GetSword()->GetHitBounds());
    breakable.Update();
    CHECK(fixture.Execute(248, {Argument::Short(10)}, true) == 1);
    CHECK(fixture.Execute(248, {Argument::Short(10)}, true) == 0);
    CHECK(!breakable.IsDestroying());
    fixture.scene->GetPlayer()->ResetForMap();
    breakable.Update();

    fixture.Execute(221, AddArguments(11, 1, 0));
    auto& destructible = dynamic_cast<game::Breakable&>(*manager->FindUnit(2, 11));
    fixture.scene->GetPlayer()->EnterState(game::PlayerAttackA2);
    fixture.scene->GetPlayer()->GetSword()->PostUpdate();
    fixture.scene->GetPlayer()->GetSword()->Update();
    destructible.SetCollisionBounds(fixture.scene->GetPlayer()->GetSword()->GetHitBounds());
    destructible.Update();
    CHECK(destructible.IsDestroying() && destructible.GetDestructionDuration() == 32);

    fixture.Execute(221, AddArguments(12, 2, -1));
    auto& pushable = dynamic_cast<game::Pushable&>(*manager->FindUnit(2, 12));
    game::Player* player = fixture.scene->GetPlayer();
    player->SetPosition({1, 2, 3});
    player->Update();
    player->SetPosition({0x10001, 0x20002, 0x30003});
    pushable.SetPusher(player);
    pushable.Update();
    CHECK(pushable.GetVelocity().x == 0x10000 && pushable.GetVelocity().y == 0x20000 &&
          pushable.GetVelocity().z == 0x30000);
    CHECK(pushable.GetCurrentLeaf() == -1);

    fixture.Execute(221, AddArguments(13, 0, -1));
    auto* cube = dynamic_cast<soft3d::Things*>(manager->FindUnit(2, 13));
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 2);
    cube->SetAxisX({0, 0x10000, 0});
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 0);
    cube->SetAxisX({0, -0x10000, 0});
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 1);
    cube->SetAxisX({0x10000, 0, 0});
    cube->SetAxisY({0, -0x10000, 0});
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 3);
    cube->SetAxisY({0x10000, 0, 0});
    cube->SetAxisZ({0, 0x10000, 0});
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 4);
    cube->SetAxisZ({0, -0x10000, 0});
    CHECK(fixture.Execute(254, {Argument::Short(13)}, true) == 5);
}

void TestPersistenceAndElectricSaw()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    fixture.Execute(221, AddArguments(30, 2, -1));
    auto& original = dynamic_cast<game::Pushable&>(*manager->FindUnit(2, 30));
    original.LoadAlternateModel(u"ow_dianju_a");
    original.SetAlternateAnimation(u"ow_dianju_walk_a");
    original.SetPushSound(u"t_saw");
    original.StartDestruction(20);
    original.Update();
    original.SetSerializedState67c(7);
    soft3d::OutputDataStream output;
    original.SaveState(output);
    game::Pushable restored(fixture.scene->GetResources(), 3, &fixture.engine.GetRenderer(),
                            fixture.scene->GetAudio(), fixture.scene->GetBsp());
    soft3d::InputDataStream input;
    input.SetData(output.GetData(), output.GetSize());
    int32_t savedType = 0;
    input.ReadInt32(savedType);
    restored.LoadState(input);
    CHECK(restored.IsDestroying() && restored.GetDestructionDuration() == 20);
    CHECK(restored.GetDestructionTimer() == 1 && restored.GetSerializedState67c() == 7);
    CHECK(restored.GetCurrentLeaf() == -1);
    CHECK(restored.GetAlternateObject() != nullptr && restored.GetAlternateAnimation() != nullptr);
    soft3d::OutputDataStream second;
    restored.SaveState(second);
    CHECK(second.GetSize() == output.GetSize());
    CHECK(std::memcmp(second.GetData(), output.GetData(), output.GetSize()) == 0);

    game::Trap trap;
    trap.SetUnitId(9);
    trap.SetImpulse({1, 2, 3});
    output.Clear();
    trap.SaveState(output);
    game::Trap restoredTrap;
    input.SetData(output.GetData(), output.GetSize());
    input.ReadInt32(savedType);
    restoredTrap.LoadState(input);
    CHECK(restoredTrap.GetUnitId() == 9 && restoredTrap.GetImpulse().z == 3);

    game::ElectricSaw saw;
    saw.SetPosition({0x10000, 0x20000, 0x30000});
    saw.SetLocalBounds({{-1, -2, -3}, {4, 5, 6}});
    saw.Update();
    CHECK(saw.GetCollisionBounds().minimum.x == 0x10000 - 1 - 0x20000);
    CHECK(saw.GetCollisionBounds().maximum.y == 0x20000 + 5 + 0xa0000);
    CHECK(saw.GetCollisionBounds().maximum.z == 0x30000 + 6 + 0x20000);
}

class TestStayBoxHost final : public game::StayBoxHost {
public:
    int SpawnStayBoxEnemy(uint8_t id, uint8_t type, uint8_t active, const soft3d::Vec3& position,
                          const soft3d::Vec3&) override
    {
        ++spawnCalls;
        unit.SetUnitId(id);
        unitType = type;
        activeState = active;
        spawnPosition = position;
        return 1;
    }
    soft3d::GameUnit* FindStayBoxEnemy(uint8_t id) override
    {
        return present && unit.GetUnitId() == id ? &unit : nullptr;
    }
    const soft3d::Vec3& GetStayBoxEnemySpawnPosition(const soft3d::GameUnit*) const override
    {
        return spawnPosition;
    }
    int GetStayBoxEnemyState(const soft3d::GameUnit*) const override { return state; }
    uint8_t GetStayBoxEnemyType(const soft3d::GameUnit*) const override { return unitType; }
    uint8_t GetStayBoxEnemyActive(const soft3d::GameUnit*) const override { return activeState; }
    uint8_t GetStayBoxEnemySpawnState(const soft3d::GameUnit*) const override { return spawnState; }
    void SetStayBoxEnemySpawnState(soft3d::GameUnit*, uint8_t value) override { spawnState = value; }
    void BindStayBoxEnemy(soft3d::GameUnit*, game::StayBox* box, const soft3d::Vec3&) override
    {
        bound = box;
    }
    void AttachStayBoxEnemy(soft3d::GameUnit*, game::StayBox* box) override { bound = box; }
    void RemoveStayBoxEnemy(soft3d::GameUnit*) override { present = false; ++removeCalls; }

    soft3d::GameUnit unit;
    soft3d::Vec3 spawnPosition{7, 8, 9};
    game::StayBox* bound = nullptr;
    int spawnCalls = 0;
    int removeCalls = 0;
    int state = 0;
    uint8_t unitType = 0;
    uint8_t activeState = 0;
    uint8_t spawnState = 0;
    bool present = true;
};

void TestStayBox()
{
    soft3d::GameUnit enemy;
    enemy.SetUnitId(44);
    enemy.SetCollisionBounds({{1, 1, 1}, {2, 2, 2}});
    TestStayBoxHost host;
    game::StayBox box;
    CHECK(!box.IsEnemyOutside() && box.HasSpawnedEnemies());
    box.Configure(&enemy, &host);
    box.SetLocalBounds({{10, 10, 10}, {20, 20, 20}});
    soft3d::OutputDataStream initialState;
    box.SaveState(initialState);
    std::vector<uint8_t> initialBytes(initialState.GetData(),
        initialState.GetData() + initialState.GetSize());
    initialBytes[initialBytes.size() - 1] = 0;
    soft3d::InputDataStream initialInput;
    initialInput.SetData(initialBytes.data(), initialBytes.size());
    int32_t initialType = 0;
    initialInput.ReadInt32(initialType);
    box.LoadState(initialInput);
    CHECK(!box.HasSpawnedEnemies());
    box.AddAllowedRegion({{0, 0, 0}, {3, 3, 3}});
    game::StayBox::SpawnRecord record;
    record.position = {4, 5, 6};
    record.unitId = 44;
    record.type = 7;
    record.active = 1;
    record.spawnState = 3;
    box.AddSpawnRecord(record);
    box.Update();
    CHECK(box.IsEnemyOutside() && box.HasSpawnedEnemies());
    CHECK(host.spawnCalls == 1 && host.spawnState == 3 && host.bound == &box);
    host.spawnPosition = {10, 11, 12};
    host.unitType = 8;
    host.activeState = 2;
    host.spawnState = 4;
    box.AddDeniedRegion({{0, 0, 0}, {3, 3, 3}});
    box.Update();
    CHECK(!box.HasSpawnedEnemies() && host.removeCalls == 1);
    CHECK(box.GetSpawnRecords()[0].position.x == 10);
    CHECK(box.GetSpawnRecords()[0].type == 8 && box.GetSpawnRecords()[0].active == 2);
    CHECK(box.GetSpawnRecords()[0].spawnState == 4);

    soft3d::OutputDataStream output;
    box.SaveState(output);
    game::StayBox restored;
    soft3d::InputDataStream input;
    input.SetData(output.GetData(), output.GetSize());
    int32_t savedType = 0;
    input.ReadInt32(savedType);
    restored.LoadState(input);
    CHECK(restored.IsEnemyOutside() && !restored.HasSpawnedEnemies());
}

void TestStayBoxCommands()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    TestStayBoxHost host;
    manager->SetStayBoxHost(&host);
    game::StayBox* box = manager->RestoreStayBox();
    CHECK(box != nullptr && manager->FindStayBox(70) == nullptr);
    box->SetUnitId(70);
    box->SetLocalBounds({{-0x10000, -0x10000, -0x10000},
                         {0x10000, 0x10000, 0x10000}});
    CHECK(manager->FindStayBox(70) == box && !box->IsEnemyOutside() && box->HasSpawnedEnemies());
    soft3d::GameUnit enemy;
    enemy.SetUnitId(44);
    manager->AddUnit(&enemy, 0);
    fixture.Execute(207, {Argument::Short(44), Argument::Short(70)});
    CHECK(host.bound == box && box->GetSpawnRecords().size() == 1);
    CHECK(box->GetSpawnRecords()[0].unitId == 44);
    host.unit.SetUnitId(44);
    fixture.Execute(205, {Argument::Short(999), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)});
    fixture.Execute(207, {Argument::Short(999), Argument::Short(70)});
    CHECK(box->GetSpawnRecords().size() == 1);
    const std::vector<Argument> denied = {Argument::Short(70), Argument::Fixed(10),
        Argument::Fixed(10), Argument::Fixed(10), Argument::Fixed(11), Argument::Fixed(11),
        Argument::Fixed(11)};
    const std::vector<Argument> allowed = {Argument::Short(70), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(1), Argument::Fixed(1),
        Argument::Fixed(1)};
    fixture.Execute(205, denied);
    fixture.Execute(206, allowed);
    fixture.scene->GetPlayer()->SetCollisionBounds({{10, 10, 10}, {10, 10, 10}});
    manager->Update();
    CHECK(!box->HasSpawnedEnemies() && host.removeCalls == 1);
    fixture.scene->GetPlayer()->SetCollisionBounds({{0, 0, 0}, {0, 0, 0}});
    manager->Update();
    CHECK(box->HasSpawnedEnemies());
    manager->RemoveUnit(&enemy, 0, false);
    manager->FinishMapLoad(false);
    CHECK(manager->FindStayBox(70) == nullptr);
}

void LoadRealMap(GameFixture& fixture, int mapId)
{
    fixture.engine.StartNewGame();
    fixture.RunFrames(57);
    fixture.engine.GetScriptEngine().Unload();
    fixture.engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
    CHECK(fixture.scene->LoadMap(mapId) == 0);
    for (int frame = 0; frame < 50 && !fixture.scene->IsMapLoaded(); ++frame) {
        fixture.RunFrames(1);
    }
    CHECK(fixture.scene->IsMapLoaded());
}

void TestRealMapEntities()
{
    {
        GameFixture fixture;
        LoadRealMap(fixture, 2008);
        fixture.engine.GetRenderQueues().Clear();
        std::size_t mapUnits = 0;
        for (const auto& entity : fixture.scene->GetBsp()->GetEntities()) {
            auto* unit = dynamic_cast<game::BspUnit*>(entity.get());
            if (unit == nullptr) continue;
            ++mapUnits;
            CHECK(unit->GetUnitType() == 3 && unit->GetUnitId() == 0);
            CHECK(unit->GetObject() != nullptr);
            unit->GetObject()->SetExternalTransformEnabled(true);
            unit->Submit(fixture.engine.GetRenderQueues());
        }
        CHECK(mapUnits == 15);
        CHECK(fixture.engine.GetRenderQueues().GetCount(3) +
              fixture.engine.GetRenderQueues().GetCount(4) == 15);
        fixture.RunFrames(2);
        CHECK(fixture.scene->GetUnitManager()->IsMapReady());
        fixture.scene->ReturnToMainMenu();
        CHECK(fixture.scene->GetBsp()->GetEntities().empty());
    }
    {
        GameFixture fixture;
        LoadRealMap(fixture, 2009);
        CHECK(fixture.scene->GetUnitManager()->GetLensflareCount() == 1);
        CHECK(fixture.scene->GetUnitManager()->GetLensflare(0) != nullptr);
        soft3d::RenderQueues queues;
        fixture.scene->GetUnitManager()->SubmitRenderables(queues);
        CHECK(queues.GetCount(6) == 1);
        fixture.scene->ReturnToMainMenu();
        CHECK(fixture.scene->GetUnitManager()->GetLensflareCount() == 0);
    }
}

void TestRealCameraTriggers()
{
    GameFixture fixture;
    LoadRealMap(fixture, 2012);
    for (int frame = 0; frame < 50 && fixture.scene->GetTransitionStage() != 0; ++frame) {
        fixture.RunFrames(1);
    }
    CHECK(fixture.scene->GetTransitionStage() == 0);
    int count = 0;
    for (soft3d::GameUnit* unit = fixture.scene->GetUnitManager()->GetFirstUnit(6);
         unit != nullptr; unit = unit->GetNextUnit()) {
        auto* trigger = dynamic_cast<game::CameraTrigger*>(unit);
        if (trigger == nullptr) continue;
        fixture.scene->GetPlayer()->SetPosition(soft3d::Center(trigger->GetCollisionBounds()));
        fixture.RunFrames(1);
        CHECK(!trigger->IsOutside());
        CHECK(fixture.scene->GetCameraController()->GetMode() == trigger->GetMode());
        ++count;
    }
    CHECK(count == 2);
}

} // namespace

int main()
{
    try {
        TestAddVariantsAndBasicCommands();
        TestBreakPushAndOrientation();
        TestPersistenceAndElectricSaw();
        TestStayBox();
        TestStayBoxCommands();
        TestRealMapEntities();
        TestRealCameraTriggers();
        std::cout << "game map-unit tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

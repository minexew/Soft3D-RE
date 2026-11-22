#include "game/GameSI.hpp"
#include "game/Scene.hpp"
#include "game/Triggers.hpp"
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
        engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Accept));
        SetInside(true);
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

    void SetInside(bool inside)
    {
        scene->GetPlayer()->SetCollisionBounds(inside
            ? soft3d::ObjectBounds{{-1, -1, -1}, {1, 1, 1}}
            : soft3d::ObjectBounds{{0x100000, 0x100000, 0x100000},
                                   {0x100001, 0x100001, 0x100001}});
    }

    void Click()
    {
        engine.GetInput().ClearKey();
        engine.GetInput().KeyDown(soft3d::Key::Accept);
        engine.GetInput().KeyUp(soft3d::Key::Accept);
    }

    game::InteractionState& Ui() { return scene->GetUnitManager()->GetInteractionState(); }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
};

std::vector<Argument> BoundsArguments(int)
{
    return {Argument::Fixed(-0x10000), Argument::Fixed(-0x10000), Argument::Fixed(-0x10000),
            Argument::Fixed(0x10000), Argument::Fixed(0x10000), Argument::Fixed(0x10000)};
}

std::vector<Argument> AddThingArguments(int id, const char* model = "oi_yaoping_a")
{
    return {Argument::Short(static_cast<int16_t>(id)), Argument::Int(3), Argument::Int(-1),
        Argument::String(model), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0x10000), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0x10000), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0x10000), Argument::Bool(true)};
}

std::vector<Argument> AddDoorArguments(int id, int item, int text, int resource, int map, int episode)
{
    std::vector<Argument> result = {Argument::Short(static_cast<int16_t>(id)),
        Argument::Short(static_cast<int16_t>(item)), Argument::Short(static_cast<int16_t>(text)),
        Argument::Short(static_cast<int16_t>(resource)), Argument::Short(static_cast<int16_t>(map)),
        Argument::Short(static_cast<int16_t>(episode)), Argument::Fixed(1), Argument::Fixed(2),
        Argument::Fixed(3), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0x10000)};
    const auto bounds = BoundsArguments(12);
    result.insert(result.end(), bounds.begin(), bounds.end());
    return result;
}

void CheckSecondSave(const soft3d::GameUnit& original, soft3d::GameUnit& restored)
{
    soft3d::OutputDataStream first;
    original.SaveState(first);
    soft3d::InputDataStream input;
    input.SetData(first.GetData(), first.GetSize());
    int32_t type = 0;
    input.ReadInt32(type);
    restored.LoadState(input);
    soft3d::OutputDataStream second;
    restored.SaveState(second);
    CHECK(second.GetSize() == first.GetSize());
    CHECK(std::memcmp(second.GetData(), first.GetData(), first.GetSize()) == 0);
}

void TestEventTrigger()
{
    GameFixture fixture;
    auto arguments = std::vector<Argument>{Argument::Short(1), Argument::Int(2), Argument::Short(77)};
    const auto bounds = BoundsArguments(3);
    arguments.insert(arguments.end(), bounds.begin(), bounds.end());
    fixture.Execute(149, arguments);
    auto* trigger = dynamic_cast<game::EventTrigger*>(fixture.scene->GetUnitManager()->FindUnit(4, 1));
    CHECK(trigger != nullptr && trigger->GetUnitType() == 0x1b);
    CHECK(trigger->GetRemainingCount() == 2 && trigger->GetTextId() == 77);
    CHECK(!trigger->IsInside());
    fixture.Execute(151, {Argument::Short(1), Argument::Short(78)});
    fixture.Execute(150, {Argument::Short(1), Argument::Int(1)});
    fixture.scene->GetUnitManager()->Update();
    CHECK(trigger->GetRemainingCount() == 0 && trigger->IsInside());
    CHECK(fixture.Execute(152, {Argument::Short(1)}, true) == 1);
    CHECK(fixture.Ui().GetDialogKind() == 0 && fixture.Ui().GetDialogText() == 78);
    CHECK(fixture.scene->GetUnitManager()->IsPaused());
    fixture.Ui().SetDialogResult(0);
    fixture.SetInside(false);
    fixture.scene->GetUnitManager()->Update();
    CHECK(trigger->IsInside());
    CHECK(fixture.Execute(152, {Argument::Short(1)}, true) == 1);
    fixture.SetInside(true);
    fixture.scene->GetUnitManager()->Update();
    CHECK(fixture.Ui().GetDialogText() == 78 && !fixture.scene->GetUnitManager()->IsPaused());

    fixture.SetInside(false);
    fixture.Execute(150, {Argument::Short(1), Argument::Int(-1)});
    fixture.scene->GetUnitManager()->Update();
    CHECK(!trigger->IsInside() && !fixture.scene->GetUnitManager()->IsPaused());
    fixture.SetInside(true);
    fixture.scene->GetUnitManager()->Update();
    CHECK(trigger->GetRemainingCount() == -1 && fixture.scene->GetUnitManager()->IsPaused());
    fixture.Ui().SetDialogResult(0);
    fixture.SetInside(false);
    fixture.scene->GetUnitManager()->Update();
    fixture.SetInside(true);
    fixture.scene->GetUnitManager()->Update();
    CHECK(trigger->GetRemainingCount() == -1 && fixture.scene->GetUnitManager()->IsPaused());
    fixture.Ui().SetDialogResult(0);

    trigger->SetRemainingCount(-3);
    trigger->SetTextId(1234);
    game::EventTrigger restored;
    CheckSecondSave(*trigger, restored);
    CHECK(restored.GetRemainingCount() == -3 && restored.GetTextId() == 1234);
    CHECK(restored.IsInside());
}

void TestPickTrigger()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    fixture.Execute(221, AddThingArguments(10));
    fixture.Execute(223, {Argument::Short(10), Argument::Short(2)});
    auto arguments = std::vector<Argument>{Argument::Short(20), Argument::Short(10), Argument::Short(7)};
    const auto bounds = BoundsArguments(3);
    arguments.insert(arguments.end(), bounds.begin(), bounds.end());
    fixture.Execute(153, arguments);
    auto* trigger = dynamic_cast<game::PickTrigger*>(manager->FindUnit(4, 20));
    auto* pickable = dynamic_cast<game::Pickable*>(manager->FindUnit(2, 10));
    CHECK(trigger != nullptr && pickable != nullptr && trigger->GetUnitType() == 0x25);
    CHECK(trigger->GetDialogTitleId() == 7 && trigger->IsAvailable());

    manager->Update();
    CHECK(trigger->IsPromptActive() && fixture.Ui().GetPrompt() == 0);
    CHECK(fixture.scene->GetPlayer()->AreAttacksDisabled());
    fixture.Click();
    manager->Update();
    CHECK(trigger->IsDialogActive() && manager->IsPaused());
    CHECK(fixture.Ui().GetDialogKind() == 1 && fixture.Ui().GetDialogText() == 7);
    CHECK(fixture.Ui().GetDialogOptionCount() == 2);
    CHECK(fixture.Ui().GetDialogOption(0) == 3 && fixture.Ui().GetDialogOption(1) == 4);
    CHECK(fixture.Ui().GetDialogOption(4) ==
          static_cast<int32_t>(fixture.engine.GetContent().items[2].nameString));
    fixture.Ui().SetDialogResult(1);
    manager->Update();
    CHECK(!trigger->IsDialogActive() && manager->FindUnit(2, 10) == pickable);

    soft3d::OutputDataStream saved;
    trigger->SaveState(saved);
    game::PickTrigger restored;
    soft3d::InputDataStream input;
    input.SetData(saved.GetData(), saved.GetSize());
    int32_t type = 0;
    int32_t linkedId = 0;
    input.ReadInt32(type);
    restored.LoadState(input);
    input.ReadInt32(linkedId);
    CHECK(type == 0x25 && linkedId == 10 && restored.GetDialogTitleId() == 7);
    restored.Configure(pickable, manager, fixture.scene->GetPlayer(), fixture.scene->GetControlMap(),
                       &fixture.Ui());
    soft3d::OutputDataStream second;
    restored.SaveState(second);
    CHECK(second.GetSize() == saved.GetSize());
    CHECK(std::memcmp(second.GetData(), saved.GetData(), saved.GetSize()) == 0);

    fixture.Click();
    manager->Update();
    CHECK(trigger->IsDialogActive());
    fixture.Ui().SetDialogResult(0);
    manager->Update();
    CHECK(manager->FindUnit(2, 10) == nullptr && trigger->GetPickable() == nullptr);
    CHECK(fixture.scene->GetPlayer()->HasItem(2) && !trigger->IsPromptActive());

    fixture.Execute(221, AddThingArguments(11));
    fixture.Execute(223, {Argument::Short(11), Argument::Short(3)});
    arguments[0] = Argument::Short(21);
    arguments[1] = Argument::Short(11);
    fixture.Execute(153, arguments);
    auto* fullTrigger = dynamic_cast<game::PickTrigger*>(manager->FindUnit(4, 21));
    for (int item = 10; item < 19; ++item) fixture.scene->GetPlayer()->AddItem(item, false);
    fixture.Click();
    manager->Update();
    CHECK(fullTrigger != nullptr && !fullTrigger->IsDialogActive());
    CHECK(fixture.Ui().GetDialogKind() == 3 && fixture.Ui().GetDialogText() == 5);
    CHECK(manager->FindUnit(2, 11) != nullptr && !fixture.scene->GetPlayer()->HasItem(3));
    fixture.Ui().SetDialogResult(0);
    fixture.Execute(154, {Argument::Short(21), Argument::Bool(false)});
    CHECK(!fullTrigger->IsEnabled() && fixture.Ui().GetPrompt() == -1);
    manager->Update();
    CHECK(!fullTrigger->IsPromptActive() && !fixture.scene->GetPlayer()->AreAttacksDisabled());
}

void TestDoorTrigger()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    fixture.Execute(155, AddDoorArguments(30, 12, 99, 4, 2010, 2020));
    auto* door = dynamic_cast<game::DoorTrigger*>(manager->FindUnit(4, 30));
    CHECK(door != nullptr && door->GetUnitType() == 0x22 && door->IsEnabled());
    fixture.Execute(156, {Argument::Short(30), Argument::Short(13)});
    fixture.Execute(157, {Argument::Short(30), Argument::Short(100)});
    fixture.Click();
    manager->Update();
    CHECK(door->IsDialogActive() && fixture.Ui().GetDialogText() == 100);
    CHECK(!fixture.scene->IsNextLevelPending() && !door->IsUnlocking());
    fixture.Ui().SetDialogResult(0);
    manager->Update();
    CHECK(!door->IsDialogActive());

    fixture.scene->GetPlayer()->AddItem(13, false);
    fixture.Click();
    manager->Update();
    CHECK(door->IsUnlocking() && door->GetUnlockTimer() == 0);
    CHECK(!fixture.scene->GetPlayer()->HasItem(13));
    for (int frame = 0; frame < 7; ++frame) manager->Update();
    CHECK(door->GetUnlockTimer() == 7 && !fixture.scene->IsNextLevelPending());
    manager->Update();
    CHECK(door->GetUnlockTimer() == 8 && fixture.scene->IsNextLevelPending());
    CHECK(fixture.scene->GetNextMapId() == 2010 && fixture.scene->GetNextEpisodeId() == 2020);
    CHECK(fixture.scene->GetDoorResourceIndex() == 4);
    CHECK(fixture.scene->GetSpawnPosition().x == 1 && fixture.scene->GetSpawnDirection().z == 0x10000);

    fixture.Execute(155, AddDoorArguments(10001, -1, 101, 5, 2011, 2021));
    auto* tracked = dynamic_cast<game::DoorTrigger*>(manager->FindUnit(4, 10001));
    fixture.Click();
    manager->Update();
    CHECK(tracked != nullptr && manager->GetDoorState(10001) == 3);
    fixture.Ui().SetDialogResult(0);
    manager->Update();
    fixture.Execute(156, {Argument::Short(10001), Argument::Short(14)});
    fixture.Click();
    manager->Update();
    CHECK(manager->GetDoorState(10001) == 2);
    fixture.Ui().SetDialogResult(0);
    manager->Update();
    fixture.scene->GetPlayer()->AddItem(14, false);
    fixture.Click();
    manager->Update();
    CHECK(manager->GetDoorState(10001) == 1 && tracked->IsUnlocking());
    fixture.Execute(158, {Argument::Short(10001), Argument::Bool(false)});
    CHECK(!tracked->IsEnabled() && !tracked->IsUnlocking() && tracked->GetUnlockTimer() == 0);
    fixture.Execute(158, {Argument::Short(999), Argument::Bool(false)});

    fixture.Execute(155, AddDoorArguments(10000, 0, 102, 6, 2012, 2022));
    fixture.Click();
    manager->Update();
    CHECK(fixture.scene->GetAuthorizationStage() == 1 && fixture.scene->GetAuthorizationTimer() == 1);
    CHECK(manager->GetDoorState(10000) == 1 && fixture.scene->GetNextMapId() == 2012);

    game::DoorTrigger restored;
    CheckSecondSave(*tracked, restored);
    CHECK(restored.GetRequiredItem() == 14 && restored.GetLockedText() == 101);
    CHECK(restored.GetNextMap() == 2011 && restored.GetNextEpisode() == 2021);
    CHECK(restored.GetSpawnPosition().y == 2 && restored.GetSpawnDirection().z == 0x10000);
}

class AttackingEnemy final : public soft3d::GameUnit, public game::CombatTarget {
public:
    bool IsAttackable() const override { return true; }
    bool IsAttacking() const override { return true; }
    int32_t GetAttackPower() const override { return 1; }
    soft3d::Fixed GetPlayerDistance() const override { return 0; }
};

void TestActTrigger()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto arguments = std::vector<Argument>{Argument::Short(40)};
    const auto bounds = BoundsArguments(1);
    arguments.insert(arguments.end(), bounds.begin(), bounds.end());
    fixture.Execute(159, arguments);
    auto* trigger = dynamic_cast<game::ActTrigger*>(manager->FindUnit(4, 40));
    CHECK(trigger != nullptr && trigger->GetUnitType() == 0x21);
    CHECK(fixture.Execute(162, {Argument::Short(40)}, true) == 0);
    fixture.Execute(163, {Argument::Short(40)});
    CHECK(fixture.Execute(162, {Argument::Short(40)}, true) == -1);
    CHECK(fixture.Execute(162, {Argument::Short(999)}, true) == 0);
    fixture.Execute(163, {Argument::Short(999)});
    fixture.Execute(164, {Argument::Short(999), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)});

    fixture.Execute(161, {Argument::Short(40), Argument::Int(5), Argument::Short(200)});
    fixture.Execute(161, {Argument::Short(40), Argument::Int(2), Argument::Short(201)});
    fixture.Execute(161, {Argument::Short(40), Argument::Int(3), Argument::Short(202)});
    fixture.Execute(161, {Argument::Short(40), Argument::Int(0), Argument::Short(299)});
    fixture.Execute(161, {Argument::Short(40), Argument::Int(6), Argument::Short(205)});
    fixture.Execute(160, {Argument::Short(40), Argument::Int(2)});
    fixture.Execute(164, {Argument::Short(40), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0x10000)});
    manager->Update();
    CHECK(trigger->IsPromptActive() && fixture.Ui().GetPrompt() == 3);
    fixture.Click();
    manager->Update();
    CHECK(trigger->IsDialogActive() && fixture.Ui().GetDialogText() == 200);
    CHECK(fixture.Ui().GetDialogOptionCount() == 2 && fixture.Ui().GetDialogOption(0) == 299);
    CHECK(fixture.Ui().GetDialogOption(1) == 202);
    CHECK(fixture.scene->GetCameraController()->IsFrozen() && !fixture.scene->GetPlayer()->IsVisible());
    fixture.Ui().SetDialogResult(1);
    manager->Update();
    CHECK(!trigger->IsDialogActive() && fixture.Execute(162, {Argument::Short(40)}, true) == 2);
    CHECK(!fixture.scene->GetCameraController()->IsFrozen());
    CHECK(fixture.scene->GetCameraController()->IsInstant() && fixture.scene->GetPlayer()->IsVisible());
    fixture.Execute(163, {Argument::Short(40)});
    CHECK(fixture.Execute(162, {Argument::Short(40)}, true) == -1);

    fixture.Execute(165, {Argument::Short(40), Argument::Bool(false)});
    CHECK(!trigger->IsEnabled() && trigger->GetCameraPosition().x == trigger->GetCameraTarget().x);
    manager->Update();
    CHECK(!trigger->IsPromptActive() && !fixture.scene->GetPlayer()->AreAttacksDisabled());
    fixture.engine.GetInput().ClearKey();

    manager->RemoveUnitById(40, 4, true);
    arguments[0] = Argument::Short(41);
    fixture.Execute(159, arguments);
    auto* simple = dynamic_cast<game::ActTrigger*>(manager->FindUnit(4, 41));
    fixture.Execute(161, {Argument::Short(41), Argument::Int(5), Argument::Short(300)});
    AttackingEnemy enemy;
    manager->AddUnit(&enemy, 0);
    manager->Update();
    CHECK(!simple->IsPromptActive() && fixture.Ui().GetPrompt() == -1);
    fixture.Execute(160, {Argument::Short(41), Argument::Int(1)});
    manager->Update();
    CHECK(simple->IsPromptActive());
    manager->RemoveUnit(&enemy, 0, false);
    fixture.Execute(160, {Argument::Short(41), Argument::Int(0)});
    manager->Update();
    CHECK(simple->IsPromptActive());
    CHECK(simple->GetOptionCount() == 0 && simple->IsEnabled());
    fixture.Click();
    manager->Update();
    CHECK(simple->IsDialogActive() && fixture.Ui().GetDialogKind() == 3);
    CHECK(fixture.Ui().GetDialogText() == 300);
    fixture.Ui().SetDialogResult(0);
    manager->Update();
    CHECK(simple->GetSelection() == 1);

    for (int index = 2; index <= 6; ++index) {
        fixture.Execute(161, {Argument::Short(41), Argument::Int(index),
                              Argument::Short(static_cast<int16_t>(400 + index))});
    }
    fixture.Execute(161, {Argument::Short(41), Argument::Int(1), Argument::Short(401)});
    CHECK(simple->IsPromptActive());
    game::ActTrigger restored;
    CheckSecondSave(*simple, restored);
    CHECK(restored.GetReserved5c() == 401 && restored.GetTitleId() == 405);
    CHECK(restored.GetOptionId(0) == 402 && restored.GetOptionId(2) == 404);
    CHECK(restored.GetOptionId(3) == -1 && !restored.IsPromptActive());
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
    fixture.RunFrames(2);
    CHECK(fixture.scene->GetUnitManager()->IsMapReady());
}

void TestRealMapItems()
{
    {
        GameFixture fixture;
        LoadRealMap(fixture, 2008);
        auto* manager = fixture.scene->GetUnitManager();
        auto* pickable = dynamic_cast<game::Pickable*>(manager->FindUnit(2, 9999));
        auto* trigger = dynamic_cast<game::PickTrigger*>(manager->FindUnit(4, 9999));
        CHECK(pickable != nullptr && trigger != nullptr && trigger->GetPickable() == pickable);
        CHECK(pickable->GetItemId() == 1005 && trigger->GetDialogTitleId() == 26);
        game::GameItemGroup* group = manager->GetData().FindGameItemGroup(2008);
        CHECK(group != nullptr && group->records.size() == 1 && group->records[0].itemId == 1005);
        pickable->PickUp(fixture.scene->GetPlayer());
        CHECK(group->records[0].itemId == 0 && fixture.scene->GetPlayer()->HasItem(1005));
        manager->SpawnMapItems();
        CHECK(manager->FindUnit(2, 9998) == nullptr && manager->FindUnit(4, 9998) == nullptr);
    }
    {
        GameFixture fixture;
        LoadRealMap(fixture, 2008);
        auto* manager = fixture.scene->GetUnitManager();
        manager->ClearBucket(2, true);
        manager->ClearBucket(4, true);
        manager->FinishMapLoad(true);
        CHECK(manager->FindUnit(2, 9999) == nullptr && manager->FindUnit(4, 9999) == nullptr);
    }
}

void TestRealStartupBoundary()
{
    GameFixture fixture;
    fixture.engine.StartNewGame({2, 1});
    fixture.RunFrames(57);
    for (int frame = 0; frame < 40 && !fixture.scene->IsMapLoaded(); ++frame) {
        fixture.RunFrames(1);
    }
    CHECK(fixture.scene->IsMapLoaded());
    bool unsupported = false;
    for (int frame = 0; frame < 40 && !unsupported; ++frame) {
        try {
            fixture.RunFrames(1);
        } catch (const game::GameSIError& error) {
            CHECK(error.GetCommandId() == 102);
            CHECK(error.GetArmAddress() == 0x000671ac);
            unsupported = true;
        }
    }
    CHECK(unsupported);
}

} // namespace

int main()
{
    try {
        TestEventTrigger();
        TestPickTrigger();
        TestDoorTrigger();
        TestActTrigger();
        TestRealMapItems();
        TestRealStartupBoundary();
        std::cout << "game trigger tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

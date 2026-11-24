#include "game/GameSI.hpp"
#include "game/Player.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"

#include <algorithm>
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
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
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
    static Argument String(const char* value)
    {
        return {soft3d::ScriptValueType::String, 0, value};
    }
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

std::vector<uint8_t> MakeHostScript(uint16_t id, const std::vector<Argument>& arguments,
                                    bool output)
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
        hooks.returnToMenu = [this] { scene->ReturnToMainMenu(); };
        engine.SetGameplayHooks(std::move(hooks));
        engine.Start();
    }

    ~GameFixture() { engine.SetScriptInterface(nullptr); }

    static soft3d::EngineConfig MakeConfig()
    {
        soft3d::EngineConfig config;
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

    void LoadRealMap()
    {
        engine.StartNewGame();
        RunFrames(57);
        engine.GetScriptEngine().Unload();
        engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
        CHECK(scene->LoadMap(2008) == 0);
        for (int frame = 0; frame < 80 &&
             (scene->GetMapProgress() != -1 || scene->GetTransitionStage() != 0); ++frame) {
            RunFrames(1);
        }
        CHECK(scene->GetMapProgress() == -1);
        CHECK(scene->GetTransitionStage() == 0);
        scene->GetPlayer()->ChangeState(game::PlayerStand);
        scene->GetCameraController()->Reset();
        scene->GetCameraController()->SetInstant(true);
        for (int action = 1; action <= 7; ++action) {
            engine.GetControlMap().SetActionEnabled(action, true);
        }
        engine.GetControlMap().SetEnabled(true);
        engine.SetPreUpdateEnabled(true);
        engine.SetInGame(true);
    }

    uint32_t FrameHash()
    {
        CHECK(engine.RunFrame());
        uint32_t hash = 2166136261U;
        const auto* backend = dynamic_cast<const soft3d::platform::DummyVideoBackend*>(
            engine.GetVideo().GetBackend());
        CHECK(backend != nullptr);
        for (uint32_t pixel : backend->GetPresentedPixels()) {
            hash = (hash ^ pixel) * 16777619U;
        }
        return hash;
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
};

class TestEnemy final : public soft3d::Creature, public game::CombatTarget {
public:
    TestEnemy()
    {
        SetCollisionBounds({{-0x100000, -0x100000, -0x100000},
                            {0x100000, 0x100000, 0x100000}});
    }

    bool IsAttackable() const override { return attackable; }
    bool IsAttacking() const override { return attacking; }
    int32_t GetAttackPower() const override { return attack; }
    soft3d::Fixed GetPlayerDistance() const override { return distance; }

    bool attackable = true;
    bool attacking = false;
    int32_t attack = 7;
    soft3d::Fixed distance = soft3d::fixedOne;
};

class TestPushable final : public soft3d::VisUnit, public game::PushableTarget {
public:
    TestPushable(soft3d::s3dResManager* resources, soft3d::Renderer* renderer)
        : VisUnit(resources, 2, renderer)
    {
        SetUnitType(0x1d);
    }

    bool CanBePushed() const override { return true; }
    soft3d::AudioData* GetPushSound() const override { return nullptr; }
    void SetPusher(game::Player* value) override { pusher = value; }

    game::Player* pusher = nullptr;
};

void FinishAnimation(game::Player* player)
{
    soft3d::Object* object = player->GetObject(0);
    CHECK(object != nullptr && object->GetAnimation() != nullptr);
    object->SetAnimationTime(object->GetAnimation()->GetEndTime() - object->GetAnimationSpeed());
    player->PostUpdate();
    CHECK(player->IsAnimationFinished());
}

void TestEveryStateAndRegistration()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    game::Player* player = fixture.scene->GetPlayer();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);
    for (int id = 0; id < 20; ++id) {
        CHECK(player->GetState(id) != nullptr);
        CHECK(player->GetState(id)->IsEnabled());
        player->ChangeState(id);
        CHECK(player->GetCurrentStateId() == id);
        player->Update();
        player->PostUpdate();
        player->ChangeState(game::PlayerStand);
    }
    CHECK(player->GetCurrentStateId() == game::PlayerStand);
}

void TestAttackChainsAndDamage()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    game::Player* player = fixture.scene->GetPlayer();
    player->PrepareWeapon(fixture.scene->GetUnitManager()->GetData(), 1);
    player->SetWeapon(1);

    player->StartLightAttack();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackA1);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackA2);
    CHECK(player->GetSword()->IsHitActive());
    FinishAnimation(player);
    player->Update();
    player->Update();
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackA3);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->ChangeState(game::PlayerAttackB1);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackB2);
    FinishAnimation(player);
    player->Update();
    player->Update();
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackB3);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->StartHeavyAttack();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackC1);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackC2);
    CHECK(player->GetGunFire().GetFramesRemaining() == 1);
    FinishAnimation(player);
    player->Update();
    player->Update();
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackC3);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    TestEnemy enemy;
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    CHECK(player->GetAttackDamage(&enemy) == 20);
    player->ChangeState(game::PlayerStand);
    player->DamageFrom(&enemy);
    CHECK(player->GetHealth() == 93);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt);

    player->PrepareWeapon(fixture.scene->GetUnitManager()->GetData(), 3);
    player->SetWeapon(3);
    fixture.scene->GetUnitManager()->AddUnit(&enemy, 0);
    fixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Accept));
    fixture.engine.GetInput().KeyDown(soft3d::Key::Accept);
    player->ChangeState(game::PlayerAttackC1);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackC2);
    CHECK(player->GetAttackDamage(&enemy) == 27);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Accept);
    fixture.scene->GetUnitManager()->RemoveUnit(&enemy, 0, false);
    player->ChangeState(game::PlayerStand);

    player->AddGunExperience(49);
    CHECK(player->GetGunLevel() == 0);
    player->AddGunExperience(1);
    CHECK(player->GetGunLevel() == 1);
    CHECK(fixture.scene->GetTutorialId() == 41);
    player->AddGunExperience(50);
    CHECK(player->GetGunLevel() == 2);
    CHECK(fixture.scene->GetTutorialId() == 42);
}

void TestHurtDeathQteHideAndPush()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    game::Player* player = fixture.scene->GetPlayer();

    player->Damage(10);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->DamageHurt2();
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt2);
    CHECK(player->GetHealth() == 60);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    TestEnemy enemy;
    player->LoadSpecialAnimations(fixture.scene->GetUnitManager()->GetData(), false);
    player->DamageHurt3(&enemy, 0x8000);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt3);
    FinishAnimation(player);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->RequestHurt4();
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt4);
    for (int frame = 0; frame < 24; ++frame) player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->BeginQTE(1, 2, 1);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerQTE);
    CHECK(player->GetQteResult() == -1);
    CHECK(player->GetQteKey() >= 1 && player->GetQteKey() <= 4);
    CHECK(fixture.scene->GetQteUiKey() == player->GetQteKey() - 1);
    const soft3d::Key key = static_cast<soft3d::Key>(
        fixture.engine.GetControlMap().GetPrimaryBinding(player->GetQteKey()));
    fixture.engine.GetInput().KeyDown(key);
    fixture.engine.GetInput().KeyUp(key);
    player->Update();
    CHECK(player->GetQteResult() == 1);

    player->BeginQTE(1, 2, 1);
    player->Update();
    const soft3d::Key wrong = player->GetQteKey() == 1 ? soft3d::Key::Right : soft3d::Key::Left;
    fixture.engine.GetInput().KeyDown(wrong);
    fixture.engine.GetInput().KeyUp(wrong);
    player->Update();
    CHECK(player->GetQteResult() == 0);
    CHECK(fixture.scene->GetQteUiFailure() == 1);

    player->BeginQTE(0, 1, 3);
    player->Update();
    CHECK(player->GetQteResult() == 0);

    player->ChangeState(game::PlayerStand);
    player->BeginHide({0x10000, 0x20000, 0x30000}, {0, 0, soft3d::fixedOne}, 3,
                      0x9999, 0xcccc);
    CHECK(player->IsHiddenOrFPS());
    CHECK(fixture.scene->IsHideMaskActive());
    CHECK(player->GetPosition().x == 0x10000);
    player->ExitHide();
    CHECK(!player->IsHiddenOrFPS());
    CHECK(!fixture.scene->IsHideMaskActive());
    CHECK(player->GetPosition().x == 0);

    player->EnterFPS();
    CHECK(player->GetCurrentStateId() == game::PlayerFPS);
    player->BeginHide({0x40000, 0, 0}, {0, 0, soft3d::fixedOne}, 4, 0x9999, 0xcccc);
    player->ExitHide();
    CHECK(player->GetCurrentStateId() == game::PlayerFPS);
    player->ChangeState(game::PlayerStand);

    TestPushable pushable(fixture.scene->GetResources(), &fixture.engine.GetRenderer());
    CHECK(pushable.LoadObjects(u"r_ken_a", nullptr, 1) == soft3d::S3D_OK);
    player->StartPush(&pushable);
    CHECK(player->GetCurrentStateId() == game::PlayerPush);
    CHECK(pushable.pusher == player);
    CHECK(!pushable.IsCollisionEnabled());
    CHECK(player->IsPushingUnit(pushable.GetUnitId()));
    player->ChangeState(game::PlayerStand);
    CHECK(pushable.pusher == nullptr);
    CHECK(pushable.IsCollisionEnabled());

    player->SetHealth(0);
    player->Update();
    CHECK(player->GetCurrentStateId() == game::PlayerDie);
    player->Update();
    CHECK(player->IsDeathComplete());
    fixture.RunFrames(56);
    CHECK(fixture.scene->GetGameOverStage() == 2);
}

void TestInventoryAndPersistence()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    game::Player* player = fixture.scene->GetPlayer();
    player->AddItem(1, false);
    player->AddItem(1, false);
    player->AddItem(2, false);
    CHECK(player->GetHealthPackCount() == 2);
    CHECK(player->GetInventoryItem(0) == 1);
    CHECK(player->GetInventoryItem(1) == 2);
    CHECK(player->UseItem(0) == 0);
    player->Damage(50);
    CHECK(player->UseItem(0) == 1);
    CHECK(player->GetHealth() == 100);
    player->SetHealth(25);
    CHECK(player->UseItem(0) == 1);
    CHECK(player->GetHealth() == 75);
    CHECK(!player->HasItem(1));

    player->AddItem(1001, false);
    player->AddItem(2001, false);
    CHECK(player->HasItem(1001));
    CHECK(player->HasItem(2001));
    CHECK(fixture.engine.GetConfiguration().unlocks.documents[0] == 1);
    CHECK(fixture.engine.GetConfiguration().unlocks.photos[0] == 1);
    player->SnapshotDayState();
    player->RemoveItem(1001);
    player->RemoveItem(2001);
    player->RestoreDayState();
    CHECK(player->HasItem(1001) && player->HasItem(2001));

    player->RemoveItem(2);
    player->AddItem(17, false);
    player->AddItem(18, false);
    CHECK(player->CombineItems(0, 1));
    CHECK(player->HasItem(19));
    player->RemoveItem(19);
    game::ItemTrigger itemTrigger;
    itemTrigger.AddItem(5);
    player->RegisterItemTrigger(&itemTrigger);
    player->AddItem(5, false);
    CHECK(player->UseItem(0) == 2);
    CHECK(player->GetUsedItem() == 5);
    player->ClearUsedItem();
    CHECK(player->GetUsedItem() == 0);

    player->SetHealth(100);
    soft3d::OutputDataStream output;
    player->SaveState(output);
    const std::vector<uint8_t> saved(output.GetData(), output.GetData() + output.GetSize());
    player->SetHealth(20);
    player->RemoveItem(5);
    soft3d::InputDataStream input(saved.data(), static_cast<int32_t>(saved.size()));
    player->LoadState(input);
    CHECK(player->GetHealth() == 100);
    CHECK(player->HasItem(5));
    soft3d::OutputDataStream second;
    player->SaveState(second);
    CHECK(second.GetSize() == output.GetSize());
    CHECK(std::memcmp(second.GetData(), saved.data(), saved.size()) == 0);
}

void TestPlayerCommands()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    game::Player* player = fixture.scene->GetPlayer();
    fixture.Execute(118, {Argument::Bool(true)});
    CHECK(player->GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::Normal);
    fixture.Execute(118, {Argument::Bool(false)});
    CHECK(player->GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::SecondaryStreams);
    soft3d::RenderQueues secondaryQueues;
    player->SubmitRenderables(secondaryQueues);
    CHECK(secondaryQueues.GetCount(3) + secondaryQueues.GetCount(4) > 0);
    fixture.Execute(118, {Argument::Bool(true)});
    fixture.Execute(119, {Argument::String("r_ken_stand_a"), Argument::Bool(true)});
    CHECK(fixture.Execute(120, {}, true) == 0);
    fixture.Execute(121, {Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)});
    CHECK(fixture.Execute(122, {Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)}, true) == 1);
    CHECK(fixture.Execute(123, {Argument::Fixed(1), Argument::Fixed(0), Argument::Fixed(0),
                                Argument::Fixed(soft3d::fixedOne)}, true) == 1);
    const auto bounds = player->GetCollisionBounds();
    const std::vector<Argument> point = {Argument::Fixed(bounds.minimum.x),
        Argument::Fixed(bounds.minimum.y), Argument::Fixed(bounds.minimum.z),
        Argument::Fixed(bounds.maximum.x), Argument::Fixed(bounds.maximum.y),
        Argument::Fixed(bounds.maximum.z)};
    const int inside = fixture.Execute(124, point, true);
    CHECK(fixture.Execute(125, point, true) == !inside);
    CHECK(fixture.Execute(126, {Argument::Short(1), Argument::Fixed(1)}, true) == 0);
    CHECK(fixture.Execute(127, {Argument::Short(1), Argument::Fixed(1)}, true) == 0);
    fixture.Execute(128, {Argument::Int(0)});
    CHECK(fixture.Execute(136, {}, true) == 1);
    CHECK(fixture.Execute(129, {}, true) == 0);
    CHECK(fixture.Execute(130, {}, true) == 100);
    fixture.Execute(131, {Argument::Short(50)});
    CHECK(player->GetHealth() == 50);
    fixture.Execute(132, {Argument::Short(10)});
    CHECK(player->GetHealth() == 60);
    fixture.Execute(133, {Argument::Short(5)});
    CHECK(player->GetHealth() == 55);
    player->Reset(fixture.scene->GetUnitManager()->GetData());
    fixture.Execute(134, {Argument::Int(0)});
    CHECK(fixture.Execute(135, {}, true) == 0);
    CHECK(fixture.Execute(137, {}, true) == 0);
    fixture.Execute(138, {Argument::Short(1), Argument::Short(2), Argument::Short(1)});
    CHECK(fixture.Execute(139, {}, true) == -1);
    player->SetUsedItem(9);
    CHECK(fixture.Execute(140, {}, true) == 9);
    fixture.Execute(141);
    CHECK(player->GetUsedItem() == 0);
    fixture.Execute(142);
    CHECK(fixture.Execute(137, {}, true) == 1);
    CHECK(fixture.Execute(143, {}, true) == 0);
    fixture.Execute(144, {Argument::Fixed(0x12345)});
    CHECK(player->GetNavigationSpeed() == 0x12345);
    CHECK(fixture.Execute(145, {Argument::Short(1)}, true) == 0);
    fixture.Execute(146, {Argument::Bool(true)});
    CHECK(fixture.Execute(147, {}, true) == 1);
    fixture.Execute(146, {Argument::Bool(false)});
    CHECK(fixture.Execute(147, {}, true) == 0);
    fixture.Execute(148);
    fixture.Execute(255, {Argument::Short(2)});
    CHECK(fixture.Execute(256, {Argument::Short(2)}, true) == 1);
    fixture.Execute(257, {Argument::Short(2)});
    CHECK(fixture.Execute(256, {Argument::Short(2)}, true) == 0);
}

void TestEngineInputAndFrames()
{
    GameFixture fixture;
    fixture.LoadRealMap();
    game::Player* player = fixture.scene->GetPlayer();
    CHECK(fixture.engine.IsInGame());
    CHECK(fixture.engine.IsPreUpdateEnabled());
    const soft3d::Vec3 initial = player->GetPosition();
    const uint32_t stand = fixture.FrameHash();
    fixture.engine.GetInput().KeyDown(soft3d::Key::Left);
    CHECK(fixture.engine.GetControlMap().ActionDown(fixture.engine.GetInput(), 1, false));
    const uint32_t run = fixture.FrameHash();
    CHECK(player->GetCurrentStateId() == game::PlayerRun);
    CHECK(player->GetPosition().x != initial.x || player->GetPosition().z != initial.z);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Left);
    fixture.RunFrames(2);
    CHECK(player->GetCurrentStateId() == game::PlayerStand);

    player->PrepareWeapon(fixture.scene->GetUnitManager()->GetData(), 1);
    player->SetWeapon(1);
    fixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Accept));
    fixture.engine.GetInput().KeyDown(soft3d::Key::Accept);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Accept);
    CHECK(fixture.engine.GetControlMap().ActionClick(fixture.engine.GetInput(), 5, false));
    const uint32_t attack = fixture.FrameHash();
    CHECK(player->GetCurrentStateId() == game::PlayerAttackA1);
    player->ChangeState(game::PlayerStand);
    player->Damage(1);
    const uint32_t hurt = fixture.FrameHash();
    CHECK(player->GetCurrentStateId() == game::PlayerHurt);
    player->ChangeState(game::PlayerStand);
    player->BeginQTE(1, 10, 1);
    const uint32_t qte = fixture.FrameHash();
    CHECK(player->GetCurrentStateId() == game::PlayerQTE);
    CHECK(stand == 0xf2a51c5dU);
    CHECK(run == 0x6d6dfde5U);
    CHECK(attack == 0x3240a1c5U);
    CHECK(hurt == 0xd8f2ec35U);
    CHECK(qte == 0x83da4c65U);
}

} // namespace

int main()
{
    try {
        TestEveryStateAndRegistration();
        TestAttackChainsAndDamage();
        TestHurtDeathQteHideAndPush();
        TestInventoryAndPersistence();
        TestPlayerCommands();
        TestEngineInputAndFrames();
        std::cout << "game player tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "game/Enemy.hpp"
#include "game/GameSI.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/Animation.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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

void CheckVec(const soft3d::Vec3& actual, const soft3d::Vec3& expected, int line)
{
    Check(actual.x == expected.x && actual.y == expected.y && actual.z == expected.z, line);
}

#define CHECK_VEC(actual, ...) CheckVec((actual), soft3d::Vec3 __VA_ARGS__, __LINE__)

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
    static Argument Bool(bool value) { return {soft3d::ScriptValueType::Bool, value ? 1 : 0, {}}; }
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

    void Add(int32_t id, int32_t type = 0, int32_t health = 50,
             soft3d::Fixed x = 0, soft3d::Fixed z = 0)
    {
        Execute(180, {Argument::Short(static_cast<int16_t>(id)), Argument::Int(type),
            Argument::Short(static_cast<int16_t>(health)), Argument::Fixed(x), Argument::Fixed(0),
            Argument::Fixed(z), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0x10000)});
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
};

class ProbeEnemy final : public game::Enemy {
public:
    explicit ProbeEnemy(const soft3d::UnitManagerContext& context)
        : Enemy(context.resources, context.renderer, context.audio, context.bsp) {}

    using Enemy::AdvanceWaypoint;
    using Enemy::BeginChase;
    using Enemy::StopMotion;
    using Enemy::UpdateWaitTimer;

    void ForceState(int32_t value) { state = value; }
    void ForceDistance(soft3d::Fixed value) { playerDistance = value; }
    void ForceFinished(bool value) { SetAnimationFinished(value); }
    void ForceStateTime(int16_t value) { stateTime = value; }
    void ForceWaitTimer(int16_t value) { waitTimer = value; }
    void ForceVisible(bool value) { playerVisible = value ? 1 : 0; }
    void ForceCurrentWaypoint(int32_t value) { currentWaypoint = value; }
    void ForceWaypointDirection(int16_t value) { waypointDirection = value; }
    void SnapshotResources()
    {
        std::copy_n(animations.begin(), defaultAnimations.size(), defaultAnimations.begin());
        std::copy_n(sounds.begin(), defaultSounds.size(), defaultSounds.begin());
    }
};

void PrepareProbe(GameFixture& fixture, ProbeEnemy& enemy)
{
    enemy.ConfigureCommonDefaults(game::GetEnemyCommonConfig(0));
    enemy.SetManager(fixture.scene->GetUnitManager());
    enemy.SetPlayer(fixture.scene->GetPlayer());
    enemy.LoadResources(u"r_nvyong_a", nullptr, nullptr);
    for (int index = 0; index < 6; ++index) {
        enemy.SetAnimationResource(index,
            fixture.scene->GetUnitManager()->GetData().GetAnimationName(87 + index));
        enemy.SetSoundResource(index,
            fixture.scene->GetUnitManager()->GetData().GetSoundName(33 + index));
    }
    enemy.SnapshotResources();
    CHECK(enemy.GetObject(0) != nullptr);
}

void TestConstructorDefaults()
{
    static constexpr std::array<game::EnemyCommonConfig, 17> expected{{
        {10, 50, 15, 0x90000, 0x88000, 24, 3, 6, 64, 24, 0x2666, 0x9999, 250, 0xb333, 0xa0000, 0, true},
        {11, 999999999, 30, 0x80000, 0x78000, 24, 11, 14, 64, 32, 0x8000, 0x8000, 800, 0x8000, 0x27100000, 0, false},
        {12, 200, 15, 0x80000, 0x80000, 8, 11, 14, 64, 32, 0x8000, 0x8000, 800, 0x8000, 0x27100000, 0, false},
        {13, 9999, 25, 0x88000, 0x80000, 48, 8, 10, 0, 0, 0x6666, 0x6666, 1000, 0, 0x27100000, 0, true},
        {14, 1, 15, 0x78000, 0x80000, 32, 8, 10, 0, 0, 0, 0, 0, 0, 0x80000, 0, false},
        {15, 80, 15, 0xb0000, 0xb0000, 24, 7, 11, 36, 96, 0x3333, 0x3333, 500, 0x8000, 0xa0000, 0, true},
        {16, 9999, 25, 0x90000, 0x88000, 24, 8, 10, 32, 96, 0x4000, 0x4000, 999, 0, 0xb0000, 0, true},
        {17, 50, 15, 0x78000, 0x70000, 16, 3, 6, 16, 80, 0x3333, 0x9999, 100, 0xb333, 0xa0000, 0, true},
        {18, 240, 0, 0xb0000, 0xa0000, 8, 14, 16, 0, 0, 0x4ccc, 0x4ccc, 1000, 0, 0x27100000, 0, false},
        {19, 60, 20, 0x70000, 0x68000, 24, 7, 9, 32, 90, 0x1999, 0x1999, 500, 0x9999, 0x90000, 0, true},
        {20, 1, 5, 0, 0x80000, 32, 255, 0, 8, 24, 0x3333, 0x3333, 0, 0, 0x80000, 0, false},
        {21, 60, 0, 0, 0, 255, 0, 0, 16, 64, 0x3333, 0x3333, 0, 0xb333, 0, 0, true},
        {22, 150, 30, 0x80000, 0x78000, 32, 7, 9, 64, 32, 0x8000, 0xb333, 800, 0x6666, 0x140000, 8, true},
        {23, 260, 25, 0x98000, 0x98000, 40, 6, 7, 64, 32, 0x5999, 0x5999, 1000, 0x6666, 0x27100000, 30, false},
        {24, 40, 35, 0, 0x1e0000, 40, 0, 0, 0, 0, 0x13333, 0x13333, 1000, 0, 0x3e80000, 0, false},
        {25, 1, 0, 0, 0, 255, 0, 0, 9999, 1, 0x4ccc, 0x4ccc, 0, 0, 0, 10, true},
        {26, 9999, 0, 0, 0x40000, 255, 0, 0, 0, 0, 0x8ccc, 0x8ccc, 0, 0, 0x27100000, 0, true},
    }};
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    for (int32_t type = 0; type < 17; ++type) {
        const auto& config = expected[static_cast<std::size_t>(type)];
        const game::Enemy* enemy = manager->GetPooledEnemy(type, 0);
        CHECK(enemy != nullptr && enemy->GetEnemyType() == type);
        CHECK(enemy->GetUnitType() == config.unitType);
        CHECK(enemy->GetMaxHealth() == config.maxHealth);
        CHECK(enemy->GetAttackPower() == config.attackPower);
        CHECK(enemy->GetHitDistance() == config.hitDistance);
        CHECK(enemy->GetAttackRange() == config.attackRange);
        CHECK(enemy->GetAttackInterval() == config.attackInterval);
        CHECK(enemy->GetAttackStart() == config.attackStart);
        CHECK(enemy->GetAttackEnd() == config.attackEnd);
        CHECK(enemy->GetAlertRecoveryDelay() == config.alertRecoveryDelay);
        CHECK(enemy->GetIdleAlertDelay() == config.idleAlertDelay);
        CHECK(enemy->GetAlertSpeed() == config.alertSpeed);
        CHECK(enemy->GetChaseSpeed() == config.chaseSpeed);
        CHECK(enemy->GetKnockbackChance() == config.knockbackChance);
        CHECK(enemy->GetKnockbackSpeed() == config.knockbackSpeed);
        CHECK(enemy->GetPerceptionRange() == config.perceptionRange);
        CHECK(enemy->GetGunExperienceReward() == config.gunExperienceReward);
        CHECK(enemy->IsPositionalAudio() == config.positionalAudio);
    }
}

void TestCommonAiAndPersistence()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    ProbeEnemy enemy(manager->GetContext());
    PrepareProbe(fixture, enemy);
    enemy.SetUnitId(100);
    enemy.Spawn({});
    enemy.SetPosition({});
    CHECK(enemy.GetState() == game::EnemyIdle && enemy.GetHealth() == 50);
    CHECK(enemy.GetObject(0)->GetAnimation() != nullptr);
    player->SetPosition({0, 0, 0x90000});
    enemy.SetDirection(soft3d::Subtract(player->GetPosition(), enemy.GetPosition()));
    enemy.Update();
    CHECK(enemy.GetState() == game::EnemyChase && enemy.IsPlayerVisible());
    CHECK(enemy.GetPlayerDistance() == 0x90000);
    CHECK(enemy.SelectAttack() == 0);
    enemy.ForceDistance(0x20000);
    CHECK(enemy.SelectAttack() == 1);
    CHECK(enemy.RequestAttack() && enemy.GetState() == game::EnemyAttack);
    enemy.ForceStateTime(enemy.GetAttackStart());
    player->SetCollisionBounds({
        soft3d::Subtract(player->GetPosition(), {0x10000, 0x10000, 0x10000}),
        soft3d::Add(player->GetPosition(), {0x10000, 0x10000, 0x10000})});
    const int32_t playerHealth = player->GetHealth();
    CHECK(enemy.TestAttackHit());
    CHECK(player->GetHealth() == playerHealth - enemy.GetAttackPower());
    CHECK(!enemy.TestAttackHit());
    enemy.ForceFinished(true);
    enemy.UpdateAttack();
    CHECK(enemy.GetState() == game::EnemyIdle);
    CHECK(enemy.GetAttackCooldown() == enemy.GetAttackInterval());

    enemy.Damage(10);
    CHECK(enemy.GetHealth() == 40 && enemy.GetState() == game::EnemyKnockback);
    CHECK(enemy.GetHitCooldown() == 8 && enemy.GetBloodLife() == 9);
    enemy.ForceFinished(true);
    enemy.UpdateKnockback();
    CHECK(enemy.GetState() == game::EnemyIdle);
    enemy.Damage(1000);
    CHECK(enemy.GetState() == game::EnemyDying && enemy.GetDeathCountdown() == 12);
    CHECK(player->GetKillCount() == 1 && player->GetGunExperience() == 0);
    for (int index = 0; index < 12; ++index) {
        enemy.ForceFinished(true);
        enemy.UpdateDeath();
    }
    CHECK(enemy.GetState() == game::EnemyDead && enemy.IsRemoved());
    CHECK(!enemy.IsCollisionEnabled() && enemy.QueryPoint(enemy.GetPosition()) == nullptr);
    enemy.Reset();
    CHECK(enemy.GetState() == game::EnemyRemoved && enemy.GetWaypointCount() == 0);

    enemy.Spawn({});
    enemy.AddWaypoint({}, 10, {0, 0, 0x10000});
    enemy.AddWaypoint({}, 20, {0, 0, 0x10000});
    enemy.AddWaypoint({}, 30, {0, 0, 0x10000});
    CHECK(enemy.GetWaypointCount() == 3 && enemy.GetCurrentWaypointIndex() == 0);
    CHECK(enemy.AdvanceWaypoint() && enemy.GetCurrentWaypointIndex() == 1);
    CHECK(enemy.GetWaitTimer() == 20);
    CHECK(enemy.AdvanceWaypoint() && enemy.GetCurrentWaypointIndex() == 2);
    CHECK(enemy.GetWaitTimer() == 30);
    CHECK(enemy.AdvanceWaypoint() && enemy.GetCurrentWaypointIndex() == 1);
    CHECK(enemy.GetWaypointDirection() == 0);
    enemy.ForceCurrentWaypoint(2);
    enemy.ForceWaypointDirection(1);
    enemy.ForceVisible(true);
    enemy.SetWaypointLoop(true);
    CHECK(enemy.AdvanceWaypoint() && enemy.GetCurrentWaypointIndex() == 0);
    CHECK(enemy.GetWaypointDirection() == 1);
    enemy.ClearWaypoints();
    CHECK(!enemy.HasCurrentWaypoint());

    enemy.ForceState(game::EnemyIdle);
    enemy.ForceWaitTimer(0);
    CHECK(enemy.UpdateWaitTimer() && enemy.GetWaitTimer() == -1);
    enemy.AddWaypoint({}, 1, {});
    enemy.ForceWaitTimer(-1);
    enemy.UpdateAlertDelay();
    CHECK(enemy.GetWaitTimer() >= 24);
    enemy.ForceWaitTimer(0);
    enemy.UpdateAlertDelay();
    CHECK(enemy.GetState() == game::EnemyAlert);
    enemy.SetNavigationEnabled(true);
    enemy.Update();
    CHECK(enemy.IsNavigationEnabled());
    enemy.SetNavigationEnabled(false);
    CHECK(!enemy.IsNavigationEnabled());

    enemy.SetStopped(true);
    CHECK(enemy.IsStopped() && enemy.GetMoveMode() == game::EnemyNoMovement &&
          !enemy.IsPlayerVisible());
    CHECK(enemy.MoveTo({0x100000, 0, 0}));
    CHECK(enemy.GetPosition().x == 0);
    enemy.SetStopped(false);
    CHECK(!enemy.MoveTo({0x100000, 0, 0}));
    CHECK(enemy.GetMoveMode() == game::EnemyMoveToTarget);
    CHECK(enemy.MoveTo({0, 0x40000, 0}) == 0);
    enemy.StopMotion();
    enemy.ForceState(game::EnemyIdle);
    enemy.ForceDistance(0x20000);
    enemy.FollowPlayer();
    CHECK(enemy.GetMoveMode() == game::EnemyFollowPlayer);
    CHECK(enemy.SelectAttack() == 0);

    soft3d::OutputDataStream saved;
    enemy.SaveState(saved);
    ProbeEnemy restored(manager->GetContext());
    PrepareProbe(fixture, restored);
    soft3d::InputDataStream input;
    input.SetData(saved.GetData(), saved.GetSize());
    int32_t serializedType = 0;
    CHECK(input.ReadInt32(serializedType) == soft3d::S3D_OK);
    restored.LoadState(input);
    CHECK(input.IsEnd());
    soft3d::OutputDataStream second;
    restored.SaveState(second);
    CHECK(second.GetSize() == saved.GetSize());
    CHECK(std::equal(saved.GetData(), saved.GetData() + saved.GetSize(), second.GetData()));
    CHECK(restored.GetEnemyType() == enemy.GetEnemyType() && restored.GetHealth() == enemy.GetHealth());
    CHECK(restored.GetState() == enemy.GetState());
    CHECK(saved.GetSize() >= 16);
    const uint8_t* tail = saved.GetData() + saved.GetSize() - 16;
    CHECK(tail[0] == 0 && tail[1] == 0 && tail[2] == 0 && tail[3] == 0);
}

void TestPoolsAndCommands()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(2);
    CHECK(manager->GetPooledEnemy(-1, 0) == nullptr);
    CHECK(manager->GetPooledEnemy(17, 0) == nullptr);
    CHECK(game::IsSingletonEnemyType(1) && !game::IsSingletonEnemyType(0));
    CHECK(manager->GetPooledEnemy(0, 0)->GetMaxHealth() == 50);
    CHECK(manager->GetPooledEnemy(1, 0)->GetMaxHealth() == 999999999);
    CHECK(manager->GetPooledEnemy(0, 0)->GetObject(0) != nullptr);
    CHECK(manager->GetPooledEnemy(0, 0)->GetAnimationResource(0) != nullptr);
    CHECK(manager->GetPooledEnemy(0, 0)->GetSoundResource(1) != nullptr);

    for (int index = 0; index < 8; ++index) fixture.Add(100 + index, 0, 50, index * 0x10000);
    CHECK(manager->GetBucketCount(0) == 8);
    fixture.Add(200);
    CHECK(manager->GetBucketCount(0) == 8);
    fixture.Add(201, -1);
    CHECK(manager->GetBucketCount(0) == 8);
    fixture.Add(202, 0, 0);
    CHECK(manager->GetBucketCount(0) == 8);
    fixture.Add(203, 1, -1, 0x100000);
    fixture.Add(204, 1, -1, 0x200000);
    CHECK(manager->GetBucketCount(0) == 9);

    fixture.Execute(197, {Argument::Short(100)});
    CHECK(manager->FindEnemy(100) == nullptr);
    fixture.Add(205, 0, -1, 0x1000000);
    CHECK(manager->FindEnemy(205) == manager->GetPooledEnemy(0, 0));
    CHECK(manager->FindEnemy(205)->GetHealth() == 50);
    CHECK(fixture.Execute(185, {Argument::Short(100)}, true) == 1);
    CHECK(fixture.Execute(185, {Argument::Short(205)}, true) == 0);

    auto* enemy = manager->FindEnemy(101);
    CHECK(enemy != nullptr);
    fixture.Execute(181, {Argument::Short(101)});
    CHECK(enemy->GetSpawnRecord() != nullptr);
    CHECK(enemy->GetSpawnRecord()->enemyType == 0 && enemy->GetSpawnRecord()->unitId == 101);
    fixture.Execute(182, {Argument::Short(101), Argument::Bool(false)});
    CHECK(enemy->GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::SecondaryStreams);
    fixture.Execute(182, {Argument::Short(101), Argument::Bool(true)});
    CHECK(enemy->GetObject(0)->GetRenderMode() == soft3d::ObjectRenderMode::Normal);
    fixture.Execute(183, {Argument::Short(101), Argument::String("r_nvyong_walk_a"), Argument::Bool(false)});
    CHECK(enemy->GetAnimation() != nullptr && !enemy->GetObject(0)->IsAnimationLooping());
    enemy->GetObject(0)->SetAnimationTime(enemy->GetAnimation()->GetEndTime());
    enemy->PostUpdate();
    CHECK(fixture.Execute(184, {Argument::Short(101)}, true) == 1);
    CHECK(fixture.Execute(184, {Argument::Short(9999)}, true) == 0);
    CHECK(fixture.Execute(186, {Argument::Short(101)}, true) == 50);
    fixture.Execute(187, {Argument::Short(101), Argument::Short(25)});
    CHECK(fixture.Execute(186, {Argument::Short(101)}, true) == 25);
    fixture.Execute(187, {Argument::Short(101), Argument::Short(-1)});
    CHECK(enemy->GetHealth() == 50);
    CHECK(fixture.Execute(188, {Argument::Short(101)}, true) == 15);
    fixture.Execute(189, {Argument::Short(101), Argument::Int(30)});
    CHECK(enemy->GetAttackPower() == 30);
    CHECK(fixture.Execute(188, {Argument::Short(9999)}, true) == 0);

    fixture.Execute(190, {Argument::Short(101), Argument::Fixed(0x10000), Argument::Fixed(0),
                          Argument::Fixed(0)});
    CHECK(enemy->GetObject(0)->GetAxisZ().x == 0x10000);
    fixture.Execute(191, {Argument::Short(101), Argument::Fixed(0x10000), Argument::Fixed(0x20000),
                          Argument::Fixed(0x30000)});
    CHECK_VEC(enemy->GetPosition(), {0x10000, 0x20000, 0x30000});
    CHECK(fixture.Execute(192, {Argument::Short(101), Argument::Fixed(0x10000),
        Argument::Fixed(0x40000), Argument::Fixed(0x30000)}, true) == 1);
    CHECK(enemy->GetPosition().y == 0x40000);
    CHECK(fixture.Execute(192, {Argument::Short(101), Argument::Fixed(0x1000000),
        Argument::Fixed(0), Argument::Fixed(0)}, true) == 0);
    CHECK(fixture.Execute(193, {Argument::Short(101), Argument::Fixed(enemy->GetPosition().x),
        Argument::Fixed(enemy->GetPosition().y), Argument::Fixed(enemy->GetPosition().z)}, true) == 1);
    CHECK(fixture.Execute(193, {Argument::Short(101), Argument::Fixed(enemy->GetPosition().x),
        Argument::Fixed(enemy->GetPosition().y), Argument::Fixed(enemy->GetPosition().z)}, true) == 0);
    enemy->Update();
    const soft3d::ObjectBounds& bounds = enemy->GetCollisionBounds();
    CHECK(fixture.Execute(194, {Argument::Short(101), Argument::Fixed(bounds.minimum.x),
        Argument::Fixed(bounds.minimum.y), Argument::Fixed(bounds.minimum.z),
        Argument::Fixed(bounds.maximum.x), Argument::Fixed(bounds.maximum.y),
        Argument::Fixed(bounds.maximum.z)}, true) == 1);
    CHECK(fixture.Execute(194, {Argument::Short(9999), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)}, true) == 0);

    fixture.Execute(195, {Argument::Short(101)});
    CHECK(enemy->GetMoveMode() == game::EnemyFollowPlayer);
    fixture.Execute(196, {Argument::Short(101)});
    CHECK(enemy->GetState() != game::EnemyAttack);
    fixture.Execute(203, {Argument::Short(101), Argument::Bool(true)});
    fixture.Execute(203, {Argument::Short(101), Argument::Bool(false)});
    CHECK(enemy->UpdatePerception());
    fixture.Execute(196, {Argument::Short(101)});
    CHECK(enemy->GetState() == game::EnemyAttack);
    fixture.Execute(200, {Argument::Short(101), Argument::Bool(true)});
    CHECK(enemy->IsNavigationEnabled());
    fixture.Execute(200, {Argument::Short(101), Argument::Bool(false)});
    CHECK(!enemy->IsNavigationEnabled());
    fixture.Execute(201, {Argument::Short(0), Argument::Bool(true)});
    CHECK(enemy->IsNavigationEnabled());
    fixture.Execute(202, {Argument::Bool(false)});
    CHECK(!enemy->IsNavigationEnabled());
    fixture.Execute(203, {Argument::Short(101), Argument::Bool(true)});
    CHECK(enemy->IsStopped() && enemy->GetMoveMode() == game::EnemyNoMovement);
    fixture.Execute(203, {Argument::Short(101), Argument::Bool(false)});

    const soft3d::ObjectBounds stayBounds{{-0x10000, -0x10000, -0x10000},
                                          {0x10000, 0x10000, 0x10000}};
    fixture.Execute(204, {Argument::Short(70), Argument::Fixed(stayBounds.minimum.x),
        Argument::Fixed(stayBounds.minimum.y), Argument::Fixed(stayBounds.minimum.z),
        Argument::Fixed(stayBounds.maximum.x), Argument::Fixed(stayBounds.maximum.y),
        Argument::Fixed(stayBounds.maximum.z)});
    game::StayBox* box = manager->FindStayBox(70);
    CHECK(box != nullptr);
    fixture.Execute(207, {Argument::Short(101), Argument::Short(70)});
    CHECK(enemy->GetStayBox() == box && box->GetSpawnRecords().size() == 1);
    fixture.Execute(208, {Argument::Short(101), Argument::Fixed(7), Argument::Fixed(8), Argument::Fixed(9)});
    CHECK_VEC(enemy->GetStayBoxReturnPosition(), {7, 8, 9});
    fixture.Execute(209, {Argument::Short(101), Argument::Fixed(1), Argument::Fixed(2),
        Argument::Fixed(3), Argument::Short(4), Argument::Fixed(5), Argument::Fixed(6),
        Argument::Fixed(7)});
    CHECK(enemy->GetWaypointCount() == 1 && enemy->GetCurrentWaypointIndex() == 0);
    fixture.Execute(210, {Argument::Short(101), Argument::Bool(true)});
    CHECK(enemy->GetWaypointLoop() == 1);
    fixture.Execute(211, {Argument::Short(101)});
    CHECK(enemy->GetWaypointCount() == 0);
    fixture.Execute(212, {Argument::Short(101), Argument::Fixed(0x123456)});
    CHECK(enemy->GetPerceptionRange() == 0x123456);
    CHECK(fixture.Execute(213, {Argument::Short(101)}, true) == 0);
    CHECK(fixture.Execute(213, {Argument::Short(9999)}, true) == 0);
    fixture.Execute(214, {Argument::Short(101), Argument::Short(1),
                          Argument::String("r_nvyong_run_a")});
    CHECK(enemy->GetAnimationResource(1) != nullptr);
    fixture.Execute(215, {Argument::Short(101), Argument::Short(1), Argument::String("c_biddy_run")});
    CHECK(enemy->GetSoundResource(1) != nullptr);
    fixture.Execute(216, {Argument::Short(101), Argument::Fixed(0x5555)});
    CHECK(enemy->GetAlertSpeed() == 0x5555);

    fixture.Execute(217, {Argument::Short(88), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(1), Argument::Fixed(1), Argument::Fixed(1)});
    CHECK(manager->FindUnit(1, 88) != nullptr);
    fixture.Execute(218, {Argument::Short(88)});
    CHECK(manager->FindUnit(1, 88) == nullptr);

    fixture.Execute(198, {Argument::Short(1)});
    CHECK(manager->FindEnemy(203) == nullptr);
    fixture.Execute(199);
    CHECK(manager->GetBucketCount(0) == 0);
    CHECK(manager->GetPooledEnemy(0, 0)->GetState() == game::EnemyRemoved);

    fixture.Add(300, 0, 50, 0, 0x20000);
    enemy = manager->FindEnemy(300);
    CHECK(enemy != nullptr);
    enemy->SetHealth(0);
    CHECK(enemy->GetState() == game::EnemyDying);
    CHECK(fixture.scene->GetPlayer()->GetKillCount() == 1);
    fixture.Execute(181, {Argument::Short(9999)});
    fixture.Execute(190, {Argument::Short(9999), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)});
    fixture.Execute(195, {Argument::Short(9999)});
    fixture.Execute(196, {Argument::Short(9999)});
    fixture.Execute(197, {Argument::Short(9999)});
    fixture.Execute(200, {Argument::Short(9999), Argument::Bool(true)});
    fixture.Execute(203, {Argument::Short(9999), Argument::Bool(true)});
    fixture.Execute(211, {Argument::Short(9999)});
}

void TestRealEnemyDataAndStayBox()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(2);
    CHECK(manager->LoadDayContent(2, fixture.engine) == soft3d::S3D_OK);
    const auto& groups = manager->GetData().GetEnemyInfoGroups();
    CHECK(groups.size() == 4);
    std::size_t total = 0;
    for (const game::EnemyInfoGroup& group : groups) {
        manager->SetMapId(group.mapId);
        manager->Reset();
        manager->SpawnMapEnemies();
        std::size_t expected = 0;
        for (const auto& record : group.records) if (record.active != 0) ++expected;
        CHECK(manager->GetBucketCount(0) == expected);
        total += expected;
        int32_t allocatedId = 9999;
        for (const auto& record : group.records) {
            if (record.active == 0) continue;
            if (record.unitId != -1) allocatedId = record.unitId;
            game::Enemy* enemy = manager->FindEnemy(allocatedId);
            CHECK(enemy != nullptr && enemy->GetSpawnRecord() == &record);
            CHECK(enemy->GetWaypointCount() == record.waypoints.size());
            --allocatedId;
        }
    }
    CHECK(total == 4);

    manager->SetMapId(groups[0].mapId);
    manager->Reset();
    manager->SpawnMapEnemies();
    const int32_t firstId = groups[0].records[0].unitId == -1 ? 9999 : groups[0].records[0].unitId;
    game::Enemy* enemy = manager->FindEnemy(firstId);
    CHECK(enemy != nullptr);
    enemy->SetHealth(23);
    manager->UpdateEnemySpawnRecords();
    CHECK(groups[0].records[0].active == 23);
    CHECK(groups[0].records[0].unitId == enemy->GetUnitId());

    game::StayBox* box = manager->AddStayBox(99, {{-1, -1, -1}, {1, 1, 1}});
    CHECK(box != nullptr);
    manager->AttachStayBoxEnemy(*enemy, *box);
    box->AddSpawnRecord(*enemy);
    enemy->SetStayBoxReturnPosition({11, 12, 13});
    box->AddSpawnRecord({{11, 12, 13}, static_cast<uint8_t>(enemy->GetUnitId()), 0, 23, 2});
    manager->RemoveEnemy(enemy);
    CHECK(manager->FindEnemy(enemy->GetUnitId()) == nullptr);
    CHECK(box->GetSpawnRecords().size() == 2);
}

void LoadRealMap(GameFixture& fixture)
{
    fixture.engine.StartNewGame();
    fixture.RunFrames(57);
    fixture.engine.GetScriptEngine().Unload();
    fixture.engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
    CHECK(fixture.scene->LoadMap(2008) == 0);
    for (int frame = 0; frame < 80 &&
         (fixture.scene->GetMapProgress() != -1 || fixture.scene->GetTransitionStage() != 0);
         ++frame) {
        fixture.RunFrames(1);
    }
    CHECK(fixture.scene->GetMapProgress() == -1 && fixture.scene->GetTransitionStage() == 0);
    fixture.engine.SetPreUpdateEnabled(true);
    fixture.engine.SetInGame(true);
}

void TestEngineFrameCombat()
{
    GameFixture fixture;
    LoadRealMap(fixture);
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(2);
    auto* player = fixture.scene->GetPlayer();
    soft3d::Vec3 position = soft3d::Add(player->GetPosition(), {0, 0, 0x20000});
    soft3d::Vec3 spawnPosition{0x1000000, 0, 0};
    CHECK(manager->AddEnemy(500, 0, 50, spawnPosition, {0, 0, 0x10000}, false));
    auto* enemy = manager->FindEnemy(500);
    CHECK(enemy != nullptr);
    enemy->SetPosition(position);
    enemy->SetDirection(soft3d::Subtract(player->GetPosition(), position));
    enemy->BeginAttack();
    const int32_t health = player->GetHealth();
    fixture.RunFrames(4);
    CHECK(player->GetHealth() < health);
    CHECK(enemy->GetStateTime() >= 3);
    enemy->Damage(10);
    CHECK(enemy->GetBloodLife() > 0);
    fixture.RunFrames(8);
    CHECK(enemy->GetBloodLife() > 0);
    const auto* video = dynamic_cast<const soft3d::platform::DummyVideoBackend*>(
        fixture.engine.GetVideo().GetBackend());
    CHECK(video != nullptr && !video->GetPresentedPixels().empty());
    fixture.RunFrames(2);
    CHECK(fixture.scene->IsInitialized());
}

void Run(const char* name, void (*test)())
{
    test();
    std::cout << "PASS " << name << '\n';
}

} // namespace

int main()
{
    try {
        Run("constructor defaults", TestConstructorDefaults);
        Run("common AI and persistence", TestCommonAiAndPersistence);
        Run("pools and commands", TestPoolsAndCommands);
        Run("real enemy data and StayBox", TestRealEnemyDataAndStayBox);
        Run("engine frame combat", TestEngineFrameCombat);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

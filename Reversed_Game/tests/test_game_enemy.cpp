#include "game/Enemy.hpp"
#include "game/GameSI.hpp"
#include "game/MaidBully.hpp"
#include "game/Scene.hpp"
#include "game/SpecialEnemies.hpp"
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

template <typename T>
class VariantProbe : public T {
public:
    VariantProbe(const soft3d::UnitManagerContext& context, const game::GameData& data)
        : T(context, data) {}
    ~VariantProbe() { this->StopSound(); }

    void ForceState(int32_t value) { this->state = value; }
    void ForceDistance(soft3d::Fixed value) { this->playerDistance = value; }
    void ForceFinished(bool value) { this->SetAnimationFinished(value); }
    void ForceStateTime(int16_t value) { this->stateTime = value; }
    void ForceVisible(bool value) { this->playerVisible = value ? 1 : 0; }
    void ForceHitCooldown(int16_t value) { this->hitCooldown = value; }
    void ForceGroundCollision(bool value) { this->groundTrace->startSolid = value; }
};

class FatMaidProbe final : public VariantProbe<game::FatMaid> {
public:
    using VariantProbe<game::FatMaid>::VariantProbe;
    using game::FatMaid::CanThrowProjectile;
    using game::FatMaid::UpdateProjectile;
    using game::FatMaid::UpdateThrow;
    void ForceThrowCooldown(int32_t value) { throwCooldown = value; }
};

class BullyDay3Probe final : public VariantProbe<game::BullyDay3> {
public:
    using VariantProbe<game::BullyDay3>::VariantProbe;
    using game::BullyDay3::BeginQte;
    using game::BullyDay3::UpdateCharge;
    using game::BullyDay3::UpdateQte;
    void ForceAttackKind(int32_t value) { attackKind = value; }
};

class BullyDay5Probe final : public VariantProbe<game::BullyDay5> {
public:
    using VariantProbe<game::BullyDay5>::VariantProbe;
    using game::BullyDay5::BeginSlamAttack;
    using game::BullyDay5::BeginThrowAttack;
    using game::BullyDay5::UpdateChargeAttack;
    using game::BullyDay5::UpdatePickup;
    using game::BullyDay5::UpdateSlam;
    using game::BullyDay5::UpdateThrow;
    using game::BullyDay5::UpdateThrownSaw;
    void ForceThrowPhase(int32_t value) { throwPhase = value; }
};

class WormProbe final : public VariantProbe<game::Worm> {
public:
    using VariantProbe<game::Worm>::VariantProbe;
    void ForcePlayerCollision()
    {
        unitQueryResult.count = 1;
        unitQueryResult.units[0] = player;
    }
    void ForceVerticalVelocity(soft3d::Fixed value) { velocity.y = value; }
};

template <typename T>
void PrepareVariant(GameFixture& fixture, T& enemy, const char16_t* model = nullptr)
{
    enemy.SetManager(fixture.scene->GetUnitManager());
    enemy.SetPlayer(fixture.scene->GetPlayer());
    if (model != nullptr) {
        enemy.LoadResources(model, nullptr, nullptr);
    }
    enemy.Spawn({});
    CHECK(enemy.GetObject(0) != nullptr);
}

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
        {19, 60, 20, 0x70000, 0x68000, 24, 7, 9, 32, 90, 0x1999, 0x1999, 500, 0x9999, 0x90000, 2, true},
        {20, 1, 5, 0, 0x80000, 32, 255, 0, 8, 24, 0x3333, 0x3333, 0, 0, 0x80000, 1, false},
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

void LoadRealMap(GameFixture& fixture, int mapId = 2008, int day = 1)
{
    if (day == 1) {
        fixture.engine.StartNewGame();
    } else {
        fixture.engine.SetPreUpdateEnabled(true);
        fixture.engine.SetInGame(true);
        CHECK(fixture.scene->LoadDay(day) == soft3d::S3D_OK);
    }
    fixture.RunFrames(57);
    fixture.engine.GetScriptEngine().Unload();
    fixture.engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
    CHECK(fixture.scene->LoadMap(mapId) == 0);
    for (int frame = 0; frame < 500 &&
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

void TestStage10TypesAndResources()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(2);
    for (std::size_t slot = 0; slot < 8; ++slot) {
        CHECK(dynamic_cast<game::Maid*>(manager->GetPooledEnemy(0, slot)) != nullptr);
    }
    CHECK(dynamic_cast<game::Bully*>(manager->GetPooledEnemy(1, 0)) != nullptr);
    CHECK(manager->GetPooledEnemy(1, 1) == nullptr);

    manager->PrepareDayEnemies(4);
    for (std::size_t slot = 0; slot < 8; ++slot) {
        auto* fat = dynamic_cast<game::FatMaid*>(manager->GetPooledEnemy(6, slot));
        auto* maid2 = dynamic_cast<game::Maid2*>(manager->GetPooledEnemy(7, slot));
        CHECK(fat != nullptr && maid2 != nullptr);
        CHECK(fat->GetObject(0) != nullptr && fat->GetProjectileObject() != nullptr);
        CHECK(fat->GetThrowAnimation() != nullptr && fat->GetProjectileAnimation() != nullptr);
        CHECK(fat->GetProjectileSound() != nullptr);
        CHECK(fat->GetAnimationResource(4) == nullptr && fat->GetAnimationResource(5) == nullptr);
        CHECK(maid2->GetObject(0) != nullptr && maid2->GetAnimationResource(5) != nullptr);
        CHECK(maid2->GetSoundResource(5) != nullptr);
    }

    manager->PrepareDayEnemies(5);
    for (std::size_t slot = 0; slot < 8; ++slot) {
        auto* maid3 = dynamic_cast<game::Maid3*>(manager->GetPooledEnemy(9, slot));
        CHECK(maid3 != nullptr && maid3->GetObject(0) != nullptr);
        CHECK(maid3->GetAnimationResource(5) != nullptr && maid3->GetSoundResource(5) != nullptr);
        CHECK(maid3->GetGunExperienceReward() == 2);
    }

    fixture.Add(800, 2, -1, 0x1000000);
    auto* day3 = dynamic_cast<game::BullyDay3*>(manager->FindEnemy(800));
    CHECK(day3 != nullptr && day3->GetQteObject() != nullptr);
    for (std::size_t index = 0; index < 11; ++index) {
        CHECK(day3->GetExtraAnimation(index) != nullptr);
    }
    CHECK(day3->GetExtraSound(0) == nullptr);
    for (std::size_t index = 1; index < 9; ++index) {
        CHECK(day3->GetExtraSound(index) != nullptr);
    }

    fixture.Add(801, 13, -1, 0x2000000);
    auto* day5 = dynamic_cast<game::BullyDay5*>(manager->FindEnemy(801));
    CHECK(day5 != nullptr && day5->GetSaw() != nullptr);
    CHECK(day5->GetSaw()->GetObject(0) != nullptr &&
          day5->GetSaw()->GetShadowObject() != nullptr);
    for (std::size_t index = 0; index < 4; ++index) {
        CHECK(day5->GetExtraAnimation(index) != nullptr);
    }
    for (std::size_t index = 0; index < 9; ++index) {
        CHECK(day5->GetExtraSound(index) != nullptr);
    }
    CHECK(day5->GetSaw()->GetAnimationResource(3) == nullptr);
    for (std::size_t index = 0; index < 11; ++index) {
        if (index != 3) CHECK(day5->GetSaw()->GetAnimationResource(index) != nullptr);
    }
    CHECK(day5->GetGunExperienceReward() == 30);
}

void TestMaid3AndBullyAttacks()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    VariantProbe<game::Maid3> maid(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, maid, u"r_bingnvyong_a");
    CHECK(maid.GetChaseSpeed() >= 0x1999 && maid.GetChaseSpeed() < 0x3333);
    maid.SetHealth(0);
    CHECK(maid.GetState() == game::EnemyDying && player->GetGunExperience() == 2);
    for (int index = 0; index < 12; ++index) {
        maid.GetObject(0)->SetAnimationTime(maid.GetAnimation()->GetEndTime());
        maid.PostUpdate();
        maid.UpdateDeath();
    }
    CHECK(maid.GetState() == game::EnemyRemoved);

    VariantProbe<game::Bully> bully(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, bully, u"r_bully_a");
    bully.SetDirection({0, 0, soft3d::fixedOne});
    player->SetPosition({0, 0, 0x40000});
    player->SetCollisionBounds({{-0x100000, -0x100000, -0x100000},
                                {0x100000, 0x100000, 0x100000}});
    bully.BeginAttack();
    const int32_t health = player->GetHealth();
    bully.ForceStateTime(10);
    CHECK(!bully.TestAttackHit());
    bully.ForceStateTime(11);
    CHECK(bully.TestAttackHit() && player->GetHealth() == health - 30);
    bully.ForceStateTime(22);
    CHECK(!bully.TestAttackHit());
    bully.ForceStateTime(23);
    CHECK(bully.TestAttackHit() && player->GetHealth() == health - 60);
    CHECK(bully.GetSecondAttackStart() == 23 && bully.GetSecondAttackEnd() == 25);
}

void TestFatMaidProjectile()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    FatMaidProbe enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy, u"r_pangnvyong_a");
    player->SetPosition({0, 0, 0x100000});
    enemy.SetDirection(soft3d::Subtract(player->GetPosition(), enemy.GetPosition()));
    enemy.ForceState(game::EnemyChase);
    enemy.ForceDistance(0x100000);
    enemy.SetBsp(nullptr);
    CHECK(enemy.CanThrowProjectile());
    for (uint32_t seed = 0; seed < 1000 && enemy.GetState() != 6; ++seed) {
        enemy.ForceState(game::EnemyChase);
        enemy.ForceThrowCooldown(0);
        manager->SetRandomSeed(seed);
        enemy.UpdateChase();
    }
    CHECK(enemy.GetState() == 6);
    CHECK(enemy.GetThrowAnimation() == enemy.GetAnimation());
    for (int index = 0; index < 22; ++index) enemy.UpdateThrow();
    CHECK(enemy.GetProjectileState() == 1);
    CHECK(enemy.GetProjectileObject()->GetAnimation() == enemy.GetProjectileAnimation());
    CHECK(enemy.GetProjectileVelocity().z > 0);

    const int32_t health = enemy.GetHealth();
    enemy.Damage(1000);
    CHECK(enemy.GetHealth() == health && !enemy.ReceivePlayerAttack());
    const soft3d::Vec3 oldPosition = enemy.GetProjectileObject()->GetPosition();
    enemy.UpdateProjectile();
    CHECK(enemy.GetProjectileObject()->GetPosition().z != oldPosition.z);
    CHECK(enemy.GetProjectileVelocity().y == -0x1999);

    enemy.GetProjectileObject()->SetPosition(
        soft3d::Subtract(player->GetPosition(), enemy.GetProjectileVelocity()));
    player->SetCollisionBounds({soft3d::Subtract(player->GetPosition(), {0x10000, 0x10000, 0x10000}),
                                soft3d::Add(player->GetPosition(), {0x10000, 0x10000, 0x10000})});
    enemy.ForceThrowCooldown(0);
    const int32_t playerHealth = player->GetHealth();
    enemy.UpdateProjectile();
    CHECK(enemy.GetProjectileState() == 2 && player->GetHealth() == playerHealth - 15);
    CHECK(enemy.GetAttackPower() == 25);
    enemy.GetProjectileObject()->SetAnimationTime(
        enemy.GetProjectileObject()->GetAnimation()->GetEndTime());
    enemy.PostUpdate();
    enemy.UpdateProjectile();
    CHECK(enemy.GetProjectileState() == 0);
}

void CompleteVariantQte(GameFixture& fixture, bool success)
{
    auto* player = fixture.scene->GetPlayer();
    for (int frame = 0; frame < 128 && player->GetQteResult() == -1; ++frame) {
        const int32_t action = player->GetQteKey();
        if (success && action >= 1 && action <= 4) {
            const auto key = static_cast<soft3d::Key>(
                fixture.engine.GetControlMap().GetPrimaryBinding(action));
            fixture.engine.GetInput().KeyDown(key);
            fixture.engine.GetInput().KeyUp(key);
        }
        player->Update();
    }
    CHECK(player->GetQteResult() == (success ? 1 : 0));
}

void TestBullyDay3Encounter()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    BullyDay3Probe enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy);
    player->SetCollisionBounds({{-0x100000, -0x100000, -0x100000},
                                {0x100000, 0x100000, 0x100000}});

    for (int desired = 0; desired < 2; ++desired) {
        manager->SetRandomSeed(0);
        for (int attempt = 0; attempt < 1000; ++attempt) {
            enemy.BeginAttack();
            if (enemy.GetAttackKind() == desired) break;
        }
        CHECK(enemy.GetAttackKind() == desired);
        CHECK(enemy.GetAttackPower() == (desired == 0 ? 20 : 25));
        enemy.ForceStateTime(desired == 0 ? 11 : 10);
        CHECK(enemy.TestAttackHit());
        if (desired == 0) {
            enemy.ForceStateTime(23);
            CHECK(enemy.TestAttackHit());
        }
    }

    enemy.ForceState(6);
    enemy.ForceStateTime(12);
    enemy.ForceFinished(true);
    enemy.UpdateCharge();
    CHECK(enemy.GetChargePhase() == 1);
    CHECK(enemy.GetAnimation() == enemy.GetExtraAnimation(2));
    enemy.ForceFinished(false);
    enemy.ForceGroundCollision(true);
    enemy.UpdateCharge();
    CHECK(enemy.GetChargePhase() == 2);
    enemy.ForceFinished(true);
    enemy.UpdateCharge();
    CHECK(enemy.GetChargePhase() == 3 && enemy.GetStateTime() == 40);
    enemy.ForceStateTime(1);
    enemy.ForceFinished(false);
    enemy.UpdateCharge();
    CHECK(enemy.GetState() == game::EnemyIdle && enemy.GetStateTime() == 12);

    enemy.Spawn({});
    player->PrepareWeapon(manager->GetData(), 1);
    player->SetWeapon(1);
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    enemy.SetPosition(player->GetPosition());
    enemy.SetCollisionBounds({{INT32_MIN, INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX, INT32_MAX}});
    CHECK(enemy.ReceivePlayerAttack());
    while (enemy.GetHealth() > 100) {
        enemy.ForceHitCooldown(0);
        CHECK(enemy.ReceivePlayerAttack());
    }
    CHECK(enemy.GetHealth() == 100 && enemy.GetState() == 7 && enemy.GetQtePhase() == 0);
    CHECK(player->AreAttacksDisabled());
    enemy.ForceFinished(true);
    enemy.UpdateQte();
    CHECK(enemy.GetQtePhase() == 1 && fixture.engine.GetControlMap().IsEnabled());
    enemy.SetCollisionBounds({soft3d::Subtract(player->GetPosition(), {0x10000, 0x10000, 0x10000}),
                              soft3d::Add(player->GetPosition(), {0x10000, 0x10000, 0x10000})});
    fixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Accept));
    fixture.engine.GetInput().KeyDown(soft3d::Key::Accept);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Accept);
    enemy.UpdateQte();
    CHECK(enemy.GetQtePhase() == 2);
    for (int index = 0; index < 8; ++index) enemy.UpdateQte();
    CHECK(enemy.GetQtePhase() == 3);
    enemy.ForceFinished(true);
    enemy.UpdateQte();
    CHECK(enemy.GetQtePhase() == 4 && player->GetCurrentStateId() == game::PlayerQTE);
    CompleteVariantQte(fixture, true);
    enemy.UpdateQte();
    CHECK(enemy.GetQteSuccessCount() == 1 && enemy.GetQtePhase() == 4);
    CompleteVariantQte(fixture, true);
    enemy.UpdateQte();
    CHECK(enemy.GetQteSuccessCount() == 2 && enemy.GetQtePhase() == 6);
    enemy.ForceFinished(true);
    enemy.UpdateQte();
    for (int index = 0; index < 8; ++index) enemy.UpdateQte();
    CHECK(enemy.GetEncounterMode() == 1 && enemy.GetState() == game::EnemyIdle);

    enemy.Spawn({});
    fixture.scene->SetEventFlag(1002);
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    enemy.SetPosition(player->GetPosition());
    enemy.SetCollisionBounds({{INT32_MIN, INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX, INT32_MAX}});
    enemy.SetHealth(20);
    enemy.ForceHitCooldown(0);
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetQtePhase() == 6 && enemy.GetEncounterMode() == 1);
    enemy.ForceFinished(true);
    enemy.UpdateQte();
    for (int index = 0; index < 8; ++index) enemy.UpdateQte();
    CHECK(enemy.GetState() == game::EnemyDead && player->GetKillCount() == 1);

    GameFixture failureFixture;
    auto* failureManager = failureFixture.scene->GetUnitManager();
    BullyDay3Probe failure(failureManager->GetContext(), failureManager->GetData());
    PrepareVariant(failureFixture, failure);
    failure.BeginQte(false);
    failure.ForceFinished(true);
    failure.UpdateQte();
    failure.SetCollisionBounds({{-0x10000, -0x10000, -0x10000}, {0x10000, 0x10000, 0x10000}});
    failureFixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Accept));
    failureFixture.engine.GetInput().KeyDown(soft3d::Key::Accept);
    failureFixture.engine.GetInput().KeyUp(soft3d::Key::Accept);
    failure.UpdateQte();
    for (int index = 0; index < 8; ++index) failure.UpdateQte();
    failure.ForceFinished(true);
    failure.UpdateQte();
    CompleteVariantQte(failureFixture, false);
    failure.UpdateQte();
    CHECK(failure.GetQtePhase() == 5);
    failure.ForceFinished(true);
    failure.UpdateQte();
    for (int index = 0; index < 8; ++index) failure.UpdateQte();
    CHECK(failure.GetState() == game::EnemyIdle && failure.GetHealth() == 150);
    CHECK(failure.GetQteOutcome() == 0);
}

void TestBullyDay5Encounter()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    BullyDay5Probe enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy);
    enemy.SetBsp(nullptr);
    CHECK(enemy.GetSawState() == 2 && enemy.GetSlamCooldown() == 220);
    player->SetPosition({0, 0, 0x40000});
    enemy.BeginThrowAttack();
    enemy.UpdateThrow();
    CHECK(enemy.GetState() == 6 && enemy.GetThrowPhase() == 1 && enemy.GetSawState() == 0);
    soft3d::Object* sawObject = enemy.GetSaw()->GetObject(0);
    sawObject->SetAnimationTime(sawObject->GetAnimation()->GetEndTime() -
                                sawObject->GetAnimationSpeed());
    enemy.GetSaw()->PostUpdate();
    enemy.UpdateThrow();
    CHECK(enemy.GetState() == 7 && enemy.GetThrowPhase() == 2 && enemy.GetThrowTimer() == 9);
    CHECK(enemy.GetThrowVelocity().z != 0);
    for (int index = 0; index < 10; ++index) enemy.UpdateThrownSaw();
    CHECK(enemy.GetChargePhase() == 1);
    enemy.GetSaw()->SetPosition({0, 0, 0x100000});
    player->SetPosition({0, 0, 0x4000000});
    player->SetCollisionBounds({soft3d::Subtract(player->GetPosition(), {0x10000, 0x10000, 0x10000}),
                                soft3d::Add(player->GetPosition(), {0x10000, 0x10000, 0x10000})});
    enemy.UpdateChargeAttack();
    CHECK(enemy.GetChargePhase() == 2);
    enemy.ForceGroundCollision(true);
    enemy.UpdateChargeAttack();
    CHECK(enemy.GetState() == 8 && enemy.GetChargePhase() == 0);
    enemy.UpdatePickup();
    CHECK(enemy.GetSawState() == 1 && enemy.GetAnimation() == enemy.GetExtraAnimation(3));
    enemy.ForceFinished(true);
    enemy.UpdatePickup();
    CHECK(enemy.GetSawState() == 2 && enemy.GetState() == game::EnemyIdle);

    enemy.BeginSlamAttack();
    enemy.UpdateSlam();
    CHECK(enemy.GetSlamPhase() == 1 && enemy.GetAnimation() == enemy.GetExtraAnimation(0));
    enemy.GetObject(0)->SetAnimationTime(0xf3c);
    enemy.UpdateSlam();
    CHECK(player->IsHurt4Requested());
    enemy.ForceFinished(true);
    enemy.UpdateSlam();
    CHECK(enemy.GetState() == game::EnemyIdle && enemy.GetSlamPhase() == 0);

    enemy.Spawn({});
    player->PrepareWeapon(manager->GetData(), 1);
    player->SetWeapon(1);
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    enemy.SetPosition(player->GetPosition());
    enemy.SetCollisionBounds({{INT32_MIN, INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX, INT32_MAX}});
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetHealth() == 240 && enemy.GetState() == game::EnemyHurt);
    CHECK(enemy.IsKnockbackPending());
    enemy.SetHealth(20);
    enemy.ForceHitCooldown(0);
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetState() == game::EnemyDying && enemy.GetThrowPhase() == 3);
    CHECK(enemy.GetSaw()->GetAnimation() == enemy.GetSaw()->GetAnimationResource(9));
    CHECK(player->GetGunExperience() == 30 && player->GetKillCount() == 1);

    GameFixture segmentFixture;
    auto* segmentManager = segmentFixture.scene->GetUnitManager();
    auto* segmentPlayer = segmentFixture.scene->GetPlayer();
    BullyDay5Probe segment(segmentManager->GetContext(), segmentManager->GetData());
    PrepareVariant(segmentFixture, segment);
    segment.SetBsp(nullptr);
    segment.ForceThrowPhase(2);
    segment.GetSaw()->SetCollisionBounds({{-1, -1, -1}, {1, 1, 1}});
    segmentPlayer->PrepareWeapon(segmentManager->GetData(), 3);
    segmentPlayer->SetWeapon(3);
    segmentPlayer->ChangeState(game::PlayerAttackC2);
    CHECK(segmentPlayer->GetAttackPhase() == 2);
    segment.IsSegmentInside({}, {1, 0, 0});
    CHECK(segment.GetImpactSoundPending() == 1);
    segment.UpdateThrownSaw();
    CHECK(segment.GetImpactSoundPending() == 0 && segment.GetThrowPhase() == 3);
}

void TestStage10EngineFrames()
{
    GameFixture fixture;
    LoadRealMap(fixture);
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(2);
    manager->PrepareDayEnemies(4);
    manager->PrepareDayEnemies(5);
    const soft3d::Vec3 origin = fixture.scene->GetPlayer()->GetPosition();
    const int types[] = {0, 1, 2, 6, 7, 9, 13};
    for (int index = 0; index < 7; ++index) {
        fixture.Add(900 + index, types[index], -1,
                    origin.x + (index + 1) * 0x40000, origin.z + 0x40000);
        CHECK(manager->FindEnemy(900 + index) != nullptr);
    }
    CHECK(manager->GetBucketCount(0) == 7);
    fixture.RunFrames(30);
    for (int index = 0; index < 7; ++index) {
        CHECK(manager->FindEnemy(900 + index) != nullptr);
        fixture.Execute(196, {Argument::Short(900 + index)});
    }
    fixture.RunFrames(10);
    const auto* video = dynamic_cast<const soft3d::platform::DummyVideoBackend*>(
        fixture.engine.GetVideo().GetBackend());
    CHECK(video != nullptr && !video->GetPresentedPixels().empty());
    CHECK(fixture.scene->IsInitialized());
}

void TestStage11TypesAndResources()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(3);
    auto* ghost = dynamic_cast<game::Ghost*>(manager->GetPooledEnemy(3, 0));
    CHECK(ghost != nullptr && manager->GetPooledEnemy(3, 1) == nullptr);
    CHECK(ghost->GetObject(0) == nullptr && ghost->GetProximitySound() != nullptr);
    CHECK(ghost->GetProximitySound() != ghost->GetSoundResource(5));
    for (std::size_t slot = 0; slot < 8; ++slot) {
        auto* lamper = dynamic_cast<game::Lamper*>(manager->GetPooledEnemy(4, slot));
        auto* weed = dynamic_cast<game::Weed*>(manager->GetPooledEnemy(5, slot));
        CHECK(lamper != nullptr && weed != nullptr);
        CHECK(lamper->GetObject(0) != nullptr && !lamper->IsShadowEnabled());
        CHECK(lamper->GetAnimationResource(5) != nullptr && lamper->GetSoundResource(5) != nullptr);
        CHECK(weed->GetObject(0) != nullptr && weed->GetAnimationResource(5) != nullptr);
        CHECK(weed->GetSoundResource(1) != nullptr && weed->GetSoundResource(4) != nullptr &&
              weed->GetSoundResource(5) != nullptr);
    }

    manager->PrepareDayEnemies(5);
    for (std::size_t slot = 0; slot < 8; ++slot) {
        auto* worm = dynamic_cast<game::Worm*>(manager->GetPooledEnemy(10, slot));
        CHECK(worm != nullptr && worm->GetObject(0) != nullptr);
        CHECK(worm->GetAnimationResource(0) != nullptr && worm->GetAnimationResource(3) != nullptr &&
              worm->GetAnimationResource(5) != nullptr);
        CHECK(worm->GetAnimationResource(4) == nullptr && worm->GetGunExperienceReward() == 1);
    }
}

void TestWormColorsAndAttack()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    struct ColorCase {
        uint32_t seed;
        uint32_t speedRandom;
        uint32_t scaleRandom;
        int32_t variant;
    };
    static constexpr ColorCase cases[] = {
        {1646, 5413, 4876, 0}, {10409, 1262, 17929, 0},
        {10088, 213, 8331, 1}, {31689, 5217, 23377, 1},
        {31368, 4169, 13779, 2}, {5795, 18962, 2766, 2},
    };
    WormProbe enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy, u"r_bug_a");
    for (const ColorCase& value : cases) {
        manager->SetRandomSeed(value.seed);
        enemy.Spawn({});
        CHECK(enemy.GetColorVariant() == value.variant);
        CHECK(enemy.GetChaseSpeed() == 0x3333 + static_cast<int32_t>(value.speedRandom % 0x3333));
        const soft3d::Fixed scale = 0x10000 + static_cast<int32_t>(value.scaleRandom % 80000);
        CHECK(enemy.GetObject(0)->GetAxisX().x == scale);
        CHECK(enemy.GetObject(0)->GetAxisY().y == soft3d::fixedOne);
        CHECK(enemy.GetObject(0)->GetAxisZ().z == scale);
    }

    auto* player = fixture.scene->GetPlayer();
    enemy.SetBsp(nullptr);
    player->SetPosition({0, 0, 0x20000});
    enemy.ForceVisible(true);
    enemy.Update();
    CHECK(enemy.IsAttackPathClear());
    enemy.ForceDistance(0x20000);
    CHECK(enemy.SelectAttack() == 1);
    player->SetPosition({0, enemy.GetPosition().y + 0x50001, 0x20000});
    enemy.Update();
    CHECK(!enemy.IsAttackPathClear());
    CHECK(enemy.SelectAttack() == 0);
    player->SetPosition({0, 0, 0x20000});
    enemy.Update();
    CHECK(enemy.IsAttackPathClear());
    enemy.ForceDistance(0x20000);
    enemy.BeginAttack();
    CHECK((enemy.GetPhysicsFlags() & soft3d::PhysicKeepDirection) != 0);
    CHECK((enemy.GetPhysicsFlags() & soft3d::PhysicSimpleMovement) == 0);
    enemy.ForceStateTime(13);
    enemy.UpdateAttack();
    CHECK(enemy.GetStateTime() == 14 && enemy.GetVelocity().y == 0xcccc);
    CHECK((enemy.GetPhysicsFlags() & soft3d::PhysicSeparateAxisCollision) != 0);
    enemy.ForceVerticalVelocity(-1);
    enemy.ForceGroundCollision(true);
    enemy.ForcePlayerCollision();
    const int32_t playerHealth = player->GetHealth();
    enemy.UpdateAttack();
    CHECK(enemy.GetLandingCount() == 1 && enemy.IsAttackHitDone());
    CHECK(player->GetHealth() == playerHealth - 5);
    CHECK(enemy.GetObject(0)->GetAnimationTime() == 0x13ec);
    CHECK((enemy.GetPhysicsFlags() & soft3d::PhysicSeparateAxisCollision) == 0);

    enemy.SetHealth(0);
    CHECK(enemy.GetState() == game::EnemyDying && player->GetGunExperience() == 1);
    for (int index = 0; index < 12; ++index) {
        enemy.ForceFinished(true);
        enemy.UpdateDeath();
    }
    CHECK(enemy.GetState() == game::EnemyRemoved);
}

void TestGhostEncounter()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    VariantProbe<game::Ghost> enemy(manager->GetContext(), manager->GetData());
    enemy.SetManager(manager);
    enemy.SetPlayer(player);
    enemy.LoadResources(u"", nullptr, nullptr);
    enemy.Spawn({});
    CHECK(enemy.GetObject(0) == nullptr && !enemy.ReceivePlayerAttack());

    player->SetPosition({0, 0, 0x140000});
    enemy.Update();
    CHECK(fixture.engine.GetRenderer().GetActiveShader() == fixture.scene->GetTwistShader());
    CHECK(fixture.scene->GetTwistShader()->GetAmplitude() == 1);
    CHECK(enemy.GetProximityVoice() != -1);
    const int32_t voice = enemy.GetProximityVoice();
    player->SetPosition({0, 0, 0xa0000});
    enemy.Update();
    CHECK(fixture.scene->GetTwistShader()->GetAmplitude() == 2);
    CHECK(enemy.GetProximityVoice() == voice);
    enemy.SetNavigationEnabled(true);
    CHECK(enemy.GetProximityVoice() == -1);
    enemy.SetNavigationEnabled(false);
    player->SetPosition({0, 0, 0x140000});
    enemy.Update();
    CHECK(enemy.GetProximityVoice() != -1);
    enemy.SetStopped(true);
    enemy.SetPosition({});
    player->SetPosition({0, 0, 0x150000});
    enemy.Update();
    CHECK(fixture.engine.GetRenderer().GetActiveShader() !=
          fixture.scene->GetTwistShader());
    CHECK(enemy.GetProximityVoice() == -1);

    player->SetPosition({});
    player->SetCollisionBounds({{-0x100000, -0x100000, -0x100000},
                                {0x100000, 0x100000, 0x100000}});
    enemy.SetPosition({});
    enemy.BeginAttack();
    enemy.ForceStateTime(7);
    const int32_t health = player->GetHealth();
    enemy.UpdateAttack();
    CHECK(player->GetHealth() == health - 25 && enemy.GetStateTime() == 8);
    enemy.ForceStateTime(19);
    enemy.UpdateAttack();
    CHECK(enemy.GetState() == game::EnemyIdle && enemy.GetStateTime() == 0);
    CHECK(enemy.GetAttackCooldown() == 48);
    soft3d::RenderQueues queues;
    enemy.SubmitRenderables(queues);
    enemy.StopSound();
}

void TestLamperEncounter()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    VariantProbe<game::Lamper> enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy, u"r_lamper_a");
    CHECK(!enemy.IsCollisionEnabled());
    enemy.ForceDistance(0x70000);
    CHECK(enemy.UpdatePerception() && enemy.IsPlayerVisible());
    enemy.ForceDistance(0x90000);
    CHECK(!enemy.UpdatePerception() && !enemy.IsPlayerVisible());

    enemy.SetPosition({});
    player->SetPosition({});
    player->SetCollisionBounds({{-0x100000, -0x100000, -0x100000},
                                {0x100000, 0x100000, 0x100000}});
    enemy.BeginAttack();
    enemy.ForceStateTime(7);
    CHECK(!enemy.TestAttackHit());
    enemy.ForceStateTime(8);
    const int32_t health = player->GetHealth();
    CHECK(enemy.TestAttackHit() && player->GetHealth() == health - 15);
    CHECK(!enemy.TestAttackHit());

    enemy.Spawn({});
    enemy.SetHealth(50);
    player->PrepareWeapon(manager->GetData(), 1);
    player->SetWeapon(1);
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    enemy.SetCollisionBounds({{INT32_MIN, INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX, INT32_MAX}});
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetHealth() == 30 && enemy.GetState() == game::EnemyDying);
    CHECK(!enemy.IsCollisionEnabled() && enemy.GetBloodLife() == 0);
    enemy.Damage(1);
    CHECK(enemy.GetBloodLife() == 0);
}

void TestWeedEncounter()
{
    GameFixture fixture;
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    VariantProbe<game::Weed> enemy(manager->GetContext(), manager->GetData());
    PrepareVariant(fixture, enemy, u"r_weed_a");
    enemy.ForceDistance(0x90000);
    CHECK(enemy.UpdatePerception());
    enemy.ForceDistance(0x100000);
    CHECK(enemy.UpdatePerception() && enemy.IsPlayerVisible());

    player->PrepareWeapon(manager->GetData(), 1);
    player->SetWeapon(1);
    player->ChangeState(game::PlayerAttackA2);
    player->Update();
    enemy.SetPosition(player->GetPosition());
    enemy.SetCollisionBounds({{INT32_MIN, INT32_MIN, INT32_MIN}, {INT32_MAX, INT32_MAX, INT32_MAX}});
    manager->SetRandomSeed(0);
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetHealth() == 60 && enemy.GetState() == game::EnemyKnockback);
    CHECK(enemy.GetHitCooldown() == 8 && enemy.GetBloodLife() == 0);

    uint32_t hurtSeed = 0;
    for (; hurtSeed < 1000; ++hurtSeed) {
        manager->SetRandomSeed(hurtSeed);
        if (manager->NextRandom() % 1000 > 500) break;
    }
    CHECK(hurtSeed < 1000);
    enemy.SetHealth(80);
    enemy.ForceHitCooldown(0);
    manager->SetRandomSeed(hurtSeed);
    CHECK(enemy.ReceivePlayerAttack());
    CHECK(enemy.GetHealth() == 60 && enemy.GetState() == game::EnemyHurt);
    enemy.Damage(1);
    CHECK(enemy.GetBloodLife() == 0);
}

void ExecuteEnemySpawnCommandTree(GameFixture& fixture, soft3d::ScriptCommand* command,
                                  int32_t enemyType)
{
    if (command->GetId() == 180 && command->GetArgumentCount() >= 2 &&
        command->GetArgument(1).GetInt() == enemyType) {
        fixture.gameInterface->Execute(*command);
    }
    for (soft3d::ScriptCommand* child : command->GetBody()) {
        ExecuteEnemySpawnCommandTree(fixture, child, enemyType);
    }
    for (soft3d::ScriptCommand* child : command->GetConditions()) {
        ExecuteEnemySpawnCommandTree(fixture, child, enemyType);
    }
}

void ExecuteEnemySpawnCommands(GameFixture& fixture, int32_t enemyType)
{
    auto& script = fixture.engine.GetScriptEngine();
    for (std::size_t task = 0; task < script.GetFunctionCount(); ++task) {
        const auto& function = script.GetFunction(static_cast<uint32_t>(task));
        for (soft3d::ScriptCommand* command : function.GetEntryCommands()) {
            ExecuteEnemySpawnCommandTree(fixture, command, enemyType);
        }
        for (soft3d::ScriptCommand* command : function.GetCommands()) {
            ExecuteEnemySpawnCommandTree(fixture, command, enemyType);
        }
    }
}

void TestStage11RealContent()
{
#ifndef SOFT3D_SPK_ENGLISH_LAYOUT
    {
        GameFixture fixture;
        LoadRealMap(fixture);
        auto* manager = fixture.scene->GetUnitManager();
        CHECK(manager->LoadDayContent(3, fixture.engine) == soft3d::S3D_OK);
        manager->PrepareDayEnemies(3);
        fixture.engine.SetPreUpdateEnabled(true);
        fixture.engine.SetInGame(true);
        bool sawLamper = false;
        bool sawWeed = false;
        for (const auto& group : manager->GetData().GetEnemyInfoGroups()) {
            manager->SetMapId(group.mapId);
            manager->SpawnMapEnemies();
            for (soft3d::GameUnit* unit = manager->GetFirstUnit(0); unit != nullptr;
                 unit = unit->GetNextUnit()) {
                auto* enemy = dynamic_cast<game::Enemy*>(unit);
                if (enemy == nullptr) continue;
                sawLamper = sawLamper || dynamic_cast<game::Lamper*>(enemy) != nullptr;
                sawWeed = sawWeed || dynamic_cast<game::Weed*>(enemy) != nullptr;
            }
            fixture.RunFrames(3);
            manager->RemoveAllEnemies();
        }
        CHECK(sawLamper && sawWeed);
    }
    {
        GameFixture fixture;
        LoadRealMap(fixture, 1053, 5);
        auto* manager = fixture.scene->GetUnitManager();
        manager->PrepareDayEnemies(5);
        auto& script = fixture.engine.GetScriptEngine();
        CHECK(script.LoadFromPak(u".\\day5\\ep1053",
                                 fixture.engine.GetResourceManager().GetFilePak()) == soft3d::S3D_OK);
        ExecuteEnemySpawnCommands(fixture, 10);
        for (int id : {2, 3, 5}) {
            CHECK(dynamic_cast<game::Worm*>(manager->FindEnemy(id)) != nullptr);
        }
        fixture.engine.SetPreUpdateEnabled(true);
        fixture.engine.SetInGame(true);
        fixture.RunFrames(12);
    }
    {
        GameFixture fixture;
        LoadRealMap(fixture);
        auto* manager = fixture.scene->GetUnitManager();
        manager->PrepareDayEnemies(3);
        auto& script = fixture.engine.GetScriptEngine();
        CHECK(script.LoadFromPak(u".\\day3\\ep1020",
                                 fixture.engine.GetResourceManager().GetFilePak()) == soft3d::S3D_OK);
        ExecuteEnemySpawnCommands(fixture, 3);
        CHECK(dynamic_cast<game::Ghost*>(manager->FindEnemy(2)) != nullptr);
        fixture.engine.SetPreUpdateEnabled(true);
        fixture.engine.SetInGame(true);
        fixture.RunFrames(12);
    }
#endif
}

void TestStage11EngineFrames()
{
    GameFixture fixture;
    LoadRealMap(fixture);
    auto* manager = fixture.scene->GetUnitManager();
    manager->PrepareDayEnemies(3);
    manager->PrepareDayEnemies(5);
    const soft3d::Vec3 origin = fixture.scene->GetPlayer()->GetPosition();
    const int types[] = {3, 4, 5, 10};
    for (int index = 0; index < 4; ++index) {
        fixture.Add(1100 + index, types[index], -1,
                    origin.x + (index + 1) * 0x40000, origin.z + 0x40000);
        CHECK(manager->FindEnemy(1100 + index) != nullptr);
    }
    CHECK(manager->GetBucketCount(0) == 4);
    fixture.RunFrames(30);
    for (int index = 0; index < 4; ++index) {
        CHECK(manager->FindEnemy(1100 + index) != nullptr);
        fixture.Execute(196, {Argument::Short(1100 + index)});
    }
    fixture.RunFrames(20);
    const auto* video = dynamic_cast<const soft3d::platform::DummyVideoBackend*>(
        fixture.engine.GetVideo().GetBackend());
    CHECK(video != nullptr && !video->GetPresentedPixels().empty());
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
        Run("stage 10 types and resources", TestStage10TypesAndResources);
        Run("Maid3 and Bully attacks", TestMaid3AndBullyAttacks);
        Run("FatMaid projectile", TestFatMaidProjectile);
        Run("BullyDay3 encounter", TestBullyDay3Encounter);
        Run("BullyDay5 encounter", TestBullyDay5Encounter);
        Run("stage 10 engine frames", TestStage10EngineFrames);
        Run("stage 11 types and resources", TestStage11TypesAndResources);
        Run("Worm colors and attack", TestWormColorsAndAttack);
        Run("Ghost encounter", TestGhostEncounter);
        Run("Lamper encounter", TestLamperEncounter);
        Run("Weed encounter", TestWeedEncounter);
        Run("stage 11 real content", TestStage11RealContent);
        Run("stage 11 engine frames", TestStage11EngineFrames);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

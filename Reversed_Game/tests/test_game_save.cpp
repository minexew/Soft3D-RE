#include "game/GameSI.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameUIDlg.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
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

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        path = std::filesystem::temp_directory_path() / "soft3d-game-save-tests";
        std::error_code error;
        std::filesystem::remove_all(path, error);
        CHECK(std::filesystem::create_directories(path));
    }
    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    std::filesystem::path path;
};

std::vector<uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
}

void WriteFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    CHECK(output.good());
}

struct GameFixture {
    explicit GameFixture(const std::filesystem::path& persistence)
        : engine(MakeConfig())
    {
        engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
        engine.SetPersistenceDirectory(persistence.u16string());
        CHECK(engine.Init() == soft3d::S3D_OK);
        auto sceneOwner = std::make_unique<game::Scene>(engine);
        scene = sceneOwner.get();
        CHECK(sceneOwner->Initialize() == soft3d::S3D_OK);
        engine.SetSceneHost(std::move(sceneOwner));
        interface = std::make_unique<game::GameSI>();
        interface->Bind({scene, scene->GetUnitManager(), scene->GetCameraController(),
                         scene->GetPlayer(), scene->GetGameUi(), scene->GetAudio(),
                         scene->GetResources(), scene->GetBsp(), scene->GetControlMap()});
        engine.SetScriptInterface(interface.get());
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

    void RunFrames(int count)
    {
        for (int index = 0; index < count; ++index) CHECK(engine.RunFrame());
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> interface;
};

void PopulateUnits(game::MyGameUnitMgr& manager)
{
    const soft3d::ObjectBounds bounds{{-0x10000, -0x20000, -0x30000},
                                      {0x10000, 0x20000, 0x30000}};
    manager.AddCollisionUnit(100, bounds);
    for (int8_t type = 0; type <= 3; ++type) {
        soft3d::Things* thing = manager.AddThing(200 + type, type, 1, true, u"r_ken_a",
            {type * 0x10000, 0, 0}, {soft3d::fixedOne, 0, 0},
            {0, soft3d::fixedOne, 0}, {0, 0, soft3d::fixedOne});
        CHECK(thing != nullptr);
        thing->SetVisible(false);
        if (auto* pickable = dynamic_cast<game::Pickable*>(thing)) pickable->SetItemId(1005);
    }
    manager.AddTrap(300, bounds, {1, 2, 3});
    manager.AddEventTrigger(400, 3, 26, bounds);
    manager.AddActTrigger(500, bounds)->SetOptionCount(2);
    manager.AddDoorTrigger(600, 1001, 27, 2, 2009, 1010, {1, 2, 3}, {0, 0, 1}, bounds);
    manager.AddHideTrigger(700, 2, {4, 5, 6}, {0, 0, 1}, bounds);
    manager.AddItemTrigger(800, 1002, bounds);
    auto* talk = manager.AddTalkTrigger(900, bounds, true);
    talk->AddLine(28, 12, -1);
    manager.AddPickTrigger(1000, 203, 26, bounds);
    manager.AddStayBox(1100, bounds);
}

void TestManagerRoundTrip()
{
    TemporaryDirectory temporary;
    GameFixture fixture(temporary.path);
    fixture.engine.StartNewGame();
    game::MyGameUnitMgr& manager = *fixture.scene->GetUnitManager();
    PopulateUnits(manager);
    soft3d::Vec3 enemyPosition{0x100000, 0, 0};
    CHECK(manager.AddEnemy(1200, 0, 50, enemyPosition, {0, 0, soft3d::fixedOne}, false));
    manager.SetDoorState(10003, 2);
    manager.SetHealthPackCount(7);
    fixture.engine.GetContent().items[4].runtimeState = 0x12345678;

    soft3d::OutputDataStream first;
    CHECK(manager.SaveState(first) == soft3d::S3D_OK);
    CHECK(first.GetSize() > 0x100 + 0x1000 + 400);
    const std::vector<uint8_t> original(first.GetData(), first.GetData() + first.GetSize());
    CHECK(original[0] == 0xff && original[1] == 0xff && original[2] == 0xff && original[3] == 0xff);
    CHECK(original[0x100] == 0 && original[0x103] == 0);
    CHECK(original[0x100 + 3 * 4] == 2);

    soft3d::InputDataStream input;
    input.SetData(first.GetData(), first.GetSize());
    CHECK(manager.LoadUnitState(input) == soft3d::S3D_OK);
    CHECK(input.IsEnd() && !input.HasError());
    CHECK(manager.GetDoorState(10003) == 2);
    CHECK(manager.GetHealthPackCount() == 7);
    CHECK(manager.FindStayBox(1100) != nullptr);
    CHECK(manager.FindEnemy(1200) != nullptr && manager.FindEnemy(1200)->GetEnemyType() == 0 &&
          manager.FindEnemy(1200)->GetHealth() == 50);
    CHECK(manager.FindUnit(1, 100) != nullptr);
    for (int id = 200; id <= 203; ++id) CHECK(manager.FindUnit(2, id) != nullptr);
    for (int id : {400, 500, 600, 700, 800, 900, 1000}) CHECK(manager.FindUnit(4, id) != nullptr);
    auto* pick = dynamic_cast<game::PickTrigger*>(manager.FindUnit(4, 1000));
    CHECK(pick != nullptr && pick->GetPickable() != nullptr && pick->GetPickable()->GetUnitId() == 203);
    CHECK(fixture.engine.GetContent().items[4].runtimeState == 0x12345678);

    soft3d::OutputDataStream second;
    CHECK(manager.SaveState(second) == soft3d::S3D_OK);
    CHECK(second.GetSize() == first.GetSize());
    CHECK(std::equal(first.GetData(), first.GetData() + first.GetSize(), second.GetData()));

    std::vector<uint8_t> badLink = original;
    const std::array<uint8_t, 8> pickHeader{0, 0, 0, 0x25, 0, 0, 0x03, 0xe8};
    const auto found = std::search(badLink.begin(), badLink.end(), pickHeader.begin(), pickHeader.end());
    CHECK(found != badLink.end());
    const std::size_t linkOffset = static_cast<std::size_t>(found - badLink.begin()) + 80;
    badLink[linkOffset] = 0x7f;
    badLink[linkOffset + 1] = 0xff;
    badLink[linkOffset + 2] = 0xff;
    badLink[linkOffset + 3] = 0xff;
    input.SetData(badLink.data(), badLink.size());
    CHECK(manager.LoadUnitState(input) == soft3d::S3D_FORMAT);
}

void TestFullSceneRoundTripAndResume()
{
    TemporaryDirectory temporary;
    GameFixture fixture(temporary.path);
    fixture.engine.StartNewGame();
    fixture.RunFrames(62);
    CHECK(!fixture.scene->GetScriptName().empty());
    fixture.scene->SetEventFlag(123);
    fixture.scene->GetEventFlag(123);
    fixture.scene->GetGameUi()->SetGunHudEnabled(1);
    CHECK(fixture.scene->AddEnvironmentSound(0, u"e_wind01", 20, 7));
    fixture.scene->GetAudio()->PlayMusic(u"m_begin", true);
    fixture.scene->GetUnitManager()->SetDoorState(10001, 1);
    fixture.scene->GetPlayer()->SetVisible(true);
    PopulateUnits(*fixture.scene->GetUnitManager());

    CHECK(fixture.scene->SaveSaveSlot(1));
    const auto slot1 = temporary.path / "slot1.sav";
    const auto sidecar1 = temporary.path / "spt" / "slot1.sav";
    const std::vector<uint8_t> saved = ReadFile(slot1);
    const std::vector<uint8_t> script = ReadFile(sidecar1);
    CHECK(saved.size() > 44 + 8 + 0x5000 && saved.back() == 0);
    CHECK(!script.empty());
    fixture.engine.RefreshSaveSlots();
    CHECK(fixture.engine.GetSaveSlots()[0].present);
    CHECK(fixture.engine.GetSaveSlots()[0].GetSaveNumber() == 1);
    CHECK(fixture.engine.GetSaveSlots()[0].GetPlayTime() ==
          static_cast<uint32_t>(fixture.scene->GetPlayer()->GetPlayTime()));

    fixture.scene->SetEventFlag(124);
    CHECK(fixture.engine.LoadSaveSlot(1));
    CHECK(fixture.engine.IsInGame());
    CHECK(fixture.scene->GetEventFlag(123));
    CHECK(!fixture.scene->GetEventFlag(124));
    CHECK(fixture.scene->GetGameUi()->IsGunHudEnabled() == 1);
    CHECK(fixture.scene->GetEnvironmentChannel(0).resourceCount == 1);
    CHECK(fixture.scene->GetEnvironmentChannel(0).baseDelay == 20);
    CHECK(fixture.scene->GetEnvironmentChannel(0).randomDelay == 7);
    CHECK(fixture.scene->GetUnitManager()->GetDoorState(10001) == 1);
    CHECK(fixture.scene->GetUnitManager()->FindStayBox(1100) != nullptr);

    CHECK(fixture.scene->SaveSaveSlot(2));
    const std::vector<uint8_t> restored = ReadFile(temporary.path / "slot2.sav");
    CHECK(restored.size() == saved.size());
    CHECK(std::equal(saved.begin() + 44, saved.end(), restored.begin() + 44));
    CHECK(ReadFile(temporary.path / "spt" / "slot2.sav") == script);
    CHECK(fixture.engine.GetSaveSlots()[1].GetSaveNumber() == 2);

    fixture.RunFrames(40);
    CHECK(fixture.scene->GetMapProgress() == -1);
    CHECK(fixture.scene->GetUnitManager()->IsMapReady());
    CHECK(fixture.scene->GetPlayer()->IsVisible());
    CHECK(fixture.engine.RunFrame());
}

void TestMalformedInputs()
{
    TemporaryDirectory temporary;
    GameFixture fixture(temporary.path);
    fixture.engine.StartNewGame();
    fixture.RunFrames(62);
    CHECK(fixture.scene->SaveSaveSlot(1));
    const auto slot = temporary.path / "slot1.sav";
    const auto sidecar = temporary.path / "spt" / "slot1.sav";
    const std::vector<uint8_t> saved = ReadFile(slot);
    const std::vector<uint8_t> script = ReadFile(sidecar);

    CHECK(!fixture.scene->LoadSaveSlot(0));
    CHECK(!fixture.scene->LoadSaveSlot(4));
    for (std::size_t size : {std::size_t(0), std::size_t(43), std::size_t(44), std::size_t(45),
                             std::size_t(50), std::size_t(60), std::size_t(100),
                             saved.size() / 2, saved.size() - 2}) {
        WriteFile(slot, std::vector<uint8_t>(saved.begin(), saved.begin() + size));
        CHECK(!fixture.scene->LoadSaveSlot(1));
    }

    WriteFile(slot, saved);
    std::filesystem::remove(sidecar);
    CHECK(!fixture.scene->LoadSaveSlot(1));
    WriteFile(sidecar, std::vector<uint8_t>(script.begin(), script.end() - 1));
    CHECK(!fixture.scene->LoadSaveSlot(1));
    WriteFile(sidecar, script);
    CHECK(fixture.scene->LoadSaveSlot(1));

    WriteFile(slot, std::vector<uint8_t>(saved.begin(), saved.begin() + 44));
    fixture.engine.ShowMainMenu(1);
    fixture.engine.ActivateMenuDialog(5);
    CHECK(!fixture.engine.LoadSaveSlot(1));
    CHECK(fixture.engine.GetUiManager()->GetDialogRuntime(5)->IsActive());
    CHECK(!fixture.scene->GetGameUi()->IsActive());
    WriteFile(slot, saved);

    game::MyGameUnitMgr& manager = *fixture.scene->GetUnitManager();
    soft3d::OutputDataStream valid;
    CHECK(manager.SaveState(valid) == soft3d::S3D_OK);
    std::vector<uint8_t> invalid(valid.GetData(), valid.GetData() + valid.GetSize());
    invalid.resize(0x100 + 0x1000 + 4);
    soft3d::InputDataStream input;
    input.SetData(invalid.data(), invalid.size());
    CHECK(manager.LoadUnitState(input) == soft3d::S3D_FORMAT);

    std::vector<uint8_t> badType(valid.GetData(), valid.GetData() + valid.GetSize());
    badType.insert(badType.end(), {0, 0, 0, 0x7f});
    input.SetData(badType.data(), badType.size());
    CHECK(manager.LoadUnitState(input) == soft3d::S3D_FORMAT);
}

} // namespace

int main()
{
    try {
        TestManagerRoundTrip();
        TestFullSceneRoundTripAndResume();
        TestMalformedInputs();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

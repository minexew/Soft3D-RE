#include "game/GameSI.hpp"
#include "game/GameUiController.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/MenuDialogs.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
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

    static Argument Int(int32_t value) { return {soft3d::ScriptValueType::Int, value}; }
    static Argument Short(int16_t value) { return {soft3d::ScriptValueType::Short, value}; }
    static Argument Bool(bool value) { return {soft3d::ScriptValueType::Bool, value ? 1 : 0}; }
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
        Append16(bytes, 4);
        Append32(bytes, static_cast<uint32_t>(argument.value));
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
    GameFixture()
        : engine(MakeConfig())
    {
        directory = std::filesystem::temp_directory_path() /
            ("soft3d_game_ui_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        std::filesystem::create_directories(directory);
        engine.SetPersistenceDirectory(directory.u16string());
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
        scene->GetUiController()->InstallHooks();
        engine.Start();
    }

    ~GameFixture()
    {
        engine.SetScriptInterface(nullptr);
        std::filesystem::remove_all(directory);
    }

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

    void Start()
    {
        engine.StartNewGame();
        RunFrames(58);
        engine.GetScriptEngine().Unload();
        engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
        for (int count = 0; count < 50 && !scene->IsMapLoaded(); ++count) RunFrames(1);
        CHECK(scene->IsMapLoaded());
        for (int count = 0; count < 5 && scene->GetMapProgress() != -1; ++count) RunFrames(1);
        CHECK(scene->GetUnitManager()->IsMapReady());
        scene->GetPlayer()->ChangeState(game::PlayerStand);
        for (int action = 1; action <= 7; ++action) {
            engine.GetControlMap().SetActionEnabled(action, true);
        }
        engine.GetControlMap().SetEnabled(true);
    }

    void Click(soft3d::Key key = soft3d::Key::Accept)
    {
        engine.GetInput().KeyDown(key);
        engine.GetInput().KeyUp(key);
        RunFrames(1);
    }

    uint32_t FrameHash()
    {
        RunFrames(1);
        const auto* backend = dynamic_cast<const soft3d::platform::DummyVideoBackend*>(
            engine.GetVideo().GetBackend());
        CHECK(backend != nullptr);
        uint32_t hash = 2166136261U;
        for (uint32_t pixel : backend->GetPresentedPixels()) hash = (hash ^ pixel) * 16777619U;
        return hash;
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
    std::filesystem::path directory;
};

void TestCommandsAndStatistics()
{
    GameFixture fixture;
    auto* ui = fixture.scene->GetGameUi();
    auto* manager = fixture.scene->GetUnitManager();
    auto* player = fixture.scene->GetPlayer();
    fixture.Execute(60);
    CHECK(ui->GetMode() == 12 && ui->IsMenuOpen());
    fixture.Execute(61);
    CHECK(ui->GetMode() == 13);
    fixture.Execute(62, {Argument::Bool(true)});
    CHECK(manager->IsPaused());
    fixture.Execute(62, {Argument::Bool(false)});
    CHECK(!manager->IsPaused());
    ui->ResetLevelState();

    fixture.Execute(73, {Argument::Short(20)});
    CHECK(ui->IsDialogActive() && ui->GetDialogKind() == 0 && ui->GetDialogTextId() == 20);
    for (int count = 0; count < 8 && ui->IsDialogActive(); ++count) fixture.Click();
    CHECK(!ui->IsDialogActive());
    fixture.Execute(74, {Argument::Int(0), Argument::Short(30)});
    fixture.Execute(74, {Argument::Int(4), Argument::Short(31)});
    CHECK(ui->GetDialogOption(0) == 30 && ui->GetDialogOption(4) == 31);
    fixture.Execute(75, {Argument::Short(32), Argument::Int(2), Argument::Bool(true)});
    CHECK(ui->GetDialogKind() == 1 && ui->GetDialogOptionCount() == 2 &&
          ui->GetDialogResult() == -1);
    CHECK(fixture.Execute(78, {}, true) == -1);
    fixture.Execute(79);
    CHECK(!ui->IsDialogActive() && ui->GetDialogResult() == -1);
    CHECK(fixture.Execute(80, {}, true) == 1);
    fixture.Execute(76, {Argument::Int(33)});
    CHECK(ui->GetDialogKind() == 3 && ui->GetDialogTextId() == 33);
    fixture.Execute(79);
    fixture.Execute(77, {Argument::Int(34), Argument::Int(2)});
    CHECK(!ui->IsDialogActive() || ui->GetDialogTextId() == 34);
    fixture.Execute(81);
    CHECK(ui->GetMode() == 3 && ui->IsDirectMap());
    fixture.Execute(82);
    fixture.Execute(83);
    ui->ResetLevelState();
    fixture.Execute(96, {Argument::Int(35), Argument::Int(2)});
    CHECK(ui->GetDialogKind() == 4 && ui->GetDialogResult() == -1);
    fixture.Execute(97);
    CHECK(!ui->IsDialogActive());
    fixture.Execute(101, {Argument::Bool(true), Argument::Short(36)});
    CHECK(ui->IsSubtitleActive() && ui->GetSubtitleTextId() == 36);
    fixture.Execute(101, {Argument::Bool(false), Argument::Short(-1)});
    CHECK(!ui->IsSubtitleActive());

    fixture.Execute(102, {Argument::Bool(false), Argument::Bool(true),
                          Argument::Bool(false), Argument::Bool(true)});
    fixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Number3));
    fixture.engine.GetInput().KeyDown(soft3d::Key::Number3);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Number3);
    CHECK(fixture.engine.GetControlMap().ActionClick(fixture.engine.GetInput(), 5, false));
    CHECK(!fixture.engine.GetControlMap().ActionClick(fixture.engine.GetInput(), 6, false));
    CHECK(!fixture.engine.GetControlMap().ActionDown(fixture.engine.GetInput(), 1, false));
    fixture.Execute(104, {Argument::Int(3), Argument::Int(2), Argument::Int(100), Argument::Int(200)});
    CHECK(ui->GetMapMarker(3).x == 100 && ui->GetMapMarker(3).y == 200 &&
          ui->GetMapMarker(3).type == 1);
    fixture.Execute(105, {Argument::Int(3)});
    CHECK(ui->GetMapMarker(3).x == -1 && ui->GetMapMarker(3).y == -1);
    fixture.Execute(106, {Argument::Int(1)});
    CHECK(ui->IsGunHudEnabled() == 1);

    CHECK(fixture.Execute(107, {}, true) == 0);
    CHECK(fixture.Execute(108, {}, true) == 0);
    CHECK(fixture.Execute(109, {}, true) == 0);
    CHECK(fixture.Execute(110, {}, true) == 0);
    player->AddItem(1001, false);
    player->AddItem(2001, false);
    CHECK(fixture.Execute(111, {}, true) == 2);
    fixture.scene->SetEventFlag(51);
    CHECK(fixture.Execute(112, {}, true) == 1);
    CHECK(fixture.engine.GetConfiguration().unlocks.documents[0] == 1);
    CHECK(fixture.engine.GetConfiguration().unlocks.photos[0] == 1);
}

void TestInventoryMapAndUnlocks()
{
    GameFixture fixture;
    fixture.Start();
    auto* player = fixture.scene->GetPlayer();
    auto* ui = fixture.scene->GetGameUi();
    player->SetHealth(50);
    player->AddItem(1, false);
    player->AddItem(1, false);
    ui->OpenMenu();
    fixture.RunFrames(11);
    CHECK(ui->GetMode() == 1);
    fixture.Click();
    CHECK(ui->GetMode() == 2);
    fixture.Click();
    CHECK(ui->GetMode() == 10);
    fixture.Click();
    CHECK(ui->GetMode() == 2 && player->GetHealth() == 100);
    CHECK(player->GetHealthPackCount() == 1 && player->GetItemUseCount() == 1);
    fixture.Click(soft3d::Key::SoftRight);
    fixture.Click(soft3d::Key::SoftRight);
    fixture.RunFrames(11);
    CHECK(!ui->IsMenuOpen());

    player->AddItem(17, false);
    player->AddItem(18, false);
    CHECK(fixture.engine.GetGameplayHooks().combineItems(-1, 0) == 0);
    ui->OpenMenu();
    fixture.RunFrames(11);
    fixture.Click();
    fixture.Click(soft3d::Key::Number3);
    fixture.Click();
    fixture.Click(soft3d::Key::Number7);
    fixture.Click();
    CHECK(ui->GetMode() == 14);
    fixture.Click(soft3d::Key::Number3);
    fixture.Click();
    CHECK(player->HasItem(19) && !player->HasItem(17) && !player->HasItem(18));
    CHECK(ui->GetMode() == 6);

    auto* manager = fixture.scene->GetUnitManager();
    auto* pickable = dynamic_cast<game::Pickable*>(manager->FindUnit(2, 9999));
    CHECK(pickable != nullptr && pickable->GetItemId() == 1005);
    pickable->PickUp(player);
    CHECK(player->HasItem(1005));
    CHECK(fixture.engine.GetConfiguration().unlocks.documents[4] == 1);
    fixture.scene->GetUiController()->SynchronizeContent();
    CHECK(fixture.engine.GetContent().noteIds[0] == 1005);
    CHECK(fixture.engine.GetContent().statistics.filesFound == 1);

    fixture.scene->SetEventFlag(998);
    fixture.scene->SetEventFlag(999);
    fixture.scene->GetUiController()->FinishGame();
    fixture.engine.SaveGameState();
    CHECK(fixture.engine.GetConfiguration().unlocks.extraWeapon0 == 1);
    CHECK(fixture.engine.GetConfiguration().unlocks.extraWeapon1 == 1);
    CHECK(fixture.engine.GetConfiguration().unlocks.newGameOptions == 1);
    CHECK(std::filesystem::file_size(fixture.directory / "config.sdt") == 1252);

    manager->SetDoorState(10001, 1);
    CHECK(manager->GetDoorState(10001) == 1);
    const auto& bounds = manager->GetMapHeaderValues();
    CHECK(fixture.scene->GetUiController()->ProjectMapPosition(
        {bounds[3], 0, bounds[5]}, 2008).x == 219);
    CHECK(fixture.scene->GetUiController()->ProjectMapPosition(
        {bounds[3], 0, bounds[5]}, 2008).y == 112);
    CHECK(fixture.scene->GetUiController()->ProjectMapPosition(
        {bounds[0], 0, bounds[2]}, 2008).x == 134);
    CHECK(fixture.scene->GetUiController()->ProjectMapPosition(
        {bounds[0], 0, bounds[2]}, 2008).y == 168);
    CHECK(soft3d::FixedArcCosine(0) == 0x800);
    CHECK(soft3d::FixedArcCosine(soft3d::fixedOne) == 0xea);
    CHECK(soft3d::FixedArcCosine(-soft3d::fixedOne) == 0xf17);
}

void TestExactRendering()
{
    GameFixture fixture;
    fixture.Start();
    auto* ui = fixture.scene->GetGameUi();
    auto* player = fixture.scene->GetPlayer();
    fixture.RunFrames(1);
    CHECK(ui->GetElementCurrentTime(7, 0) == 0);
    player->SetHealth(10);
    fixture.RunFrames(1);
    CHECK(ui->GetElementCurrentTime(7, 0) == 1);
    player->SetHealth(100);
    ui->SetElementCurrentTime(7, 0, 0);
    ui->ResetLevelState();
    fixture.scene->GetUnitManager()->SetPaused(false);
    std::array<std::pair<const char*, uint32_t>, 13> hashes{};
    std::size_t index = 0;
    hashes[index++] = {"hud", fixture.FrameHash()};
    ui->ShowEventMessage(20);
    hashes[index++] = {"event", fixture.FrameHash()};
    ui->CloseDialog();
    ui->ShowSimpleMessage(21);
    hashes[index++] = {"simple", fixture.FrameHash()};
    ui->CloseDialog();
    ui->SetDialogString(0, 3);
    ui->SetDialogString(1, 4);
    ui->ShowChoiceDialog(22, 2, true);
    hashes[index++] = {"choice", fixture.FrameHash()};
    ui->CloseDialog();
    ui->ShowTimedMessage(23, 5);
    hashes[index++] = {"timed", fixture.FrameHash()};
    ui->CloseDialog();
    fixture.scene->ShowQte(1, 10, 0);
    hashes[index++] = {"qte", fixture.FrameHash()};
    fixture.scene->FailQte();
    hashes[index++] = {"qte_failure", fixture.FrameHash()};
    ui->SetSubtitle(true, 24);
    hashes[index++] = {"subtitle", fixture.FrameHash()};
    ui->SetSubtitle(false, -1);
    fixture.scene->SetHideMask(0, true);
    hashes[index++] = {"hide", fixture.FrameHash()};
    fixture.scene->SetHideMask(0, false);

    fixture.scene->GetPlayer()->AddItem(2, false);
    ui->OpenMenu();
    fixture.RunFrames(11);
    hashes[index++] = {"menu", fixture.FrameHash()};
    fixture.Click();
    hashes[index++] = {"inventory", fixture.FrameHash()};
    fixture.Click();
    hashes[index++] = {"item_actions", fixture.FrameHash()};
    fixture.Click(soft3d::Key::Right);
    fixture.Click();
    hashes[index++] = {"item_examine", fixture.FrameHash()};
    ui->CloseMenu();
    fixture.RunFrames(11);
    fixture.scene->GetUnitManager()->SetDoorState(10001, 1);
    ui->OpenMap();
    const uint32_t map = fixture.FrameHash();
    fixture.engine.SetControlOrientation(2);
    const uint32_t mapOtherOrientation = fixture.FrameHash();

    fixture.engine.ReturnToMainMenu();
    fixture.engine.GetConfiguration().unlocks.models[2] = 1;
    fixture.engine.ShowMainMenu(2);
    fixture.Click();
    fixture.Click();
    const uint32_t about = fixture.FrameHash();
    CHECK(index == hashes.size());
    constexpr std::array<uint32_t, 13> expected = {
        0x9ef3185dU, 0x6bf63cc5U, 0xa70acd85U, 0xd10f2ad5U,
        0x24a71b05U, 0x8fa0e5ddU, 0x81a48ad5U, 0x62f6354dU,
        0x5d6e1fedU, 0x839fe2bdU, 0xac5fcb4dU, 0x08f9a64dU, 0x29718445U,
    };
    for (std::size_t index = 0; index < hashes.size(); ++index) {
        CHECK(hashes[index].second == expected[index]);
    }
    CHECK(map == 0xac605735U);
    CHECK(mapOtherOrientation == 0x96792195U);
    CHECK(about == 0x40cddb75U);
}

} // namespace

int main()
{
    try {
        TestCommandsAndStatistics();
        TestInventoryMapAndUnlocks();
        TestExactRendering();
        std::cout << "game UI tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

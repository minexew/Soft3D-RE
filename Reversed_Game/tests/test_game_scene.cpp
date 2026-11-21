#include "game/GameSI.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/MenuBackdrop.hpp"

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
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

class TestClock final : public soft3d::EngineClock {
public:
    uint32_t NowMicroseconds() override { return now; }
    void SleepMicroseconds(uint32_t) override {}

private:
    uint32_t now = 0;
};

struct Argument {
    soft3d::ScriptValueType type = soft3d::ScriptValueType::Int;
    int32_t value = 0;

    static Argument Int(int32_t value) { return {soft3d::ScriptValueType::Int, value}; }
    static Argument Short(int16_t value) { return {soft3d::ScriptValueType::Short, value}; }
    static Argument Fixed(int32_t value) { return {soft3d::ScriptValueType::Fixed, value}; }
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
    if (output) {
        Append16(bytes, static_cast<uint16_t>(soft3d::ScriptValueType::Int));
    }
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

    ~GameFixture()
    {
        engine.SetScriptInterface(nullptr);
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
        if (id == 259) {
            CHECK(script.GetSharedVariable(255) == 1);
        }
        return output ? script.GetGlobalVariable(0)->GetInt() : 0;
    }

    void RunFrames(int count)
    {
        for (int index = 0; index < count; ++index) {
            CHECK(engine.RunFrame());
        }
    }

    soft3d::GameEngine engine;
    game::Scene* scene = nullptr;
    std::unique_ptr<game::GameSI> gameInterface;
};

void TestCallbackTableAndCommands()
{
    GameFixture fixture;
    fixture.engine.SetLanguage(0);
    game::GameSI& interface = *fixture.gameInterface;
    for (int id = game::gameSIFirstCommand; id <= game::gameSILastCommand; ++id) {
        const auto& callback = interface.GetCallback(id);
        CHECK(callback.armAddress == game::GameSI::GetArmHandlerAddress(id));
        CHECK(callback.armAddress != 0);
        CHECK(callback.thisAdjustment == 0);
        CHECK(callback.handler != nullptr);
    }
    CHECK(interface.GetArmHandlerAddress(43) == 0x000653c0);
    CHECK(interface.GetArmHandlerAddress(54) == 0x000659ac);
    CHECK(interface.GetArmHandlerAddress(55) == 0x00065b18);
    CHECK(interface.GetArmHandlerAddress(56) == 0x00065b78);
    CHECK(interface.GetArmHandlerAddress(57) == 0x00065bc0);
    CHECK(interface.GetArmHandlerAddress(58) == 0x00065c10);
    CHECK(interface.GetArmHandlerAddress(59) == 0x00065c64);
    CHECK(interface.GetArmHandlerAddress(149) == 0x00068b78);
    CHECK(interface.GetArmHandlerAddress(255) == 0x0006d988);
    CHECK(interface.GetArmHandlerAddress(258) == 0x00064fd0);
    CHECK(interface.GetArmHandlerAddress(259) == 0x00064ff8);
    CHECK(interface.GetArmHandlerAddress(260) == 0x00065034);

    const game::GameSIContext& context = interface.GetContext();
    CHECK(context.scene == fixture.scene);
    CHECK(context.unitManager == fixture.scene->GetUnitManager());
    CHECK(context.cameraController == fixture.scene->GetCameraController());
    CHECK(context.player == fixture.scene->GetPlayer());
    CHECK(context.gameUi == fixture.scene->GetGameUi());
    CHECK(context.audio == fixture.scene->GetAudio());
    CHECK(context.resources == fixture.scene->GetResources());
    CHECK(context.bsp == fixture.scene->GetBsp());
    CHECK(context.controlMap == fixture.scene->GetControlMap());

    bool rejected = false;
    try {
        game::GameSI empty;
        empty.Bind({});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    CHECK(rejected);
    rejected = false;
    try {
        interface.GetCallback(42);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    CHECK(rejected);

    fixture.Execute(53);
    CHECK(fixture.scene->GetDay() == 1);
    CHECK(fixture.scene->IsDayIntroPending());
    CHECK(fixture.engine.GetBaseFont()->GetCount() != 0);
    CHECK(fixture.scene->GetUnitManager()->GetData().GetGameItemGroups().size() == 9);

    fixture.Execute(54, {
        Argument::Int(3), Argument::Fixed(1), Argument::Fixed(2), Argument::Fixed(3),
        Argument::Fixed(4), Argument::Fixed(5), Argument::Fixed(6),
        Argument::Short(255), Argument::Short(128), Argument::Short(64),
    });
    CHECK(fixture.scene->GetDoorResourceIndex() == 3);
    CHECK(fixture.scene->GetSpawnPosition().x == 1);
    CHECK(fixture.scene->GetSpawnPosition().z == 3);
    CHECK(fixture.scene->GetSpawnDirection().y == 5);
    CHECK(fixture.scene->GetTransitionColor() == 0xfc08);

    fixture.Execute(55, {Argument::Short(2008), Argument::Short(1010)});
    CHECK(fixture.scene->IsNextLevelPending());
    CHECK(fixture.scene->GetNextMapId() == 2008);
    CHECK(fixture.scene->GetNextEpisodeId() == 1010);
    CHECK(fixture.Execute(57, {}, true) == 0);
    fixture.Execute(58, {Argument::Short(123)});
    CHECK(fixture.scene->GetEventFlag(123));
    CHECK(fixture.Execute(59, {Argument::Short(123)}, true) == 1);
    CHECK(fixture.Execute(59, {Argument::Short(124)}, true) == 0);
    fixture.Execute(69);
    fixture.Execute(258);
    fixture.Execute(260);
    fixture.Execute(259);

    bool unsupported = false;
    try {
        fixture.Execute(121);
    } catch (const game::GameSIError& error) {
        unsupported = error.GetCommandId() == 121 && error.GetArmAddress() == 0x00067ba4;
    }
    CHECK(unsupported);
    bool malformed = false;
    try {
        fixture.Execute(56);
    } catch (const game::GameSIError& error) {
        malformed = error.GetCommandId() == 56;
    }
    CHECK(malformed);
    malformed = false;
    try {
        fixture.scene->SetEventFlag(-1);
    } catch (const std::out_of_range&) {
        malformed = true;
    }
    CHECK(malformed);
    malformed = false;
    try {
        fixture.scene->LoadSaveSlot(1);
    } catch (const std::runtime_error& error) {
        malformed = std::string(error.what()).find("stage-13") != std::string::npos;
    }
    CHECK(malformed);
}

void TestRealNewGameHandshake()
{
    GameFixture fixture;
    fixture.engine.StartNewGame({2, 1});
    CHECK(fixture.scene->GetDay() == 1);
    CHECK(fixture.scene->IsDayIntroPending());
    CHECK(fixture.scene->IsDayTitleVisible());
    CHECK(fixture.scene->GetMapProgress() == -1);
    fixture.RunFrames(55);
    CHECK(fixture.scene->IsDayIntroPending());
    CHECK(!fixture.scene->IsNextLevelPending());
    fixture.RunFrames(1);
    CHECK(!fixture.scene->IsDayIntroPending());
    CHECK(fixture.scene->IsNextLevelPending());
    fixture.RunFrames(1);
    CHECK(fixture.scene->GetScriptName() == u".\\day1\\ep0");
    CHECK(!fixture.scene->IsNextLevelPending());

    int previous = -1;
    int loadSteps = 0;
    for (int frame = 0; frame < 40 && !fixture.scene->IsMapLoaded(); ++frame) {
        CHECK(fixture.engine.RunFrame());
        const int progress = fixture.scene->GetMapProgress();
        if (progress >= 0 && progress != previous) {
            ++loadSteps;
        }
        previous = progress;
    }
    CHECK(fixture.scene->IsMapLoaded());
    CHECK(loadSteps == 26);
    CHECK(fixture.scene->GetMapId() == 2008);
    CHECK(fixture.scene->GetBsp()->IsLoaded());
    CHECK(!fixture.scene->IsUsingDay1Fallback());
    CHECK(fixture.engine.GetContent().mapId == 2008);
    CHECK(fixture.engine.GetScriptEngine().GetState() == soft3d::ScriptState::Initializing);

    std::size_t nullEntities = 0;
    for (const auto& entity : fixture.scene->GetBsp()->GetEntities()) {
        nullEntities += entity == nullptr;
    }
    CHECK(!fixture.scene->GetBsp()->GetEntities().empty());
    CHECK(nullEntities != 0);

    bool unsupported = false;
    for (int frame = 0; frame < 4 && !unsupported; ++frame) {
        try {
            CHECK(fixture.engine.RunFrame());
        } catch (const game::GameSIError& error) {
            CHECK(error.GetCommandId() == 121);
            CHECK(error.GetArmAddress() == 0x00067ba4);
            unsupported = true;
        }
    }
    CHECK(unsupported);
    fixture.engine.ReturnToMainMenu();
    CHECK(fixture.scene->GetDay() == 0);
    CHECK(!fixture.scene->GetBsp()->IsLoaded());
    CHECK(fixture.engine.GetScriptEngine().GetState() == soft3d::ScriptState::Unloaded);
    CHECK(fixture.engine.GetMenuBackdrop()->IsLoaded());
}

void TestMapFinishAndRepeatedStartup()
{
    GameFixture fixture;
    fixture.engine.StartNewGame();
    fixture.RunFrames(57);
    CHECK(fixture.scene->GetScriptName() == u".\\day1\\ep0");
    fixture.engine.GetScriptEngine().Unload();
    fixture.engine.GetScriptEngine().SetState(soft3d::ScriptState::Initialized);
    CHECK(fixture.scene->LoadMap(2008) == 0);
    CHECK(fixture.scene->GetMapProgress() == 0);
    CHECK(fixture.scene->GetTransitionStage() == 1);

    for (int frame = 0; frame < 40 && !fixture.scene->IsMapLoaded(); ++frame) {
        CHECK(fixture.engine.RunFrame());
    }
    CHECK(fixture.scene->IsMapLoaded());
    CHECK(fixture.engine.RunFrame());
    CHECK(fixture.scene->GetMapProgress() == 103);
    CHECK(fixture.engine.RunFrame());
    CHECK(fixture.scene->GetMapProgress() == -1);
    CHECK(fixture.scene->GetUnitManager()->IsMapReady());
    CHECK(fixture.scene->GetPlayer()->IsVisible());
    CHECK(fixture.engine.GetScriptEngine().GetState() == soft3d::ScriptState::Running);

    fixture.RunFrames(40);
    CHECK(fixture.scene->GetTransitionStage() == 0);
    fixture.engine.ReturnToMainMenu();
    CHECK(!fixture.scene->GetBsp()->IsLoaded());
    CHECK(!fixture.scene->GetUnitManager()->IsMapReady());
    CHECK(fixture.engine.GetMenuBackdrop()->IsLoaded());
    fixture.engine.StartNewGame();
    CHECK(fixture.scene->GetDay() == 1);
    CHECK(fixture.scene->IsDayIntroPending());
    CHECK(fixture.scene->GetUnitManager()->GetData().GetGameItemGroups().size() == 9);
    fixture.engine.ReturnToMainMenu();
    CHECK(fixture.scene->GetDay() == 0);
    CHECK(fixture.engine.GetMenuBackdrop()->IsLoaded());
}

void TestNextScriptAndDay1Fallback()
{
    {
        GameFixture fixture;
        fixture.engine.StartNewGame();
        fixture.RunFrames(57);
        fixture.scene->LoadNextLevel(0, 1010);
        CHECK(fixture.engine.RunFrame());
        CHECK(fixture.scene->GetScriptName() == u".\\day1\\ep1010");
        CHECK(!fixture.scene->IsNextLevelPending());
        CHECK(fixture.scene->GetMapId() == 2008);
        CHECK(fixture.scene->GetMapProgress() == 0);
    }
    {
        GameFixture fixture;
        fixture.engine.StartNewGame();
        fixture.engine.GetScriptEngine().Unload();
        CHECK(fixture.scene->LoadDay(2) == soft3d::S3D_OK);
        fixture.RunFrames(57);
        CHECK(fixture.scene->GetScriptName() == u".\\day2\\ep0");
        for (int frame = 0; frame < 40 && !fixture.scene->IsMapLoaded(); ++frame) {
            CHECK(fixture.engine.RunFrame());
        }
        CHECK(fixture.scene->IsMapLoaded());
        CHECK(fixture.scene->GetMapId() == 1001);
        CHECK(fixture.scene->IsUsingDay1Fallback());
        CHECK(fixture.scene->GetBsp()->IsLoaded());
        CHECK(fixture.scene->GetBsp()->GetDirectory() == u".\\day1\\");
    }
}

} // namespace

int main()
{
    try {
        TestCallbackTableAndCommands();
        TestRealNewGameHandshake();
        TestMapFinishAndRepeatedStartup();
        TestNextScriptAndDay1Fallback();
        std::cout << "game scene tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

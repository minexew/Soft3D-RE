#include "game/GameSI.hpp"
#include "game/GameUiController.hpp"
#include "game/Scene.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
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
    static Argument Bool(bool value) { return {soft3d::ScriptValueType::Bool, value ? 1 : 0, {}}; }
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
    explicit GameFixture(const std::filesystem::path& persistence = {}) : engine(MakeConfig())
    {
        engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
        if (!persistence.empty()) engine.SetPersistenceDirectory(persistence.u16string());
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
        scene->GetLicense()->SetCheckPolicy(game::LicenseCheckPolicy::OfflineAuthorized);
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
        if (id == 259) CHECK(script.GetSharedVariable(255) == 1);
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

std::u16string Wide(const std::string& text)
{
    return std::u16string(text.begin(), text.end());
}

std::vector<std::string> ShippedScripts(soft3d::s3dFilePak& archive)
{
    std::vector<std::string> names;
    for (int index = 0; index < archive.GetEntryCount(); ++index) {
        const char* entry = archive.GetEntryName(index);
        CHECK(entry != nullptr);
        std::string name(entry);
        if (name.size() >= 4 && name.compare(name.size() - 4, 4, ".sst") == 0) {
            names.push_back(name.substr(0, name.size() - 4));
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

struct Inventory {
    int scripts = 0;
    int total = 0;
    int generic = 0;
    int game = 0;
    std::set<int> used;
};

void CountCommand(const soft3d::ScriptCommand& command, Inventory& inventory)
{
    if (command.GetId() >= 36) {
        ++inventory.total;
        if (command.GetId() < 43) {
            ++inventory.generic;
        } else {
            ++inventory.game;
            CHECK(command.GetId() <= game::gameSILastCommand);
            inventory.used.insert(command.GetId());
        }
    }
    for (const auto* child : command.GetBody()) CountCommand(*child, inventory);
    for (const auto* child : command.GetConditions()) CountCommand(*child, inventory);
}

void CountFunction(const soft3d::ScriptFunction& function, Inventory& inventory)
{
    for (const auto* command : function.GetEntryCommands()) CountCommand(*command, inventory);
    for (const auto* command : function.GetCommands()) CountCommand(*command, inventory);
}

Inventory ReadInventory(const std::vector<std::string>& names, soft3d::s3dFilePak& archive)
{
    Inventory inventory;
    for (const std::string& name : names) {
        soft3d::SimpleScript script;
        CHECK(script.Init() == soft3d::S3D_OK);
        CHECK(script.LoadFromPak(Wide(name).c_str(), &archive) == soft3d::S3D_OK);
        if (script.GetInitializer() != nullptr) CountFunction(*script.GetInitializer(), inventory);
        for (std::size_t index = 0; index < script.GetFunctionCount(); ++index) {
            CountFunction(script.GetFunction(index), inventory);
        }
        ++inventory.scripts;
    }
    return inventory;
}

std::vector<Argument> BoundsArguments()
{
    return {Argument::Fixed(-0x10000), Argument::Fixed(-0x10000), Argument::Fixed(-0x10000),
            Argument::Fixed(0x10000), Argument::Fixed(0x10000), Argument::Fixed(0x10000)};
}

std::vector<Argument> WithBounds(std::vector<Argument> prefix)
{
    const auto bounds = BoundsArguments();
    prefix.insert(prefix.end(), bounds.begin(), bounds.end());
    return prefix;
}

std::vector<Argument> AddThingArguments(int id, int type)
{
    return {Argument::Short(static_cast<int16_t>(id)), Argument::Int(type), Argument::Int(-1),
        Argument::String("ow_dianju_a"), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(soft3d::fixedOne), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(soft3d::fixedOne), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(soft3d::fixedOne),
        Argument::Bool(true)};
}

void TestUnusedSyntheticScripts(const std::set<int>& unused)
{
    GameFixture fixture;
    std::set<int> covered;
    auto call = [&](int id, const std::vector<Argument>& arguments = {}, bool output = false) {
        if (unused.count(id) != 0) covered.insert(id);
        return fixture.Execute(static_cast<uint16_t>(id), arguments, output);
    };

    fixture.engine.GetControlMap().SetBinding(5, static_cast<uint32_t>(soft3d::Key::Number3));
    fixture.engine.GetInput().KeyDown(soft3d::Key::Number3);
    CHECK(call(43, {Argument::Int(5)}, true) == 1);
    fixture.engine.GetInput().KeyPress(soft3d::Key::Number3);
    CHECK(call(45, {Argument::Int(5)}, true) == 1);
    fixture.engine.GetInput().KeyUp(soft3d::Key::Number3);
    CHECK(call(46, {Argument::Int(5)}, true) == 1);
    CHECK(call(48, {}, true) == 0);
    CHECK(call(49, {}, true) == 1);
    call(62, {Argument::Bool(true)});
    CHECK(fixture.scene->GetUnitManager()->IsPaused());
    call(62, {Argument::Bool(false)});
    call(64, {Argument::Fixed(1), Argument::Fixed(2), Argument::Fixed(3),
              Argument::Fixed(4), Argument::Fixed(5), Argument::Fixed(6)});
    call(65, {Argument::Fixed(0x10000), Argument::Fixed(0x20000), Argument::Fixed(0x30000),
              Argument::Fixed(0x10000), Argument::Fixed(0x40000), Argument::Fixed(0x50000),
              Argument::Fixed(0x100000)}, true);
    call(68);
    call(69);
    call(82);
    call(83);
    call(95, {Argument::Int(3)});
    call(98, {Argument::Short(3), Argument::Bool(true)});
    CHECK(call(99, {Argument::Short(3)}, true) == 1);
    call(104, {Argument::Int(3), Argument::Int(2), Argument::Int(100), Argument::Int(200)});
    call(105, {Argument::Int(3)});
    call(113, {Argument::Fixed(1), Argument::Fixed(2), Argument::Fixed(3),
               Argument::Fixed(4), Argument::Fixed(5), Argument::Fixed(6)});
    call(123, {Argument::Fixed(1), Argument::Fixed(0), Argument::Fixed(0),
               Argument::Fixed(soft3d::fixedOne)}, true);
    CHECK(call(129, {}, true) == 0);
    CHECK(call(136, {}, true) == 0);
    CHECK(call(147, {}, true) == 0);

    call(149, WithBounds({Argument::Short(1), Argument::Int(1), Argument::Short(77)}));
    call(150, {Argument::Short(1), Argument::Int(2)});
    call(151, {Argument::Short(1), Argument::Short(78)});
    call(152, {Argument::Short(1)}, true);
    call(155, WithBounds({Argument::Short(30), Argument::Short(0), Argument::Short(99),
        Argument::Short(4), Argument::Short(2010), Argument::Short(2020),
        Argument::Fixed(1), Argument::Fixed(2), Argument::Fixed(3),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(soft3d::fixedOne)}));
    call(156, {Argument::Short(30), Argument::Short(13)});
    call(157, {Argument::Short(30), Argument::Short(100)});
    call(172, WithBounds({Argument::Short(70), Argument::Bool(false)}));
    call(176, {Argument::Short(70)}, true);

    call(180, {Argument::Short(101), Argument::Int(0), Argument::Short(50),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0),
        Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(soft3d::fixedOne)});
    call(182, {Argument::Short(101), Argument::Bool(false)});
    CHECK(call(186, {Argument::Short(101)}, true) == 50);
    call(188, {Argument::Short(101)}, true);
    call(189, {Argument::Short(101), Argument::Int(30)});
    call(193, {Argument::Short(101), Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)}, true);
    call(196, {Argument::Short(101)});
    call(198, {Argument::Short(1)});
    call(201, {Argument::Short(0), Argument::Bool(true)});
    call(204, WithBounds({Argument::Short(70)}));
    call(205, WithBounds({Argument::Short(70)}));
    call(206, WithBounds({Argument::Short(70)}));
    call(207, {Argument::Short(101), Argument::Short(70)});
    call(208, {Argument::Short(101), Argument::Fixed(7), Argument::Fixed(8), Argument::Fixed(9)});
    call(213, {Argument::Short(101)}, true);
    call(215, {Argument::Short(101), Argument::Short(1), Argument::String("c_biddy_run")});
    call(199);

    auto trap = WithBounds({Argument::Short(20)});
    trap.insert(trap.end(), {Argument::Fixed(0), Argument::Fixed(0), Argument::Fixed(0)});
    call(219, trap);
    call(220, {Argument::Short(20)});
    call(221, AddThingArguments(1, 0));
    call(221, AddThingArguments(2, 1));
    call(227, {Argument::Short(1)}, true);
    call(230, {Argument::Short(1)}, true);
    call(232, {Argument::Short(1)}, true);
    call(235, {Argument::Short(2), Argument::String("ow_dianju_daiji_a")});
    call(236, {Argument::Short(2), Argument::String("t_swordbreak")});
    call(239, {Argument::Short(1), Argument::Short(90)});
    call(248, {Argument::Short(2)}, true);
    call(250, WithBounds({Argument::Short(1)}), true);
    call(258);
    call(259);
    call(260);

    CHECK(covered == unused);
    CHECK(covered.size() == 53);
}

void TestShippedMapLoading(soft3d::s3dFilePak& archive)
{
    std::array<std::vector<std::pair<int, std::string>>, 8> maps;
    for (int index = 0; index < archive.GetEntryCount(); ++index) {
        const char* entry = archive.GetEntryName(index);
        CHECK(entry != nullptr);
        const std::string name(entry);
        const std::size_t dayMarker = name.find("day");
        if (dayMarker == std::string::npos || name.size() < 4 ||
            name.compare(name.size() - 4, 4, ".sbp") != 0) {
            continue;
        }
        const std::size_t separator = name.find_last_of("\\/");
        CHECK(separator != std::string::npos);
        const int day = std::stoi(name.substr(dayMarker + 3, separator - dayMarker - 3));
        const int map = std::stoi(name.substr(separator + 1, name.size() - separator - 5));
        maps.at(static_cast<std::size_t>(day)).push_back({map, name});
    }

    int mapCount = 0;
    int modelCount = 0;
    int cameraCount = 0;
    int lightCount = 0;
    int lensflareCount = 0;
    for (int day = 1; day <= 7; ++day) {
        GameFixture fixture;
        CHECK(fixture.scene->LoadDay(day) == soft3d::S3D_OK);
        for (const auto& entry : maps[static_cast<std::size_t>(day)]) {
            const int map = entry.first;
            const std::string& name = entry.second;
            int progress = 0;
            try {
                CHECK(fixture.scene->LoadMap(map) == 0);
                const std::size_t separator = name.find_last_of("\\/");
                const std::string directory = name.substr(0, separator + 1);
                const std::string file = name.substr(separator + 1);
                int steps = 0;
                while (progress != 100) {
                    const soft3d::result status = soft3d::LoadSbpMapChunks(fixture.scene->GetBsp(),
                        Wide(directory).c_str(), Wide(file).c_str(), &progress);
                    if (status != soft3d::S3D_OK) {
                        throw std::runtime_error("loader status " + std::to_string(status) +
                                                 " at progress " + std::to_string(progress));
                    }
                    CHECK(++steps <= 26);
                }
                CHECK(fixture.scene->GetBsp()->IsLoaded());
                for (const auto& entity : fixture.scene->GetBsp()->GetEntities()) {
                    if (entity != nullptr) {
                        CHECK(dynamic_cast<game::BspUnit*>(entity.get()) != nullptr);
                        ++modelCount;
                    }
                }
                for (soft3d::GameUnit* unit = fixture.scene->GetUnitManager()->GetFirstUnit(6);
                     unit != nullptr; unit = unit->GetNextUnit()) {
                    cameraCount += unit->GetUnitType() == 0x20;
                }
                for (soft3d::GameUnit* unit = fixture.scene->GetUnitManager()->GetFirstUnit(7);
                     unit != nullptr; unit = unit->GetNextUnit()) {
                    lightCount += unit->GetUnitType() == 0x24;
                }
                lensflareCount += static_cast<int>(fixture.scene->GetUnitManager()->GetLensflareCount());
                ++mapCount;
            } catch (const std::exception& error) {
                throw std::runtime_error("shipped map " + name + ": " + error.what());
            }
        }
    }
    CHECK(mapCount == 151);
    CHECK(modelCount > 0);
    if (cameraCount != 802 || lightCount != 29 || lensflareCount != 4) {
        throw std::runtime_error("entity totals: cameras " + std::to_string(cameraCount) +
            ", lights " + std::to_string(lightCount) + ", lensflares " +
            std::to_string(lensflareCount));
    }
}

void TestShippedEpisodeExecution(const std::vector<std::string>& names)
{
    for (const std::string& name : names) {
        std::unique_ptr<GameFixture> fixture;
        try {
            const std::size_t dayMarker = name.find("day");
            const std::size_t episodeMarker = name.find("ep", dayMarker);
            CHECK(dayMarker != std::string::npos && episodeMarker != std::string::npos);
            const int day = std::stoi(name.substr(dayMarker + 3, episodeMarker - dayMarker - 4));
            const int episode = std::stoi(name.substr(episodeMarker + 2));
            fixture = std::make_unique<GameFixture>();
            fixture->engine.SetInGame(true);
            CHECK(fixture->scene->LoadDay(day) == soft3d::S3D_OK);
            fixture->RunFrames(56);
            if (episode != 0) fixture->scene->LoadNextLevel(0, episode);
            fixture->RunFrames(1);
            CHECK(fixture->engine.GetScriptEngine().GetState() == soft3d::ScriptState::Initializing);
            CHECK(fixture->scene->GetScriptName() == Wide(name));
            fixture->RunFrames(120);
        } catch (const std::exception& error) {
            const std::string state = fixture == nullptr ? std::string() :
                "; day " + std::to_string(fixture->scene->GetDay()) + ", map " +
                std::to_string(fixture->scene->GetMapId()) + ", progress " +
                std::to_string(fixture->scene->GetMapProgress()) + ", next episode " +
                std::to_string(fixture->scene->GetNextEpisodeId());
            throw std::runtime_error("shipped episode " + name + state + ": " + error.what());
        }
    }
}

struct TemporaryDirectory {
    explicit TemporaryDirectory(const std::string& name)
    {
        path = std::filesystem::temp_directory_path() / ("soft3d-game-integration-" + name);
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

void TestDailySaveContinuations()
{
    for (int day = 1; day <= 7; ++day) {
        TemporaryDirectory temporary("day" + std::to_string(day));
        GameFixture fixture(temporary.path);
        try {
            fixture.engine.SetInGame(true);
            CHECK(fixture.scene->LoadDay(day) == soft3d::S3D_OK);
            fixture.RunFrames(120);
            const int map = fixture.scene->GetMapId();
            const std::u16string script = fixture.scene->GetScriptName();
            fixture.scene->SetEventFlag(500 + day);
            CHECK(fixture.scene->SaveSaveSlot(1));
            CHECK(std::filesystem::is_regular_file(temporary.path / "slot1.sav"));
            CHECK(std::filesystem::is_regular_file(temporary.path / "spt" / "slot1.sav"));

            fixture.scene->ClearEventFlags();
            CHECK(!fixture.scene->GetEventFlag(500 + day));
            CHECK(fixture.engine.LoadSaveSlot(1));
            CHECK(fixture.engine.IsInGame());
            CHECK(fixture.scene->GetDay() == day);
            CHECK(fixture.scene->GetMapId() == map);
            CHECK(fixture.scene->GetScriptName() == script);
            CHECK(fixture.scene->GetEventFlag(500 + day));
            fixture.RunFrames(40);
            CHECK(fixture.scene->GetMapProgress() == -1);
            CHECK(fixture.scene->GetUnitManager()->IsMapReady());
        } catch (const std::exception& error) {
            throw std::runtime_error("day " + std::to_string(day) + " continuation: " + error.what());
        }
    }

    TemporaryDirectory temporary("final-cycle");
    GameFixture fixture(temporary.path);
    fixture.engine.StartNewGame();
    fixture.RunFrames(62);
    CHECK(fixture.engine.IsInGame());
    fixture.engine.ReturnToMainMenu();
    CHECK(!fixture.engine.IsInGame());
    CHECK(fixture.scene->GetDay() == 0);
    fixture.engine.StartNewGame();
    fixture.RunFrames(62);
    CHECK(fixture.engine.IsInGame());
    CHECK(fixture.scene->GetDay() == 1);
    fixture.engine.ReturnToMainMenu();
    CHECK(!fixture.engine.IsInGame());
}

} // namespace

int main()
{
    try {
        GameFixture fixture;
        auto* archive = fixture.engine.GetResourceManager().GetFilePak();
        const std::vector<std::string> names = ShippedScripts(*archive);
        const Inventory inventory = ReadInventory(names, *archive);
        CHECK(inventory.scripts == 217);
        CHECK(inventory.total == 19251);
        CHECK(inventory.generic == 2261);
        CHECK(inventory.game == 16990);
        CHECK(inventory.used.size() == 162);
        std::set<int> unused;
        for (int id = 43; id <= 257; ++id) {
            if (inventory.used.count(id) == 0) unused.insert(id);
        }
        CHECK(unused.size() == 53);
        TestUnusedSyntheticScripts(unused);
        TestShippedMapLoading(*archive);
        TestShippedEpisodeExecution(names);
        TestDailySaveContinuations();
        std::cout << "game integration tests passed: 217 scripts, 152 maps, 19251 host calls, "
                     "218 callbacks, 7 daily save continuations, final new-game/exit cycle\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

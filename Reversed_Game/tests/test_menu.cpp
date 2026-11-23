#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/MenuBackdrop.hpp"
#include "soft3d/MenuDialogs.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

#define CHECK(condition) Check((condition), __LINE__)

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        path = std::filesystem::temp_directory_path() / "soft3d_stage6_menu_tests";
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory()
    {
        std::filesystem::remove_all(path);
    }

    std::filesystem::path path;
};

std::u16string Wide(const std::string& text)
{
    return std::u16string(text.begin(), text.end());
}

void Click(soft3d::GameEngine& engine, soft3d::Key key)
{
    engine.GetInput().KeyDown(key);
    engine.GetInput().KeyUp(key);
    CHECK(engine.RunFrame());
}

void RunFrames(soft3d::GameEngine& engine, int count)
{
    for (int index = 0; index < count; ++index) {
        CHECK(engine.RunFrame());
    }
}

void RunUntilMode(soft3d::GameEngine& engine, int mode)
{
    for (int count = 0; count < 300 && engine.GetMainMenu()->GetMode() != mode; ++count) {
        CHECK(engine.RunFrame());
    }
    CHECK(engine.GetMainMenu()->GetMode() == mode);
}

bool AnyVoiceActive(soft3d::GameEngine& engine)
{
    for (int voice = 1; voice < 16; ++voice) {
        if (!engine.GetAudioManager().IsVoiceFinished(voice)) {
            return true;
        }
    }
    return false;
}

void TestConfigurationBytes()
{
    soft3d::GameConfiguration source;
    for (size_t index = 0; index < source.controlBindings.size(); ++index) {
        source.controlBindings[index] = static_cast<uint32_t>(index + 2);
    }
    source.masterVolume = 128;
    source.vibrationEnabled = 1;
    for (size_t index = 0; index < soft3d::gameUnlockCount; ++index) {
        source.unlocks.models[index] = static_cast<uint32_t>(index + 10);
        source.unlocks.documents[index] = static_cast<uint32_t>(index + 110);
        source.unlocks.photos[index] = static_cast<uint32_t>(index + 210);
    }
    source.unlocks.extraWeapon0 = 1;
    source.unlocks.extraWeapon1 = 2;
    source.unlocks.newGameOptions = 3;

    const auto bytes = soft3d::SerializeGameConfiguration(source);
    CHECK(bytes.size() == 1252);
    auto word = [&](size_t offset) {
        uint32_t value;
        std::memcpy(&value, bytes.data() + offset, 4);
        return value;
    };
    CHECK(word(0) == 2 && word(28) == 9);
    CHECK(word(32) == 128 && word(36) == 1);
    CHECK(word(40) == 10 && word(436) == 109);
    CHECK(word(440) == 110 && word(836) == 209);
    CHECK(word(840) == 210 && word(1236) == 309);
    CHECK(word(1240) == 1 && word(1244) == 2 && word(1248) == 3);

    soft3d::GameConfiguration loaded;
    CHECK(soft3d::DeserializeGameConfiguration(bytes.data(), bytes.size(), loaded) == soft3d::S3D_OK);
    CHECK(loaded.controlBindings == source.controlBindings);
    CHECK(loaded.masterVolume == 128 && loaded.vibrationEnabled == 1);
    CHECK(loaded.unlocks.models == source.unlocks.models);
    CHECK(loaded.unlocks.documents == source.unlocks.documents);
    CHECK(loaded.unlocks.photos == source.unlocks.photos);
    CHECK(loaded.unlocks.extraWeapon0 == 1 && loaded.unlocks.extraWeapon1 == 2);
    CHECK(loaded.unlocks.newGameOptions == 3);
    CHECK(soft3d::DeserializeGameConfiguration(bytes.data(), bytes.size() - 1, loaded) ==
          soft3d::S3D_FORMAT);
}

void CreateSlot(const TemporaryDirectory& directory, int slot, uint32_t day, uint32_t playTime)
{
    std::array<uint32_t, 11> words{};
    words[0] = day;
    words[5] = playTime;
    std::ofstream output(directory.path / ("slot" + std::to_string(slot) + ".sav"),
                         std::ios::binary);
    output.write(reinterpret_cast<const char*>(words.data()), sizeof(words));
    CHECK(output.good());
}

void TestMenuFlow(const TemporaryDirectory& directory)
{
    soft3d::EngineConfig config;
    config.width = 320;
    config.height = 240;
    config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    soft3d::GameEngine engine(std::move(config));
    engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
    engine.SetPersistenceDirectory(Wide(directory.path.string()));

    int newGames = 0;
    int loadedSlot = 0;
    int modelFrames = 0;
    int mapFrames = 0;
    soft3d::NewGameOptions lastOptions;
    soft3d::GameplayHooks hooks;
    hooks.startNewGame = [&](const soft3d::NewGameOptions& options) {
        ++newGames;
        lastOptions = options;
    };
    hooks.loadGame = [&](int slot) {
        loadedSlot = slot;
        return true;
    };
    hooks.renderAboutModel = [&](int model, int, int, int) {
        CHECK(model == 1);
        ++modelFrames;
    };
    hooks.renderMap = [&](int) { ++mapFrames; };
    engine.SetGameplayHooks(hooks);
    CreateSlot(directory, 1, 1, 1234);

    CHECK(engine.Init() == soft3d::S3D_OK);
    CHECK(engine.GetUiManager()->GetDialogCount() == 7);
    CHECK(dynamic_cast<soft3d::MainUIDlg*>(engine.GetUiManager()->GetDialogRuntime(0)) != nullptr);
    CHECK(dynamic_cast<soft3d::OptionDlg*>(engine.GetUiManager()->GetDialogRuntime(1)) != nullptr);
    CHECK(dynamic_cast<soft3d::AboutDlg*>(engine.GetUiManager()->GetDialogRuntime(2)) != nullptr);
    CHECK(dynamic_cast<soft3d::HelpDlg*>(engine.GetUiManager()->GetDialogRuntime(3)) != nullptr);
    CHECK(dynamic_cast<soft3d::PlayDlg*>(engine.GetUiManager()->GetDialogRuntime(4)) != nullptr);
    CHECK(dynamic_cast<soft3d::LoadDlg*>(engine.GetUiManager()->GetDialogRuntime(5)) != nullptr);
    CHECK(dynamic_cast<soft3d::GameUIDlg*>(engine.GetUiManager()->GetDialogRuntime(6)) != nullptr);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::Handedness);
    engine.Start();

    Click(engine, soft3d::Key::Right);
    CHECK(engine.GetMainMenu()->GetSelection() == 1);
    CHECK(engine.GetControlMap().GetOrientation() == 1);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetControlMap().GetOrientation() == 2);
    RunUntilMode(engine, soft3d::MainUIDlg::SoundPrompt);
    CHECK(engine.GetSaveSlots()[0].present);
    CHECK(engine.GetSaveSlots()[0].GetDay() == 1);
    CHECK(engine.GetSaveSlots()[0].GetSaveNumber() == 1234);

    Click(engine, soft3d::Key::Right);
    CHECK(engine.GetMainMenu()->IsSoundEnabled());
    CHECK(AnyVoiceActive(engine));
    Click(engine, soft3d::Key::Down);
    CHECK(!engine.GetMainMenu()->IsSoundEnabled());
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::Authorization);
    CHECK(engine.GetMainMenu()->GetAuthorizationState() == 0);
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetMainMenu()->GetAuthorizationState() == 2);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);
    CHECK(engine.GetAudioManager().GetDevice().GetMasterVolume() == 1);
    CHECK(engine.GetMenuBackdrop()->IsLoaded());

    Click(engine, soft3d::Key::Left);
    CHECK(engine.GetMainMenu()->GetSelection() == 1);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetUiManager()->GetDialogRuntime(1)->IsActive());
    Click(engine, soft3d::Key::Down);
    CHECK(engine.GetAudioManager().GetDevice().GetMasterVolume() == 4);
    Click(engine, soft3d::Key::Left);
    Click(engine, soft3d::Key::Left);
    Click(engine, soft3d::Key::Up);
    CHECK(engine.GetConfiguration().vibrationEnabled == 1);
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);
    CHECK(engine.GetMainMenu()->GetSelection() == 1);

    engine.GetConfiguration().unlocks.models[1] = 1;
    engine.GetConfiguration().unlocks.documents[1] = 1;
    engine.GetConfiguration().unlocks.photos[1] = 1;
    engine.ShowMainMenu(2);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetUiManager()->GetDialogRuntime(2)->IsActive());
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetAboutDialog()->GetPage() == 1);
    CHECK(engine.GetAboutDialog()->GetModelIndex() == 1);
    CHECK(engine.RunFrame());
    CHECK(modelFrames > 0);
    Click(engine, soft3d::Key::SoftRight);
    Click(engine, soft3d::Key::Down);
    CHECK(engine.GetAboutDialog()->GetPage() == 2);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetAboutDialog()->GetPage() == 3);
    CHECK(!engine.GetAboutDialog()->IsPhotoMode());
    CHECK(engine.GetAboutDialog()->GetTextIndex() == 1);
    Click(engine, soft3d::Key::Down);
    CHECK(engine.GetAboutDialog()->IsPhotoMode());
    Click(engine, soft3d::Key::Up);
    CHECK(!engine.GetAboutDialog()->IsPhotoMode());
    Click(engine, soft3d::Key::SoftRight);
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);

    engine.ShowMainMenu(3);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetUiManager()->GetDialogRuntime(3)->IsActive());
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetMainMenu()->GetMode() == soft3d::MainUIDlg::MainMenu);
    engine.ActivateMenuDialog(3);
    static_cast<soft3d::HelpDlg*>(engine.GetUiManager()->GetDialogRuntime(3))->ShowStatistics();
    engine.GetContent().statistics.elapsedSeconds = 3661;
    CHECK(engine.RunFrame());

    engine.ActivateMenuDialog(4);
    RunFrames(engine, 2);
    Click(engine, soft3d::Key::Accept);
    CHECK(newGames == 1);
    CHECK(engine.GetUiManager()->GetDialogRuntime(6)->IsActive());
    CHECK(lastOptions.weapon == 0 && lastOptions.outfit == 0);

    engine.GetGameUi()->OpenMenu();
    RunFrames(engine, 12);
    CHECK(engine.GetGameUi()->GetMode() == 1);
    Click(engine, soft3d::Key::Right);
    CHECK(engine.GetGameUi()->GetSelection() == 3);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetGameUi()->GetMode() == 5);
    Click(engine, soft3d::Key::Down);
    CHECK(engine.GetAudioManager().GetDevice().GetMasterVolume() == 8);
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetGameUi()->GetMode() == 1);
    Click(engine, soft3d::Key::SoftRight);
    RunFrames(engine, 12);
    CHECK(!engine.GetGameUi()->IsMenuOpen());

    engine.GetContent().inventory[0] = 1;
    engine.GetGameUi()->OpenMenu();
    RunFrames(engine, 12);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetGameUi()->GetMode() == 2);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetGameUi()->GetMode() == 10);
    Click(engine, soft3d::Key::Right);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetGameUi()->GetMode() == 6);
    Click(engine, soft3d::Key::SoftRight);
    CHECK(engine.GetGameUi()->GetMode() == 2);
    Click(engine, soft3d::Key::SoftRight);
    Click(engine, soft3d::Key::SoftRight);
    RunFrames(engine, 12);

    engine.GetGameUi()->OpenMenu();
    RunFrames(engine, 12);
    Click(engine, soft3d::Key::Left);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetGameUi()->GetMode() == 3);
    CHECK(engine.RunFrame());
    CHECK(mapFrames > 0);
    Click(engine, soft3d::Key::SoftRight);
    RunFrames(engine, 12);

    engine.ShowMainMenu(0);
    Click(engine, soft3d::Key::Accept);
    CHECK(engine.GetUiManager()->GetDialogRuntime(5)->IsActive());
    Click(engine, soft3d::Key::Accept);
    CHECK(loadedSlot == 1);
    CHECK(engine.GetUiManager()->GetDialogRuntime(6)->IsActive());

    engine.GetConfiguration().controlBindings[1] = 12;
    engine.GetConfiguration().unlocks.extraWeapon0 = 1;
    engine.GetConfiguration().unlocks.extraWeapon1 = 1;
    engine.GetConfiguration().unlocks.newGameOptions = 1;
    engine.SaveGameState();
    engine.GetConfiguration() = soft3d::GameConfiguration{};
    engine.LoadGameState();
    CHECK(engine.GetConfiguration().controlBindings[1] == 12);
    CHECK(engine.GetConfiguration().masterVolume == 8);
    CHECK(engine.GetConfiguration().vibrationEnabled == 1);
    CHECK(engine.GetConfiguration().unlocks.models[1] == 1);
    CHECK(engine.GetConfiguration().unlocks.documents[1] == 1);
    CHECK(engine.GetConfiguration().unlocks.photos[1] == 1);
    CHECK(engine.GetConfiguration().unlocks.extraWeapon0 == 1);
    CHECK(engine.GetConfiguration().unlocks.extraWeapon1 == 1);
    CHECK(engine.GetConfiguration().unlocks.newGameOptions == 1);
    CHECK(std::filesystem::file_size(directory.path / "config.sdt") == 1252);
}

} // namespace

int main()
{
    try {
        TestConfigurationBytes();
        TemporaryDirectory directory;
        TestMenuFlow(directory);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << "Menu flow, concrete dialogs, and configuration tests passed\n";
    return 0;
}

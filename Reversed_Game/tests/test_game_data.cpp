#include "game/GameData.hpp"
#include "game/MyGameUnitMgr.hpp"
#include "platform/dummy/DummyVideoBackend.hpp"
#include "soft3d/GameEngine.hpp"
#include "soft3d/MenuDialogs.hpp"
#include "soft3d/s3dFilePak.hpp"
#include "soft3d/s3dIO.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
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

struct TestArchive {
    TestArchive()
    {
        CHECK(filePak.Open(SOFT3D_TEST_ARCHIVE, 0) == soft3d::S3D_OK);
    }

    soft3d::s3dFilePak filePak;
};

std::vector<uint8_t> ReadEntry(soft3d::s3dFilePak& filePak, const char16_t* name)
{
    CHECK(filePak.Select(name) == soft3d::S3D_OK);
    CHECK(filePak.Seek(0, SEEK_END) == soft3d::S3D_OK);
    const int size = filePak.Tell();
    CHECK(size >= 0);
    CHECK(filePak.Seek(0, SEEK_SET) == soft3d::S3D_OK);
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    if (size != 0) {
        CHECK(filePak.Read(data.data(), size) == soft3d::S3D_OK);
    }
    return data;
}

std::vector<uint8_t> ReadDayEntry(soft3d::s3dFilePak& filePak, const char16_t* prefix, int day)
{
    char16_t name[64];
    soft3d::WideSprintf(name, prefix, day);
    return ReadEntry(filePak, name);
}

void Write32(std::vector<uint8_t>& data, std::size_t offset, int32_t value)
{
    CHECK(offset + sizeof(value) <= data.size());
    std::memcpy(data.data() + offset, &value, sizeof(value));
}

void Append32(std::vector<uint8_t>& data, int32_t value)
{
    const std::size_t offset = data.size();
    data.resize(offset + sizeof(value));
    Write32(data, offset, value);
}

std::u16string Wide(const char* text)
{
    std::u16string result;
    while (*text != '\0') {
        result.push_back(static_cast<unsigned char>(*text++));
    }
    return result;
}

void TestResourceNames()
{
    game::GameData data;
    CHECK(Wide("r_ken_stand_a") == data.GetAnimationName(0));
    CHECK(Wide("r_xiyangjian_zhong_c") == data.GetAnimationName(17));
    CHECK(Wide("r_pangnvyong_walk_a") == data.GetAnimationName(123));
    CHECK(Wide("r_nanjue_tengkong_a") == data.GetAnimationName(149));
    CHECK(Wide("ow_dajian_yingzhi_a") == data.GetAnimationName(169));
    CHECK(Wide("r_bingnvyong_walk_a") == data.GetAnimationName(185));
    CHECK(Wide("r_bug_run") == data.GetAnimationName(191));
    CHECK(Wide("r_qiutu_walk_a") == data.GetAnimationName(199));
    CHECK(Wide("r_victor_leapattack_c") == data.GetAnimationName(212));
    CHECK(Wide("ow_dianju_zhendi_a") == data.GetAnimationName(233));
    CHECK(Wide("r_sam_walk_a") == data.GetAnimationName(234));
    CHECK(Wide("r_kate_run_a") == data.GetAnimationName(237));
    CHECK(Wide("oscj_logo_aa") == data.GetAnimationName(238));
    CHECK(Wide("oscj_door_ra") == data.GetAnimationName(259));
    CHECK(data.GetAnimationName(260) == nullptr);
    CHECK(*data.GetAnimationName(128) == u'\0');
    CHECK(*data.GetAnimationName(200) == u'\0');

    CHECK(Wide("ui_select") == data.GetSoundName(0));
    CHECK(Wide("c_damnedken_whistle") == data.GetSoundName(27));
    CHECK(Wide("c_ghost_walk02") == data.GetSoundName(46));
    CHECK(Wide("c_weed_walk") == data.GetSoundName(57));
    CHECK(Wide("c_baron_walk01") == data.GetSoundName(94));
    CHECK(Wide("c_bully_attack_a") == data.GetSoundName(109));
    CHECK(Wide("c_victor_stand") == data.GetSoundName(118));
    CHECK(Wide("a_getitem") == data.GetSoundName(140));
    CHECK(Wide("a_gunlevelup") == data.GetSoundName(141));
    CHECK(data.GetSoundName(142) == nullptr);
    CHECK(data.GetSoundNameTable() + 140 * game::resourceNameCapacity == data.GetSoundName(140));
    CHECK(*data.GetSoundName(31) == u'\0');

    static constexpr int openSounds[] = {
        0, 10, 10, 11, 12, 14, 18, 10, 13, 19, 20,
        12, 12, 11, 13, 14, 11, 11, 12, 12, 11, 10,
    };
    static constexpr int closeSounds[] = {
        0, 15, 15, 16, 17, 0, 0, 15, 15, 0, 0,
        17, 17, 16, 15, 0, 16, 16, 17, 17, 16, 15,
    };
    for (std::size_t index = 0; index < game::doorResourceCount; ++index) {
        const auto& door = data.GetDoorResource(index);
        CHECK(door.openSoundIndex == openSounds[index]);
        CHECK(door.closeSoundIndex == closeSounds[index]);
        CHECK(std::u16string(door.modelName.data()) == data.GetAnimationName(238 + index));
    }
    CHECK(Wide("oscj_door_ra") == data.GetDoorResource(21).modelName.data());
}

void TestRealItemData(TestArchive& archive)
{
    const auto data = ReadEntry(archive.filePak, u".\\common\\itemdata.sdt");
    CHECK(data.size() == 4272);
    soft3d::GameContent content;
    content.inventory[0] = 42;
    content.noteIds[0] = 43;
    content.statistics.filesFound = 44;
    content.mapId = 45;
    CHECK(game::GameData::ParseItemData(data.data(), data.size(), content) == soft3d::S3D_OK);

    std::size_t itemCount = 0;
    for (const auto& item : content.items) {
        itemCount += !item.modelName.empty();
    }
    CHECK(itemCount == 67);
    std::size_t textCount = 0;
    for (const auto& text : content.documents) {
        textCount += text.titleString != 0 || text.bodyString != 0;
    }
    for (const auto& text : content.photos) {
        textCount += text.titleString != 0 || text.bodyString != 0;
    }
    CHECK(textCount == 65);

    CHECK(content.items[1].nameString == 250);
    CHECK(content.items[1].descriptionString == 251);
    CHECK(content.items[1].modelName == Wide("oi_yaoping_a"));
    CHECK(content.items[17].combineWithItemId == 18);
    CHECK(content.items[17].combinedItemId == 19);
    CHECK(content.items[18].combineWithItemId == 17);
    CHECK(content.items[18].combinedItemId == 19);
    CHECK(content.items[77].nameString == 380);
    CHECK(content.models[1].nameString == 250);
    CHECK(content.models[1].descriptionString == 251);
    CHECK(content.models[1].modelName == content.items[1].modelName);
    CHECK(content.models[1].previewZoom == 0x3333);
    CHECK(content.documents[1].titleString == 50);
    CHECK(content.documents[1].bodyString == 51);
    CHECK(content.documents[58].titleString == 143);
    CHECK(content.documents[58].bodyString == 144);
    CHECK(content.photos[1].titleString == 150);
    CHECK(content.photos[1].bodyString == 1);
    CHECK(content.photos[28].titleString == 165);
    CHECK(content.photos[28].bodyString == 0);
    CHECK(content.inventory[0] == 42);
    CHECK(content.noteIds[0] == 43);
    CHECK(content.statistics.filesFound == 44);
    CHECK(content.mapId == 45);

    const std::array<std::size_t, 8> truncations = {0, 3, 4, 55, 3487, 3488, 3491, 4271};
    for (std::size_t size : truncations) {
        CHECK(game::GameData::ParseItemData(data.data(), size, content) == soft3d::S3D_FORMAT);
        CHECK(content.items[1].nameString == 250);
    }
    CHECK(game::GameData::ParseItemData(nullptr, 1, content) == soft3d::S3D_PARAM);

    auto malformed = data;
    Write32(malformed, 0, -1);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 0, 67);
    Write32(malformed, 4, 100);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    malformed = data;
    std::memset(malformed.data() + 12, 'a', 32);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    malformed = data;
    Write32(malformed, 3488, -1);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 3488, 65);
    Write32(malformed, 3492, 999);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 3492, 2100);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
    malformed.push_back(0);
    CHECK(game::GameData::ParseItemData(malformed.data(), malformed.size(), content) ==
          soft3d::S3D_FORMAT);
}

void TestRealGameItemData(TestArchive& archive)
{
    static constexpr int groupCounts[] = {9, 13, 13, 18, 14, 4, 3};
    for (int day = 1; day <= 7; ++day) {
        const auto data = ReadDayEntry(archive.filePak, u".\\common\\gameitem%d.sdt", day);
        std::vector<game::GameItemGroup> groups;
        CHECK(game::GameData::ParseGameItemData(data.data(), data.size(), groups) == soft3d::S3D_OK);
        CHECK(groups.size() == static_cast<std::size_t>(groupCounts[day - 1]));
    }

    const auto day1 = ReadEntry(archive.filePak, u".\\common\\gameitem1.sdt");
    std::vector<game::GameItemGroup> groups;
    CHECK(game::GameData::ParseGameItemData(day1.data(), day1.size(), groups) == soft3d::S3D_OK);
    CHECK(groups[0].mapId == 1010);
    CHECK(groups[0].records.size() == 1);
    const auto& record = groups[0].records[0];
    CHECK(record.itemId == 1038);
    CHECK(record.pickupSoundIndex == 26);
    CHECK(record.position.x == 1568250);
    CHECK(record.position.y == -1886696);
    CHECK(record.position.z == 14811136);
    CHECK(record.axisX.z == -65536);
    CHECK(record.axisZ.x == 32768);
    CHECK(record.pickupBounds.minimum.x == 1376256);
    CHECK(record.pickupBounds.maximum.z == 15007744);
    CHECK(record.packedModelName[0] != 0);

    const auto day4 = ReadEntry(archive.filePak, u".\\common\\gameitem4.sdt");
    CHECK(game::GameData::ParseGameItemData(day4.data(), day4.size(), groups) == soft3d::S3D_OK);
    CHECK(groups[4].mapId == 3009 && groups[13].mapId == 3009);
    CHECK(groups[4].records[0].position.x == 2463898);
    CHECK(groups[13].records[0].position.x == 2589117);

    auto malformed = day1;
    Write32(malformed, 0, -1);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 0, 9);
    Write32(malformed, 4, -1);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 4, 1010);
    Write32(malformed, 8, -1);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 8, 1);
    Write32(malformed, 12, 2100);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 12, 1038);
    Write32(malformed, 16, 142);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    CHECK(game::GameData::ParseGameItemData(day1.data(), day1.size() - 1, groups) ==
          soft3d::S3D_FORMAT);
    CHECK(game::GameData::ParseGameItemData(day1.data(), 11, groups) == soft3d::S3D_FORMAT);
    malformed = day1;
    malformed.push_back(0);
    CHECK(game::GameData::ParseGameItemData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    CHECK(game::GameData::ParseGameItemData(nullptr, 1, groups) == soft3d::S3D_PARAM);
}

void TestRealEnemyInfoData(TestArchive& archive)
{
    static constexpr int groupCounts[] = {0, 4, 9, 1, 5};
    for (int day = 1; day <= 5; ++day) {
        const auto data = ReadDayEntry(archive.filePak, u".\\common\\einfo%d.sdt", day);
        std::vector<game::EnemyInfoGroup> groups;
        CHECK(game::GameData::ParseEnemyInfoData(data.data(), data.size(), groups) == soft3d::S3D_OK);
        CHECK(groups.size() == static_cast<std::size_t>(groupCounts[day - 1]));
    }

    const auto day2 = ReadEntry(archive.filePak, u".\\common\\einfo2.sdt");
    std::vector<game::EnemyInfoGroup> groups;
    CHECK(game::GameData::ParseEnemyInfoData(day2.data(), day2.size(), groups) == soft3d::S3D_OK);
    CHECK(groups[1].mapId == 2001);
    const auto& record = groups[1].records[0];
    CHECK(record.enemyType == 0);
    CHECK(record.active == -1);
    CHECK(record.unitId == -1);
    CHECK(record.position.x == 1400596);
    CHECK(record.position.y == -2228224);
    CHECK(record.direction.z == 0);
    CHECK(record.waypoints.size() == 3);
    CHECK(record.waypoints[0].position.x == 1396296);
    CHECK(record.waypoints[0].waitTime == 20);
    CHECK(record.waypoints[0].direction.x == -65536);
    CHECK(record.waypoints[2].waitTime == 20);
    CHECK(record.waypoints[2].direction.z == 65536);

    const auto day5 = ReadEntry(archive.filePak, u".\\common\\einfo5.sdt");
    CHECK(game::GameData::ParseEnemyInfoData(day5.data(), day5.size(), groups) == soft3d::S3D_OK);
    CHECK(groups[0].mapId == 1052);
    CHECK(groups[0].records.size() == 4);
    CHECK(groups[0].records[0].enemyType == 10);
    CHECK(groups[0].records[1].waypoints.size() == 3);
    CHECK(groups[0].records[1].waypoints[2].waitTime == -1);
    CHECK(groups[2].mapId == 103 && groups[2].records.size() == 5);

    auto malformed = day2;
    Write32(malformed, 0, 101);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 0, 4);
    Write32(malformed, 8, 33);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 8, 1);
    Write32(malformed, 40, -1);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 40, 3);
    Write32(malformed, 40, 0x7fffffff);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    Write32(malformed, 40, 3);
    Write32(malformed, 12, 32768);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    CHECK(game::GameData::ParseEnemyInfoData(day2.data(), day2.size() - 1, groups) ==
          soft3d::S3D_FORMAT);
    malformed = day2;
    malformed.push_back(0);
    CHECK(game::GameData::ParseEnemyInfoData(malformed.data(), malformed.size(), groups) ==
          soft3d::S3D_FORMAT);
    CHECK(game::GameData::ParseEnemyInfoData(nullptr, 1, groups) == soft3d::S3D_PARAM);

    std::vector<uint8_t> oversized;
    Append32(oversized, 1);
    Append32(oversized, 1);
    Append32(oversized, 33);
    CHECK(game::GameData::ParseEnemyInfoData(oversized.data(), oversized.size(), groups) ==
          soft3d::S3D_FORMAT);
}

void TestDayLoadingAndMissingFiles(TestArchive& archive)
{
    game::GameData data;
    for (int repeat = 0; repeat < 2; ++repeat) {
        for (int day = 1; day <= 7; ++day) {
            CHECK(data.LoadDayContent(day, archive.filePak) == soft3d::S3D_OK);
        }
    }
    CHECK(data.GetGameItemGroups().size() == 3);
    CHECK(data.GetEnemyInfoGroups().empty());
    CHECK(data.FindGameItemGroup(1014) != nullptr);
    CHECK(data.FindEnemyInfoGroup(2001) == nullptr);
    CHECK(data.LoadDayContent(2, archive.filePak) == soft3d::S3D_OK);
    CHECK(data.GetEnemyInfoGroups().size() == 4);
    CHECK(data.FindEnemyInfoGroup(2001) != nullptr);
    CHECK(data.LoadDayContent(6, archive.filePak) == soft3d::S3D_OK);
    CHECK(data.GetGameItemGroups().size() == 4);
    CHECK(data.GetEnemyInfoGroups().empty());
    CHECK(data.LoadDayContent(0, archive.filePak) == soft3d::S3D_PARAM);
    CHECK(data.LoadDayContent(8, archive.filePak) == soft3d::S3D_PARAM);

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "soft3d_stage1_empty.spk";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        const int16_t count = 1;
        output.write(reinterpret_cast<const char*>(&count), sizeof(count));
        std::array<char, 64> name{};
        std::memcpy(name.data(), ".\\common\\unused.sdt", 20);
        output.write(name.data(), name.size());
        const uint32_t offset = 2 + 64 + 4;
        output.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
        const uint32_t zero = 0;
        output.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
        CHECK(output.good());
    }
    soft3d::s3dFilePak missing;
    CHECK(missing.Open(path.u16string().c_str(), 0) == soft3d::S3D_OK);
    soft3d::GameContent content;
    CHECK(data.LoadItemData(missing, content) == soft3d::S3D_FILE);
    CHECK(data.LoadDayContent(1, missing) == soft3d::S3D_FILE);
    CHECK(data.LoadDayContent(6, missing) == soft3d::S3D_FILE);
    missing.Close();
    std::filesystem::remove(path);
}

void TestEngineIntegration()
{
    soft3d::EngineConfig config;
    config.width = 320;
    config.height = 240;
    config.videoBackend = std::make_unique<soft3d::platform::DummyVideoBackend>();
    soft3d::GameEngine engine(std::move(config));
    engine.SetResourceArchive(SOFT3D_TEST_ARCHIVE);
    CHECK(engine.Init() == soft3d::S3D_OK);
    game::MyGameUnitMgr manager;
    CHECK(manager.Initialize(engine) == soft3d::S3D_OK);
    soft3d::GameplayHooks hooks;
    hooks.startNewGame = [&](const soft3d::NewGameOptions&) {
        CHECK(manager.LoadDayContent(1, engine) == soft3d::S3D_OK);
    };
    engine.SetGameplayHooks(std::move(hooks));
    engine.GetConfiguration().unlocks.models[1] = 1;
    engine.GetConfiguration().unlocks.documents[1] = 1;
    engine.GetConfiguration().unlocks.photos[1] = 1;
    engine.Start();
    engine.StartNewGame();
    CHECK(engine.RunFrame());

    CHECK(engine.GetContent().items[1].descriptionString == 251);
    CHECK(engine.GetContent().models[1].modelName == Wide("oi_yaoping_a"));
    CHECK(engine.GetContent().documents[1].bodyString == 51);
    CHECK(engine.GetContent().photos[1].titleString == 150);
    CHECK(engine.GetConfiguration().unlocks.models[1] == 1);
    CHECK(engine.GetConfiguration().unlocks.documents[1] == 1);
    CHECK(engine.GetConfiguration().unlocks.photos[1] == 1);
    CHECK(manager.GetData().GetGameItemGroups().size() == 9);
    CHECK(manager.GetData().GetEnemyInfoGroups().empty());
    CHECK(manager.GetData().FindGameItemGroup(1010)->records[0].itemId == 1038);
    CHECK(engine.GetAudioManager().PlaySound(140, false) >= 0);

    engine.ShowMainMenu(2);
    engine.ActivateMenuDialog(2);
    engine.GetInput().KeyDown(soft3d::Key::Accept);
    engine.GetInput().KeyUp(soft3d::Key::Accept);
    CHECK(engine.RunFrame());
    CHECK(engine.GetAboutDialog()->GetPage() == 1);
    CHECK(engine.GetAboutDialog()->GetModelIndex() == 1);
}

} // namespace

int main()
{
    try {
        TestArchive archive;
        TestResourceNames();
        TestRealItemData(archive);
        TestRealGameItemData(archive);
        TestRealEnemyInfoData(archive);
        TestDayLoadingAndMissingFiles(archive);
        TestEngineIntegration();
        std::cout << "game data tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

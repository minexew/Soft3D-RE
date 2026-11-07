#include "soft3d/Animation.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/TextureRes.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, int line)
{
    if (!condition) {
        throw std::runtime_error("check failed at line " + std::to_string(line));
    }
}

#define CHECK(condition) Check((condition), __LINE__)

void Append16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(std::vector<uint8_t>& bytes, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>(value >> shift));
    }
}

struct EntrySpec {
    std::string name;
    std::vector<uint8_t> data;
};

std::vector<uint8_t> MakeArchive(const std::vector<EntrySpec>& entries)
{
    uint32_t nextOffset = static_cast<uint32_t>(2 + entries.size() * 0x44);
    std::vector<uint8_t> bytes;
    Append16(bytes, static_cast<uint16_t>(entries.size()));
    for (const EntrySpec& entry : entries) {
        std::array<char, 64> name{};
        std::copy(entry.name.begin(), entry.name.end(), name.begin());
        bytes.insert(bytes.end(), name.begin(), name.end());
        Append32(bytes, nextOffset);
        nextOffset += static_cast<uint32_t>(entry.data.size());
    }
    for (const EntrySpec& entry : entries) {
        bytes.insert(bytes.end(), entry.data.begin(), entry.data.end());
    }
    return bytes;
}

class TempArchive {
public:
    explicit TempArchive(const std::vector<uint8_t>& bytes)
        : path(std::filesystem::temp_directory_path() / "soft3d-resmanager-tests.spk")
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        output.close();
        CHECK(output.good());
    }

    ~TempArchive()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    std::filesystem::path path;
};

std::vector<uint8_t> MakeStx()
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 0x00031200);
    Append32(bytes, 2);
    Append32(bytes, 2);
    bytes.insert(bytes.end(), {10, 20, 30, 40});
    return bytes;
}

std::vector<uint8_t> MakeSoj()
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 1);
    const std::array<uint32_t, 13> object = {
        0x10000, 0x20000, 0x04ff8020, 7, 9, 10, 11,
        12, 13, 14, 15, 0, 1
    };
    for (uint32_t value : object) {
        Append32(bytes, value);
    }
    Append32(bytes, 1);
    Append32(bytes, 22);
    Append32(bytes, 23);
    Append32(bytes, 30);
    Append32(bytes, 4);
    Append32(bytes, 2);
    Append32(bytes, 32);
    Append32(bytes, 4);
    Append32(bytes, 8);
    Append32(bytes, 0);
    Append32(bytes, 0x05040302);
    Append32(bytes, 0x09080706);
    Append32(bytes, 1);
    Append32(bytes, 8);
    Append32(bytes, 0x00010000);
    Append32(bytes, 0x00030002);
    for (uint32_t value = 40; value < 46; ++value) {
        Append32(bytes, value);
    }
    return bytes;
}

std::vector<uint8_t> MakeSai()
{
    std::vector<uint8_t> bytes;
    Append32(bytes, 0x111);
    Append32(bytes, 0x222);
    Append32(bytes, 0x101);
    for (uint32_t value = 1; value <= 6; ++value) {
        Append32(bytes, value);
    }
    Append16(bytes, 1);
    Append16(bytes, 0);
    Append32(bytes, 1);
    Append32(bytes, 100);
    Append16(bytes, 0x101);
    for (uint16_t value = 1; value <= 9; ++value) {
        Append16(bytes, value);
    }
    Append32(bytes, 10);
    Append32(bytes, 11);
    Append32(bytes, 12);
    Append16(bytes, 2);
    Append16(bytes, 1);
    Append32(bytes, 77);
    for (uint32_t value = 50; value < 56; ++value) {
        Append32(bytes, value);
    }
    return bytes;
}

std::vector<EntrySpec> SyntheticEntries()
{
    return {
        {".\\common\\anim.sai", MakeSai()},
        {".\\common\\model.soj", MakeSoj()},
        {".\\common\\tex.stx", MakeStx()},
        {".\\common\\tone.sau", std::vector<uint8_t>(8, 0)},
        {".\\custom\\tone.sau", std::vector<uint8_t>(4, 0)},
    };
}

void TestSyntheticResourcesAndCache()
{
    TempArchive file(MakeArchive(SyntheticEntries()));
    soft3d::s3dResManager manager;
    CHECK(manager.GetResource(soft3d::ResourceAudio, 0, u"tone") == nullptr);
    soft3d::OpenGameResourceArchive(&manager, file.path.u16string().c_str());
    CHECK(soft3d::GetFilePakFromManager(&manager) == manager.GetFilePak());
    CHECK(manager.GetFilePak()->GetEntryCount() == 5);

    CHECK(manager.GetResource(soft3d::ResourceAudio, 0, nullptr) == nullptr);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 0, u"") == nullptr);
    CHECK(manager.GetResource(99, 0, u"tone") == nullptr);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 0, u"missing") == nullptr);

    auto* audio = dynamic_cast<soft3d::AudioData*>(
        manager.GetResource(soft3d::ResourceAudio, 3, u"tone"));
    CHECK(audio != nullptr);
    CHECK(audio->GetPcmSampleCount() == 16);
    CHECK(audio->GetPcmBuffer() != nullptr);
    CHECK(std::all_of(audio->GetPcmBuffer(), audio->GetPcmBuffer() + 16,
                      [](int16_t sample) { return sample == 0; }));
    CHECK(std::u16string(audio->GetPath()) == u".\\common\\tone.sau");
    CHECK(std::u16string(audio->GetFileName()) == u"tone");
    CHECK(std::u16string(audio->GetDirectory()) == u".\\common\\");
    CHECK(manager.GetResource(soft3d::ResourceAudio, 3, u"tone") == audio);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 1, u"tone") == audio);

    auto* custom = dynamic_cast<soft3d::AudioData*>(
        manager.GetResource(soft3d::ResourceAudio, 2, u"tone", u"custom"));
    CHECK(custom != nullptr && custom != audio);
    CHECK(custom->GetPcmSampleCount() == 8);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 2, u"tone", u"\\custom\\") == custom);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 2, u"tone", u".\\custom\\") == custom);

    auto* texture = dynamic_cast<soft3d::TextureRes*>(
        manager.GetResource(soft3d::ResourceTexture, 2, u"tex"));
    CHECK(texture != nullptr);
    CHECK(texture->GetTexture()->GetWidth() == 2);
    CHECK(texture->GetTexture()->GetHeight() == 2);
    CHECK(texture->GetTexture()->GetPixelSize() == 4);
    const std::array<uint8_t, 4> pixels = {10, 20, 30, 40};
    CHECK(std::equal(pixels.begin(), pixels.end(), texture->GetTexture()->GetPixels()));

    auto* object = dynamic_cast<soft3d::ObjectRes*>(
        manager.GetResource(soft3d::ResourceObject, 2, u"model"));
    CHECK(object != nullptr);
    CHECK(object->GetObjectCount() == 1);
    CHECK(object->GetSurfaceCount() == 1);
    CHECK(object->GetObjects()[0].color == 0x241f);
    CHECK(object->GetObjects()[0].textureIndex == 7);
    CHECK(object->GetObjects()[0].primitiveType == 4);
    CHECK(object->GetSurfaces()[0].indexCount == 6);
    CHECK(object->GetSurfaces()[0].vertices.count == 1);
    CHECK(object->GetSurfaces()[0].vertices.data.size() == 8);
    CHECK(object->GetTrailer()[5] == 45);

    auto* animation = dynamic_cast<soft3d::Animation*>(
        manager.GetResource(soft3d::ResourceAnimation, 2, u"anim"));
    CHECK(animation != nullptr);
    CHECK(animation->GetHeader0() == 0x111);
    CHECK(animation->GetHeader1() == 0x222);
    CHECK(animation->GetTransformCount() == 1);
    CHECK(animation->GetFirstTransform() == animation->GetSecondTransform());
    CHECK(animation->GetTrackCount() == 1);
    const auto& keyframe = animation->GetTracks()[0].keyframes[0];
    CHECK(keyframe.time == 100);
    CHECK(keyframe.value0 == -1 && keyframe.value1 == 2 && keyframe.value8 == -9);
    CHECK((keyframe.translation == std::array<int32_t, 3>{10, 11, 12}));
    CHECK(keyframe.scale == 0x10000);
    CHECK(animation->GetMorphFrameCount() == 2);
    CHECK(animation->GetMorphs()[0].frames[1][2] == 55);

    manager.ClearCache(3);
    CHECK(audio->GetPcmSampleCount() == 16);
    manager.ReleaseResource(audio);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 1, u"tone") != nullptr);
    manager.ClearCache(1);
    CHECK(manager.GetResource(soft3d::ResourceAudio, 0, u"tone") != nullptr);
    manager.ReleaseResource(nullptr);
    manager.ClearCache(0);
    manager.ClearCache(2);
}

void TestRealArchive()
{
    soft3d::s3dResManager manager;
    manager.GetFilePak()->Open(std::filesystem::path(SOFT3D_TEST_ARCHIVE).u16string().c_str(), 0);
    CHECK(manager.GetFilePak()->GetEntryCount() == 3322);

    auto* audio = dynamic_cast<soft3d::AudioData*>(manager.GetResource(
        soft3d::ResourceAudio, 0, u"7DaysPiano03", u".\\audio\\"));
    CHECK(audio != nullptr);
    CHECK(audio->GetPcmSampleCount() != 0 && audio->GetPcmBuffer() != nullptr);

    auto* texture = dynamic_cast<soft3d::TextureRes*>(
        manager.GetResource(soft3d::ResourceTexture, 0, u"aj_zhiwu_a"));
    CHECK(texture != nullptr);
    CHECK(texture->GetTexture()->GetPixels() != nullptr);
    CHECK(texture->GetTexture()->GetPixelSize() != 0);

    auto* object = dynamic_cast<soft3d::ObjectRes*>(
        manager.GetResource(soft3d::ResourceObject, 0, u"cg_bully_tuomao_a"));
    CHECK(object != nullptr);
    CHECK(object->GetObjectCount() != 0);
    CHECK(object->GetSurfaceCount() != 0);

    auto* animation = dynamic_cast<soft3d::Animation*>(
        manager.GetResource(soft3d::ResourceAnimation, 0, u"cg_bully_cc_aa"));
    CHECK(animation != nullptr);
    CHECK(animation->GetTrackCount() > 0);
}

template <typename Test>
void Run(const char* name, Test test)
{
    test();
    std::cout << "PASS: " << name << '\n';
}

} // namespace

int main()
{
    try {
        Run("synthetic resources and cache semantics", TestSyntheticResourcesAndCache);
        Run("real archive resource loading", TestRealArchive);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

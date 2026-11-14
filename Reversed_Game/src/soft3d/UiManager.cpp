#include "soft3d/UiManager.hpp"

#include "soft3d/Input.hpp"
#include "soft3d/TextureRes.hpp"

#include <cstdio>
#include <cstring>

namespace soft3d {

static_assert(sizeof(SdtAnimationKey) == 13 * sizeof(uint32_t));

namespace {

struct SdtReader {
    const uint8_t* data;
    size_t size;

    bool Read32(uint32_t& value)
    {
        if (size < 4) {
            return false;
        }
        std::memcpy(&value, data, 4);
        data += 4;
        size -= 4;
        return true;
    }
};

result ReadEntry(s3dResManager* manager, const char16_t* name, std::vector<uint8_t>& data)
{
    if (manager == nullptr) {
        return S3D_STATE;
    }
    s3dFilePak* filePak = manager->GetFilePak();
    if (filePak->Select(name) != S3D_OK) {
        return S3D_FILE;
    }
    filePak->Seek(0, SEEK_END);
    const int bytes = filePak->Tell();
    if (bytes == S3D_EOF) {
        return S3D_FILE;
    }
    data.resize(static_cast<size_t>(bytes));
    filePak->Seek(0, SEEK_SET);
    filePak->Read(data.data(), bytes);
    return S3D_OK;
}

std::u16string ReadSdtName(const uint8_t* record)
{
    const uint32_t length = (static_cast<uint32_t>(record[0]) << 8) | record[1];
    std::u16string result;
    result.reserve(length);
    for (uint32_t index = 0; index < length; ++index) {
        result.push_back(static_cast<char16_t>(record[index + 2]));
    }
    return result;
}

} // namespace

void UiManager::SetResourceManager(s3dResManager* manager)
{
    resourceManager = manager;
}

void UiManager::SetScreenBlitter(ScreenBlitter* blitter)
{
    screenBlitter = blitter;
}

void UiManager::SetFont(StringManager* manager)
{
    font = manager;
}

void UiManager::SetLanguage(int value)
{
    language = value;
}

result UiManager::LoadStrSdt(const char16_t* entryName)
{
    std::vector<uint8_t> data;
    result status = ReadEntry(resourceManager, entryName, data);
    if (status != S3D_OK) {
        return status;
    }
    status = ParseStrSdt(data.data(), data.size());
    if (status != S3D_OK || resourceManager == nullptr) {
        return status;
    }
    resources.assign(resourceNames.size(), nullptr);
    for (size_t index = 0; index < resourceNames.size(); ++index) {
        resources[index] = static_cast<TextureRes*>(resourceManager->GetResource(
            ResourceTexture, 1, resourceNames[index].c_str(), language == 2 ? u"\\ui" : u"\\uien"));
        if (resources[index] == nullptr) {
            return S3D_FILE;
        }
    }
    return S3D_OK;
}

result UiManager::LoadStateSdt(const char16_t* entryName)
{
    std::vector<uint8_t> data;
    const result status = ReadEntry(resourceManager, entryName, data);
    return status == S3D_OK ? ParseStateSdt(data.data(), data.size()) : status;
}

result UiManager::ParseStrSdt(const void* data, size_t size)
{
    SdtReader input{static_cast<const uint8_t*>(data), size};
    uint32_t textureCount;
    if (!input.Read32(textureCount) || !input.Read32(strReserved0)) {
        return S3D_FORMAT;
    }
    resourceNames.clear();
    resourceNames.reserve(textureCount);
    for (uint32_t index = 0; index < textureCount; ++index) {
        if (input.size < 24 || input.data[0] * 256 + input.data[1] > 22) {
            return S3D_FORMAT;
        }
        resourceNames.push_back(ReadSdtName(input.data));
        input.data += 24;
        input.size -= 24;
    }
    uint32_t soundCount;
    if (!input.Read32(soundCount)) {
        return S3D_FORMAT;
    }
    soundNames.clear();
    soundNames.reserve(soundCount);
    for (uint32_t index = 0; index < soundCount; ++index) {
        if (input.size < 24 || input.data[0] * 256 + input.data[1] > 22) {
            return S3D_FORMAT;
        }
        soundNames.push_back(ReadSdtName(input.data));
        input.data += 24;
        input.size -= 24;
    }
    return input.size == 0 ? S3D_OK : S3D_FORMAT;
}

result UiManager::ParseStateSdt(const void* data, size_t size)
{
    SdtReader input{static_cast<const uint8_t*>(data), size};
    uint32_t dialogCount;
    if (!input.Read32(declaredVisualTotal) || !input.Read32(dialogCount)) {
        return S3D_FORMAT;
    }
    dialogs.clear();
    dialogs.reserve(dialogCount);
    for (uint32_t dialogIndex = 0; dialogIndex < dialogCount; ++dialogIndex) {
        auto dialog = std::make_unique<Dialog>(this, static_cast<uint16_t>(dialogIndex));
        SdtDialog& definition = dialog->GetData();
        uint32_t visualCount;
        if (!input.Read32(visualCount) || input.size / 12 < visualCount) {
            return S3D_FORMAT;
        }
        definition.visuals.resize(visualCount);
        for (uint32_t visualIndex = 0; visualIndex < visualCount; ++visualIndex) {
            SdtVisual& visual = definition.visuals[visualIndex];
            uint32_t x;
            uint32_t y;
            if (!input.Read32(visual.resourceIndex) || !input.Read32(x) || !input.Read32(y)) {
                return S3D_FORMAT;
            }
            visual.x = static_cast<int32_t>(x);
            visual.y = static_cast<int32_t>(y);
            visual.resource = GetResourceByIndex(visual.resourceIndex);
            if (visual.resource != nullptr) {
                const Texture* texture = visual.resource->GetTexture();
                visual.width = static_cast<int16_t>(texture->GetWidth());
                visual.height = static_cast<int16_t>(texture->GetHeight());
                visual.sourceWidth = visual.width;
                visual.sourceHeight = visual.height;
            }
        }
        uint32_t stateCount;
        if (!input.Read32(stateCount)) {
            return S3D_FORMAT;
        }
        definition.states.resize(stateCount);
        for (uint32_t stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
            SdtState& state = definition.states[stateIndex];
            uint32_t elementCount;
            if (!input.Read32(state.id) || !input.Read32(elementCount)) {
                return S3D_FORMAT;
            }
            state.elements.resize(elementCount);
            for (uint32_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
                SdtElement& element = state.elements[elementIndex];
                uint32_t keyCount;
                if (!input.Read32(element.visualIndex) || !input.Read32(keyCount) ||
                    !input.Read32(element.behavior) || !input.Read32(element.secondaryTextureIndex)) {
                    return S3D_FORMAT;
                }
                if (element.secondaryTextureIndex != 0) {
                    if (element.visualIndex >= definition.visuals.size()) {
                        return S3D_FORMAT;
                    }
                    const uint32_t primaryIndex = definition.visuals[element.visualIndex].resourceIndex;
                    if (primaryIndex >= resourceNames.size()) {
                        return S3D_FORMAT;
                    }
                    element.secondaryTextureIndex =
                        InternTextureName(resourceNames[primaryIndex] + u"_alp");
                }
                const int32_t signedKeyCount = static_cast<int32_t>(keyCount);
                if (signedKeyCount > 0) {
                    if (input.size / sizeof(SdtAnimationKey) < static_cast<uint32_t>(signedKeyCount)) {
                        return S3D_FORMAT;
                    }
                    element.keys.resize(static_cast<size_t>(signedKeyCount));
                }
                for (SdtAnimationKey& key : element.keys) {
                    uint32_t fields[13];
                    for (uint32_t& field : fields) {
                        if (!input.Read32(field)) {
                            return S3D_FORMAT;
                        }
                    }
                    std::memcpy(&key, fields, sizeof(key));
                }
            }
        }
        dialogs.push_back(std::move(dialog));
    }
    return input.size == 0 ? S3D_OK : S3D_FORMAT;
}

void UiManager::SelectState(uint32_t stateIndex, int32_t time)
{
    Dialog* dialog = GetDialogRuntime(0);
    if (dialog != nullptr) {
        dialog->SetStateTime(static_cast<int32_t>(stateIndex), time);
    }
}

void UiManager::Render()
{
    Update(0);
}

void UiManager::Update(uint32_t context)
{
    for (const auto& dialog : dialogs) {
        if (dialog->IsActive()) {
            dialog->UpdateAndRender(context);
        }
    }
}

void UiManager::SetEngine(Engine* value)
{
    s3dBase::SetEngine(value);
}

size_t UiManager::GetDialogCount() const
{
    return dialogs.size();
}

const SdtDialog* UiManager::GetDialog(size_t index) const
{
    const Dialog* dialog = GetDialogRuntime(index);
    return dialog != nullptr ? &dialog->GetData() : nullptr;
}

Dialog* UiManager::GetDialogRuntime(size_t index)
{
    return index < dialogs.size() ? dialogs[index].get() : nullptr;
}

const Dialog* UiManager::GetDialogRuntime(size_t index) const
{
    return index < dialogs.size() ? dialogs[index].get() : nullptr;
}

TextureRes* UiManager::GetResourceByIndex(size_t index) const
{
    return index < resources.size() ? resources[index] : nullptr;
}

ScreenBlitter* UiManager::GetScreenBlitter() const
{
    return screenBlitter;
}

const std::vector<std::u16string>& UiManager::GetResourceNames() const
{
    return resourceNames;
}

const std::vector<std::u16string>& UiManager::GetSoundNames() const
{
    return soundNames;
}

uint32_t UiManager::GetStrReserved0() const
{
    return strReserved0;
}

uint32_t UiManager::GetDeclaredVisualTotal() const
{
    return declaredVisualTotal;
}

uint32_t UiManager::InternTextureName(const std::u16string& name)
{
    for (size_t index = 0; index < resourceNames.size(); ++index) {
        if (resourceNames[index] == name) {
            return static_cast<uint32_t>(index);
        }
    }
    if (name.size() > 30) {
        return 0;
    }
    TextureRes* resource = nullptr;
    if (resourceManager != nullptr) {
        resource = static_cast<TextureRes*>(resourceManager->GetResource(
            ResourceTexture, 1, name.c_str(), language == 2 ? u"\\ui" : u"\\uien"));
        if (resource == nullptr) {
            return 0;
        }
    }
    resourceNames.push_back(name);
    resources.push_back(resource);
    return static_cast<uint32_t>(resourceNames.size() - 1);
}

void GameMenu::Update(UiManager& uiManager, const soft3d::Input& input)
{
    if (input.IsKeyDown(Key::Left)) {
        selection = 0;
    }
    if (input.IsKeyDown(Key::Right)) {
        selection = 1;
    }
    uiManager.SelectState(7, selection);
}

int32_t GameMenu::GetSelection() const
{
    return selection;
}

} // namespace soft3d

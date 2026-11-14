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

    bool Skip(size_t bytes)
    {
        if (size < bytes) {
            return false;
        }
        data += bytes;
        size -= bytes;
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
            ResourceTexture, 1, resourceNames[index].c_str(), u"\\ui"));
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
    if (!input.Read32(textureCount) || !input.Skip(4)) {
        return S3D_FORMAT;
    }
    resourceNames.clear();
    resourceNames.reserve(textureCount);
    for (uint32_t index = 0; index < textureCount; ++index) {
        if (input.size < 24 || input.data[0] * 256 + input.data[1] > 22) {
            return S3D_FORMAT;
        }
        resourceNames.push_back(ReadSdtName(input.data));
        input.Skip(24);
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
        input.Skip(24);
    }
    return S3D_OK;
}

result UiManager::ParseStateSdt(const void* data, size_t size)
{
    SdtReader input{static_cast<const uint8_t*>(data), size};
    uint32_t totalStates;
    uint32_t dialogCount;
    if (!input.Read32(totalStates) || !input.Read32(dialogCount)) {
        return S3D_FORMAT;
    }
    (void)totalStates;
    dialogs.clear();
    dialogs.resize(dialogCount);
    for (uint32_t dialogIndex = 0; dialogIndex < dialogCount; ++dialogIndex) {
        SdtDialog& dialog = dialogs[dialogIndex];
        uint32_t visualCount;
        if (!input.Read32(visualCount)) {
            return S3D_FORMAT;
        }
        dialog.visuals.resize(visualCount);
        for (uint32_t visualIndex = 0; visualIndex < visualCount; ++visualIndex) {
            SdtVisual& visual = dialog.visuals[visualIndex];
            uint32_t x;
            uint32_t y;
            if (!input.Read32(visual.resourceIndex) || !input.Read32(x) || !input.Read32(y)) {
                return S3D_FORMAT;
            }
            visual.x = static_cast<int32_t>(x);
            visual.y = static_cast<int32_t>(y);
            if (visual.resourceIndex < resources.size()) {
                visual.resource = resources[visual.resourceIndex];
            }
        }
        uint32_t stateCount;
        if (!input.Read32(stateCount)) {
            return S3D_FORMAT;
        }
        dialog.states.resize(stateCount);
        for (uint32_t stateIndex = 0; stateIndex < stateCount; ++stateIndex) {
            SdtState& state = dialog.states[stateIndex];
            uint32_t elementCount;
            if (!input.Read32(state.id) || !input.Read32(elementCount)) {
                return S3D_FORMAT;
            }
            state.elements.resize(elementCount);
            for (uint32_t elementIndex = 0; elementIndex < elementCount; ++elementIndex) {
                SdtElement& element = state.elements[elementIndex];
                uint32_t keyCount;
                if (!input.Read32(element.visualIndex) || !input.Read32(keyCount) ||
                    !input.Read32(element.text) || !input.Read32(element.flags)) {
                    return S3D_FORMAT;
                }
                if (static_cast<int32_t>(keyCount) > 0) {
                    element.keys.resize(keyCount);
                }
                for (uint32_t keyIndex = 0; keyIndex < element.keys.size(); ++keyIndex) {
                    uint32_t* fields = reinterpret_cast<uint32_t*>(&element.keys[keyIndex]);
                    for (size_t field = 0; field < sizeof(SdtAnimationKey) / sizeof(uint32_t); ++field) {
                        if (!input.Read32(fields[field])) {
                            return S3D_FORMAT;
                        }
                    }
                }
            }
        }
    }
    return input.size == 0 ? S3D_OK : S3D_FORMAT;
}

void UiManager::SelectState(uint32_t stateIndex, int32_t time)
{
    selectedState = stateIndex;
    stateTime = time;
}

void UiManager::Render()
{
    Update(0);
}

void UiManager::Update(uint32_t context)
{
    (void)context;
    if (screenBlitter == nullptr || dialogs.empty() || selectedState >= dialogs[0].states.size()) {
        return;
    }
    const SdtState& state = dialogs[0].states[selectedState];
    const SdtDialog& dialog = dialogs[0];
    for (const SdtElement& element : state.elements) {
        if (element.text != 0 || element.visualIndex >= dialog.visuals.size()) {
            continue;
        }
        const SdtVisual& visual = dialog.visuals[element.visualIndex];
        if (visual.resource == nullptr) {
            continue;
        }
        for (const SdtAnimationKey& key : element.keys) {
            if (key.time != stateTime || key.widthDivisor == 0 || key.heightDivisor == 0) {
                continue;
            }
            const ScreenRect source{key.sourceX, key.sourceY,
                key.sourceX + key.sourceWidth, key.sourceY + key.sourceHeight};
            const ScreenRect destination{key.x, key.y,
                key.x + key.sourceWidth * key.width / key.widthDivisor,
                key.y + key.sourceHeight * key.height / key.heightDivisor};
            screenBlitter->Blit(visual.resource->GetTexture(), destination, source,
                                static_cast<uint32_t>(key.mode));
            break;
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
    return index < dialogs.size() ? &dialogs[index] : nullptr;
}

const std::vector<std::u16string>& UiManager::GetResourceNames() const
{
    return resourceNames;
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

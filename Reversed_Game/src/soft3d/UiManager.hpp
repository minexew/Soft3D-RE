#pragma once

#include "soft3d/Dialog.hpp"
#include "soft3d/Renderable.hpp"
#include "soft3d/ScreenBlitter.hpp"
#include "soft3d/s3dBase.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace soft3d {

class StringManager;

class UiManager : public Renderable, public s3dBase {
public:
    UiManager() = default;

    void SetResourceManager(s3dResManager* manager);
    void SetScreenBlitter(ScreenBlitter* blitter);
    void SetFont(StringManager* manager);
    void SetLanguage(int value);
    result LoadStrSdt(const char16_t* entryName);
    result LoadStateSdt(const char16_t* entryName);
    result ParseStrSdt(const void* data, size_t size);
    result ParseStateSdt(const void* data, size_t size);

    void SelectState(uint32_t stateIndex, int32_t time);
    void Render() override;
    virtual void Update(uint32_t context);
    void SetEngine(Engine* engine) override;

    size_t GetDialogCount() const;
    const SdtDialog* GetDialog(size_t index) const;
    Dialog* GetDialogRuntime(size_t index);
    const Dialog* GetDialogRuntime(size_t index) const;
    TextureRes* GetResourceByIndex(size_t index) const;
    ScreenBlitter* GetScreenBlitter() const;
    const std::vector<std::u16string>& GetResourceNames() const;
    const std::vector<std::u16string>& GetSoundNames() const;
    uint32_t GetStrReserved0() const;
    uint32_t GetDeclaredVisualTotal() const;

private:
    uint32_t InternTextureName(const std::u16string& name);

    s3dResManager* resourceManager = nullptr;
    ScreenBlitter* screenBlitter = nullptr;
    StringManager* font = nullptr;
    std::vector<std::u16string> resourceNames;
    std::vector<std::u16string> soundNames;
    std::vector<TextureRes*> resources;
    std::vector<std::unique_ptr<Dialog>> dialogs;
    uint32_t strReserved0 = 0;
    uint32_t declaredVisualTotal = 0;
    int language = 0;
};

class Input;

class GameMenu {
public:
    void Update(UiManager& uiManager, const Input& input);
    int32_t GetSelection() const;

private:
    int32_t selection = 0;
};

} // namespace soft3d

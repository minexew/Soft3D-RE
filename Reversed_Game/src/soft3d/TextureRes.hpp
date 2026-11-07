#pragma once

#include "soft3d/Resource.hpp"
#include "soft3d/Texture.hpp"

namespace soft3d {

class TextureRes : public Resource {
public:
    TextureRes() = default;

    void SetEngine(Engine* engine) override;
    int Load(const char16_t* entryName, s3dFilePak* filePak) override;
    Texture* GetTexture();
    const Texture* GetTexture() const;

private:
    Texture texture;
};

} // namespace soft3d

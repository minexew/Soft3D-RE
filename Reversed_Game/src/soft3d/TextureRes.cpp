#include "soft3d/TextureRes.hpp"

namespace soft3d {

void TextureRes::SetEngine(Engine* value)
{
    texture.SetEngine(value);
    Resource::SetEngine(value);
}

result TextureRes::Load(const char16_t* entryName, s3dFilePak* filePak)
{
    return texture.LoadStx(entryName, filePak);
}

Texture* TextureRes::GetTexture()
{
    return &texture;
}

const Texture* TextureRes::GetTexture() const
{
    return &texture;
}

} // namespace soft3d

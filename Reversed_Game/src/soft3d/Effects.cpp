#include "soft3d/Effects.hpp"

#include "soft3d/s3dResManager.hpp"

#include <cstring>

namespace soft3d {
namespace {

SojVertices MakeVertices(uint32_t format, uint32_t strideBytes, uint32_t count)
{
    SojVertices vertices;
    vertices.format = format;
    vertices.strideBytes = strideBytes;
    vertices.vertexCount = count;
    vertices.dataByteSize = strideBytes * count;
    vertices.data.resize(vertices.dataByteSize, 0);
    return vertices;
}

void WriteValue(uint8_t* destination, Fixed value)
{
    std::memcpy(destination, &value, sizeof(value));
}

void SetPositionUv(SojVertices& vertices, uint32_t index, const Vec3& position,
                   Fixed u, Fixed v)
{
    uint8_t* output = vertices.data.data() + index * vertices.strideBytes;
    WriteValue(output, position.x);
    WriteValue(output + 4, position.y);
    WriteValue(output + 8, position.z);
    if (vertices.strideBytes >= 20) {
        WriteValue(output + 12, u);
        WriteValue(output + 16, v);
    }
}

Vec3 ReadPosition(const SojVertices& vertices, uint32_t index)
{
    Vec3 position;
    std::memcpy(&position, vertices.data.data() + index * vertices.strideBytes, sizeof(position));
    return position;
}

} // namespace

Lensflare::Lensflare(Renderer* effectRenderer, ScreenBlitter* effectBlitter,
                     s3dResManager* manager)
    : renderer(effectRenderer)
    , blitter(effectBlitter)
    , resourceManager(manager)
{
}

void Lensflare::SetSource(const Vec3& newSource)
{
    source = newSource;
}

result Lensflare::LoadElement(uint32_t index, const char16_t* name, Fixed offset,
                              int cacheLevel, const char16_t* directory)
{
    if (index >= elements.size() || resourceManager == nullptr) {
        return S3D_FILE;
    }
    auto* resource = dynamic_cast<TextureRes*>(resourceManager->GetResource(
        ResourceTexture, cacheLevel, name, directory));
    if (resource == nullptr) {
        return S3D_FILE;
    }
    elements[index].resource = resource;
    SetElement(index, resource->GetTexture(), offset);
    return S3D_OK;
}

void Lensflare::SetElement(uint32_t index, const Texture* texture, Fixed offset)
{
    if (index >= elements.size()) {
        return;
    }
    elements[index].enabled = true;
    elements[index].offset = offset;
    elements[index].texture = texture;
}

void Lensflare::ClearElements()
{
    for (Element& element : elements) {
        element.enabled = false;
    }
}

bool Lensflare::IsElementEnabled(uint32_t index) const
{
    return index < elements.size() && elements[index].enabled;
}

void Lensflare::Render()
{
    Vec3 projected;
    if (!renderer->ProjectPoint(source, projected)) {
        return;
    }
    const int32_t screenX = projected.x >> 16;
    const int32_t screenY = projected.y >> 16;
    if (screenX < 0 || screenX >= renderer->GetWidth() ||
        screenY < 0 || screenY >= renderer->GetHeight() ||
        renderer->GetDepth(screenX, screenY) > projected.z + 100) {
        return;
    }
    const Fixed centerX = ((renderer->GetWidth() >> 1) << 16) - 0x8000;
    const Fixed centerY = ((renderer->GetHeight() >> 1) << 16) - 0x8000;
    const Fixed deltaX = SubtractFixed(centerX, projected.x);
    const Fixed deltaY = SubtractFixed(centerY, projected.y);
    blitter->SetTexture(renderer->GetCurrentTexture());
    for (const Element& element : elements) {
        if (!element.enabled) {
            continue;
        }
        const int32_t x = (AddFixed(projected.x, FixedMultiply(deltaX, element.offset)) >> 16) -
                          static_cast<int32_t>(element.texture->GetWidth() >> 1);
        const int32_t y = (AddFixed(projected.y, FixedMultiply(deltaY, element.offset)) >> 16) -
                          static_cast<int32_t>(element.texture->GetHeight() >> 1);
        const ScreenRect sourceRect = {0, 0, static_cast<int32_t>(element.texture->GetWidth()),
                                      static_cast<int32_t>(element.texture->GetHeight())};
        blitter->BlitUnscaled(element.texture, x, y, sourceRect, 1);
    }
}

BillboardGroup::BillboardGroup(Renderer* effectRenderer, Camera* effectCamera,
                               uint16_t billboardCapacity)
    : renderer(effectRenderer)
    , camera(effectCamera)
    , capacity(billboardCapacity)
    , vertices(MakeVertices(5, 20, 4))
{
}

bool BillboardGroup::AddBillboard(const Billboard& billboard)
{
    if (billboards.size() >= capacity) {
        return false;
    }
    billboards.push_back(billboard);
    return true;
}

void BillboardGroup::ClearBillboards()
{
    billboards.clear();
}

void BillboardGroup::SetTexture(const Texture* newTexture)
{
    texture = newTexture;
}

void BillboardGroup::SetBlendMode(uint32_t mode)
{
    blendMode = mode;
}

uint16_t BillboardGroup::GetBillboardCount() const
{
    return static_cast<uint16_t>(billboards.size());
}

void BillboardGroup::Render()
{
    renderer->SetLightingEnabled(false);
    renderer->SetTexture(texture);
    renderer->SetTextureCoordinateMode(0);
    renderer->SetTextureOffset(0, 0);
    renderer->SetBlendMode(blendMode);
    renderer->SetCullMode(0);
    const Vec3 forward = Subtract(camera->GetTarget(), camera->GetPosition());
    for (const Billboard& billboard : billboards) {
        const Vec3 relative = Subtract(billboard.position, camera->GetPosition());
        const Fixed facing = AddFixed(FixedMultiply(relative.x, forward.x),
                                      FixedMultiply(relative.z, forward.z));
        if (facing < 0) {
            continue;
        }
        Matrix4 transform = camera->GetRollMatrix();
        transform.values[12] = billboard.position.x;
        transform.values[13] = billboard.position.y;
        transform.values[14] = billboard.position.z;
        renderer->SetModelMatrix(transform);
        SetPositionUv(vertices, 0, {-billboard.halfWidth, -billboard.halfHeight, 0}, 0, 0);
        SetPositionUv(vertices, 1, {-billboard.halfWidth, billboard.halfHeight, 0}, 0, 0);
        SetPositionUv(vertices, 2, {billboard.halfWidth, -billboard.halfHeight, 0}, 0, 0);
        SetPositionUv(vertices, 3, {billboard.halfWidth, billboard.halfHeight, 0}, 0, 0);
        renderer->DrawSequential(vertices, 5, 0, 2);
    }
    renderer->SetBlendMode(0);
    renderer->SetCullMode(2);
    renderer->SetLightingEnabled(true);
}

FakeShadow::FakeShadow(Renderer* effectRenderer)
    : renderer(effectRenderer)
{
}

void FakeShadow::Initialize(Fixed radius)
{
    if (vertices.vertexCount != 0) {
        return;
    }
    vertices = MakeVertices(1, 12, 12);
    constexpr Fixed ring[][2] = {
        {-0x14000, 0}, {-0x11999, 0xa666}, {-0xa666, 0x11999}, {0, 0x14000},
        {0xa666, 0x11999}, {0x11999, 0xa666}, {0x14000, 0},
        {0x11999, -0xa666}, {0xa666, -0x11999}, {0, -0x14000},
        {-0xa666, -0x11999}, {-0x11999, -0xa666},
    };
    for (uint32_t index = 0; index < 12; ++index) {
        SetPositionUv(vertices, index,
            {FixedMultiply(ring[index][0], radius), 0, FixedMultiply(ring[index][1], radius)}, 0, 0);
    }
}

void FakeShadow::ScaleXZ(Fixed scale)
{
    if (vertices.vertexCount == 0) {
        return;
    }
    for (uint32_t index = 0; index < 12; ++index) {
        Vec3 position = ReadPosition(vertices, index);
        position.x = FixedMultiply(position.x, scale);
        position.z = FixedMultiply(position.z, scale);
        SetPositionUv(vertices, index, position, 0, 0);
    }
}

void FakeShadow::SetColor(uint8_t red, uint8_t green, uint8_t blue)
{
    color = static_cast<uint16_t>((red >> 3) << 11) |
            static_cast<uint16_t>((green >> 2) << 5) |
            static_cast<uint16_t>(blue >> 3);
}

void FakeShadow::SetTransform(const Matrix4& matrix)
{
    transform = matrix;
}

void FakeShadow::SetSecondaryTransform(const Matrix4& matrix)
{
    secondaryTransform = matrix;
}

const SojVertices& FakeShadow::GetVertices() const
{
    return vertices;
}

void FakeShadow::Render()
{
    renderer->SetModelMatrix(Multiply(transform, secondaryTransform));
    renderer->SetLightingEnabled(false);
    renderer->SetDepthWriteEnabled(false);
    renderer->SetDepthBias(0x12c0000);
    renderer->SetTexture(nullptr);
    renderer->SetFlatColor(color);
    renderer->SetCullMode(0);
    renderer->DrawSequential(vertices, 6, 0, 10);
    renderer->SetCullMode(2);
    renderer->SetDepthBias(0);
    renderer->SetDepthWriteEnabled(true);
    renderer->SetLightingEnabled(true);
}

GunFire::GunFire(Renderer* effectRenderer, Camera* effectCamera)
    : renderer(effectRenderer)
    , camera(effectCamera)
    , vertices(MakeVertices(5, 20, 4))
{
    SetPositionUv(vertices, 0, {-fixedOne, -fixedOne, 0}, 0, 0x200000);
    SetPositionUv(vertices, 1, {-fixedOne, fixedOne, 0}, 0, 0);
    SetPositionUv(vertices, 2, {fixedOne, -fixedOne, 0}, 0x200000, 0x200000);
    SetPositionUv(vertices, 3, {fixedOne, fixedOne, 0}, 0x200000, 0);
}

result GunFire::LoadTextures(s3dResManager* manager)
{
    auto* primary = dynamic_cast<TextureRes*>(manager->GetResource(
        ResourceTexture, 2, u"zp_huoguang_a"));
    auto* detail = dynamic_cast<TextureRes*>(manager->GetResource(
        ResourceTexture, 2, u"zp_huoguang_a_alp"));
    if (primary == nullptr || detail == nullptr) {
        return S3D_FILE;
    }
    textureResource = primary;
    detailTextureResource = detail;
    SetTextures(primary->GetTexture(), detail->GetTexture());
    return S3D_OK;
}

void GunFire::SetTextures(const Texture* newTexture, const Texture* newDetailTexture)
{
    texture = newTexture;
    detailTexture = newDetailTexture;
}

void GunFire::SetPosition(const Vec3& newPosition)
{
    position = newPosition;
}

void GunFire::Trigger(const Vec3& newPosition, int32_t frames)
{
    position = newPosition;
    framesRemaining = frames;
}

int32_t GunFire::GetFramesRemaining() const
{
    return framesRemaining;
}

const SojVertices& GunFire::GetVertices() const
{
    return vertices;
}

void GunFire::Render()
{
    if (framesRemaining <= 0 || texture == nullptr || detailTexture == nullptr) {
        return;
    }
    renderer->SetTexture(texture);
    renderer->SetDetailTexture(detailTexture);
    renderer->SetLightingEnabled(false);
    renderer->SetTextureCoordinateMode(0);
    renderer->SetTextureOffset(0, 0);
    renderer->SetBlendMode(0);
    renderer->SetCullMode(0);
    Matrix4 transform = camera->GetRollMatrix();
    transform.values[12] = position.x;
    transform.values[13] = position.y;
    transform.values[14] = position.z;
    renderer->SetModelMatrix(transform);
    renderer->DrawSequential(vertices, 5, 0, 2);
    renderer->SetCullMode(2);
    renderer->SetLightingEnabled(true);
    --framesRemaining;
}

} // namespace soft3d

#include "soft3d/Units.hpp"

#include "soft3d/Bsp.hpp"
#include "soft3d/s3dResManager.hpp"

#include <algorithm>
#include <utility>

namespace soft3d {
namespace {

ObjectBounds Translate(const ObjectBounds& bounds, const Vec3& offset)
{
    return {Add(bounds.minimum, offset), Add(bounds.maximum, offset)};
}

bool Contains(const ObjectBounds& bounds, const Vec3& point)
{
    return point.x >= bounds.minimum.x && point.y >= bounds.minimum.y &&
           point.z >= bounds.minimum.z && point.x <= bounds.maximum.x &&
           point.y <= bounds.maximum.y && point.z <= bounds.maximum.z;
}

bool Overlaps(const ObjectBounds& first, const ObjectBounds& second)
{
    return second.maximum.x >= first.minimum.x && second.maximum.y >= first.minimum.y &&
           second.maximum.z >= first.minimum.z && second.minimum.x <= first.maximum.x &&
           second.minimum.y <= first.maximum.y && second.minimum.z <= first.maximum.z;
}

bool SegmentHitsBounds(const Vec3& start, const Vec3& end, const ObjectBounds& bounds)
{
    const Vec3 center = Scale(Add(bounds.minimum, bounds.maximum), 0x8000);
    const Vec3 extents = Subtract(bounds.maximum, center);
    const Vec3 direction = Scale(Subtract(end, start), 0x8000);
    const Vec3 midpoint = Subtract(Add(start, direction), center);
    const Vec3 magnitude{AbsFixed(direction.x), AbsFixed(direction.y), AbsFixed(direction.z)};
    if (AbsFixed(midpoint.x) > AddFixed(extents.x, magnitude.x) ||
        AbsFixed(midpoint.y) > AddFixed(extents.y, magnitude.y) ||
        AbsFixed(midpoint.z) > AddFixed(extents.z, magnitude.z)) {
        return false;
    }
    const Vec3 cross = Cross(direction, midpoint);
    if (AbsFixed(cross.x) > AddFixed(FixedMultiply(extents.y, magnitude.z),
                                    FixedMultiply(extents.z, magnitude.y)) ||
        AbsFixed(cross.y) > AddFixed(FixedMultiply(extents.x, magnitude.z),
                                    FixedMultiply(extents.z, magnitude.x)) ||
        AbsFixed(cross.z) > AddFixed(FixedMultiply(extents.x, magnitude.y),
                                    FixedMultiply(extents.y, magnitude.x))) {
        return false;
    }
    return true;
}

Vec3 ProjectVelocity(const Vec3& value, const Vec3& normal)
{
    const Fixed scale = FixedMultiply(Dot(value, normal), 0x1028f);
    return Subtract(value, Scale(normal, scale));
}

Matrix4 ShadowMatrix(const Vec3& direction, const Vec3& normal, Fixed distance)
{
    Matrix4 result;
    const std::array<Fixed, 4> plane{normal.x, normal.y, normal.z, distance};
    const std::array<Fixed, 3> origin{direction.x, direction.y, direction.z};
    for (unsigned column = 0; column < 4; ++column) {
        for (unsigned row = 0; row < 4; ++row) {
            const Fixed base = row == column ? fixedOne : 0;
            const Fixed product = row < 3 ? FixedMultiply(origin[row], plane[column]) : 0;
            result.values[column * 4 + row] = SubtractFixed(base, product);
        }
    }
    return result;
}

} // namespace

MsgReceiver* MsgReceiverInterface::GetMsgReceiver() { return nullptr; }
void MsgReceiverInterface::ProcessMessages() {}

MsgReceiver::MsgReceiver() = default;

void MsgReceiver::AddMessage(uint32_t type, uint32_t value, uint32_t data)
{
    if (messageCount >= messages.size()) {
        return;
    }
    messages[messageCount++] = {type, value, data};
}

void MsgReceiver::ClearMessages() { messageCount = 0; }
std::size_t MsgReceiver::GetMessageCount() const { return messageCount; }
const UnitMessage& MsgReceiver::GetMessage(std::size_t index) const { return messages.at(index); }

void SendUnitMessage(MsgReceiverInterface* receiver, uint32_t type, uint32_t value, uint32_t data)
{
    if (receiver == nullptr) {
        return;
    }
    MsgReceiver* queue = receiver->GetMsgReceiver();
    if (queue != nullptr) {
        queue->AddMessage(type, value, data);
    }
}

GameUnit::GameUnit() = default;

GameUnit* GameUnit::QueryPoint(const Vec3& point)
{
    return collisionEnabled && Contains(collisionBounds, point) ? this : nullptr;
}

GameUnit* GameUnit::QuerySegment(const Vec3& start, const Vec3& end)
{
    return collisionEnabled && SegmentHitsBounds(start, end, collisionBounds) ? this : nullptr;
}

GameUnit* GameUnit::QueryBounds(const ObjectBounds& bounds)
{
    return collisionEnabled && Overlaps(collisionBounds, bounds) ? this : nullptr;
}

void GameUnit::SubmitRenderables(RenderQueues&) {}
void GameUnit::Update() {}
void GameUnit::PostUpdate() {}

void GameUnit::SaveState(OutputStream& stream) const
{
    stream.WriteInt32(unitType);
    stream.WriteInt32(unitId);
    stream.WriteInt32(visible ? 1 : 0);
    WriteVec3(stream, position);
    WriteBounds(stream, localBounds);
    WriteBounds(stream, collisionBounds);
}

void GameUnit::LoadState(InputStream& stream)
{
    int32_t flags = 0;
    stream.ReadInt32(unitId);
    stream.ReadInt32(flags);
    visible = flags != 0;
    ReadVec3(stream, position);
    ReadBounds(stream, localBounds);
    ReadBounds(stream, collisionBounds);
}

int32_t GameUnit::GetUnitType() const { return unitType; }
int32_t GameUnit::GetUnitId() const { return unitId; }
const Vec3& GameUnit::GetPosition() const { return position; }
const ObjectBounds& GameUnit::GetLocalBounds() const { return localBounds; }
const ObjectBounds& GameUnit::GetCollisionBounds() const { return collisionBounds; }
bool GameUnit::IsVisible() const { return visible; }
bool GameUnit::IsCollisionEnabled() const { return collisionEnabled; }
void GameUnit::SetUnitType(int32_t value) { unitType = value; }
void GameUnit::SetUnitId(int32_t value) { unitId = value; }
void GameUnit::SetPosition(const Vec3& value) { position = value; }
void GameUnit::SetLocalBounds(const ObjectBounds& value) { localBounds = value; }
void GameUnit::SetCollisionBounds(const ObjectBounds& value) { collisionBounds = value; }
void GameUnit::SetVisible(bool value) { visible = value; }
void GameUnit::SetCollisionEnabled(bool value) { collisionEnabled = value; }

VisUnit::VisUnit(s3dResManager* resources, int level, Renderer* objectRenderer,
                 AudioManager* audio)
    : resourceManager(resources)
    , renderer(objectRenderer)
    , cacheLevel(level)
    , audioManager(audio)
{
    unitType = 1;
}

VisUnit::~VisUnit() = default;

result VisUnit::LoadObjects(const char16_t* name, const char16_t* directory, int count)
{
    if (count <= 0 || name == nullptr) {
        return S3D_PARAM;
    }
    objectGroup = std::make_unique<ObjectGroup>();
    objectGroup->objects.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        auto object = std::make_unique<Object>(renderer);
        if (resourceManager == nullptr) {
            return S3D_FILE;
        }
        const result status = object->Load(resourceManager, name, cacheLevel, directory);
        if (status != S3D_OK) {
            return status;
        }
        objectGroup->objects.push_back(std::move(object));
    }
    primaryObject = objectGroup->objects.front().get();
    localBounds = primaryObject->GetPrimaryBounds();
    return S3D_OK;
}

result VisUnit::LoadAlternateObject(const char16_t* name, const char16_t* directory)
{
    if (shadowObject == nullptr) {
        shadowObject = std::make_unique<Object>(renderer);
    }
    if (resourceManager == nullptr) {
        return S3D_FILE;
    }
    shadowObject->Load(resourceManager, name, cacheLevel, directory);
    shadowObject->SetTransformMode(2);
    fakeShadow.reset();
    return S3D_OK;
}

void VisUnit::EnableShadow(bool enabled)
{
    shadowEnabled = enabled;
    if (enabled && shadowObject == nullptr && fakeShadow == nullptr && renderer != nullptr) {
        fakeShadow = std::make_unique<FakeShadow>(renderer);
        fakeShadow->Initialize(fixedOne);
    }
}

void VisUnit::SetPositionalAudio(bool enabled) { positionalAudio = enabled; }
bool VisUnit::IsPositionalAudio() const { return positionalAudio; }

void VisUnit::StopSound()
{
    if (audioManager == nullptr) {
        soundVoice = -1;
        return;
    }
    if (!positionalAudio) {
        if (soundVoice != -1) {
            audioManager->StopVoice(soundVoice);
        }
        soundVoice = -1;
    } else if (soundInstance != nullptr && soundInstance->GetListIndex() >= 0) {
        audioManager->RemoveSoundInstance(soundInstance->GetListIndex());
    }
}

int VisUnit::PlaySound(AudioData* data, bool looping)
{
    if (data == nullptr) {
        return -1;
    }
    StopSound();
    if (audioManager == nullptr) {
        return -1;
    }
    if (!positionalAudio) {
        soundVoice = audioManager->PlaySound(data, looping);
        if (!looping) {
            soundVoice = -1;
        }
    } else {
        if (soundInstance == nullptr) {
            soundInstance = std::make_unique<SoundInstance>();
        }
        soundInstance->Initialize(audioManager, data, looping, false);
        soundInstance->SetPosition(position);
        audioManager->AddSoundInstance(soundInstance.get());
    }
    return soundVoice;
}

int VisUnit::PlaySound(int nameIndex, bool looping)
{
    StopSound();
    if (audioManager == nullptr) {
        return -1;
    }
    if (!positionalAudio) {
        soundVoice = audioManager->PlaySound(nameIndex, looping);
        if (!looping) {
            soundVoice = -1;
        }
    } else {
        if (soundInstance == nullptr) {
            soundInstance = std::make_unique<SoundInstance>();
        }
        soundInstance->Initialize(audioManager, nullptr, looping, false);
        soundInstance->SetPosition(position);
        audioManager->LoadSoundInstance(soundInstance.get(), nameIndex);
    }
    return soundVoice;
}

void VisUnit::SetMaterialIntensityBounce(Fixed minimum, Fixed maximum, int32_t period)
{
    if (primaryObject != nullptr && primaryObject->GetModel() != nullptr) {
        for (std::size_t index = 0; index < primaryObject->GetModel()->GetObjects().size(); ++index) {
            primaryObject->SetMaterialIntensityBounce(static_cast<uint32_t>(index), minimum,
                                                      maximum, period);
        }
    }
}

void VisUnit::SetDepthBias(Fixed bias)
{
    if (primaryObject != nullptr) {
        primaryObject->SetDepthBias(bias);
    }
}

result VisUnit::LoadMaterials(const char16_t* directory)
{
    if (objectGroup == nullptr) {
        return S3D_FILE;
    }
    for (const auto& object : objectGroup->objects) {
        const result status = object->LoadMaterials(resourceManager, cacheLevel, directory);
        if (status != S3D_OK) {
            return status;
        }
    }
    return S3D_OK;
}

void VisUnit::SetAnimation(const Animation* resource, int32_t direction, bool loop)
{
    animation = resource;
    if (objectGroup != nullptr) {
        for (const auto& object : objectGroup->objects) {
            object->SetAnimation(resource);
            if (resource != nullptr) {
                object->SetAnimationPlayback(direction, loop);
            }
            object->SetAnimationTime(0);
        }
    }
    if (shadowEnabled && shadowObject != nullptr) {
        shadowObject->SetAnimation(resource);
        if (resource != nullptr) {
            shadowObject->SetAnimationPlayback(direction, loop);
        }
    }
    if (primaryObject != nullptr) {
        localBounds = primaryObject->GetPrimaryBounds();
    }
    animationFinished = false;
}

result VisUnit::LoadAnimation(const char16_t* name, const char16_t* directory)
{
    if (resourceManager == nullptr || name == nullptr) {
        return S3D_FILE;
    }
    const auto* resource = dynamic_cast<const Animation*>(resourceManager->GetResource(
        ResourceAnimation, cacheLevel, name, directory));
    if (resource == nullptr) {
        return S3D_FILE;
    }
    SetAnimation(resource, 1, true);
    return S3D_OK;
}

void VisUnit::SaveState(OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(cacheLevel);
    const uint32_t count = objectGroup != nullptr
        ? static_cast<uint32_t>(objectGroup->objects.size()) : 0;
    stream.WriteUInt32(count);
    if (count != 0) {
        const ObjectRes* model = objectGroup->objects.front()->GetModel();
        stream.WriteWideString(model != nullptr ? model->GetFileName() : u"");
        stream.WriteWideString(model != nullptr ? model->GetDirectory() : u"");
        for (const auto& object : objectGroup->objects) {
            WriteVec3(stream, object->GetAxisZ());
            WriteVec3(stream, object->GetAxisX());
            WriteVec3(stream, object->GetAxisY());
            WriteFixed(stream, object->GetDepthBias());
            WriteFixed(stream, object->GetObjectIntensity());
            stream.WriteInt32(static_cast<int32_t>(object->GetRenderMode()));
            stream.WriteByte(object->IsLightingEnabled() ? 1 : 0);
            stream.WriteByte(object->IsAmbientLightOnly() ? 1 : 0);
            stream.WriteByte(object->IsCullingDisabled() ? 1 : 0);
            stream.WriteByte(object->IsExternalTransformEnabled() ? 1 : 0);
            stream.WriteInt32(object->IsAnimationPaused() ? 1 : 0);
            stream.WriteInt32(object->GetAnimationTime());
            stream.WriteInt32(object->GetAnimationDirection());
            stream.WriteInt32(object->IsAnimationLooping() ? 1 : 0);
            stream.WriteInt32(object->GetAnimationSpeed());
            const auto intensity = object->GetMaterialIntensityState(0);
            for (Fixed value : intensity) {
                WriteFixed(stream, value);
            }
        }
    }
    stream.WriteByte(shadowEnabled ? 1 : 0);
    stream.WriteByte(shadowTransformDirty ? 1 : 0);
    stream.WriteByte(shadowObject != nullptr ? 1 : 0);
    if (shadowObject != nullptr) {
        const ObjectRes* model = shadowObject->GetModel();
        stream.WriteWideString(model != nullptr ? model->GetFileName() : u"");
    }
    stream.WriteByte(animation != nullptr ? 1 : 0);
    if (animation != nullptr) {
        stream.WriteWideString(animation->GetFileName());
        stream.WriteWideString(animation->GetDirectory());
    }
    stream.WriteInt32(animationFinished ? 1 : 0);
    stream.WriteInt32(positionalAudio ? 1 : 0);
}

void VisUnit::LoadState(InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(cacheLevel);
    uint32_t count = 0;
    stream.ReadUInt32(count);
    int32_t paused = 0;
    int32_t time = 0;
    int32_t direction = 1;
    int32_t loop = 1;
    int32_t speed = 300;
    if (count != 0) {
        char16_t name[256]{};
        char16_t directory[256]{};
        if (stream.ReadWideString(name, 256) != S3D_OK ||
            stream.ReadWideString(directory, 256) != S3D_OK || count > INT32_MAX) {
            return;
        }
        LoadObjects(name, directory, static_cast<int32_t>(count));
        LoadMaterials(directory);
        for (uint32_t index = 0; index < count; ++index) {
            Object* object = GetObject(index);
            if (object == nullptr) {
                return;
            }
            Vec3 axisZ;
            Vec3 axisX;
            Vec3 axisY;
            ReadVec3(stream, axisZ);
            ReadVec3(stream, axisX);
            ReadVec3(stream, axisY);
            object->RestoreAxes(axisZ, axisX, axisY);
            Fixed fixedValue = 0;
            ReadFixed(stream, fixedValue);
            object->SetDepthBias(fixedValue);
            ReadFixed(stream, fixedValue);
            object->SetObjectIntensity(fixedValue);
            int32_t integerValue = 0;
            stream.ReadInt32(integerValue);
            object->SetRenderMode(static_cast<ObjectRenderMode>(integerValue));
            uint8_t byteValue = 0;
            stream.ReadByte(byteValue);
            object->SetLightingEnabled(byteValue != 0);
            stream.ReadByte(byteValue);
            object->SetAmbientLightOnly(byteValue != 0);
            stream.ReadByte(byteValue);
            object->SetCullingDisabled(byteValue != 0);
            stream.ReadByte(byteValue);
            object->SetExternalTransformEnabled(byteValue != 0);
            stream.ReadInt32(paused);
            stream.ReadInt32(time);
            stream.ReadInt32(direction);
            stream.ReadInt32(loop);
            stream.ReadInt32(speed);
            object->RestoreAnimationState(paused, time, direction, loop, speed);
            std::array<Fixed, 4> intensity{};
            for (Fixed& value : intensity) {
                ReadFixed(stream, value);
            }
            object->SetMaterialIntensityState(0, intensity[0], intensity[1], intensity[2],
                                              intensity[3]);
        }
    }
    bool flag = false;
    stream.ReadBool(flag);
    shadowEnabled = flag;
    stream.ReadBool(flag);
    shadowTransformDirty = flag;
    uint8_t present = 0;
    stream.ReadByte(present);
    if (present != 0) {
        char16_t name[256]{};
        if (stream.ReadWideString(name, 256) != S3D_OK) {
            return;
        }
        LoadAlternateObject(name);
    }
    stream.ReadByte(present);
    if (present != 0) {
        char16_t name[256]{};
        char16_t directory[256]{};
        if (stream.ReadWideString(name, 256) != S3D_OK ||
            stream.ReadWideString(directory, 256) != S3D_OK) {
            return;
        }
        LoadAnimation(name, directory);
        for (uint32_t index = 0; index < count; ++index) {
            Object* object = GetObject(index);
            if (object != nullptr) {
                object->RestoreAnimationState(paused, time, direction, loop, speed);
            }
        }
    }
    int32_t integerValue = 0;
    stream.ReadInt32(integerValue);
    animationFinished = integerValue != 0;
    stream.ReadInt32(integerValue);
    positionalAudio = integerValue != 0;
}

void VisUnit::SetDirection(const Vec3& direction)
{
    if (primaryObject == nullptr) {
        return;
    }
    if (AbsFixed(direction.x) < 0x28f && AbsFixed(direction.z) < 0x28f) {
        primaryObject->UpdateTransform();
        if (shadowEnabled && shadowObject != nullptr) {
            shadowObject->UpdateTransform();
        }
        return;
    }
    Vec3 forward = direction;
    forward.y = 0;
    forward = Normalize(forward);
    const Vec3 up{0, fixedOne, 0};
    const Vec3 right = Normalize(Cross(up, forward));
    primaryObject->SetAxisX(right);
    primaryObject->SetAxisY(up);
    primaryObject->SetAxisZ(forward);
    primaryObject->UpdateTransform();
    if (shadowEnabled && shadowObject != nullptr) {
        shadowObject->SetAxisX(right);
        shadowObject->SetAxisY(up);
        shadowObject->SetAxisZ(forward);
        shadowObject->UpdateTransform();
    }
}

void VisUnit::SetAxisX(const Vec3& axis)
{
    if (primaryObject != nullptr) {
        primaryObject->SetAxisX(axis);
    }
}

void VisUnit::SetAxisY(const Vec3& axis)
{
    if (primaryObject != nullptr) {
        primaryObject->SetAxisY(axis);
    }
}

void VisUnit::SetAxisZ(const Vec3& axis)
{
    if (primaryObject != nullptr) {
        primaryObject->SetAxisZ(axis);
    }
}

void VisUnit::UpdateVisuals()
{
    if (objectGroup == nullptr || objectGroup->objects.empty()) {
        return;
    }
    Object* first = objectGroup->objects.front().get();
    first->SetPosition(position);
    first->UpdateTransform();
    objectGroup->cameraDepth = first->GetCameraDepth();
    if (objectGroup->objects.size() == 1) {
        objectGroup->current = 0;
    } else {
        Fixed threshold = 0x460000;
        objectGroup->current = objectGroup->objects.size() - 1;
        while (objectGroup->current != 0 && objectGroup->cameraDepth <= threshold) {
            --objectGroup->current;
            threshold >>= 1;
        }
    }
    primaryObject = objectGroup->objects[objectGroup->current].get();
    const Matrix4 matrix = first->GetModelMatrix();
    for (const auto& object : objectGroup->objects) {
        object->SetModelMatrix(matrix);
    }
    if (shadowEnabled) {
        if (shadowObject != nullptr) {
            shadowObject->SetPosition(position);
            shadowObject->SetModelMatrix(matrix);
        } else if (fakeShadow != nullptr) {
            fakeShadow->SetTransform(matrix);
        }
    }
    if (soundInstance != nullptr) {
        soundInstance->SetPosition(position);
    }
}

void VisUnit::Update()
{
    UpdateVisuals();
}

void VisUnit::PostUpdate()
{
    if (animation != nullptr && primaryObject != nullptr) {
        animationFinished = primaryObject->AdvanceAnimation();
        if (shadowEnabled && shadowObject != nullptr) {
            shadowObject->AdvanceAnimation();
        }
    }
}

void VisUnit::SubmitRenderables(RenderQueues& queues)
{
    if (primaryObject == nullptr || primaryObject->GetRenderMode() != ObjectRenderMode::Normal) {
        return;
    }
    primaryObject->sortKey = primaryObject->HasTransparency() ? primaryObject->GetCameraDepth()
                                                               : -primaryObject->GetCameraDepth();
    queues.Submit(primaryObject, primaryObject->HasTransparency() ? 4 : 3);
    if (shadowEnabled && shadowTransformDirty) {
        if (shadowObject != nullptr) {
            queues.Submit(shadowObject.get(), 2);
        } else if (fakeShadow != nullptr) {
            queues.Submit(fakeShadow.get(), 2);
        }
        shadowTransformDirty = false;
    }
}

void VisUnit::UpdateShadowTransform(const Matrix4& matrix)
{
    if (shadowObject != nullptr) {
        shadowObject->SetSecondaryTransform(matrix);
    } else if (fakeShadow != nullptr) {
        fakeShadow->SetSecondaryTransform(matrix);
    }
    shadowTransformDirty = true;
}

Object* VisUnit::GetObject(std::size_t index) const
{
    if (objectGroup == nullptr || index >= objectGroup->objects.size()) {
        return nullptr;
    }
    return objectGroup->objects[index].get();
}

std::size_t VisUnit::GetObjectCount() const
{
    return objectGroup != nullptr ? objectGroup->objects.size() : 0;
}

const Animation* VisUnit::GetAnimation() const { return animation; }
bool VisUnit::IsAnimationFinished() const { return animationFinished; }
bool VisUnit::IsShadowEnabled() const { return shadowEnabled; }
bool VisUnit::IsShadowTransformDirty() const { return shadowTransformDirty; }
int VisUnit::GetCurrentObjectIndex() const
{
    return objectGroup != nullptr ? static_cast<int>(objectGroup->current) : 0;
}

PhysicUnit::PhysicUnit(s3dResManager* resources, int level, Renderer* objectRenderer,
                       AudioManager* audio, Bsp* map)
    : VisUnit(resources, level, objectRenderer, audio)
    , groundTrace(std::make_unique<BspTraceResult>())
    , bsp(map)
    , shadowTrace(std::make_unique<BspTraceResult>())
{
    unitType = 2;
}

PhysicUnit::~PhysicUnit() = default;

void PhysicUnit::SetAnimation(const Animation* resource, int32_t direction, bool loop)
{
    VisUnit::SetAnimation(resource, direction, loop);
    if (primaryObject != nullptr) {
        secondaryLocalBounds = primaryObject->GetSecondaryBounds();
    }
}

void PhysicUnit::SetPhysicsFlags(uint32_t value) { physicsFlags = value; }
void PhysicUnit::SetVelocity(const Vec3& value) { velocity = value; }
void PhysicUnit::SetFriction(Fixed value) { friction = value; }
void PhysicUnit::SetYawVelocity(uint16_t value) { yawVelocity = value; }
void PhysicUnit::SetUnitManager(GameUnitManager* manager) { unitManager = manager; }
void PhysicUnit::SetBsp(Bsp* value) { bsp = value; }
uint32_t PhysicUnit::GetPhysicsFlags() const { return physicsFlags; }
const Vec3& PhysicUnit::GetVelocity() const { return velocity; }
const BspTraceResult& PhysicUnit::GetGroundTrace() const { return *groundTrace; }
const UnitQueryResult& PhysicUnit::GetUnitQueryResult() const { return unitQueryResult; }
bool PhysicUnit::HasUnitCollision() const { return unitCollision; }
const std::array<bool, 3>& PhysicUnit::GetAxisCollisionFlags() const { return axisCollision; }
bool PhysicUnit::IsOnGround() const { return groundTrace->startSolid; }

bool PhysicUnit::IsVelocityNearlyZero() const
{
    return AbsFixed(velocity.x) <= 0x28f && AbsFixed(velocity.z) <= 0x28f &&
           ((physicsFlags & PhysicSeparateAxisCollision) == 0 || AbsFixed(velocity.y) <= 0x28f);
}

void PhysicUnit::QueryUnitCollision()
{
    if (unitManager != nullptr) {
        unitQueryResult = unitManager->QueryPoint(position, -1, this);
    } else {
        unitQueryResult = {};
    }
    unitCollision = unitQueryResult.count > 0;
}

void PhysicUnit::MoveSimple()
{
    if (IsVelocityNearlyZero()) {
        return;
    }
    if (unitManager != nullptr) {
        const ObjectBounds query = Translate(collisionBounds, Add(position, velocity));
        unitQueryResult = unitManager->QueryBounds(query, -1, this);
        unitCollision = unitQueryResult.count > 0;
        if (unitCollision) {
            velocity = {};
            return;
        }
    }
    const Vec3 target = Add(position, velocity);
    if (bsp != nullptr) {
        const BspTraceResult trace = bsp->Trace(position, target, localBounds);
        if (trace.startSolid) {
            velocity = {};
        }
        position = trace.endPosition;
    } else {
        position = target;
    }
}

void PhysicUnit::MoveSlide()
{
    if (IsVelocityNearlyZero()) {
        return;
    }
    const Vec3 originalVelocity = velocity;
    const Fixed originalLength = Length(velocity);
    if (unitManager != nullptr) {
        std::array<bool, 3> hits{};
        for (unsigned axis = 0; axis < 3; ++axis) {
            if (axis == 1 && (physicsFlags & PhysicSeparateAxisCollision) == 0) {
                continue;
            }
            Vec3 candidate = position;
            (&candidate.x)[axis] = AddFixed((&candidate.x)[axis], (&velocity.x)[axis]);
            unitQueryResult = unitManager->QueryBounds(Translate(collisionBounds, candidate), -1, this);
            unitCollision = unitQueryResult.count > 0;
            hits[axis] = unitCollision;
            (&velocity.x)[axis] = 0;
        }
        axisCollision = hits;
        const bool all = (physicsFlags & PhysicSeparateAxisCollision) != 0
            ? hits[0] && hits[1] && hits[2]
            : hits[0] && hits[2];
        if (all) {
            velocity = {};
            return;
        }
        if (hits[0] || hits[1] || hits[2]) {
            if (AbsFixed(velocity.x) < 0x65 && AbsFixed(velocity.y) < 0x65 &&
                AbsFixed(velocity.z) < 0x65) {
                return;
            }
            velocity = Scale(Normalize(velocity), originalLength);
        }
    }

    Fixed remaining = fixedOne;
    std::array<Vec3, 5> normals{};
    std::size_t normalCount = 0;
    for (int iteration = 0; iteration < 5; ++iteration) {
        const Vec3 target = Add(position, Scale(velocity, remaining));
        BspTraceResult trace;
        if (bsp != nullptr) {
            trace = bsp->Trace(position, target, localBounds);
        } else {
            trace.endPosition = target;
        }
        if (trace.allSolid) {
            velocity.y = 0;
            return;
        }
        if (trace.fraction > 0) {
            position = trace.endPosition;
        }
        if (trace.fraction == fixedOne) {
            return;
        }
        remaining = FixedMultiply(remaining, trace.fraction);
        if (normalCount < normals.size()) {
            normals[normalCount++] = trace.planeNormal;
        }
        for (std::size_t index = 0; index < normalCount; ++index) {
            if (Dot(velocity, normals[index]) <= 0x1999) {
                velocity = ProjectVelocity(velocity, normals[index]);
                if (IsVelocityNearlyZero()) {
                    return;
                }
                velocity = Scale(Normalize(velocity), originalLength);
            }
        }
        if (Dot(velocity, originalVelocity) <= 0) {
            return;
        }
    }
}

void PhysicUnit::CheckGround()
{
    if (bsp == nullptr) {
        *groundTrace = BspTraceResult{};
        groundTrace->endPosition = position;
        return;
    }
    Vec3 end = position;
    end.y = SubtractFixed(end.y, 0x8000);
    *groundTrace = bsp->Trace(position, end, localBounds);
}

void PhysicUnit::UpdateShadow()
{
    if (!shadowEnabled || bsp == nullptr) {
        return;
    }
    Vec3 start = position;
    start.y = AddFixed(start.y, 0x20000);
    Vec3 end = position;
    end.y = SubtractFixed(end.y, 0x140000);
    *shadowTrace = bsp->Trace(start, end, localBounds);
    if (shadowTrace->planeNormal.x != 0 || shadowTrace->planeNormal.y != 0 ||
        shadowTrace->planeNormal.z != 0) {
        Vec3 direction{0, fixedOne, 0};
        if (shadowObject != nullptr) {
            direction = {bsp->GetHeaderValues()[0] >> 1, 0, bsp->GetHeaderValues()[1] >> 1};
            direction = Normalize(direction);
        }
        UpdateShadowTransform(ShadowMatrix(direction, shadowTrace->planeNormal,
                                          shadowTrace->planeDistance));
    }
}

void PhysicUnit::Update()
{
    axisCollision = {};
    previousPosition = position;
    if (!IsOnGround() && (physicsFlags & PhysicApplyFriction) != 0) {
        velocity.y = SubtractFixed(velocity.y, friction);
    }
    if ((physicsFlags & PhysicKeepDirection) == 0) {
        SetDirection(velocity);
    }
    if (yawVelocity != 0 && primaryObject != nullptr) {
        SetDirection(RotateY(primaryObject->GetModelMatrix().values[8] != 0 ||
                             primaryObject->GetModelMatrix().values[9] != 0 ||
                             primaryObject->GetModelMatrix().values[10] != 0
                                 ? Vec3{primaryObject->GetModelMatrix().values[8],
                                        primaryObject->GetModelMatrix().values[9],
                                        primaryObject->GetModelMatrix().values[10]}
                                 : Vec3{0, 0, fixedOne}, yawVelocity));
        yawVelocity = 0;
    }

    if ((physicsFlags & PhysicSlideMovement) == 0) {
        if ((physicsFlags & 0xe0U) == 0) {
            position = Add(position, velocity);
        }
    } else {
        if ((physicsFlags & PhysicSeparateAxisCollision) == 0 && IsOnGround()) {
            velocity = ProjectVelocity(velocity, groundTrace->planeNormal);
        }
        if ((physicsFlags & PhysicStepMovement) != 0 &&
            (physicsFlags & PhysicRestoreY) == 0 && IsOnGround() &&
            (physicsFlags & PhysicStepOffset) != 0) {
            position.y = AddFixed(position.y, 0x4000);
        }
        if ((physicsFlags & PhysicSimpleMovement) != 0) {
            MoveSimple();
        } else {
            MoveSlide();
        }
        if ((physicsFlags & PhysicRestoreX) != 0) {
            position.x = previousPosition.x;
        }
        if ((physicsFlags & PhysicRestoreY) != 0) {
            position.y = previousPosition.y;
        }
        if ((physicsFlags & PhysicRestoreZ) != 0) {
            position.z = previousPosition.z;
        }
        if ((physicsFlags & PhysicStepMovement) != 0) {
            if ((physicsFlags & PhysicRestoreY) == 0 && IsOnGround() &&
                (physicsFlags & PhysicStepOffset) != 0) {
                Vec3 end = position;
                end.y = SubtractFixed(end.y, 0x8000);
                if (bsp != nullptr) {
                    const BspTraceResult trace = bsp->Trace(position, end, localBounds);
                    if (trace.startSolid) {
                        position = trace.endPosition;
                    }
                }
            }
            CheckGround();
        }
    }
    if ((physicsFlags & PhysicSlideMovement) == 0) {
        if ((physicsFlags & PhysicRestoreX) != 0) {
            position.x = previousPosition.x;
        }
        if ((physicsFlags & PhysicRestoreY) != 0) {
            position.y = previousPosition.y;
        }
        if ((physicsFlags & PhysicRestoreZ) != 0) {
            position.z = previousPosition.z;
        }
    }

    UpdateVisuals();
    if (primaryObject != nullptr && bsp != nullptr) {
        primaryObject->SetObjectIntensity(SubtractFixed(bsp->GetLightIntensity(position), 0x4ccc));
    }
    UpdateShadow();
    collisionBounds = Translate(localBounds, position);
    secondaryCollisionBounds = Translate(secondaryLocalBounds, position);
}

Things::Things(s3dResManager* resources, int level, Renderer* objectRenderer,
               AudioManager* audio, Bsp* map)
    : PhysicUnit(resources, level, objectRenderer, audio, map)
{
    unitType = 4;
    physicsFlags = 0x1e0;
}

Things::~Things() = default;

GameUnit* Things::QueryPoint(const Vec3& point)
{
    return destroying ? nullptr : GameUnit::QueryPoint(point);
}

GameUnit* Things::QuerySegment(const Vec3& start, const Vec3& end)
{
    return destroying ? nullptr : GameUnit::QuerySegment(start, end);
}

GameUnit* Things::QueryBounds(const ObjectBounds& bounds)
{
    return destroying ? nullptr : GameUnit::QueryBounds(bounds);
}

void Things::SubmitRenderables(RenderQueues& queues)
{
    if (!destroying) {
        const bool visibleLeaf = currentLeaf < 0 || bsp == nullptr ||
            (static_cast<std::size_t>(currentLeaf) < bsp->GetLeaves().size() &&
             bsp->GetLeaves()[currentLeaf].cluster == bsp->GetCurrentCluster());
        const uint32_t blendMode = primaryObject != nullptr
            ? primaryObject->GetMaterialBlendMode(0) : 0;
        if (blendMode != 0 || visibleLeaf) {
            VisUnit::SubmitRenderables(queues);
        }
    } else if (alternateObject != nullptr) {
        queues.Submit(alternateObject.get(), 3);
    }
}

void Things::Update()
{
    if (!placedOnGround) {
        Vec3 start = position;
        start.y = AddFixed(start.y, 0x80000);
        Vec3 end = position;
        end.y = SubtractFixed(end.y, 0xc0000);
        if (bsp != nullptr) {
            const BspTraceResult trace = bsp->Trace(start, end, localBounds);
            if (trace.startSolid) {
                position = trace.endPosition;
            }
        }
        placedOnGround = true;
    }
    if (!destroying) {
        PhysicUnit::Update();
    } else {
        destructionTimer = static_cast<uint16_t>(destructionTimer + 1);
        if (static_cast<int16_t>(destructionDuration) - 6 <= destructionTimer &&
            alternateObject != nullptr) {
            alternateObject->SetMaterialBlendMode(0, 3);
        }
        if (static_cast<int16_t>(destructionDuration) - 4 <= destructionTimer &&
            alternateObject != nullptr) {
            alternateObject->SetMaterialBlendMode(0, 2);
        }
        if (static_cast<int16_t>(destructionDuration) - 2 <= destructionTimer &&
            alternateObject != nullptr) {
            alternateObject->SetMaterialBlendMode(0, 1);
        }
        if (destructionTimer == destructionDuration) {
            if (unitManager != nullptr) {
                unitManager->RemoveUnit(this, 2, true);
            }
            return;
        }
    }
    if (destroying && alternateObject != nullptr && primaryObject != nullptr) {
        alternateObject->SetModelMatrix(primaryObject->GetModelMatrix());
    }
    if (primaryObject != nullptr) {
        position = primaryObject->GetWorldBounds().minimum;
    }
}

void Things::PostUpdate()
{
    if (!destroying) {
        VisUnit::PostUpdate();
    } else if (alternateAnimation != nullptr && alternateObject != nullptr) {
        animationFinished = alternateObject->AdvanceAnimation();
    }
}

result Things::LoadMaterials(const char16_t* directory)
{
    if (alternateObject != nullptr) {
        const result status = alternateObject->LoadMaterials(resourceManager, cacheLevel, directory);
        if (status != S3D_OK) {
            return status;
        }
    }
    return VisUnit::LoadMaterials(directory);
}

void Things::SetAnimation(const Animation* resource, int32_t direction, bool loop)
{
    PhysicUnit::SetAnimation(resource, direction, loop);
}

result Things::LoadAlternateModel(const char16_t* name, const char16_t* directory)
{
    if (alternateObject == nullptr) {
        alternateObject = std::make_unique<Object>(renderer);
    }
    if (resourceManager == nullptr) {
        return S3D_FILE;
    }
    alternateObject->Load(resourceManager, name, cacheLevel, directory);
    return S3D_OK;
}

result Things::SetAlternateAnimation(const char16_t* name, const char16_t* directory)
{
    if (alternateObject == nullptr) {
        return S3D_STATE;
    }
    if (name == nullptr) {
        alternateAnimation = nullptr;
        alternateObject->SetAnimation(nullptr);
        return S3D_OK;
    }
    if (resourceManager == nullptr) {
        return S3D_FILE;
    }
    alternateAnimation = dynamic_cast<const Animation*>(resourceManager->GetResource(
        ResourceAnimation, cacheLevel, name, directory));
    alternateObject->SetAnimation(alternateAnimation);
    alternateObject->SetAnimationPlayback(1, false);
    return S3D_OK;
}

void Things::StartDestruction(uint16_t duration)
{
    destroying = true;
    destructionDuration = duration;
    destructionTimer = 0;
}

bool Things::MoveTowards(const Vec3& target, Fixed distance)
{
    const Fixed squared = SquaredLength(Subtract(target, position));
    if (squared >= 0 && squared <= FixedMultiply(distance, distance)) {
        position = target;
        return true;
    }
    position = Add(position, Scale(Normalize(Subtract(target, position)), distance));
    return false;
}

void Things::UpdateCurrentLeaf()
{
    currentLeaf = bsp != nullptr ? bsp->FindLeaf(position) : -1;
}

bool Things::IsDestroying() const { return destroying; }
uint16_t Things::GetDestructionTimer() const { return destructionTimer; }

Creature::Creature(s3dResManager* resources, int level, Renderer* objectRenderer,
                   AudioManager* audio, Bsp* map)
    : PhysicUnit(resources, level, objectRenderer, audio, map)
{
    unitType = 6;
}

void Creature::SetNavigationVelocity(const Vec3& target)
{
    velocity = Subtract(target, position);
    velocity.y = 0;
    if (AbsFixed(velocity.x) >= 0x28f || AbsFixed(velocity.y) >= 0x28f ||
        AbsFixed(velocity.z) >= 0x28f) {
        velocity = Scale(Normalize(velocity), navigationSpeed);
    }
}

bool Creature::NavigateTo(const Vec3& target, Fixed radius)
{
    navigationTarget = target;
    Vec3 horizontalTarget = position;
    horizontalTarget.x = target.x;
    horizontalTarget.z = target.z;
    const Fixed distance = SquaredLength(Subtract(horizontalTarget, position));
    if (distance >= 0 && distance < FixedMultiply(radius, radius)) {
        velocity = {};
        destinationReached = false;
        position = horizontalTarget;
        return true;
    }

    ObjectBounds pathBounds = localBounds;
    pathBounds.minimum.y = AddFixed(pathBounds.minimum.y, 0xe666);
    pathBounds.maximum.y = SubtractFixed(pathBounds.maximum.y, 0xe666);
    BspTraceResult trace;
    if (bsp != nullptr) {
        trace = bsp->Trace(position, target, pathBounds);
    } else {
        trace.endPosition = target;
    }
    if (!trace.startSolid) {
        SetNavigationVelocity(target);
        return false;
    }

    const Vec3 adjusted = Add(target, trace.planeNormal);
    if (bsp != nullptr) {
        trace = bsp->Trace(position, adjusted, pathBounds);
    }
    if (!trace.startSolid) {
        SetNavigationVelocity(target);
        return false;
    }

    const BspPathNode* startNode = bsp != nullptr
        ? bsp->FindNearestPathNode(position, pathBounds, previousPathNode) : nullptr;
    const BspPathNode* targetNode = bsp != nullptr
        ? bsp->FindNearestPathNode(target, pathBounds, nullptr) : nullptr;
    if (startNode == nullptr || targetNode == nullptr) {
        SetNavigationVelocity(target);
        return false;
    }
    if (currentPathNode != startNode) {
        reachedCurrentPathNode = false;
        previousPathNode = currentPathNode;
        currentPathNode = startNode;
    }
    if (SquaredLength(Subtract(startNode->position, position)) < fixedOne) {
        reachedCurrentPathNode = true;
    }
    if (startNode == targetNode) {
        SetNavigationVelocity(reachedCurrentPathNode ? target : startNode->position);
        return false;
    }

    const BspPathNode* nextNode = nullptr;
    uint16_t cumulative = 0;
    const BspPathPair* pair = nullptr;
    for (uint16_t index = 0; index < startNode->pairCount; ++index) {
        pair = &bsp->GetPathPairs()[startNode->firstPair + index];
        cumulative = static_cast<uint16_t>(cumulative + pair->nodes[1]);
        if (targetNode->index < cumulative) {
            nextNode = bsp->GetPathNode(pair->nodes[0]);
            break;
        }
    }
    if (nextNode == nullptr) {
        SetNavigationVelocity(target);
        return false;
    }
    if (previousPathNode == nextNode) {
        previousPathNode = nullptr;
    }
    const BspTraceResult nextTrace = bsp->Trace(position, nextNode->position, pathBounds);
    if (nextTrace.startSolid && !reachedCurrentPathNode) {
        SetNavigationVelocity(startNode->position);
    } else {
        SetNavigationVelocity(nextNode->position);
    }
    return false;
}

void Creature::SetNavigationEnabled(bool enabled)
{
    navigationEnabled = enabled;
    if (enabled) {
        destinationReached = false;
        velocity = {};
    }
}

bool Creature::IsPointInside(const Vec3& point) { return QueryPoint(point) != nullptr; }
bool Creature::IsSegmentInside(const Vec3& start, const Vec3& end)
{
    return QuerySegment(start, end) != nullptr;
}
bool Creature::IsBoundsInside(const ObjectBounds& bounds) { return QueryBounds(bounds) != nullptr; }
bool Creature::IsWithinRadius(const Vec3& center, Fixed radius)
{
    return Length(Subtract(center, position)) <= radius;
}

bool Creature::HearSound(const Vec3& source, Fixed range, int32_t soundId)
{
    const Fixed distance = Length(Subtract(source, position));
    if (distance < 0 || distance >= range) {
        if (lastHeardSound == soundId) {
            lastHeardSound = -1;
        }
        if (source.x == lastHeardPosition.x && source.y == lastHeardPosition.y &&
            source.z == lastHeardPosition.z) {
            lastHeardPosition = {};
        }
        return false;
    }
    if (soundId < 0) {
        if (source.x != lastHeardPosition.x || source.y != lastHeardPosition.y ||
            source.z != lastHeardPosition.z) {
            lastHeardPosition = source;
            return true;
        }
        return false;
    }
    if (lastHeardSound == soundId) {
        return false;
    }
    lastHeardSound = soundId;
    return true;
}

void Creature::SnapToGround()
{
    if (bsp != nullptr) {
        Vec3 end = position;
        end.y = SubtractFixed(end.y, 0xa0000);
        const BspTraceResult trace = bsp->Trace(position, end, localBounds);
        if (trace.startSolid) {
            position = trace.endPosition;
        }
    }
    previousPosition = position;
}

void Creature::SetNavigationSpeed(Fixed value) { navigationSpeed = value; }
Fixed Creature::GetNavigationSpeed() const { return navigationSpeed; }
const Vec3& Creature::GetNavigationTarget() const { return navigationTarget; }
bool Creature::IsNavigationEnabled() const { return navigationEnabled; }
void Creature::ResetNavigationPath()
{
    previousPathNode = nullptr;
    currentPathNode = nullptr;
    reachedCurrentPathNode = false;
}

GameUnitManager::GameUnitManager(const UnitManagerContext& managerContext) : context(managerContext) {}

void GameUnitManager::SetEngine(Engine* engine)
{
    s3dBase::SetEngine(engine);
}

bool GameUnitManager::ValidBucket(int bucket) const
{
    return bucket >= 0 && bucket < static_cast<int>(buckets.size());
}

void GameUnitManager::AddUnit(GameUnit* unit, int bucket)
{
    if (unit == nullptr || !ValidBucket(bucket)) {
        return;
    }
    Bucket& target = buckets[bucket];
    unit->nextUnit = target.first;
    target.first = unit;
    ++target.count;
}

void GameUnitManager::RemoveUnit(GameUnit* unit, int bucket, bool destroy)
{
    if (unit == nullptr || !ValidBucket(bucket)) {
        return;
    }
    Bucket& target = buckets[bucket];
    GameUnit* previous = nullptr;
    GameUnit* current = target.first;
    while (current != nullptr && current != unit) {
        previous = current;
        current = current->nextUnit;
    }
    if (current == nullptr) {
        return;
    }
    if (previous == nullptr) {
        target.first = current->nextUnit;
    } else {
        previous->nextUnit = current->nextUnit;
    }
    current->nextUnit = nullptr;
    --target.count;
    if (destroy) {
        delete current;
    }
}

void GameUnitManager::RemoveUnitById(int32_t id, int bucket, bool destroy)
{
    if (!ValidBucket(bucket)) {
        return;
    }
    Bucket& target = buckets[bucket];
    GameUnit* previous = nullptr;
    GameUnit* current = target.first;
    while (current != nullptr && current->unitId != id) {
        previous = current;
        current = current->nextUnit;
    }
    if (current == nullptr) {
        return;
    }
    if (previous == nullptr) {
        target.first = current->nextUnit;
    } else {
        previous->nextUnit = current->nextUnit;
    }
    current->nextUnit = nullptr;
    --target.count;
    if (destroy) {
        delete current;
    }
}

void GameUnitManager::ClearBucket(int bucket, bool destroy)
{
    if (!ValidBucket(bucket)) {
        return;
    }
    if (destroy) {
        GameUnit* current = buckets[bucket].first;
        while (current != nullptr) {
            GameUnit* next = current->nextUnit;
            delete current;
            current = next;
        }
    }
    buckets[bucket] = {};
}

void GameUnitManager::ResetBuckets()
{
    for (Bucket& bucket : buckets) {
        bucket = {};
    }
}

GameUnit* GameUnitManager::FindUnit(int bucket, int32_t id) const
{
    if (!ValidBucket(bucket)) {
        return nullptr;
    }
    for (GameUnit* unit = buckets[bucket].first; unit != nullptr; unit = unit->nextUnit) {
        if (unit->unitId == id) {
            return unit;
        }
    }
    return nullptr;
}

void GameUnitManager::UpdateUnits()
{
    for (Bucket& bucket : buckets) {
        GameUnit* unit = bucket.first;
        while (unit != nullptr) {
            GameUnit* next = unit->nextUnit;
            unit->Update();
            unit = next;
        }
    }
}

void GameUnitManager::PostUpdateUnits()
{
    for (Bucket& bucket : buckets) {
        for (GameUnit* unit = bucket.first; unit != nullptr; unit = unit->nextUnit) {
            unit->PostUpdate();
        }
    }
}

void GameUnitManager::SubmitUnits(RenderQueues& queues)
{
    for (Bucket& bucket : buckets) {
        for (GameUnit* unit = bucket.first; unit != nullptr; unit = unit->nextUnit) {
            if (unit->visible) {
                unit->SubmitRenderables(queues);
            }
        }
    }
}

void GameUnitManager::ResetQuery() { queryResult = {}; }

const UnitQueryResult& GameUnitManager::QueryPoint(const Vec3& point, int bucket,
                                                   const GameUnit* excluded)
{
    ResetQuery();
    const int first = bucket == -1 ? 0 : bucket;
    const int last = bucket == -1 ? 3 : bucket + 1;
    if (first < 0 || last > static_cast<int>(buckets.size())) {
        return queryResult;
    }
    for (int index = first; index < last; ++index) {
        for (GameUnit* unit = buckets[index].first; unit != nullptr; unit = unit->nextUnit) {
            if (unit == excluded || (bucket == -1 && !unit->visible)) {
                continue;
            }
            GameUnit* hit = unit->QueryPoint(point);
            if (hit != nullptr && queryResult.count < static_cast<int>(queryResult.units.size())) {
                queryResult.units[queryResult.count++] = hit;
            }
        }
    }
    return queryResult;
}

const UnitQueryResult& GameUnitManager::QuerySegment(const Vec3& start, const Vec3& end,
                                                     int bucket, const GameUnit* excluded)
{
    ResetQuery();
    const int first = bucket == -1 ? 0 : bucket;
    const int last = bucket == -1 ? 3 : bucket + 1;
    if (first < 0 || last > static_cast<int>(buckets.size())) {
        return queryResult;
    }
    for (int index = first; index < last; ++index) {
        for (GameUnit* unit = buckets[index].first; unit != nullptr; unit = unit->nextUnit) {
            if (unit == excluded || (bucket == -1 && !unit->visible)) {
                continue;
            }
            GameUnit* hit = unit->QuerySegment(start, end);
            if (hit != nullptr && queryResult.count < static_cast<int>(queryResult.units.size())) {
                queryResult.units[queryResult.count++] = hit;
            }
        }
    }
    return queryResult;
}

const UnitQueryResult& GameUnitManager::QueryBounds(const ObjectBounds& bounds, int bucket,
                                                    const GameUnit* excluded)
{
    ResetQuery();
    const int first = bucket == -1 ? 0 : bucket;
    const int last = bucket == -1 ? 3 : bucket + 1;
    if (first < 0 || last > static_cast<int>(buckets.size())) {
        return queryResult;
    }
    for (int index = first; index < last; ++index) {
        for (GameUnit* unit = buckets[index].first; unit != nullptr; unit = unit->nextUnit) {
            if (unit == excluded) {
                continue;
            }
            GameUnit* hit = unit->QueryBounds(bounds);
            if (hit != nullptr && queryResult.count < static_cast<int>(queryResult.units.size())) {
                queryResult.units[queryResult.count++] = hit;
            }
        }
    }
    return queryResult;
}

std::size_t GameUnitManager::GetBucketCount(int bucket) const
{
    return ValidBucket(bucket) ? buckets[bucket].count : 0;
}

GameUnit* GameUnitManager::GetFirstUnit(int bucket) const
{
    return ValidBucket(bucket) ? buckets[bucket].first : nullptr;
}

const UnitManagerContext& GameUnitManager::GetContext() const { return context; }

} // namespace soft3d

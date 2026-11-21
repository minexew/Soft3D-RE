#include "game/Triggers.hpp"

#include "soft3d/s3dResManager.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

namespace game {
namespace {

std::string EntityText(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp, int index)
{
    if (index < 0 || index >= record.pairCount) {
        return {};
    }
    const auto& pair = bsp.GetEntityPairs()[static_cast<std::size_t>(record.firstPair + index)];
    char text[33];
    std::memcpy(text, pair.key.data(), 16);
    std::memcpy(text + 16, pair.value.data(), 16);
    text[32] = '\0';
    return text;
}

int32_t ParseInt(const std::string& text)
{
    return static_cast<int32_t>(std::strtol(text.c_str(), nullptr, 10));
}

soft3d::Fixed ParseFixed(const std::string& text)
{
    const char* begin = text.c_str();
    while (std::isspace(static_cast<unsigned char>(*begin)) != 0) {
        ++begin;
    }
    if (std::strchr(begin, '.') == nullptr && std::strchr(begin, 'e') == nullptr) {
        const char* digits = begin;
        if (*digits == '-' || *digits == '+') {
            ++digits;
        }
        const int32_t value = static_cast<int32_t>(std::strtol(digits, nullptr, 10));
        return soft3d::ShiftLeftFixed(value, 16);
    }
    return static_cast<soft3d::Fixed>(std::strtod(begin, nullptr) * 65536.0);
}

soft3d::Vec3 ParseVector(const std::string& text)
{
    soft3d::Vec3 result;
    const std::size_t first = text.find(',');
    const std::size_t second = first == std::string::npos ? first : text.find(',', first + 1);
    if (first == std::string::npos || second == std::string::npos) {
        return result;
    }
    const std::size_t begin = text.find('(') == std::string::npos ? 0 : text.find('(') + 1;
    result.x = ParseFixed(text.substr(begin, first - begin));
    result.y = ParseFixed(text.substr(first + 1, second - first - 1));
    result.z = ParseFixed(text.substr(second + 1));
    return result;
}

bool LoadBounds(const soft3d::BspEntityRecord& record, const soft3d::Bsp& bsp,
                soft3d::ObjectBounds& bounds)
{
    if (record.brushIndex < 0 ||
        static_cast<std::size_t>(record.brushIndex) >= bsp.GetEntityBrushes().size()) {
        return false;
    }
    const auto& values = bsp.GetEntityBrushes()[static_cast<std::size_t>(record.brushIndex)].values;
    bounds.minimum = {soft3d::ShiftLeftFixed(values[0], 16),
                      soft3d::ShiftLeftFixed(values[1], 16),
                      soft3d::ShiftLeftFixed(values[2], 16)};
    bounds.maximum = {soft3d::ShiftLeftFixed(values[3], 16),
                      soft3d::ShiftLeftFixed(values[4], 16),
                      soft3d::ShiftLeftFixed(values[5], 16)};
    return true;
}

} // namespace

CameraTrigger::CameraTrigger(CameraController* newController, soft3d::Creature* newPlayer)
    : controller(newController)
    , player(newPlayer)
{
    unitType = 0x20;
}

void CameraTrigger::Configure(CameraController* newController, soft3d::Creature* newPlayer)
{
    controller = newController;
    player = newPlayer;
}

soft3d::result CameraTrigger::LoadEntity(const soft3d::BspEntityRecord& record,
                                         const soft3d::Bsp& bsp,
                                         soft3d::s3dResManager& resources)
{
    active = false;
    mode = ParseInt(EntityText(record, bsp, 0));
    cameraPosition = ParseVector(EntityText(record, bsp, 1));
    direction = ParseVector(EntityText(record, bsp, 2));
    time = ParseFixed(EntityText(record, bsp, 3));
    snap = ParseInt(EntityText(record, bsp, 4)) != 0;
    if (mode == CameraController::Path) {
        paths = std::make_unique<soft3d::CameraPathCollection>(64);
        const std::string pathName = ".\\common\\" + EntityText(record, bsp, 5);
        std::u16string wideName;
        wideName.reserve(pathName.size());
        for (const unsigned char value : pathName) {
            wideName.push_back(value);
        }
        const soft3d::result status = soft3d::LoadSplCameraResource(
            *paths, wideName.c_str(), resources.GetFilePak());
        if (status != soft3d::S3D_OK) {
            return status;
        }
    }
    pathSpeed = ParseFixed(EntityText(record, bsp, 6));
    targetMode = ParseInt(EntityText(record, bsp, 7));
    return LoadBounds(record, bsp, collisionBounds) ? soft3d::S3D_OK : soft3d::S3D_FORMAT;
}

void CameraTrigger::SetMode(int32_t newMode, const soft3d::Vec3& newPosition,
                            const soft3d::Vec3& newDirection, soft3d::Fixed newTime,
                            std::unique_ptr<soft3d::CameraPathCollection> newPaths,
                            soft3d::Fixed newPathSpeed, bool newSnap, int32_t newTargetMode)
{
    mode = newMode;
    cameraPosition = newPosition;
    direction = newDirection;
    time = newTime;
    paths = std::move(newPaths);
    pathSpeed = newPathSpeed;
    snap = newSnap;
    targetMode = newTargetMode;
}

void CameraTrigger::Update()
{
    if (controller == nullptr || player == nullptr || controller->IsFrozen()) {
        return;
    }
    const bool inside = QueryBounds(player->GetCollisionBounds()) != nullptr;
    if (inside) {
        if (outside) {
            controller->SetMode(mode, cameraPosition, direction, time, paths.get(), pathSpeed,
                                snap, targetMode);
            if (controller->GetTriggerCount() == 0) {
                controller->SaveForTrigger();
            }
            controller->AddTrigger(this);
            outside = false;
        }
        return;
    }
    if (outside) {
        return;
    }
    if (controller->GetTriggerCount() < 2) {
        controller->RemoveTrigger(this);
        controller->RestoreForTrigger(snap);
    } else if (controller->triggers.back() == this) {
        controller->RemoveTrigger(this);
        CameraTrigger* previous = controller->triggers.back();
        controller->SetMode(previous->GetMode(), previous->GetPosition(), previous->GetDirection(),
                            previous->GetTime(), previous->GetPaths(), previous->GetPathSpeed(),
                            previous->GetSnap(), previous->GetTargetMode());
    } else {
        controller->RemoveTrigger(this);
    }
    outside = true;
}

void CameraTrigger::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(mode);
    soft3d::WriteVec3(stream, cameraPosition);
    soft3d::WriteVec3(stream, direction);
    soft3d::WriteFixed(stream, time);
    stream.WriteBool(snap);
    soft3d::WriteFixed(stream, pathSpeed);
    stream.WriteBool(active);
    stream.WriteBool(outside);
    stream.WriteInt32(targetMode);
}

void CameraTrigger::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(mode);
    soft3d::ReadVec3(stream, cameraPosition);
    soft3d::ReadVec3(stream, direction);
    soft3d::ReadFixed(stream, time);
    stream.ReadBool(snap);
    soft3d::ReadFixed(stream, pathSpeed);
    stream.ReadBool(active);
    stream.ReadBool(outside);
    stream.ReadInt32(targetMode);
}

LightTrigger::LightTrigger(soft3d::Creature* newPlayer)
    : player(newPlayer)
{
    unitType = 0x24;
}

soft3d::result LightTrigger::LoadEntity(const soft3d::BspEntityRecord& record,
                                        const soft3d::Bsp& bsp)
{
    usePlayerDirection = ParseInt(EntityText(record, bsp, 0)) != 0;
    position = ParseVector(EntityText(record, bsp, 1));
    lightDirection = ParseVector(EntityText(record, bsp, 2));
    return LoadBounds(record, bsp, collisionBounds) ? soft3d::S3D_OK : soft3d::S3D_FORMAT;
}

void LightTrigger::Update()
{
    if (player == nullptr || QueryBounds(player->GetCollisionBounds()) == nullptr) {
        return;
    }
    soft3d::Renderer* renderer = player->GetRenderer();
    if (renderer == nullptr) {
        return;
    }
    renderer->SetGlobalLightDirection(usePlayerDirection
        ? soft3d::Normalize(soft3d::Subtract(player->GetPosition(), position))
        : lightDirection);
}

} // namespace game

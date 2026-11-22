#include "game/Triggers.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/s3dResManager.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
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

InteractionState::InteractionState(MyGameUnitMgr* newManager)
    : manager(newManager)
{
}

void InteractionState::Reset()
{
    prompt = -1;
    dialogKind = -1;
    dialogText = -1;
    dialogOptionCount = 0;
    dialogResult = -1;
    dialogPortrait = false;
    dialogOptions.fill(0);
    if (manager != nullptr) {
        manager->SetPaused(false);
    }
}

void InteractionState::SetDialogResult(int32_t result)
{
    dialogResult = result;
    if (manager != nullptr) {
        manager->SetPaused(false);
    }
}

void InteractionState::ShowMessage(int32_t textId, int32_t kind)
{
    dialogKind = kind;
    dialogText = textId;
    dialogOptionCount = 0;
    dialogResult = -1;
    dialogPortrait = false;
    if (manager != nullptr) {
        manager->SetPaused(true);
    }
}

void InteractionState::SetChoiceString(int32_t index, int32_t textId)
{
    if (index >= 0 && index < static_cast<int32_t>(dialogOptions.size())) {
        dialogOptions[static_cast<std::size_t>(index)] = textId;
    }
}

void InteractionState::ShowChoice(int32_t titleId, int32_t optionCount, bool portrait)
{
    dialogKind = 1;
    dialogText = titleId;
    dialogOptionCount = optionCount;
    dialogResult = -1;
    dialogPortrait = portrait;
    if (manager != nullptr) {
        manager->SetPaused(true);
    }
}

int32_t InteractionState::GetDialogOption(int32_t index) const
{
    return index >= 0 && index < static_cast<int32_t>(dialogOptions.size())
        ? dialogOptions[static_cast<std::size_t>(index)] : -1;
}

namespace {

bool PlayerInside(soft3d::GameUnit& trigger, const Player* player)
{
    return player != nullptr && trigger.QueryBounds(player->GetInteractionBounds()) != nullptr;
}

void ClearPrompt(Player* player, TriggerUiHost* ui)
{
    if (ui != nullptr) {
        ui->SetPrompt(-1);
    }
    if (player != nullptr) {
        player->SetAttacksDisabled(false);
    }
}

bool InteractionClick(soft3d::ControlMap* controls, soft3d::GameEngine& engine)
{
    return controls != nullptr && controls->ActionClick(engine.GetInput(), 5, false);
}

} // namespace

EventTrigger::EventTrigger(Player* newPlayer, TriggerUiHost* newUi)
    : player(newPlayer)
    , ui(newUi)
{
    unitType = 0x1b;
}

void EventTrigger::Configure(Player* newPlayer, TriggerUiHost* newUi)
{
    player = newPlayer;
    ui = newUi;
}

void EventTrigger::Update()
{
    if (remainingCount == 0) {
        return;
    }
    if (PlayerInside(*this, player)) {
        if (outside != 0) {
            if (ui != nullptr) {
                ui->ShowMessage(textId, 0);
            }
            if (remainingCount > 0) {
                --remainingCount;
            }
            outside = 0;
        }
    } else {
        outside = 1;
    }
}

void EventTrigger::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt16(remainingCount);
    stream.WriteInt32(textId);
    stream.WriteInt16(outside);
}

void EventTrigger::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt16(remainingCount);
    stream.ReadInt32(textId);
    stream.ReadInt16(outside);
}

PickTrigger::PickTrigger(MyGameUnitMgr* newManager, Player* newPlayer,
                         soft3d::ControlMap* newControls, TriggerUiHost* newUi)
    : manager(newManager)
    , player(newPlayer)
    , controls(newControls)
    , ui(newUi)
{
    unitType = 0x25;
}

void PickTrigger::Configure(Pickable* newPickable, MyGameUnitMgr* newManager,
                            Player* newPlayer, soft3d::ControlMap* newControls,
                            TriggerUiHost* newUi)
{
    pickable = newPickable;
    manager = newManager;
    player = newPlayer;
    controls = newControls;
    ui = newUi;
}

void PickTrigger::SetEnabled(bool value)
{
    if (!value && promptActive != 0 && ui != nullptr) {
        ui->SetPrompt(-1);
    }
    enabled = value ? 1 : 0;
}

bool PickTrigger::IsAvailable() const
{
    return enabled != 0 && pickable != nullptr && pickable->GetItemId() != -1;
}

void PickTrigger::TryPickUp()
{
    if (!IsAvailable() || promptActive == 0) {
        return;
    }
    pickable->PickUp(player);
    if (manager != nullptr) {
        manager->RemoveUnit(pickable, 2, true);
    }
    pickable = nullptr;
    if (ui != nullptr) {
        ui->SetPrompt(-1);
    }
    promptActive = 0;
}

void PickTrigger::Update()
{
    if (player == nullptr || !IsAvailable() || player->IsDead()) {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
            promptActive = 0;
        }
        return;
    }

    if (PlayerInside(*this, player)) {
        promptActive = 1;
        player->SetAttacksDisabled(true);
        if (ui != nullptr) {
            ui->SetPrompt(0);
        }
    } else {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
        }
        promptActive = 0;
    }

    auto* scene = manager != nullptr
        ? static_cast<Scene*>(manager->GetContext().sceneContext) : nullptr;
    if (promptActive != 0 && dialogActive == 0 && scene != nullptr &&
        InteractionClick(controls, scene->GetEngine())) {
        const int32_t itemId = pickable->GetItemId();
        if (itemId < 1000 && player->IsInventoryFull(itemId)) {
            if (ui != nullptr) {
                ui->ShowMessage(5, 3);
            }
        } else {
            dialogActive = 1;
            if (ui != nullptr) {
                ui->SetChoiceString(0, 3);
                ui->SetChoiceString(1, 4);
                int32_t itemName = 0;
                const soft3d::GameContent& content = scene->GetEngine().GetContent();
                if (itemId >= 0 && itemId < 1001) {
                    itemName = static_cast<int32_t>(content.items[static_cast<std::size_t>(itemId)].nameString);
                } else if (itemId >= 1001 && itemId < 2001) {
                    itemName = static_cast<int32_t>(content.documents[static_cast<std::size_t>(itemId - 1001)].titleString);
                } else if (itemId >= 2001 && itemId < 2101) {
                    itemName = static_cast<int32_t>(content.photos[static_cast<std::size_t>(itemId - 2001)].titleString);
                }
                ui->SetChoiceString(4, itemName);
                ui->ShowChoice(dialogTitleId, 2, true);
            }
        }
    }

    if (dialogActive != 0 && ui != nullptr) {
        const int32_t result = ui->GetDialogResult();
        if (result == 0) {
            TryPickUp();
        } else if (result == 1) {
            dialogActive = 0;
        }
    }
}

void PickTrigger::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(enabled);
    stream.WriteInt32(dialogTitleId);
    stream.WriteInt32(pickable != nullptr ? pickable->GetUnitId() : 0);
}

void PickTrigger::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(enabled);
    stream.ReadInt32(dialogTitleId);
}

DoorTrigger::DoorTrigger(Scene* newScene, MyGameUnitMgr* newManager, Player* newPlayer,
                         soft3d::ControlMap* newControls, TriggerUiHost* newUi)
    : scene(newScene)
    , manager(newManager)
    , player(newPlayer)
    , controls(newControls)
    , ui(newUi)
{
    unitType = 0x22;
}

void DoorTrigger::Configure(Scene* newScene, MyGameUnitMgr* newManager, Player* newPlayer,
                            soft3d::ControlMap* newControls, TriggerUiHost* newUi)
{
    scene = newScene;
    manager = newManager;
    player = newPlayer;
    controls = newControls;
    ui = newUi;
}

void DoorTrigger::SetEnabled(bool value)
{
    if (!value && promptActive != 0) {
        if (ui != nullptr) {
            ui->SetPrompt(-1);
        }
        unlocking = 0;
        unlockTimer = 0;
    }
    enabled = value ? 1 : 0;
}

void DoorTrigger::OpenDoor()
{
    if (scene != nullptr) {
        scene->SetLoadMapParams(doorResourceIndex, spawnPosition, spawnDirection, 0);
        scene->LoadNextLevel(nextMapId, nextEpisodeId);
    }
    if (ui != nullptr) {
        ui->SetPrompt(-1);
    }
}

void DoorTrigger::Update()
{
    if (enabled == 0 || player == nullptr || player->IsDead()) {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
            promptActive = 0;
        }
        return;
    }
    if (unlocking != 0) {
        ++unlockTimer;
        if (unlockTimer == 8) {
            OpenDoor();
        }
        return;
    }

    if (PlayerInside(*this, player)) {
        promptActive = 1;
        player->SetAttacksDisabled(true);
        if (ui != nullptr) {
            ui->SetPrompt(1);
        }
    } else {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
        }
        promptActive = 0;
    }

    if (promptActive != 0 && dialogActive == 0 && scene != nullptr &&
        InteractionClick(controls, scene->GetEngine())) {
        if (unitId == 10000 && scene->GetDoorAuthorizationStatus() < 1) {
            scene->BeginAuthorization();
            scene->GetEngine().GetInput().ClearKey();
        }
        const bool tracked = unitId >= 10000 && unitId < 11024;
        if (tracked && manager != nullptr && manager->GetDoorState(unitId) == 1) {
            OpenDoor();
        } else if (requiredItem == 0 || player->HasItem(requiredItem)) {
            if (requiredItem < 1) {
                OpenDoor();
            } else {
                player->PlaySound(8, false);
                player->RemoveItem(requiredItem);
                unlocking = 1;
            }
            if (tracked && manager != nullptr) {
                manager->SetDoorState(unitId, 1);
            }
        } else {
            player->PlaySound(9, false);
            dialogActive = 1;
            if (ui != nullptr) {
                ui->ShowMessage(lockedTextId, 3);
            }
            if (tracked && manager != nullptr) {
                manager->SetDoorState(unitId, requiredItem < 0 ? 3 : 2);
            }
        }
    }
    if (dialogActive != 0 && ui != nullptr && ui->GetDialogResult() >= 0) {
        dialogActive = 0;
    }
}

void DoorTrigger::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(doorResourceIndex);
    stream.WriteInt32(nextMapId);
    stream.WriteInt32(nextEpisodeId);
    soft3d::WriteVec3(stream, spawnPosition);
    soft3d::WriteVec3(stream, spawnDirection);
    stream.WriteInt32(requiredItem);
    stream.WriteInt32(lockedTextId);
}

void DoorTrigger::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(doorResourceIndex);
    stream.ReadInt32(nextMapId);
    stream.ReadInt32(nextEpisodeId);
    soft3d::ReadVec3(stream, spawnPosition);
    soft3d::ReadVec3(stream, spawnDirection);
    stream.ReadInt32(requiredItem);
    stream.ReadInt32(lockedTextId);
}

ActTrigger::ActTrigger(CameraController* newCamera, Scene* newScene, Player* newPlayer,
                       soft3d::ControlMap* newControls, TriggerUiHost* newUi)
    : camera(newCamera)
    , scene(newScene)
    , player(newPlayer)
    , controls(newControls)
    , ui(newUi)
{
    unitType = 0x21;
}

void ActTrigger::Configure(CameraController* newCamera, Scene* newScene, Player* newPlayer,
                           soft3d::ControlMap* newControls, TriggerUiHost* newUi)
{
    camera = newCamera;
    scene = newScene;
    player = newPlayer;
    controls = newControls;
    ui = newUi;
}

void ActTrigger::SetEnabled(bool value)
{
    if (!value && promptActive != 0 && ui != nullptr) {
        ui->SetPrompt(-1);
    }
    enabled = value ? 1 : 0;
    cameraPosition = {};
    cameraTarget = {};
}

void ActTrigger::SetOptionCount(int32_t value)
{
    if (value < 0 || value > static_cast<int32_t>(optionIds.size())) {
        throw std::out_of_range("ActTrigger option count");
    }
    optionCount = value;
}

void ActTrigger::SetString(int32_t index, int32_t textId)
{
    if (index == 0) {
        optionIds[0] = textId;
    } else if (index == 1) {
        reserved5c = textId;
    } else if (index >= 2 && index <= 4) {
        optionIds[static_cast<std::size_t>(index - 2)] = textId;
    } else if (index == 5) {
        titleId = textId;
    } else if (index == 6) {
        promptActive = textId;
    } else {
        throw std::out_of_range("ActTrigger string index");
    }
}

void ActTrigger::SetCameraVectors(const soft3d::Vec3& position, const soft3d::Vec3& target)
{
    cameraPosition = position;
    cameraTarget = target;
}

int32_t ActTrigger::GetOptionId(int32_t index) const
{
    return index >= 0 && index < static_cast<int32_t>(optionIds.size())
        ? optionIds[static_cast<std::size_t>(index)] : -1;
}

bool ActTrigger::IsSuppressed() const
{
    return player != nullptr && player->HasAttackingEnemy() && optionCount == 0;
}

bool ActTrigger::HasCameraOverride() const
{
    return cameraPosition.x != cameraTarget.x || cameraPosition.y != cameraTarget.y ||
           cameraPosition.z != cameraTarget.z;
}

void ActTrigger::Update()
{
    if (enabled == 0 || player == nullptr || player->IsDead()) {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
            promptActive = 0;
        }
        return;
    }

    if (PlayerInside(*this, player) && !IsSuppressed()) {
        promptActive = 1;
        player->SetAttacksDisabled(true);
        if (ui != nullptr) {
            ui->SetPrompt(3);
        }
    } else {
        if (promptActive != 0) {
            ClearPrompt(player, ui);
        }
        promptActive = 0;
    }

    const bool canOpen = promptActive != 0 && dialogActive == 0 && scene != nullptr &&
                         InteractionClick(controls, scene->GetEngine());
    if (!canOpen) {
        if (dialogActive != 0 && ui != nullptr) {
            const int32_t result = ui->GetDialogResult();
            if (result != -1) {
                if (HasCameraOverride() && camera != nullptr) {
                    camera->SetFrozen(false);
                    scene->SetUseCameraListener(false);
                    camera->SetInstant(true);
                    if (!player->IsFpsActive()) {
                        player->SetVisible(true);
                    }
                }
                selection = result + 1;
                dialogActive = 0;
            }
        }
        return;
    }

    if (HasCameraOverride() && camera != nullptr) {
        camera->SetFrozen(true);
        camera->SetMode(CameraController::FixedMode, cameraPosition,
                        soft3d::Normalize(soft3d::Subtract(cameraTarget, cameraPosition)),
                        0, nullptr, 0, false, 0);
        camera->Update(nullptr, false);
        player->SetVisible(false);
    }
    dialogActive = 1;
    if (ui != nullptr) {
        if (optionCount < 1) {
            ui->ShowMessage(titleId, 3);
        } else {
            for (int32_t index = 0; index < optionCount; ++index) {
                ui->SetChoiceString(index, optionIds[static_cast<std::size_t>(index)]);
            }
            ui->ShowChoice(titleId, optionCount, true);
        }
    }
}

void ActTrigger::SaveState(soft3d::OutputStream& stream) const
{
    GameUnit::SaveState(stream);
    stream.WriteInt32(enabled);
    stream.WriteInt32(optionCount);
    stream.WriteInt32(reserved5c);
    for (const int32_t optionId : optionIds) {
        stream.WriteInt32(optionId);
    }
    stream.WriteInt32(titleId);
}

void ActTrigger::LoadState(soft3d::InputStream& stream)
{
    GameUnit::LoadState(stream);
    stream.ReadInt32(enabled);
    stream.ReadInt32(optionCount);
    stream.ReadInt32(reserved5c);
    for (int32_t& optionId : optionIds) {
        stream.ReadInt32(optionId);
    }
    stream.ReadInt32(titleId);
}

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

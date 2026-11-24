#include "game/Lord.hpp"

#include "game/MyGameUnitMgr.hpp"
#include "game/Scene.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/GameUIDlg.hpp"
#include "soft3d/s3dResManager.hpp"

#include <array>
#include <climits>

namespace game {
namespace {

} // namespace

Lord::Lord(const soft3d::UnitManagerContext& context, const GameData& gameData)
    : Enemy(context.resources, context.renderer, context.audio, context.bsp), data(gameData)
{
    ConfigureEnemy(*this, 8);
    cacheLevel = 3;
    sword = std::make_unique<soft3d::Object>(context.renderer);
    shield = std::make_unique<soft3d::Object>(context.renderer);
}

void Lord::InitializeAnimations(const soft3d::Animation* const*)
{
    static constexpr std::array<int32_t, 6> common{136, -1, 138, 142, 145, -1};
    for (std::size_t index = 0; index < common.size(); ++index) {
        SetAnimationResource(static_cast<int32_t>(index),
            common[index] < 0 ? nullptr : data.GetAnimationName(static_cast<std::size_t>(common[index])));
    }
    static constexpr std::array<int32_t, 19> body{
        137, 139, 140, 141, 143, 144, 146, 147, 148, 149,
        150, 151, 152, 153, 154, 155, 156, 157, 158,
    };
    for (std::size_t index = 0; index < body.size(); ++index) {
        bodyAnimations[index] = LoadEnemyAnimationResource(resourceManager, data, body[index]);
    }
    static constexpr std::array<int32_t, 16> swordNames{
        159, -1, 160, 163, 164, -1, 161, 162, 165, 166, 167, 168, 169, 170, 171, 172,
    };
    for (std::size_t index = 0; index < swordNames.size(); ++index) {
        swordAnimations[index] = LoadEnemyAnimationResource(resourceManager, data, swordNames[index]);
    }
    static constexpr std::array<int32_t, 13> shieldNames{
        173, -1, 174, 176, 177, -1, 175, 178, 179, 180, 181, 182, 183,
    };
    for (std::size_t index = 0; index < shieldNames.size(); ++index) {
        shieldAnimations[index] = LoadEnemyAnimationResource(resourceManager, data, shieldNames[index]);
    }
}

void Lord::InitializeSounds(soft3d::AudioData* const*)
{
    static constexpr std::array<int32_t, 6> common{0, 95, 96, 97, 98, 99};
    for (std::size_t index = 0; index < common.size(); ++index) {
        SetSoundResource(static_cast<int32_t>(index),
            common[index] <= 0 ? nullptr : data.GetSoundName(static_cast<std::size_t>(common[index])));
    }
    for (std::size_t index = 0; index < extraSounds.size(); ++index) {
        extraSounds[index] = LoadEnemySoundResource(resourceManager, data, 100 + static_cast<int32_t>(index));
    }
}

const soft3d::Animation* Lord::GetBodyAnimation(std::size_t index) const
{
    return index < bodyAnimations.size() ? bodyAnimations[index] : nullptr;
}

const soft3d::Animation* Lord::GetSwordAnimation(std::size_t index) const
{
    return index < swordAnimations.size() ? swordAnimations[index] : nullptr;
}

const soft3d::Animation* Lord::GetShieldAnimation(std::size_t index) const
{
    return index < shieldAnimations.size() ? shieldAnimations[index] : nullptr;
}

soft3d::AudioData* Lord::GetExtraSound(std::size_t index) const
{
    return index < extraSounds.size() ? extraSounds[index] : nullptr;
}

void Lord::SetControlsDisabled(bool disabled)
{
    if (player == nullptr || player->GetScene() == nullptr) return;
    soft3d::ControlMap* controls = player->GetScene()->GetControlMap();
    if (controls->IsEnabled() == disabled) {
        player->GetScene()->GetEngine().GetInput().ClearKey();
    }
    controls->SetEnabled(!disabled);
}

void Lord::SetFixedCamera(const soft3d::Vec3& value, const soft3d::Vec3& direction)
{
    if (player == nullptr || player->GetScene() == nullptr) return;
    CameraController* camera = player->GetScene()->GetCameraController();
    camera->SetMode(CameraController::FixedMode, value, direction, 0, nullptr, 0, true, 0);
    camera->SetFrozen(true);
}

void Lord::RestorePlayerAndCamera(bool controlsEnabled)
{
    if (player == nullptr || player->GetScene() == nullptr) return;
    Scene* scene = player->GetScene();
    SetControlsDisabled(!controlsEnabled);
    player->SetVisible(true);
    CameraController* camera = scene->GetCameraController();
    camera->SetFrozen(false);
    scene->ResetCameraWarmup();
    camera->SetInstant(true);
}

void Lord::SetEncounter(int32_t phase, int32_t timer)
{
    encounterPhase = phase;
    encounterTimer = timer;
}

void Lord::Spawn(const soft3d::Vec3& value)
{
    fakeShadow = std::make_unique<soft3d::FakeShadow>(renderer);
    fakeShadow->Initialize(0x20000);
    LoadResources(u"r_nanjue_b", nullptr, nullptr);
    if (resourceManager != nullptr) {
        sword->Load(resourceManager, u"ow_dajian_atk_a", 3);
        sword->LoadMaterials(resourceManager, 3);
        shield->Load(resourceManager, u"ow_dun_atk_a", 3);
        shield->LoadMaterials(resourceManager, 3);
    }
    sword->SetMaterialTextureScroll(0, 0, 0x60000);
    shield->SetMaterialTextureScroll(0, 0xfffa0000, 0);
    if (GetObject(0) != nullptr) {
        GetObject(0)->SetMaterialTextureScroll(0, 0, 0x60000);
    }
    Enemy::Spawn(value);
    alertSpeed = 0x4ccc;
    chaseSpeed = 0x4ccc;
    equipmentStage = 0;
    stateTime = 0;
    leapPhase = 0;
    haste = 0;
    primaryHitImpulse = soft3d::fixedOne;
    alternateHitImpulse = 0x30000;
    StopMotion();
    if (player != nullptr) {
        player->LoadSpecialAnimations(data, player->GetScene()->GetEventFlag(1002));
    }
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    if (manager == nullptr) {
        renderActive = 1;
        encounterTimer = 4;
        encounterPhase = 1;
        return;
    }

    constexpr soft3d::Vec3 axisX{0x33333, 0, 0};
    constexpr soft3d::Vec3 axisY{0, 0x33333, 0};
    constexpr soft3d::Vec3 axisZ{0, 0, 0x33333};
    manager->AddThing(1, 0, -1, true, u"oszm_shuijing_aa",
                      {0x1440000, -0x1c0000, 0xaf0000}, axisX, axisY, axisZ);
    manager->AddThing(2, 0, -1, true, u"oszm_shuijing_aa",
                      {0x1440000, -0x1c0000, 0x700000}, axisX, axisY, axisZ);
    manager->AddThing(3, 0, -1, true, u"oszm_shuijing_aa",
                      {0x1840000, -0x1c0000, 0xaf0000}, axisX, axisY, axisZ);
    manager->AddThing(4, 0, -1, true, u"oszm_shuijing_aa",
                      {0x1840000, -0x1c0000, 0x700000}, axisX, axisY, axisZ);
    const bool alternate = player != nullptr && player->GetScene()->GetEventFlag(1002);
    soft3d::Things* weapon = manager->AddThing(0, 0, -1, true,
        alternate ? u"ow_shaying_a" : u"ow_qiang_a",
        {0x01632a57, -0x19f168, 0x8e9703},
        {0, 0, 0x135c2}, {0, 0x135c2, 0}, {-0x135c2, 0, 0});
    if (weapon != nullptr) {
        weapon->SetMaterialIntensityBounce(0x1999, 0xe666, 12);
        weapon->SetCollisionEnabled(false);
    }

    struct TriggerDefinition { int32_t id; soft3d::Vec3 minimum; soft3d::Vec3 maximum; };
    static constexpr TriggerDefinition triggers[] = {
        {0, {0x1600000, -0x200000, 0x8c0000}, {0x1670000, -0x1f0000, 0x940000}},
        {1, {0x14a0000, -0x200000, 0xa40000}, {0x14f0000, -0x1f0000, 0xa90000}},
        {2, {0x14a0000, -0x200000, 0x760000}, {0x14f0000, -0x1f0000, 0x7b0000}},
        {3, {0x1790000, -0x200000, 0xa40000}, {0x17e0000, -0x1f0000, 0xa90000}},
        {4, {0x1790000, -0x200000, 0x760000}, {0x17e0000, -0x1f0000, 0x7b0000}},
    };
    for (const TriggerDefinition& definition : triggers) {
        ActTrigger* trigger = manager->AddActTrigger(definition.id,
            Bounds(definition.minimum, definition.maximum));
        trigger->SetOptionCount(1);
        trigger->SetTitleId(definition.id == 0 ? 39 : 103);
        trigger->SetReserved5c(1);
        if (definition.id != 0) trigger->SetEnabled(false);
    }
    renderActive = 1;
    encounterTimer = 4;
    encounterPhase = 1;

    for (soft3d::Object* object : {sword.get(), shield.get()}) {
        object->SetPosition(position);
        if (GetObject(0) != nullptr) {
            object->SetAxisX(GetObject(0)->GetAxisX());
            object->SetAxisY(GetObject(0)->GetAxisY());
            object->SetAxisZ(GetObject(0)->GetAxisZ());
        }
        object->UpdateTransform();
    }
}

void Lord::PlayEquipmentAnimation(const soft3d::Animation* body,
                                  const soft3d::Animation* swordAnimation,
                                  const soft3d::Animation* shieldAnimation, bool loop)
{
    SetAnimation(body, 1, loop);
    if (swordAnimation != nullptr) {
        sword->SetAnimation(swordAnimation);
        sword->SetAnimationPlayback(1, loop);
    }
    if (shieldAnimation != nullptr) {
        shield->SetAnimation(shieldAnimation);
        shield->SetAnimationPlayback(1, loop);
    }
}

void Lord::ReplaceCrystal(int32_t id, const soft3d::Vec3& crystalPosition, int32_t bouncePeriod,
                          const soft3d::Vec3& playerPosition, const soft3d::Vec3& playerDirection)
{
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    if (manager == nullptr) return;
    manager->RemoveUnitById(id, 2, true);
    constexpr soft3d::Vec3 axisX{0x33333, 0, 0};
    constexpr soft3d::Vec3 axisY{0, 0x33333, 0};
    constexpr soft3d::Vec3 axisZ{0, 0, 0x33333};
    soft3d::Things* crystal = manager->AddThing(id, 0, -1, true, u"oszm_shuijing_ab",
                                                crystalPosition, axisX, axisY, axisZ);
    if (crystal != nullptr) {
        crystal->SetMaterialIntensityBounce(0x1999, 0xe666, bouncePeriod);
        crystal->LoadAnimation(u"oszm_shuijing_ab");
        crystal->SetAnimationPlayback(1, false, 0);
    }
    if (player == nullptr) return;
    player->SetPosition(playerPosition);
    soft3d::Vec3 direction = soft3d::Normalize(playerDirection);
    player->SetDirection(direction);
    direction.y = soft3d::SubtractFixed(direction.y, 0x6666);
    direction = soft3d::Normalize(direction);
    const soft3d::Vec3 cameraPosition = soft3d::Subtract(crystalPosition,
        soft3d::Scale(direction, 0x280000));
    SetFixedCamera(cameraPosition, direction);
}

void Lord::BreakCrystal(int32_t id)
{
    if (player == nullptr || player->GetScene() == nullptr) return;
    player->GetScene()->GetCameraController()->Shake(12, 0xccc);
    player->SetAction(6, 1, false);
    player->PlaySound(24, false);
    if (audioManager != nullptr) audioManager->PlaySound(u"t_crystalbreak", false);
    auto* crystal = dynamic_cast<soft3d::VisUnit*>(
        static_cast<MyGameUnitMgr*>(unitManager)->FindUnit(2, id));
    if (crystal != nullptr) crystal->SetAnimationPlayback(1, false, 300);
}

void Lord::SetEquipmentStage(int32_t value)
{
    equipmentStage = value;
    if (value == 2) {
        animations[0] = bodyAnimations[0];
        animations[2] = bodyAnimations[1];
        animations[4] = bodyAnimations[6];
        bodyAnimations[3] = bodyAnimations[4];
        bodyAnimations[7] = bodyAnimations[8];
        bodyAnimations[10] = bodyAnimations[11];
        swordAnimations[2] = swordAnimations[6];
        swordAnimations[4] = swordAnimations[8];
        sounds[4] = extraSounds[0];
        hitDistance = 0xa0000;
        attackRange = 0x90000;
        alertSpeed = 0x6666;
        chaseSpeed = 0x6666;
        primaryHitImpulse = 0x20000;
        health = 160;
    } else if (value == 3) {
        animations[2] = bodyAnimations[2];
        animations[3] = bodyAnimations[5];
        sounds[1] = extraSounds[6];
        sounds[2] = extraSounds[6];
        hitDistance = 0x78000;
        attackRange = 0x88000;
        alertSpeed = 0x8000;
        chaseSpeed = 0x8000;
        primaryHitImpulse = 0x8000;
        alternateHitImpulse = 0x10000;
        health = 80;
    }
}

void Lord::UpdateEncounter()
{
    --encounterTimer;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    if (manager == nullptr || player == nullptr || player->GetScene() == nullptr) return;
    Scene* scene = player->GetScene();
    soft3d::GameUIDlg* ui = scene->GetGameUi();
    soft3d::FadeShader* fade = scene->GetFadeShader();
    auto act = [manager](int32_t id) { return dynamic_cast<ActTrigger*>(manager->FindUnit(4, id)); };
    auto crystal = [manager](int32_t id) {
        return dynamic_cast<soft3d::VisUnit*>(manager->FindUnit(2, id));
    };
    auto resetDialog = [ui]() { ui->CloseDialog(); ui->ClearDialogResult(); };
    auto fadeOut = [fade, this](int32_t next) {
        fade->StartFade(1, 0, 6, 1);
        encounterTimer = 6;
        encounterPhase = next;
    };
    auto waitCrystal = [&](int32_t id, int32_t next) {
        if (player->IsAnimationFinished()) player->SetAction(7, 1, false);
        soft3d::VisUnit* object = crystal(id);
        if (object != nullptr && object->IsAnimationFinished()) fadeOut(next);
    };

    switch (encounterPhase) {
    case 1: {
        ActTrigger* trigger = act(0);
        if (trigger != nullptr && trigger->GetSelection() == 1) {
            player->SetWeapon(scene->GetEventFlag(1002) ? 4 : 3);
            trigger->SetEnabled(false);
            manager->RemoveUnitById(0, 2, true);
            if (soft3d::VisUnit* object = crystal(1)) object->SetMaterialIntensityBounce(0x1999, 0xe666, 12);
            if (ActTrigger* next = act(1)) next->SetEnabled(true);
            resetDialog();
            if (!scene->GetEventFlag(1002)) ui->ShowTimedResultMessage(101, 64);
            SetEncounter(2, 160);
        } else if (encounterTimer == 0) {
            if (!ui->IsDialogActive() && ui->GetDialogResult() == -1) {
                ui->ShowTimedResultMessage(100, 48);
            }
            encounterTimer = 80;
        }
        break;
    }
    case 2:
    case 3: {
        ActTrigger* trigger = act(1);
        if (trigger != nullptr && trigger->GetSelection() == 0) {
            if (encounterTimer == 0) {
                if (encounterPhase == 2 && !ui->IsDialogActive()) ui->ShowTimedResultMessage(102, 64);
                else if (encounterPhase == 3) ui->ShowTimedResultMessage(102, 64);
                if (encounterPhase == 2) SetEncounter(3, 160);
                else SetEncounter(7, 4);
            }
        } else {
            resetDialog();
            if (trigger != nullptr) trigger->SetEnabled(false);
            SetControlsDisabled(true);
            fadeOut(4);
        }
        break;
    }
    case 4:
        if (encounterTimer == 5) {
            SetNavigationEnabled(true);
            StopSound();
            StopMotion();
            SetControlsDisabled(true);
        }
        if (encounterTimer == 0) {
            ReplaceCrystal(1, {0x1440000, -0x1c0000, 0xaf0000}, 16,
                {0x014e3a0f, -0x1b8553, 0xa50f6f}, {-0x8000, 0, 0x8000});
            renderActive = 0;
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(5, 6);
        }
        break;
    case 5:
        if (encounterTimer == 0) {
            BreakCrystal(1);
            encounterPhase = 6;
        }
        break;
    case 6: waitCrystal(1, 7); break;
    case 7:
        if (encounterTimer == 0) {
            manager->RemoveUnitById(1, 2, true);
            renderActive = 1;
            for (std::size_t index = 0; index < GetObjectCount(); ++index) {
                GetObject(index)->LoadTextureOverride(0, u"dj_nanjue_a", 3);
                GetObject(index)->SetMaterialTextureOffset(0, 0, 0);
                GetObject(index)->SetMaterialTextureScroll(0, 0, 0);
            }
            for (soft3d::Object* object : {sword.get(), shield.get()}) {
                object->LoadTextureOverride(0, u"dj_nanjue_a", 3);
                object->SetMaterialTextureOffset(0, 0, 0);
                object->SetMaterialTextureScroll(0, 0, 0);
            }
            PlaySound(extraSounds[1], false);
            player->SetVisible(false);
            soft3d::Vec3 direction = soft3d::Scale(GetForward(), -1);
            direction.y = soft3d::SubtractFixed(direction.y, 0x8000);
            direction = soft3d::Normalize(direction);
            SetFixedCamera(soft3d::Subtract(position, soft3d::Scale(direction, 0x140000)), direction);
            ui->ShowTimedResultMessage(106, 40);
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(8, 38);
        }
        break;
    case 8:
        if (encounterTimer == 0) fadeOut(9);
        break;
    case 9:
        if (encounterTimer == 0) {
            equipmentStage = 1;
            RestorePlayerAndCamera();
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(10, 14);
        }
        break;
    case 10:
        if (encounterTimer == 0) {
            ui->ShowEventMessage(104);
            encounterPhase = 11;
        }
        break;
    case 11:
    case 23:
    case 35:
        break;
    case 12:
    case 24:
    case 36:
        SetNavigationEnabled(true);
        StopSound();
        StopMotion();
        state = 7;
        ui->ShowTimedResultMessage(43, 40);
        if (encounterPhase == 36) {
            PlayEquipmentAnimation(bodyAnimations[16], nullptr, nullptr, false);
        } else {
            PlayEquipmentAnimation(bodyAnimations[12], swordAnimations[12],
                encounterPhase == 12 ? shieldAnimations[10] : nullptr, false);
        }
        ++encounterPhase;
        break;
    case 13:
    case 25:
    case 37:
        if (IsAnimationFinished()) {
            PlayEquipmentAnimation(encounterPhase == 37 ? bodyAnimations[17] : bodyAnimations[13],
                encounterPhase == 37 ? nullptr : swordAnimations[13],
                encounterPhase == 13 ? shieldAnimations[11] : nullptr, true);
            PlaySound(extraSounds[3], true);
            const int32_t id = encounterPhase == 13 ? 2 : encounterPhase == 25 ? 3 : 4;
            if (soft3d::VisUnit* object = crystal(id)) object->SetMaterialIntensityBounce(0x1999, 0xe666, 12);
            if (ActTrigger* trigger = act(id)) trigger->SetEnabled(true);
            SetEncounter(encounterPhase + 1, encounterPhase == 13 ? 120 : encounterPhase == 25 ? 90 : 70);
        }
        break;
    case 14:
    case 26:
    case 38: {
        if (ui->IsDialogActive()) {
            ++encounterTimer;
            break;
        }
        const int32_t id = encounterPhase == 14 ? 2 : encounterPhase == 26 ? 3 : 4;
        ActTrigger* trigger = act(id);
        if (trigger != nullptr && trigger->GetSelection() == 0) {
            if (encounterTimer == 0) {
                ui->ShowTimedResultMessage(105, encounterPhase == 14 ? 64 : 48);
                health = encounterPhase == 14 ? 200 : encounterPhase == 26 ? 120 : 40;
                if (soft3d::VisUnit* object = crystal(id)) object->SetMaterialIntensityBounce(0x1999, 0x1999, 12);
                trigger->SetEnabled(false);
                SetNavigationEnabled(false);
                StopMotion();
                encounterPhase = encounterPhase == 14 ? 11 : encounterPhase == 26 ? 23 : 35;
            }
        } else {
            if (trigger != nullptr) trigger->SetEnabled(false);
            SetControlsDisabled(true);
            fadeOut(encounterPhase + 1);
        }
        break;
    }
    case 15:
    case 27:
    case 39:
        if (encounterTimer == 5) SetControlsDisabled(true);
        if (encounterTimer == 0) {
            const int32_t id = encounterPhase == 15 ? 2 : encounterPhase == 27 ? 3 : 4;
            if (id == 2) {
                ReplaceCrystal(id, {0x1440000, -0x1c0000, 0x700000}, 15,
                    {0x14e48a0, -0x1b8553, 0x7a7cfa}, {-0x8000, 0, -0x8000});
            } else if (id == 3) {
                ReplaceCrystal(id, {0x1840000, -0x1c0000, 0xaf0000}, 14,
                    {0x179cc84, -0x1b8553, 0xa51e00}, {0x8000, 0, 0x8000});
            } else {
                ReplaceCrystal(id, {0x1840000, -0x1c0000, 0x700000}, 13,
                    {0x0179db15, -0x1b8553, 0x7a8b8b}, {0x8000, 0, -0x8000});
            }
            renderActive = 0;
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(encounterPhase + 1, 6);
        }
        break;
    case 16:
    case 28:
    case 40:
        if (encounterTimer == 0) {
            BreakCrystal(encounterPhase == 16 ? 2 : encounterPhase == 28 ? 3 : 4);
            ++encounterPhase;
        }
        break;
    case 17:
    case 29:
    case 41:
        waitCrystal(encounterPhase == 17 ? 2 : encounterPhase == 29 ? 3 : 4, encounterPhase + 1);
        break;
    case 18:
    case 30:
    case 42:
        if (encounterTimer == 0) {
            const int32_t id = encounterPhase == 18 ? 2 : encounterPhase == 30 ? 3 : 4;
            manager->RemoveUnitById(id, 2, true);
            renderActive = 1;
            player->SetVisible(false);
            soft3d::Vec3 direction;
            if (id == 2) {
                direction = soft3d::Scale(soft3d::Subtract(GetForward(), GetObject(0)->GetAxisX()), -1);
            } else if (id == 3) {
                direction = soft3d::Scale(soft3d::Add(GetForward(), GetObject(0)->GetAxisX()), -1);
            } else {
                direction = soft3d::Scale(GetForward(), -1);
            }
            direction = soft3d::Normalize(direction);
            direction.y = soft3d::SubtractFixed(direction.y, 0x6666);
            direction = soft3d::Normalize(direction);
            const soft3d::Fixed distance = id == 4 ? 0x120000 : 0x110000;
            soft3d::Vec3 cameraPosition = soft3d::Subtract(position, soft3d::Scale(direction, distance));
            if (id != 4) cameraPosition.y = soft3d::SubtractFixed(cameraPosition.y, 0x50000);
            SetFixedCamera(cameraPosition, direction);
            StopSound();
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(encounterPhase + 1, 22);
        }
        break;
    case 19:
        if (encounterTimer == 0) {
            if (resourceManager != nullptr) {
                shield->Load(resourceManager, u"ow_dun_dunpo_a", 3);
                shield->LoadMaterials(resourceManager, 3);
            }
            shield->LoadTextureOverride(0, u"dj_nanjue_a", 3);
            PlayEquipmentAnimation(bodyAnimations[14], swordAnimations[14], shieldAnimations[12], false);
            SetAnimationPlayback(1, false, 300);
            PlaySound(extraSounds[4], false);
            encounterPhase = 20;
        }
        break;
    case 20:
    case 32:
        if (IsAnimationFinished()) fadeOut(encounterPhase + 1);
        break;
    case 21:
        if (encounterTimer == 0) {
            SetEquipmentStage(2);
            RestorePlayerAndCamera();
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(22, 6);
        }
        break;
    case 22:
    case 34:
        if (encounterTimer == 0) {
            SetNavigationEnabled(false);
            StopMotion();
            ++encounterPhase;
        }
        break;
    case 31:
        if (encounterTimer == 0) {
            if (resourceManager != nullptr) {
                sword->Load(resourceManager, u"ow_dajian_jianpo_a", 3);
                sword->LoadMaterials(resourceManager, 3);
            }
            sword->LoadTextureOverride(0, u"dj_nanjue_a", 3);
            PlayEquipmentAnimation(bodyAnimations[15], swordAnimations[15], nullptr, false);
            SetAnimationPlayback(1, false, 300);
            PlaySound(extraSounds[4], false);
            encounterPhase = 32;
        }
        break;
    case 33:
        if (encounterTimer == 0) {
            SetEquipmentStage(3);
            RestorePlayerAndCamera();
            fade->StartFade(0, 0, 6, 1);
            SetEncounter(34, 6);
        }
        break;
    case 43:
        if (encounterTimer == 0) {
            PlaySound(sounds[3], false);
            SetAnimation(bodyAnimations[18], 1, false);
            SetAnimationPlayback(1, false, 300);
            encounterPhase = 44;
        }
        break;
    case 44:
        if (IsAnimationFinished()) {
            renderActive = 0;
            soft3d::Object* object = GetObject(0);
            soft3d::Things* corpse = manager->AddThing(0, 0, -1, true, u"r_nanjue_die_b", position,
                object != nullptr ? object->GetAxisX() : soft3d::Vec3{soft3d::fixedOne, 0, 0},
                object != nullptr ? object->GetAxisY() : soft3d::Vec3{0, soft3d::fixedOne, 0},
                object != nullptr ? object->GetAxisZ() : soft3d::Vec3{0, 0, soft3d::fixedOne});
            if (corpse != nullptr) {
                corpse->LoadAnimation(u"r_nanjue_die_b");
                corpse->SetAnimationPlayback(1, false, 300);
                corpse->SetCollisionEnabled(false);
            }
            PlaySound(extraSounds[5], false);
            encounterPhase = 45;
        }
        break;
    case 45:
        if (soft3d::VisUnit* corpse = crystal(0)) {
            if (corpse->IsAnimationFinished()) fadeOut(46);
        }
        break;
    case 46:
        if (encounterTimer == 0) {
            scene->GetCameraController()->Shake(64, 0x8000);
            equipmentStage = 4;
            RestorePlayerAndCamera();
            fade->StartFade(0, 0, 6, 1);
            scene->GetControlMap()->SetActionEnabled(6, false);
            scene->GetControlMap()->SetActionEnabled(7, false);
            if (audioManager != nullptr) {
                audioManager->StopMusic(0);
                audioManager->PlaySound(u"e_landquake", false);
            }
            SetEncounter(47, 64);
        }
        break;
    case 47:
        if (encounterTimer == 0) {
            state = EnemyDead;
            scene->GetControlMap()->SetActionEnabled(6, true);
            scene->GetControlMap()->SetActionEnabled(7, true);
            if (audioManager != nullptr) audioManager->PlayMusic(u"m_oddity_01", true, 0);
            encounterPhase = 48;
        }
        break;
    default:
        break;
    }
}

void Lord::Update()
{
    if (state >= EnemyDead) {
        StopSound();
        return;
    }
    UpdateEncounter();
    if (IsNavigationEnabled()) {
        if (GetObject(0) != nullptr) GetObject(0)->SetPosition(position);
        if (objectGroup != nullptr) {
            for (std::size_t index = 0; index < GetObjectCount(); ++index) GetObject(index)->UpdateTransform();
        }
        UpdateShadow();
        return;
    }
    UpdatePerception();
    if (state == 6) UpdateLeap();
    Enemy::Update();
    for (soft3d::Object* object : {sword.get(), shield.get()}) {
        if ((object == sword.get() && equipmentStage >= 3) ||
            (object == shield.get() && equipmentStage >= 2)) continue;
        object->SetPosition(position);
        if (GetObject(0) != nullptr) {
            object->SetAxisX(GetObject(0)->GetAxisX());
            object->SetAxisY(GetObject(0)->GetAxisY());
            object->SetAxisZ(GetObject(0)->GetAxisZ());
        }
        object->UpdateTransform();
    }
}

void Lord::PostUpdate()
{
    if (renderActive == 0) return;
    VisUnit::PostUpdate();
    if (GetObject(0) != nullptr) {
        if (equipmentStage < 3) sword->AdvanceAnimation();
        if (equipmentStage < 2) shield->AdvanceAnimation();
    }
}

void Lord::SubmitRenderables(soft3d::RenderQueues& queues)
{
    if (renderActive == 0) return;
    Enemy::SubmitRenderables(queues);
    for (soft3d::Object* object : {sword.get(), shield.get()}) {
        if ((object == sword.get() && equipmentStage >= 3) ||
            (object == shield.get() && equipmentStage >= 2)) continue;
        if (GetObject(0) != nullptr) object->sortKey = GetObject(0)->sortKey;
        object->UpdateFrustumVisibility();
        if (!object->IsFrustumCulled()) queues.Submit(object, 3);
    }
}

void Lord::SetNavigationEnabled(bool enabled)
{
    if (state >= EnemyDying) return;
    if (!IsNavigationEnabled() && enabled) StopSound();
    if (IsNavigationEnabled() && !enabled && state == EnemyChase) PlaySound(sounds[2], true);
    Creature::SetNavigationEnabled(enabled);
}

bool Lord::CanLeap() const
{
    return equipmentStage < 3 && playerDistance >= 0x100000 && playerDistance <= 0x140000;
}

bool Lord::TryBeginLeap()
{
    --stateTime;
    if (stateTime > 0) return false;
    stateTime = 0;
    if (!CanLeap()) return false;
    state = 6;
    leapPhase = 0;
    PlaySound(extraSounds[3], false);
    PlayEquipmentAnimation(bodyAnimations[7], swordAnimations[9], shieldAnimations[7], false);
    physicsFlags &= ~soft3d::PhysicKeepDirection;
    if (player != nullptr) SetDirection(soft3d::Subtract(player->GetPosition(), position));
    velocity = {};
    stateTime = 8;
    return true;
}

void Lord::UpdateIdle()
{
    if (ReceivePlayerAttack()) {
        stateTime = 0;
    } else if (!TryBeginLeap()) {
        BeginChase();
    }
}

void Lord::UpdateChase()
{
    if (!TryBeginLeap()) Enemy::UpdateChase();
}

void Lord::BeginAttack()
{
    StopSound();
    waitTimer = -1;
    stateTime = 0;
    unused7ca = 0;
    state = EnemyAttack;
    velocity = {};
    if (player != nullptr) SetDirection(soft3d::Subtract(player->GetPosition(), position));
    attackHitDone = 0;
    auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
    attackKind = manager != nullptr && manager->NextRandom() % 10000 >= 5001 ? 1 : 0;
    attackPower = attackKind == 0 ? (equipmentStage < 2 ? 25 : 20)
                                  : (equipmentStage < 2 ? 20 : 15);
    PlaySound(sounds[3], false);
    if (attackKind == 0) {
        SetAnimationByIndex(3, 1, false);
    } else {
        PlayEquipmentAnimation(bodyAnimations[3], swordAnimations[7], shieldAnimations[6], false);
    }
    if (haste != 0) {
        SetAnimationSpeed(600);
        if (equipmentStage < 3) sword->SetAnimationSpeed(600);
        if (equipmentStage < 2) shield->SetAnimationSpeed(600);
    }
}

bool Lord::TestAttackHit()
{
    if (attackHitDone != 0) return false;
    int32_t start = attackKind == 0 ? attackStart : alternateAttackStart;
    int32_t end = attackKind == 0 ? attackEnd : alternateAttackEnd;
    if (haste != 0) { start >>= 1; end >>= 1; }
    if (stateTime < start || stateTime > end || player == nullptr) return false;
    const soft3d::Vec3 endPoint = soft3d::Add(position, soft3d::Scale(GetForward(), hitDistance));
    soft3d::Fixed halfWidth = soft3d::SubtractFixed(localBounds.maximum.x, localBounds.minimum.x);
    if (attackKind != 0) halfWidth >>= 1;
    const soft3d::Fixed halfHeight =
        soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1;
    const soft3d::Vec3 across = soft3d::Scale(GetObject(0)->GetAxisX(), halfWidth);
    std::array<soft3d::Vec3, 4> corners{position, position, endPoint, endPoint};
    for (soft3d::Vec3& point : corners) point.y = soft3d::AddFixed(point.y, halfHeight);
    corners[1].y = soft3d::SubtractFixed(corners[1].y, soft3d::AddFixed(halfHeight, halfHeight));
    corners[3].y = soft3d::SubtractFixed(corners[3].y, soft3d::AddFixed(halfHeight, halfHeight));
    corners[0] = soft3d::Add(corners[0], across);
    corners[1] = soft3d::Subtract(corners[1], across);
    corners[2] = soft3d::Add(corners[2], across);
    corners[3] = soft3d::Subtract(corners[3], across);
    soft3d::ObjectBounds bounds{{INT32_MAX, INT32_MAX, INT32_MAX}, {INT32_MIN, INT32_MIN, INT32_MIN}};
    for (const soft3d::Vec3& point : corners) Expand(bounds, point);
    if (!player->IsBoundsInside(bounds)) return false;
    attackHitDone = 1;
    player->DamageHurt3(this, attackKind == 0 ? primaryHitImpulse : alternateHitImpulse);
    return true;
}

void Lord::UpdateAttack()
{
    if (IsAnimationFinished()) alternateAttackStart = 0;
    Enemy::UpdateAttack();
}

bool Lord::ReceivePlayerAttack()
{
    if (equipmentStage == 0 || state == 7 || hitCooldown != 0 || player == nullptr) return false;
    const int32_t damage = player->GetAttackDamage(this);
    if (damage == 0) return false;
    StopSound();
    health -= damage;
    blood.Trigger(soft3d::Subtract(position, soft3d::Scale(GetForward(), 0x30000)),
                  player->GetSword()->GetHitVelocity(), static_cast<MyGameUnitMgr*>(unitManager));
    PlaySound(sounds[4], false);
    hitCooldown = 8;
    if ((equipmentStage == 1 && health <= 160) ||
        (equipmentStage == 2 && health <= 80) ||
        (equipmentStage == 3 && health < 1)) {
        encounterPhase = equipmentStage == 1 ? 12 : equipmentStage == 2 ? 24 : 36;
        state = 7;
        return false;
    }
    if (state == 6 && leapPhase != 0) return false;
    stateTime = 0;
    knockbackPending = 0;
    physicsFlags |= soft3d::PhysicKeepDirection;
    physicsFlags &= ~soft3d::PhysicSimpleMovement;
    SetAnimationByIndex(4, 1, false);
    if (state == EnemyAttack) {
        knockbackPending = 1;
        SetAnimationSpeed(600);
        state = EnemyKnockback;
    } else {
        state = EnemyHurt;
    }
    if (equipmentStage != 3) haste = 1;
    SetDirection(soft3d::Subtract(player->GetPosition(), position));
    velocity = {};
    return true;
}

void Lord::Die()
{
    if (player != nullptr) player->NotifyEnemyKilled(0);
    StopSound();
    if (GetObject(0) != nullptr) GetObject(0)->SetRenderMode(soft3d::ObjectRenderMode::SecondaryStreams);
    stateTime = 0;
    collisionEnabled = false;
    state = EnemyDead;
}

void Lord::SetAnimationByIndex(int32_t index, int32_t direction, bool loop)
{
    Enemy::SetAnimationByIndex(index, direction, loop);
    if (index < 0) return;
    if (equipmentStage < 3 && static_cast<std::size_t>(index) < swordAnimations.size()) {
        sword->SetAnimation(swordAnimations[static_cast<std::size_t>(index)]);
        sword->SetAnimationPlayback(direction, loop);
    }
    if (equipmentStage < 2 && static_cast<std::size_t>(index) < shieldAnimations.size()) {
        shield->SetAnimation(shieldAnimations[static_cast<std::size_t>(index)]);
        shield->SetAnimationPlayback(direction, loop);
    }
}

void Lord::UpdateLeap()
{
    if (ReceivePlayerAttack()) return;
    if (leapPhase == 0) {
        --stateTime;
        if (stateTime > 0 && player != nullptr) {
            leapTarget = player->GetPosition();
            soft3d::Vec3 traceTarget = leapTarget;
            traceTarget.y = soft3d::AddFixed(position.y, 0x28f);
            if (bsp != nullptr) {
                const soft3d::BspTraceResult trace = bsp->Trace(position, traceTarget, localBounds);
                if (trace.startSolid && !trace.allSolid) {
                    leapTarget = soft3d::Add(leapTarget, soft3d::Scale(trace.planeNormal, 0x20000));
                }
            }
            SetDirection(soft3d::Subtract(leapTarget, position));
        }
        if (!IsAnimationFinished()) return;
        const soft3d::Fixed factor = soft3d::AddFixed(soft3d::FixedDivide(playerDistance, 0x40000),
                                                       soft3d::fixedOne);
        velocity = soft3d::Scale(GetForward(), 0x40000);
        velocity.y = soft3d::FixedMultiply(0x8000, factor) >> 1;
        SetDirection(soft3d::Subtract(leapTarget, position));
        leapPhase = 1;
        PlayEquipmentAnimation(bodyAnimations[9], swordAnimations[10], shieldAnimations[8], false);
        physicsFlags |= soft3d::PhysicKeepDirection | soft3d::PhysicSeparateAxisCollision;
        physicsFlags &= ~(soft3d::PhysicSimpleMovement | soft3d::PhysicStepOffset |
                          soft3d::PhysicStepMovement);
        shadowTrace->startSolid = false;
        return;
    }
    physicsFlags |= soft3d::PhysicStepMovement;
    if (leapPhase == 1) {
        if (velocity.y <= 0 && shadowTrace->startSolid) {
            leapPhase = 2;
            velocity = {};
            PlayEquipmentAnimation(bodyAnimations[10], swordAnimations[11], shieldAnimations[9], false);
            PlaySound(extraSounds[2], false);
            physicsFlags &= ~soft3d::PhysicSeparateAxisCollision;
            stateTime = 0;
            attackHitDone = 0;
        }
        return;
    }
    if (leapPhase == 2) {
        ++stateTime;
        if (attackHitDone == 0 && stateTime > 1 && stateTime < 5 && player != nullptr) {
            const soft3d::Vec3 endPoint = soft3d::Add(position, soft3d::Scale(GetForward(), hitDistance));
            const soft3d::Fixed halfWidth =
                soft3d::SubtractFixed(localBounds.maximum.x, localBounds.minimum.x) >> 2;
            const soft3d::Fixed halfHeight =
                soft3d::SubtractFixed(localBounds.maximum.y, localBounds.minimum.y) >> 1;
            const soft3d::Vec3 across = soft3d::Scale(GetObject(0)->GetAxisX(), halfWidth);
            std::array<soft3d::Vec3, 4> corners{position, position, endPoint, endPoint};
            for (soft3d::Vec3& point : corners) point.y = soft3d::AddFixed(point.y, halfHeight);
            corners[1].y = soft3d::SubtractFixed(corners[1].y, soft3d::AddFixed(halfHeight, halfHeight));
            corners[3].y = soft3d::SubtractFixed(corners[3].y, soft3d::AddFixed(halfHeight, halfHeight));
            corners[0] = soft3d::Add(corners[0], across);
            corners[1] = soft3d::Subtract(corners[1], across);
            corners[2] = soft3d::Add(corners[2], across);
            corners[3] = soft3d::Subtract(corners[3], across);
            soft3d::ObjectBounds bounds{{INT32_MAX, INT32_MAX, INT32_MAX}, {INT32_MIN, INT32_MIN, INT32_MIN}};
            for (const soft3d::Vec3& point : corners) Expand(bounds, point);
            if (player->IsBoundsInside(bounds)) {
                attackPower = 30;
                attackHitDone = 1;
                player->DamageHurt3(this, 0x38000);
            }
        }
        if (IsAnimationFinished()) {
            StopMotion();
            attackCooldown = attackInterval;
            physicsFlags &= ~soft3d::PhysicKeepDirection;
            if (player != nullptr) SetDirection(soft3d::Subtract(player->GetPosition(), position));
            haste = 0;
            auto* manager = static_cast<MyGameUnitMgr*>(unitManager);
            stateTime = static_cast<int16_t>(manager != nullptr ? (manager->NextRandom() % 8192 >> 8) + 32 : 32);
        }
    }
}

} // namespace game

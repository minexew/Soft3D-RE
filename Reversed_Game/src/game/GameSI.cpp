#include "game/GameSI.hpp"

#include "game/GameUiController.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/Color.hpp"
#include "soft3d/GameUIDlg.hpp"

#include <sstream>

namespace game {
namespace {

constexpr std::array<uint32_t, gameSICallbackCount> armHandlers = {
    0x000653c0, 0x00065428, 0x00065490, 0x000654f8, 0x00065560, 0x000655c8,
    0x00065614, 0x00065660, 0x000656dc, 0x00065820, 0x00065964, 0x000659ac,
    0x00065b18, 0x00065b78, 0x00065bc0, 0x00065c10, 0x00065c64, 0x00065cd0,
    0x00065d08, 0x00065d3c, 0x00065d84, 0x00065e3c, 0x00065f94, 0x000660b4,
    0x00066444, 0x00066488, 0x00066504, 0x0006652c, 0x00066664, 0x000666c8,
    0x0006672c, 0x00066774, 0x000667d4, 0x00066860, 0x000668a8, 0x00066908,
    0x0006694c, 0x00066980, 0x000669c8, 0x000669fc, 0x00066a24, 0x00066a4c,
    0x00066ab0, 0x00066ae8, 0x00066b80, 0x00066bc8, 0x00066c14, 0x00066c48,
    0x00066d1c, 0x00066d50, 0x00066d94, 0x00066ddc, 0x00066e34, 0x00066e7c,
    0x00066edc, 0x00066f10, 0x00066f70, 0x00066fc8, 0x0006714c, 0x000671ac,
    0x000672e4, 0x00067404, 0x0006749c, 0x000674e4, 0x00067524, 0x00067560,
    0x0006759c, 0x000675d8, 0x00067614, 0x000676ac, 0x00067760, 0x000678ac,
    0x000678f8, 0x00067948, 0x000679a4, 0x00067a60, 0x00067ab8, 0x00067b60,
    0x00067ba4, 0x00067cb0, 0x00067da8, 0x00067e94, 0x00067fa8, 0x000680c0,
    0x00068174, 0x00068228, 0x00068280, 0x000682d0, 0x0006830c, 0x000683c4,
    0x000684ac, 0x000684f8, 0x00068540, 0x00068584, 0x000687d0, 0x00068814,
    0x00068894, 0x000688d0, 0x0006890c, 0x00068944, 0x00068978, 0x000689bc,
    0x00068a10, 0x00068a68, 0x00068af4, 0x00068b44, 0x00068b78, 0x00068cbc,
    0x00068d24, 0x00068d8c, 0x00068df0, 0x00068f34, 0x00068fa4, 0x00069200,
    0x00069268, 0x000692d0, 0x0006934c, 0x00069450, 0x000694b8, 0x00069578,
    0x000695f8, 0x00069660, 0x0006979c, 0x0006980c, 0x000699f8, 0x00069a9c,
    0x00069b0c, 0x00069c30, 0x00069ca0, 0x00069d10, 0x00069e34, 0x00069eec,
    0x0006a020, 0x0006a0cc, 0x0006a14c, 0x0006a1d8, 0x0006a248, 0x0006a298,
    0x0006a408, 0x0006a468, 0x0006a4f4, 0x0006a5cc, 0x0006a64c, 0x0006a6f8,
    0x0006a76c, 0x0006a834, 0x0006a8a8, 0x0006a920, 0x0006aa14, 0x0006ab0c,
    0x0006ac1c, 0x0006ad44, 0x0006ae9c, 0x0006af04, 0x0006afa0, 0x0006b00c,
    0x0006b0b4, 0x0006b138, 0x0006b1c8, 0x0006b298, 0x0006b33c, 0x0006b3bc,
    0x0006b4c0, 0x0006b5e8, 0x0006b710, 0x0006b7bc, 0x0006b884, 0x0006b9c8,
    0x0006ba48, 0x0006baac, 0x0006bb34, 0x0006bbb4, 0x0006bc50, 0x0006bcec,
    0x0006bd78, 0x0006be7c, 0x0006becc, 0x0006c03c, 0x0006c08c, 0x0006c31c,
    0x0006c3e8, 0x0006c458, 0x0006c4c0, 0x0006c564, 0x0006c5cc, 0x0006c63c,
    0x0006c718, 0x0006c788, 0x0006c7ec, 0x0006c85c, 0x0006c8c0, 0x0006c924,
    0x0006c9e0, 0x0006ca54, 0x0006cac4, 0x0006cb34, 0x0006cbec, 0x0006cca4,
    0x0006cd5c, 0x0006ce18, 0x0006cf08, 0x0006cfec, 0x0006d06c, 0x0006d0dc,
    0x0006d12c, 0x0006d198, 0x0006d208, 0x0006d350, 0x0006d498, 0x0006d508,
    0x0006d574, 0x0006d600, 0x0006d988, 0x0006d9d4, 0x0006da2c, 0x00064fd0,
    0x00064ff8, 0x00065034,
};

} // namespace

GameSIError::GameSIError(int id, uint32_t address, const std::string& reason)
    : std::runtime_error("GameSI command " + std::to_string(id) + " at ARM 0x" +
                         [&address] {
                             std::ostringstream stream;
                             stream << std::hex << address;
                             return stream.str();
                         }() + ": " + reason)
    , commandId(id)
    , armAddress(address)
{
}

GameSI::GameSI()
{
    for (std::size_t index = 0; index < callbacks.size(); ++index) {
        callbacks[index] = {armHandlers[index], nullptr, 0};
    }
    callbacks[43 - gameSIFirstCommand].handler = &GameSI::IsActionFirstDown;
    callbacks[44 - gameSIFirstCommand].handler = &GameSI::IsActionDown;
    callbacks[45 - gameSIFirstCommand].handler = &GameSI::IsActionPress;
    callbacks[46 - gameSIFirstCommand].handler = &GameSI::IsActionUp;
    callbacks[47 - gameSIFirstCommand].handler = &GameSI::IsActionClicked;
    callbacks[48 - gameSIFirstCommand].handler = &GameSI::IsAnyKeyDown;
    callbacks[49 - gameSIFirstCommand].handler = &GameSI::IsAnyKeyClick;
    callbacks[50 - gameSIFirstCommand].handler = &GameSI::LogMessage;
    callbacks[92 - gameSIFirstCommand].handler = &GameSI::SetUseCameraListener;
    callbacks[93 - gameSIFirstCommand].handler = &GameSI::StartScriptTimer;
    callbacks[94 - gameSIFirstCommand].handler = &GameSI::GetScriptTimerElapsed;
    callbacks[95 - gameSIFirstCommand].handler = &GameSI::StopScriptTimer;
    callbacks[113 - gameSIFirstCommand].handler = &GameSI::SetBspScriptVectors;
    callbacks[114 - gameSIFirstCommand].handler = &GameSI::SetPlayerPresentationVisible;
    callbacks[115 - gameSIFirstCommand].handler = &GameSI::SetControlsDisabled;
    callbacks[116 - gameSIFirstCommand].handler = &GameSI::SetPlayerVisible;
    callbacks[51 - gameSIFirstCommand].handler = &GameSI::FadeOut;
    callbacks[52 - gameSIFirstCommand].handler = &GameSI::FadeIn;
    callbacks[53 - gameSIFirstCommand].handler = &GameSI::LoadNextDay;
    callbacks[54 - gameSIFirstCommand].handler = &GameSI::SetLoadMapParams;
    callbacks[55 - gameSIFirstCommand].handler = &GameSI::LoadNextLevel;
    callbacks[56 - gameSIFirstCommand].handler = &GameSI::LoadMap;
    callbacks[57 - gameSIFirstCommand].handler = &GameSI::IsMapLoaded;
    callbacks[58 - gameSIFirstCommand].handler = &GameSI::SetEventFlag;
    callbacks[59 - gameSIFirstCommand].handler = &GameSI::GetEventFlag;
    callbacks[60 - gameSIFirstCommand].handler = &GameSI::CompleteGame;
    callbacks[61 - gameSIFirstCommand].handler = &GameSI::OpenSaveMenu;
    callbacks[62 - gameSIFirstCommand].handler = &GameSI::SetPaused;
    callbacks[73 - gameSIFirstCommand].handler = &GameSI::ShowEventMessage;
    callbacks[74 - gameSIFirstCommand].handler = &GameSI::SetDialogString;
    callbacks[75 - gameSIFirstCommand].handler = &GameSI::ShowChoiceDialog;
    callbacks[76 - gameSIFirstCommand].handler = &GameSI::ShowSimpleMessage;
    callbacks[77 - gameSIFirstCommand].handler = &GameSI::ShowTimedMessage;
    callbacks[78 - gameSIFirstCommand].handler = &GameSI::GetDialogResult;
    callbacks[79 - gameSIFirstCommand].handler = &GameSI::CloseDialog;
    callbacks[80 - gameSIFirstCommand].handler = &GameSI::IsDialogFinished;
    callbacks[81 - gameSIFirstCommand].handler = &GameSI::ShowMap;
    callbacks[82 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[83 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[96 - gameSIFirstCommand].handler = &GameSI::ShowTimedResultMessage;
    callbacks[97 - gameSIFirstCommand].handler = &GameSI::CloseDialog;
    callbacks[101 - gameSIFirstCommand].handler = &GameSI::SetSubtitle;
    callbacks[102 - gameSIFirstCommand].handler = &GameSI::SetActionMask;
    callbacks[104 - gameSIFirstCommand].handler = &GameSI::SetMapMarker;
    callbacks[105 - gameSIFirstCommand].handler = &GameSI::ClearMapMarker;
    callbacks[106 - gameSIFirstCommand].handler = &GameSI::SetGunHudEnabled;
    callbacks[107 - gameSIFirstCommand].handler = &GameSI::GetItemUseCount;
    callbacks[108 - gameSIFirstCommand].handler = &GameSI::GetKillCount;
    callbacks[109 - gameSIFirstCommand].handler = &GameSI::GetPlayTime;
    callbacks[110 - gameSIFirstCommand].handler = &GameSI::GetDeathCount;
    callbacks[111 - gameSIFirstCommand].handler = &GameSI::GetNoteCount;
    callbacks[112 - gameSIFirstCommand].handler = &GameSI::GetSecretCount;
    callbacks[63 - gameSIFirstCommand].handler = &GameSI::SetCameraFrozen;
    callbacks[64 - gameSIFirstCommand].handler = &GameSI::CameraLookAt;
    callbacks[65 - gameSIFirstCommand].handler = &GameSI::MoveCamera;
    callbacks[66 - gameSIFirstCommand].handler = &GameSI::SetCameraMode;
    callbacks[67 - gameSIFirstCommand].handler = &GameSI::IsCameraPathCompleted;
    callbacks[68 - gameSIFirstCommand].handler = &GameSI::BindCameraToPlayer;
    callbacks[69 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[70 - gameSIFirstCommand].handler = &GameSI::BindCameraToEnemy;
    callbacks[71 - gameSIFirstCommand].handler = &GameSI::ShakeCamera;
    callbacks[72 - gameSIFirstCommand].handler = &GameSI::SetDirectFollow;
    callbacks[84 - gameSIFirstCommand].handler = &GameSI::PlayMusic;
    callbacks[85 - gameSIFirstCommand].handler = &GameSI::StopMusic;
    callbacks[86 - gameSIFirstCommand].handler = &GameSI::AddEnvironmentSound;
    callbacks[87 - gameSIFirstCommand].handler = &GameSI::ClearEnvironmentChannel;
    callbacks[88 - gameSIFirstCommand].handler = &GameSI::PlaySound;
    callbacks[89 - gameSIFirstCommand].handler = &GameSI::StopAllSounds;
    callbacks[90 - gameSIFirstCommand].handler = &GameSI::CreatePositionalSound;
    callbacks[91 - gameSIFirstCommand].handler = &GameSI::ClearSoundInstances;
    callbacks[98 - gameSIFirstCommand].handler = &GameSI::SetLightStyle;
    callbacks[99 - gameSIFirstCommand].handler = &GameSI::GetLightStyle;
    callbacks[100 - gameSIFirstCommand].handler = &GameSI::SetFog;
    callbacks[103 - gameSIFirstCommand].handler = &GameSI::SelectPresentationShader;
    callbacks[149 - gameSIFirstCommand].handler = &GameSI::AddEventTrigger;
    callbacks[150 - gameSIFirstCommand].handler = &GameSI::SetEventTriggerCount;
    callbacks[151 - gameSIFirstCommand].handler = &GameSI::SetEventTriggerText;
    callbacks[152 - gameSIFirstCommand].handler = &GameSI::IsEventTriggerActive;
    callbacks[153 - gameSIFirstCommand].handler = &GameSI::AddPickTrigger;
    callbacks[154 - gameSIFirstCommand].handler = &GameSI::EnablePickTrigger;
    callbacks[155 - gameSIFirstCommand].handler = &GameSI::AddDoorTrigger;
    callbacks[156 - gameSIFirstCommand].handler = &GameSI::SetDoorTriggerItem;
    callbacks[157 - gameSIFirstCommand].handler = &GameSI::SetDoorTriggerLockedText;
    callbacks[158 - gameSIFirstCommand].handler = &GameSI::EnableDoorTrigger;
    callbacks[159 - gameSIFirstCommand].handler = &GameSI::AddActTrigger;
    callbacks[160 - gameSIFirstCommand].handler = &GameSI::SetActTriggerOptionCount;
    callbacks[161 - gameSIFirstCommand].handler = &GameSI::SetActTriggerString;
    callbacks[162 - gameSIFirstCommand].handler = &GameSI::GetActTriggerSelection;
    callbacks[163 - gameSIFirstCommand].handler = &GameSI::ClearActTriggerSelection;
    callbacks[164 - gameSIFirstCommand].handler = &GameSI::SetActTriggerCamera;
    callbacks[165 - gameSIFirstCommand].handler = &GameSI::EnableActTrigger;
    callbacks[166 - gameSIFirstCommand].handler = &GameSI::AddHideTrigger;
    callbacks[167 - gameSIFirstCommand].handler = &GameSI::SetHideTriggerLimits;
    callbacks[168 - gameSIFirstCommand].handler = &GameSI::EnableHideTrigger;
    callbacks[169 - gameSIFirstCommand].handler = &GameSI::AddItemTrigger;
    callbacks[170 - gameSIFirstCommand].handler = &GameSI::AddItemTriggerItem;
    callbacks[171 - gameSIFirstCommand].handler = &GameSI::EnableItemTrigger;
    callbacks[172 - gameSIFirstCommand].handler = &GameSI::AddTalkTrigger;
    callbacks[173 - gameSIFirstCommand].handler = &GameSI::SetTalkTriggerCameraPosition;
    callbacks[174 - gameSIFirstCommand].handler = &GameSI::AddTalkTriggerCamera;
    callbacks[175 - gameSIFirstCommand].handler = &GameSI::AddTalkTriggerLine;
    callbacks[176 - gameSIFirstCommand].handler = &GameSI::IsTalkTriggerActive;
    callbacks[177 - gameSIFirstCommand].handler = &GameSI::IsTalkTriggerCompleted;
    callbacks[178 - gameSIFirstCommand].handler = &GameSI::EnableTalkTrigger;
    callbacks[179 - gameSIFirstCommand].handler = &GameSI::RemoveInteractionTrigger;
    callbacks[180 - gameSIFirstCommand].handler = &GameSI::AddEnemy;
    callbacks[181 - gameSIFirstCommand].handler = &GameSI::PersistEnemy;
    callbacks[182 - gameSIFirstCommand].handler = &GameSI::SetEnemyNormalRendering;
    callbacks[183 - gameSIFirstCommand].handler = &GameSI::SetEnemyAnimation;
    callbacks[184 - gameSIFirstCommand].handler = &GameSI::IsEnemyAnimationFinished;
    callbacks[185 - gameSIFirstCommand].handler = &GameSI::IsEnemyRemoved;
    callbacks[186 - gameSIFirstCommand].handler = &GameSI::GetEnemyHealth;
    callbacks[187 - gameSIFirstCommand].handler = &GameSI::SetEnemyHealth;
    callbacks[188 - gameSIFirstCommand].handler = &GameSI::GetEnemyAttackPower;
    callbacks[189 - gameSIFirstCommand].handler = &GameSI::SetEnemyAttackPower;
    callbacks[190 - gameSIFirstCommand].handler = &GameSI::SetEnemyDirection;
    callbacks[191 - gameSIFirstCommand].handler = &GameSI::TeleportEnemy;
    callbacks[192 - gameSIFirstCommand].handler = &GameSI::MoveEnemyTo;
    callbacks[193 - gameSIFirstCommand].handler = &GameSI::EnemyHearSound;
    callbacks[194 - gameSIFirstCommand].handler = &GameSI::EnemyBoundsInside;
    callbacks[195 - gameSIFirstCommand].handler = &GameSI::FollowEnemy;
    callbacks[196 - gameSIFirstCommand].handler = &GameSI::AttackEnemy;
    callbacks[197 - gameSIFirstCommand].handler = &GameSI::RemoveEnemy;
    callbacks[198 - gameSIFirstCommand].handler = &GameSI::RemoveEnemiesByType;
    callbacks[199 - gameSIFirstCommand].handler = &GameSI::RemoveAllEnemies;
    callbacks[200 - gameSIFirstCommand].handler = &GameSI::PauseEnemy;
    callbacks[201 - gameSIFirstCommand].handler = &GameSI::PauseEnemiesByType;
    callbacks[202 - gameSIFirstCommand].handler = &GameSI::PauseAllEnemies;
    callbacks[203 - gameSIFirstCommand].handler = &GameSI::StopEnemy;
    callbacks[204 - gameSIFirstCommand].handler = &GameSI::AddStayBox;
    callbacks[205 - gameSIFirstCommand].handler = &GameSI::AddStayBoxDeniedRegion;
    callbacks[206 - gameSIFirstCommand].handler = &GameSI::AddStayBoxAllowedRegion;
    callbacks[207 - gameSIFirstCommand].handler = &GameSI::AddStayBoxEnemy;
    callbacks[208 - gameSIFirstCommand].handler = &GameSI::SetStayBoxReturnPosition;
    callbacks[209 - gameSIFirstCommand].handler = &GameSI::AddEnemyWaypoint;
    callbacks[210 - gameSIFirstCommand].handler = &GameSI::SetEnemyWaypointLoop;
    callbacks[211 - gameSIFirstCommand].handler = &GameSI::ClearEnemyWaypoints;
    callbacks[212 - gameSIFirstCommand].handler = &GameSI::SetEnemyPerceptionRange;
    callbacks[213 - gameSIFirstCommand].handler = &GameSI::IsEnemyAlerted;
    callbacks[214 - gameSIFirstCommand].handler = &GameSI::SetEnemyAnimationResource;
    callbacks[215 - gameSIFirstCommand].handler = &GameSI::SetEnemySoundResource;
    callbacks[216 - gameSIFirstCommand].handler = &GameSI::SetEnemyAlertSpeed;
    callbacks[217 - gameSIFirstCommand].handler = &GameSI::AddCollisionUnit;
    callbacks[218 - gameSIFirstCommand].handler = &GameSI::RemoveCollisionUnit;
    callbacks[219 - gameSIFirstCommand].handler = &GameSI::AddTrap;
    callbacks[220 - gameSIFirstCommand].handler = &GameSI::RemoveTrap;
    callbacks[221 - gameSIFirstCommand].handler = &GameSI::AddThing;
    callbacks[222 - gameSIFirstCommand].handler = &GameSI::SetThingDirection;
    callbacks[223 - gameSIFirstCommand].handler = &GameSI::SetPickableItem;
    callbacks[224 - gameSIFirstCommand].handler = &GameSI::SetThingCollision;
    callbacks[225 - gameSIFirstCommand].handler = &GameSI::SetThingIntensityBounce;
    callbacks[226 - gameSIFirstCommand].handler = &GameSI::SetThingVisible;
    callbacks[227 - gameSIFirstCommand].handler = &GameSI::IsThingVisible;
    callbacks[228 - gameSIFirstCommand].handler = &GameSI::SetThingAnimation;
    callbacks[229 - gameSIFirstCommand].handler = &GameSI::SetThingAnimationPaused;
    callbacks[230 - gameSIFirstCommand].handler = &GameSI::IsThingAnimationPaused;
    callbacks[231 - gameSIFirstCommand].handler = &GameSI::SetThingAnimationTime;
    callbacks[232 - gameSIFirstCommand].handler = &GameSI::GetThingAnimationTime;
    callbacks[233 - gameSIFirstCommand].handler = &GameSI::IsThingAnimationFinished;
    callbacks[234 - gameSIFirstCommand].handler = &GameSI::SetThingBreakModel;
    callbacks[235 - gameSIFirstCommand].handler = &GameSI::SetThingBreakAnimation;
    callbacks[236 - gameSIFirstCommand].handler = &GameSI::SetBreakableSound;
    callbacks[237 - gameSIFirstCommand].handler = &GameSI::SetPushableSound;
    callbacks[238 - gameSIFirstCommand].handler = &GameSI::RotateThingX;
    callbacks[239 - gameSIFirstCommand].handler = &GameSI::RotateThingY;
    callbacks[240 - gameSIFirstCommand].handler = &GameSI::RotateThingZ;
    callbacks[241 - gameSIFirstCommand].handler = &GameSI::SetThingPosition;
    callbacks[242 - gameSIFirstCommand].handler = &GameSI::MoveThing;
    callbacks[243 - gameSIFirstCommand].handler = &GameSI::MoveThingTowards;
    callbacks[244 - gameSIFirstCommand].handler = &GameSI::DestroyThing;
    callbacks[245 - gameSIFirstCommand].handler = &GameSI::SetThingDepthBias;
    callbacks[246 - gameSIFirstCommand].handler = &GameSI::RemoveThing;
    callbacks[247 - gameSIFirstCommand].handler = &GameSI::ThingExists;
    callbacks[248 - gameSIFirstCommand].handler = &GameSI::ConsumeBreakableEvent;
    callbacks[249 - gameSIFirstCommand].handler = &GameSI::ThingBoundsInside;
    callbacks[250 - gameSIFirstCommand].handler = &GameSI::ThingBoundsOutside;
    callbacks[251 - gameSIFirstCommand].handler = &GameSI::SetThingLighting;
    callbacks[252 - gameSIFirstCommand].handler = &GameSI::SetPushableEnabled;
    callbacks[253 - gameSIFirstCommand].handler = &GameSI::SetThingTexture;
    callbacks[254 - gameSIFirstCommand].handler = &GameSI::GetMagicCubeOrientation;
    callbacks[117 - gameSIFirstCommand].handler = &GameSI::SetPlayerDirection;
    callbacks[118 - gameSIFirstCommand].handler = &GameSI::SetPlayerClipping;
    callbacks[119 - gameSIFirstCommand].handler = &GameSI::SetPlayerAnimation;
    callbacks[120 - gameSIFirstCommand].handler = &GameSI::IsPlayerAnimationFinished;
    callbacks[121 - gameSIFirstCommand].handler = &GameSI::TeleportPlayer;
    callbacks[122 - gameSIFirstCommand].handler = &GameSI::NavigatePlayer;
    callbacks[123 - gameSIFirstCommand].handler = &GameSI::PlayerHearSound;
    callbacks[124 - gameSIFirstCommand].handler = &GameSI::PlayerPointInside;
    callbacks[125 - gameSIFirstCommand].handler = &GameSI::PlayerPointOutside;
    callbacks[126 - gameSIFirstCommand].handler = &GameSI::PlayerHearEnemy;
    callbacks[127 - gameSIFirstCommand].handler = &GameSI::PlayerHearThing;
    callbacks[128 - gameSIFirstCommand].handler = &GameSI::PlayerAttack;
    callbacks[129 - gameSIFirstCommand].handler = &GameSI::IsPlayerDead;
    callbacks[130 - gameSIFirstCommand].handler = &GameSI::GetPlayerHealth;
    callbacks[131 - gameSIFirstCommand].handler = &GameSI::SetPlayerHealth;
    callbacks[132 - gameSIFirstCommand].handler = &GameSI::AddPlayerHealth;
    callbacks[133 - gameSIFirstCommand].handler = &GameSI::DamagePlayer;
    callbacks[134 - gameSIFirstCommand].handler = &GameSI::SetPlayerWeapon;
    callbacks[135 - gameSIFirstCommand].handler = &GameSI::GetPlayerWeapon;
    callbacks[136 - gameSIFirstCommand].handler = &GameSI::IsPlayerAttacking;
    callbacks[137 - gameSIFirstCommand].handler = &GameSI::IsPlayerHidden;
    callbacks[138 - gameSIFirstCommand].handler = &GameSI::StartPlayerQTE;
    callbacks[139 - gameSIFirstCommand].handler = &GameSI::GetPlayerQTEResult;
    callbacks[140 - gameSIFirstCommand].handler = &GameSI::GetPlayerUsedItem;
    callbacks[141 - gameSIFirstCommand].handler = &GameSI::ClearPlayerUsedItem;
    callbacks[142 - gameSIFirstCommand].handler = &GameSI::EnterPlayerFPS;
    callbacks[143 - gameSIFirstCommand].handler = &GameSI::HasAttackingEnemy;
    callbacks[144 - gameSIFirstCommand].handler = &GameSI::SetPlayerSpeed;
    callbacks[145 - gameSIFirstCommand].handler = &GameSI::IsPlayerPushing;
    callbacks[146 - gameSIFirstCommand].handler = &GameSI::SetPlayerYoung;
    callbacks[147 - gameSIFirstCommand].handler = &GameSI::IsPlayerYoung;
    callbacks[148 - gameSIFirstCommand].handler = &GameSI::ExitPlayerHide;
    callbacks[255 - gameSIFirstCommand].handler = &GameSI::AddPlayerItem;
    callbacks[256 - gameSIFirstCommand].handler = &GameSI::HasPlayerItem;
    callbacks[257 - gameSIFirstCommand].handler = &GameSI::RemovePlayerItem;
    callbacks[258 - gameSIFirstCommand].handler = &GameSI::NullCallback;
    callbacks[259 - gameSIFirstCommand].handler = &GameSI::SetSharedWord255;
    callbacks[260 - gameSIFirstCommand].handler = &GameSI::NullCallback;
}

void GameSI::Bind(const GameSIContext& value)
{
    if (value.scene == nullptr || value.unitManager == nullptr || value.cameraController == nullptr ||
        value.player == nullptr || value.gameUi == nullptr || value.audio == nullptr ||
        value.resources == nullptr || value.bsp == nullptr || value.controlMap == nullptr) {
        throw std::invalid_argument("GameSI requires the complete scene context");
    }
    context = value;
    scriptLog.SetEngine(&value.scene->GetEngine());
    S3D_LOG_ON(scriptLog);
}

const GameSI::Callback& GameSI::GetCallback(int commandId) const
{
    if (commandId < gameSIFirstCommand || commandId > gameSILastCommand) {
        throw std::out_of_range("GameSI callback ID " + std::to_string(commandId));
    }
    return callbacks[static_cast<std::size_t>(commandId - gameSIFirstCommand)];
}

uint32_t GameSI::GetArmHandlerAddress(int commandId)
{
    if (commandId < gameSIFirstCommand || commandId > gameSILastCommand) {
        throw std::out_of_range("GameSI ARM handler ID " + std::to_string(commandId));
    }
    return armHandlers[static_cast<std::size_t>(commandId - gameSIFirstCommand)];
}

void GameSI::RequireArguments(const soft3d::ScriptCommand& command, std::size_t count) const
{
    if (command.GetArgumentCount() < count) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "expected at least " + std::to_string(count) + " arguments, got " +
                              std::to_string(command.GetArgumentCount()));
    }
}

void GameSI::RequireContext() const
{
    if (context.scene == nullptr || context.unitManager == nullptr ||
        context.cameraController == nullptr || context.player == nullptr || context.gameUi == nullptr ||
        context.audio == nullptr || context.resources == nullptr || context.bsp == nullptr ||
        context.controlMap == nullptr) {
        throw std::runtime_error("GameSI context is not bound");
    }
}

void GameSI::ExecuteGameCommand(soft3d::ScriptCommand& command)
{
    RequireContext();
    const Callback& callback = GetCallback(command.GetId());
    if (callback.handler == nullptr) {
        throw GameSIError(command.GetId(), callback.armAddress, "no ARM handler is bound");
    }
    (this->*callback.handler)(command);
}

void GameSI::IsActionFirstDown(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.controlMap->ActionFirstDown(
            context.scene->GetEngine().GetInput(), command.GetArgument(0).GetInt(), false));
    }
}

void GameSI::IsActionDown(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.controlMap->ActionDown(
            context.scene->GetEngine().GetInput(), command.GetArgument(0).GetInt(), false));
    }
}

void GameSI::IsActionPress(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.controlMap->ActionPress(
            context.scene->GetEngine().GetInput(), command.GetArgument(0).GetInt(), false));
    }
}

void GameSI::IsActionUp(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.controlMap->ActionUp(
            context.scene->GetEngine().GetInput(), command.GetArgument(0).GetInt(), false));
    }
}

void GameSI::IsActionClicked(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.controlMap->ActionClick(
            context.scene->GetEngine().GetInput(), command.GetArgument(0).GetInt(), false));
    }
}

void GameSI::IsAnyKeyDown(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->GetEngine().GetInput().IsAnyKeyDown());
    }
}

void GameSI::IsAnyKeyClick(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->GetEngine().GetInput().IsAnyKeyClick());
    }
}

void GameSI::LogMessage(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    S3D_LOG(scriptLog, u"***%s\n", command.GetArgument(0).GetString().c_str());
}

void GameSI::SetUseCameraListener(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->SetUseCameraListener(command.GetArgument(0).GetInt() != 0);
}

void GameSI::StartScriptTimer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->StartScriptTimer(command.GetArgument(0).GetInt());
}

void GameSI::GetScriptTimerElapsed(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(
            context.unitManager->GetScriptTimerElapsed(command.GetArgument(0).GetInt()));
    }
}

void GameSI::StopScriptTimer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->StopScriptTimer(command.GetArgument(0).GetInt());
}

void GameSI::SetBspScriptVectors(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 6);
    context.bsp->SetScriptVectors(
        {command.GetArgument(0).GetFixed(), command.GetArgument(1).GetFixed(),
         command.GetArgument(2).GetFixed()},
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()});
}

void GameSI::SetPlayerPresentationVisible(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetPresentationVisible(command.GetArgument(0).GetBool());
}

void GameSI::SetControlsDisabled(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.controlMap->SetEnabled(!command.GetArgument(0).GetBool(),
                                   &context.scene->GetEngine().GetInput());
}

void GameSI::SetPlayerVisible(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetVisible(command.GetArgument(0).GetBool());
}

void GameSI::FadeOut(soft3d::ScriptCommand& command)
{
    Fade(command, false);
}

void GameSI::FadeIn(soft3d::ScriptCommand& command)
{
    Fade(command, true);
}

void GameSI::Fade(soft3d::ScriptCommand& command, bool fadeOut)
{
    RequireArguments(command, 4);
    const uint16_t color = soft3d::PackRgb565(static_cast<int16_t>(command.GetArgument(0).GetInt()),
                                      static_cast<int16_t>(command.GetArgument(1).GetInt()),
                                      static_cast<int16_t>(command.GetArgument(2).GetInt()));
    const bool completed = context.scene->FadeCommand(fadeOut, color,
        command.GetArgument(3).GetInt());
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(completed);
    }
}

void GameSI::SetCameraFrozen(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    const bool frozen = command.GetArgument(0).GetBool();
    context.cameraController->SetFrozen(frozen);
    if (!frozen) {
        context.cameraController->SetInstant(true);
    }
}

void GameSI::CameraLookAt(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 6);
    context.cameraController->LookAt(
        {command.GetArgument(0).GetFixed(), command.GetArgument(1).GetFixed(),
         command.GetArgument(2).GetFixed()},
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()});
}

void GameSI::MoveCamera(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    const bool reached = context.cameraController->MoveCamera(
        {command.GetArgument(0).GetFixed(), command.GetArgument(1).GetFixed(),
         command.GetArgument(2).GetFixed()},
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()},
        command.GetArgument(6).GetFixed());
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(reached);
    }
}

void GameSI::SetCameraMode(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 12);
    cameraCommandPaths.reset();
    const int32_t mode = command.GetArgument(0).GetInt();
    soft3d::Fixed pathSpeed = 0;
    const int32_t targetMode = command.GetArgument(11).GetShort();
    if (mode == CameraController::Path) {
        cameraCommandPaths = std::make_unique<soft3d::CameraPathCollection>(64);
        std::u16string path = u".\\common\\";
        path += command.GetArgument(8).GetString().c_str();
        if (path.size() < 5 || path.compare(path.size() - 4, 4, u".spl") != 0) {
            path += u".spl";
        }
        const soft3d::result status = soft3d::LoadSplCameraResource(
            *cameraCommandPaths, path.c_str(), context.resources->GetFilePak());
        if (status != soft3d::S3D_OK) {
            throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                              "LoadSplCameraResource failed with Soft3D result " +
                                  std::to_string(status));
        }
        pathSpeed = command.GetArgument(9).GetFixed();
    }
    const soft3d::Vec3 position{command.GetArgument(1).GetFixed(),
                                command.GetArgument(2).GetFixed(),
                                command.GetArgument(3).GetFixed()};
    const soft3d::Vec3 target{command.GetArgument(4).GetFixed(),
                              command.GetArgument(5).GetFixed(),
                              command.GetArgument(6).GetFixed()};
    soft3d::Vec3 direction = soft3d::Subtract(target, position);
    if (mode != CameraController::Path || targetMode == 2) {
        direction = soft3d::Normalize(direction);
    }
    context.cameraController->SetMode(mode, position, direction,
        command.GetArgument(7).GetFixed(), cameraCommandPaths.get(), pathSpeed,
        command.GetArgument(10).GetBool(), targetMode);
}

void GameSI::IsCameraPathCompleted(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.cameraController->IsPathCompleted());
    }
}

void GameSI::BindCameraToPlayer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.scene->BindCameraToPlayer();
}

void GameSI::BindCameraToEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.scene->BindCameraToEnemy();
}

void GameSI::ShakeCamera(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.cameraController->Shake(command.GetArgument(0).GetInt(),
                                    command.GetArgument(1).GetFixed());
}

void GameSI::SetDirectFollow(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.cameraController->SetDirectFollow(command.GetArgument(0).GetBool(),
                                              command.GetArgument(1).GetFixed());
}

void GameSI::PlayMusic(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.audio->PlayMusic(command.GetArgument(0).GetString().c_str(),
                             command.GetArgument(1).GetBool(), 0);
}

void GameSI::StopMusic(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.audio->StopMusic(0);
}

void GameSI::AddEnvironmentSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    context.scene->AddEnvironmentSound(command.GetArgument(0).GetInt(),
        command.GetArgument(1).GetString().c_str(), command.GetArgument(2).GetInt(),
        command.GetArgument(3).GetInt());
}

void GameSI::ClearEnvironmentChannel(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->ClearEnvironmentChannel(command.GetArgument(0).GetInt());
}

void GameSI::PlaySound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.audio->PlaySound(command.GetArgument(0).GetString().c_str(), false);
}

void GameSI::StopAllSounds(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.audio->StopAllSounds();
}

void GameSI::CreatePositionalSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    auto* data = static_cast<soft3d::AudioData*>(context.resources->GetResource(
        soft3d::ResourceAudio, 3, command.GetArgument(0).GetString().c_str(), u".\\audio\\"));
    context.audio->CreateSoundInstance(data,
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()}, false);
}

void GameSI::ClearSoundInstances(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.audio->ClearSoundInstances();
}

void GameSI::SetLightStyle(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    const int16_t index = command.GetArgument(0).GetShort();
    if (index < 0 || index > 255) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "lightstyle index is outside 0..255");
    }
    context.bsp->SetLightStyleEnabled(static_cast<uint8_t>(index),
                                      command.GetArgument(1).GetBool() ? 1 : 0);
    context.bsp->AdvanceLightStyles();
}

void GameSI::GetLightStyle(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    const int16_t index = command.GetArgument(0).GetShort();
    if (index < 0 || index > 255) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "lightstyle index is outside 0..255");
    }
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(
            context.bsp->GetLightStyles()[static_cast<uint8_t>(index)].enabled != 0 ? 1 : 0);
    }
}

void GameSI::SetFog(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 5);
    const soft3d::Fixed end = command.GetArgument(4).GetFixed();
    if (end <= 0) {
        context.scene->GetEngine().GetRenderer().SetFogEnabled(false);
        return;
    }
    const uint16_t color = soft3d::PackRgb565(command.GetArgument(0).GetShort(),
                                      command.GetArgument(1).GetShort(),
                                      command.GetArgument(2).GetShort());
    context.scene->GetEngine().GetRenderer().SetFog(command.GetArgument(3).GetFixed(), end, color);
    context.scene->GetEngine().GetRenderer().SetFogEnabled(true);
}

void GameSI::SelectPresentationShader(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->SelectPresentationShader(command.GetArgument(0).GetInt());
}

EventTrigger& GameSI::RequireEventTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<EventTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "event trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

PickTrigger& GameSI::RequirePickTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<PickTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "pick trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

DoorTrigger& GameSI::RequireDoorTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<DoorTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "door trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

ActTrigger& GameSI::RequireActTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<ActTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "act trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

HideTrigger& GameSI::RequireHideTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<HideTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "hide trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

ItemTrigger& GameSI::RequireItemTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<ItemTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "item trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

TalkTrigger& GameSI::RequireTalkTrigger(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    auto* trigger = dynamic_cast<TalkTrigger*>(context.unitManager->FindUnit(4, id));
    if (trigger == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "talk trigger ID " + std::to_string(id) + " does not exist");
    }
    return *trigger;
}

void GameSI::AddEventTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 9);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()},
        {command.GetArgument(6).GetFixed(), command.GetArgument(7).GetFixed(),
         command.GetArgument(8).GetFixed()}};
    context.unitManager->AddEventTrigger(command.GetArgument(0).GetShort(),
        static_cast<int16_t>(command.GetArgument(1).GetInt()), command.GetArgument(2).GetShort(), bounds);
}

void GameSI::SetEventTriggerCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireEventTrigger(command).SetRemainingCount(static_cast<int16_t>(command.GetArgument(1).GetInt()));
}

void GameSI::SetEventTriggerText(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireEventTrigger(command).SetTextId(command.GetArgument(1).GetShort());
}

void GameSI::IsEventTriggerActive(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(RequireEventTrigger(command).IsInside());
    }
}

void GameSI::AddPickTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 9);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()},
        {command.GetArgument(6).GetFixed(), command.GetArgument(7).GetFixed(),
         command.GetArgument(8).GetFixed()}};
    context.unitManager->AddPickTrigger(command.GetArgument(0).GetShort(),
        command.GetArgument(1).GetShort(), command.GetArgument(2).GetShort(), bounds);
}

void GameSI::EnablePickTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequirePickTrigger(command).SetEnabled(command.GetArgument(1).GetBool());
}

void GameSI::AddDoorTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 18);
    const soft3d::Vec3 position{command.GetArgument(6).GetFixed(),
        command.GetArgument(7).GetFixed(), command.GetArgument(8).GetFixed()};
    const soft3d::Vec3 direction{command.GetArgument(9).GetFixed(),
        command.GetArgument(10).GetFixed(), command.GetArgument(11).GetFixed()};
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(12).GetFixed(), command.GetArgument(13).GetFixed(),
         command.GetArgument(14).GetFixed()},
        {command.GetArgument(15).GetFixed(), command.GetArgument(16).GetFixed(),
         command.GetArgument(17).GetFixed()}};
    context.unitManager->AddDoorTrigger(command.GetArgument(0).GetShort(),
        command.GetArgument(1).GetShort(), command.GetArgument(2).GetShort(),
        command.GetArgument(3).GetShort(), command.GetArgument(4).GetShort(),
        command.GetArgument(5).GetShort(), position, direction, bounds);
}

void GameSI::SetDoorTriggerItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireDoorTrigger(command).SetRequiredItem(command.GetArgument(1).GetShort());
}

void GameSI::SetDoorTriggerLockedText(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireDoorTrigger(command).SetLockedText(command.GetArgument(1).GetShort());
}

void GameSI::EnableDoorTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    auto* trigger = dynamic_cast<DoorTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (trigger != nullptr) {
        trigger->SetEnabled(command.GetArgument(1).GetBool());
    }
}

void GameSI::AddActTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    context.unitManager->AddActTrigger(command.GetArgument(0).GetShort(), bounds);
}

void GameSI::SetActTriggerOptionCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireActTrigger(command).SetOptionCount(command.GetArgument(1).GetInt());
}

void GameSI::SetActTriggerString(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    RequireActTrigger(command).SetString(command.GetArgument(1).GetInt(),
                                        command.GetArgument(2).GetShort());
}

void GameSI::GetActTriggerSelection(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    auto* trigger = dynamic_cast<ActTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(trigger != nullptr ? trigger->GetSelection() : 0);
    }
}

void GameSI::ClearActTriggerSelection(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    auto* trigger = dynamic_cast<ActTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (trigger != nullptr) {
        trigger->ClearSelection();
    }
}

void GameSI::SetActTriggerCamera(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    auto* trigger = dynamic_cast<ActTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (trigger != nullptr) {
        trigger->SetCameraVectors(
            {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
             command.GetArgument(3).GetFixed()},
            {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
             command.GetArgument(6).GetFixed()});
    }
}

void GameSI::EnableActTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireActTrigger(command).SetEnabled(command.GetArgument(1).GetBool());
}

void GameSI::AddHideTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 14);
    const soft3d::Vec3 hidePosition{command.GetArgument(2).GetFixed(),
        command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed()};
    const soft3d::Vec3 cameraDirection{command.GetArgument(5).GetFixed(),
        command.GetArgument(6).GetFixed(), command.GetArgument(7).GetFixed()};
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(8).GetFixed(), command.GetArgument(9).GetFixed(),
         command.GetArgument(10).GetFixed()},
        {command.GetArgument(11).GetFixed(), command.GetArgument(12).GetFixed(),
         command.GetArgument(13).GetFixed()}};
    context.unitManager->AddHideTrigger(command.GetArgument(0).GetShort(),
        command.GetArgument(1).GetShort(), hidePosition, cameraDirection, bounds);
}

void GameSI::SetHideTriggerLimits(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    RequireHideTrigger(command).SetLimits(command.GetArgument(1).GetFixed(),
                                          command.GetArgument(2).GetFixed());
}

void GameSI::EnableHideTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireHideTrigger(command).SetEnabled(command.GetArgument(1).GetBool());
}

void GameSI::AddItemTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 8);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed(),
         command.GetArgument(4).GetFixed()},
        {command.GetArgument(5).GetFixed(), command.GetArgument(6).GetFixed(),
         command.GetArgument(7).GetFixed()}};
    context.unitManager->AddItemTrigger(command.GetArgument(0).GetShort(),
                                        command.GetArgument(1).GetShort(), bounds);
}

void GameSI::AddItemTriggerItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireItemTrigger(command).AddItem(command.GetArgument(1).GetShort());
}

void GameSI::EnableItemTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireItemTrigger(command).SetEnabled(command.GetArgument(1).GetBool());
}

void GameSI::AddTalkTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 8);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    context.unitManager->AddTalkTrigger(command.GetArgument(0).GetShort(), bounds,
                                        command.GetArgument(7).GetBool());
}

void GameSI::SetTalkTriggerCameraPosition(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    RequireTalkTrigger(command).SetCameraPosition({command.GetArgument(1).GetFixed(),
        command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed()});
}

void GameSI::AddTalkTriggerCamera(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 8);
    RequireTalkTrigger(command).AddCamera(command.GetArgument(1).GetShort(),
        {command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed(),
         command.GetArgument(4).GetFixed()},
        {command.GetArgument(5).GetFixed(), command.GetArgument(6).GetFixed(),
         command.GetArgument(7).GetFixed()});
}

void GameSI::AddTalkTriggerLine(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    RequireTalkTrigger(command).AddLine(command.GetArgument(1).GetShort(),
        static_cast<uint8_t>(command.GetArgument(2).GetShort()),
        static_cast<int8_t>(command.GetArgument(3).GetShort()));
}

void GameSI::IsTalkTriggerActive(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    auto* trigger = dynamic_cast<TalkTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(trigger != nullptr && trigger->IsActive());
    }
}

void GameSI::IsTalkTriggerCompleted(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    auto* trigger = dynamic_cast<TalkTrigger*>(context.unitManager->FindUnit(
        4, command.GetArgument(0).GetShort()));
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(trigger != nullptr && trigger->IsCompleted());
    }
}

void GameSI::EnableTalkTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireTalkTrigger(command).SetEnabled(command.GetArgument(1).GetBool());
}

void GameSI::RemoveInteractionTrigger(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveUnitById(command.GetArgument(0).GetShort(), 4, true);
}

Enemy* GameSI::FindEnemy(soft3d::ScriptCommand& command)
{
    return context.unitManager->FindEnemy(command.GetArgument(0).GetShort());
}

void GameSI::AddEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 9);
    soft3d::Vec3 position{command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
                          command.GetArgument(5).GetFixed()};
    const soft3d::Vec3 direction{command.GetArgument(6).GetFixed(),
                                 command.GetArgument(7).GetFixed(),
                                 command.GetArgument(8).GetFixed()};
    context.unitManager->AddEnemy(command.GetArgument(0).GetShort(), command.GetArgument(1).GetInt(),
                                  command.GetArgument(2).GetShort(), position, direction, false);
}

void GameSI::PersistEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (Enemy* enemy = FindEnemy(command)) enemy->EnsureSpawnRecord();
}

void GameSI::SetEnemyNormalRendering(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    Enemy* enemy = FindEnemy(command);
    if (enemy != nullptr && enemy->GetObject(0) != nullptr) {
        enemy->GetObject(0)->SetRenderMode(command.GetArgument(1).GetBool()
            ? soft3d::ObjectRenderMode::Normal : soft3d::ObjectRenderMode::SecondaryStreams);
    }
}

void GameSI::SetEnemyAnimation(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    if (Enemy* enemy = FindEnemy(command)) {
        const auto* animation = dynamic_cast<const soft3d::Animation*>(context.resources->GetResource(
            soft3d::ResourceAnimation, 3, command.GetArgument(1).GetString().c_str(),
            u".\\common\\"));
        enemy->SetAnimation(animation, 1, command.GetArgument(2).GetBool());
    }
}

void GameSI::IsEnemyAnimationFinished(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    Enemy* enemy = FindEnemy(command);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(enemy != nullptr && enemy->IsAnimationFinished());
    }
}

void GameSI::IsEnemyRemoved(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    Enemy* enemy = FindEnemy(command);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(enemy == nullptr || enemy->IsRemoved());
    }
}

void GameSI::GetEnemyHealth(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    Enemy* enemy = FindEnemy(command);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(enemy != nullptr ? enemy->GetHealth() : 0);
    }
}

void GameSI::SetEnemyHealth(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetHealth(command.GetArgument(1).GetShort());
}

void GameSI::GetEnemyAttackPower(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    Enemy* enemy = FindEnemy(command);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(enemy != nullptr ? enemy->GetAttackPower() : 0);
    }
}

void GameSI::SetEnemyAttackPower(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetAttackPower(command.GetArgument(1).GetInt());
}

void GameSI::SetEnemyDirection(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->SetDirection({command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
                             command.GetArgument(3).GetFixed()});
    }
}

void GameSI::TeleportEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->SetPosition({command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
                            command.GetArgument(3).GetFixed()});
    }
}

void GameSI::MoveEnemyTo(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    Enemy* enemy = FindEnemy(command);
    const bool reached = enemy != nullptr && enemy->MoveTo({
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
        command.GetArgument(3).GetFixed()});
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(reached);
}

void GameSI::EnemyHearSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    Enemy* enemy = FindEnemy(command);
    const bool heard = enemy != nullptr && enemy->HearSound({
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
        command.GetArgument(3).GetFixed()}, 0x50000, -1);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(heard);
}

void GameSI::EnemyBoundsInside(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    Enemy* enemy = FindEnemy(command);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(enemy != nullptr && enemy->IsBoundsInside(bounds));
    }
}

void GameSI::FollowEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (Enemy* enemy = FindEnemy(command)) enemy->FollowPlayer();
}

void GameSI::AttackEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (Enemy* enemy = FindEnemy(command)) enemy->RequestAttack();
}

void GameSI::RemoveEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveEnemy(FindEnemy(command));
}

void GameSI::RemoveEnemiesByType(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveEnemiesByType(command.GetArgument(0).GetShort());
}

void GameSI::RemoveAllEnemies(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.unitManager->RemoveAllEnemies();
}

void GameSI::PauseEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetNavigationEnabled(command.GetArgument(1).GetBool());
}

void GameSI::PauseEnemiesByType(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    const int32_t type = command.GetArgument(0).GetShort();
    const bool value = command.GetArgument(1).GetBool();
    for (soft3d::GameUnit* unit = context.unitManager->GetFirstUnit(0); unit != nullptr;
         unit = unit->GetNextUnit()) {
        auto* enemy = dynamic_cast<Enemy*>(unit);
        if (enemy != nullptr && enemy->GetEnemyType() == type) enemy->SetNavigationEnabled(value);
    }
}

void GameSI::PauseAllEnemies(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    const bool value = command.GetArgument(0).GetBool();
    for (soft3d::GameUnit* unit = context.unitManager->GetFirstUnit(0); unit != nullptr;
         unit = unit->GetNextUnit()) {
        auto* enemy = dynamic_cast<Enemy*>(unit);
        if (enemy != nullptr) enemy->SetNavigationEnabled(value);
    }
}

void GameSI::StopEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetStopped(command.GetArgument(1).GetBool());
}

void GameSI::AddStayBox(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    context.unitManager->AddStayBox(command.GetArgument(0).GetShort(), bounds);
}

void GameSI::SetStayBoxReturnPosition(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->SetStayBoxReturnPosition({command.GetArgument(1).GetFixed(),
            command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed()});
    }
}

void GameSI::AddEnemyWaypoint(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 8);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->AddWaypoint({command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
                            command.GetArgument(3).GetFixed()}, command.GetArgument(4).GetShort(),
            {command.GetArgument(5).GetFixed(), command.GetArgument(6).GetFixed(),
             command.GetArgument(7).GetFixed()});
    }
}

void GameSI::SetEnemyWaypointLoop(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetWaypointLoop(command.GetArgument(1).GetBool());
}

void GameSI::ClearEnemyWaypoints(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (Enemy* enemy = FindEnemy(command)) enemy->ClearWaypoints();
}

void GameSI::SetEnemyPerceptionRange(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetPerceptionRange(command.GetArgument(1).GetFixed());
}

void GameSI::IsEnemyAlerted(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    Enemy* enemy = FindEnemy(command);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(enemy != nullptr && enemy->IsAttacking());
    }
}

void GameSI::SetEnemyAnimationResource(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->SetAnimationResource(command.GetArgument(1).GetShort(),
                                    command.GetArgument(2).GetString().c_str());
    }
}

void GameSI::SetEnemySoundResource(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    if (Enemy* enemy = FindEnemy(command)) {
        enemy->SetSoundResource(command.GetArgument(1).GetShort(),
                                command.GetArgument(2).GetString().c_str());
    }
}

void GameSI::SetEnemyAlertSpeed(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    if (Enemy* enemy = FindEnemy(command)) enemy->SetAlertSpeed(command.GetArgument(1).GetFixed());
}

void GameSI::AddCollisionUnit(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 7);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    context.unitManager->AddCollisionUnit(command.GetArgument(0).GetShort(), bounds);
}

void GameSI::RemoveCollisionUnit(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveUnitById(command.GetArgument(0).GetShort(), 1, true);
}

void GameSI::AddStayBoxRegion(soft3d::ScriptCommand& command, bool allowed)
{
    RequireArguments(command, 7);
    StayBox* box = context.unitManager->FindStayBox(command.GetArgument(0).GetShort());
    if (box == nullptr) {
        return;
    }
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    if (allowed) {
        box->AddAllowedRegion(bounds);
    } else {
        box->AddDeniedRegion(bounds);
    }
}

void GameSI::AddStayBoxDeniedRegion(soft3d::ScriptCommand& command)
{
    AddStayBoxRegion(command, false);
}

void GameSI::AddStayBoxAllowedRegion(soft3d::ScriptCommand& command)
{
    AddStayBoxRegion(command, true);
}

void GameSI::AddStayBoxEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    soft3d::GameUnit* enemy = context.unitManager->FindUnit(0, command.GetArgument(0).GetShort());
    StayBox* box = context.unitManager->FindStayBox(command.GetArgument(1).GetShort());
    if (enemy != nullptr && box != nullptr) {
        context.unitManager->AttachStayBoxEnemy(*enemy, *box);
        box->AddSpawnRecord(*enemy);
    }
}

soft3d::Things& GameSI::RequireThing(soft3d::ScriptCommand& command)
{
    const int32_t id = command.GetArgument(0).GetShort();
    soft3d::GameUnit* unit = context.unitManager->FindUnit(2, id);
    auto* thing = dynamic_cast<soft3d::Things*>(unit);
    if (thing == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "thing ID " + std::to_string(id) + " does not exist");
    }
    return *thing;
}

Breakable& GameSI::RequireBreakable(soft3d::ScriptCommand& command)
{
    auto* breakable = dynamic_cast<Breakable*>(&RequireThing(command));
    if (breakable == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "thing is not Breakable");
    }
    return *breakable;
}

Pushable& GameSI::RequirePushable(soft3d::ScriptCommand& command)
{
    auto* pushable = dynamic_cast<Pushable*>(&RequireThing(command));
    if (pushable == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "thing is not Pushable");
    }
    return *pushable;
}

void GameSI::AddTrap(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 10);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    context.unitManager->AddTrap(command.GetArgument(0).GetShort(), bounds,
        {command.GetArgument(7).GetFixed(), command.GetArgument(8).GetFixed(),
         command.GetArgument(9).GetFixed()});
}

void GameSI::RemoveTrap(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveUnitById(command.GetArgument(0).GetShort(), 5, true);
}

void GameSI::AddThing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 17);
    soft3d::Things& thing = *context.unitManager->AddThing(
        command.GetArgument(0).GetShort(),
        static_cast<int8_t>(command.GetArgument(1).GetInt()),
        static_cast<int8_t>(command.GetArgument(2).GetInt()), true,
        command.GetArgument(3).GetString().c_str(),
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()},
        {command.GetArgument(7).GetFixed(), command.GetArgument(8).GetFixed(),
         command.GetArgument(9).GetFixed()},
        {command.GetArgument(10).GetFixed(), command.GetArgument(11).GetFixed(),
         command.GetArgument(12).GetFixed()},
        {command.GetArgument(13).GetFixed(), command.GetArgument(14).GetFixed(),
         command.GetArgument(15).GetFixed()});
    if (thing.GetObject(0) != nullptr) {
        thing.GetObject(0)->SetRenderMode(command.GetArgument(16).GetBool()
            ? soft3d::ObjectRenderMode::Normal : soft3d::ObjectRenderMode::SecondaryStreams);
    }
}

void GameSI::SetThingDirection(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    RequireThing(command).SetDirection({command.GetArgument(1).GetFixed(),
        command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed()});
}

void GameSI::SetPickableItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    auto* pickable = dynamic_cast<Pickable*>(&RequireThing(command));
    if (pickable == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "thing is not Pickable");
    }
    pickable->SetItemId(command.GetArgument(1).GetShort());
}

void GameSI::SetThingCollision(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetCollisionEnabled(command.GetArgument(1).GetBool());
}

void GameSI::SetThingIntensityBounce(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    RequireThing(command).SetMaterialIntensityBounce(command.GetArgument(1).GetFixed(),
        command.GetArgument(2).GetFixed(), command.GetArgument(3).GetShort());
}

void GameSI::SetThingVisible(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetVisible(command.GetArgument(1).GetBool());
}

void GameSI::IsThingVisible(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(RequireThing(command).IsVisible());
    }
}

void GameSI::SetThingAnimation(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 5);
    soft3d::Things& thing = RequireThing(command);
    if (thing.LoadAnimation(command.GetArgument(1).GetString().c_str(), nullptr) != soft3d::S3D_OK) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "animation resource could not be loaded");
    }
    thing.SetAnimationPlayback(static_cast<int8_t>(command.GetArgument(3).GetInt()),
        command.GetArgument(4).GetInt() != 0, command.GetArgument(2).GetInt());
}

void GameSI::SetThingAnimationPaused(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetAnimationPaused(command.GetArgument(1).GetBool());
}

void GameSI::IsThingAnimationPaused(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        soft3d::Object* object = RequireThing(command).GetObject(0);
        command.GetOutput()->SetBool(object != nullptr && object->IsAnimationPaused());
    }
}

void GameSI::SetThingAnimationTime(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetAnimationTime(command.GetArgument(1).GetInt());
}

void GameSI::GetThingAnimationTime(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        soft3d::Object* object = RequireThing(command).GetObject(0);
        command.GetOutput()->SetInt(object != nullptr ? object->GetAnimationTime() : 0);
    }
}

void GameSI::IsThingAnimationFinished(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(RequireThing(command).IsAnimationFinished());
    }
}

void GameSI::SetThingBreakModel(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    soft3d::Things& thing = RequireThing(command);
    thing.LoadAlternateModel(command.GetArgument(1).GetString().c_str(), nullptr);
    thing.LoadMaterials();
    thing.SetAlternateAnimation(command.GetArgument(2).GetString().c_str(), nullptr);
}

void GameSI::SetThingBreakAnimation(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetAlternateAnimation(command.GetArgument(1).GetString().c_str(), nullptr);
}

void GameSI::SetBreakableSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireBreakable(command).SetBreakSound(command.GetArgument(1).GetString().c_str());
}

void GameSI::SetPushableSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequirePushable(command).SetPushSound(command.GetArgument(1).GetString().c_str());
}

void GameSI::RotateThing(soft3d::ScriptCommand& command, int axis)
{
    RequireArguments(command, 2);
    const int32_t degrees = command.GetArgument(1).GetShort();
    const auto angle = static_cast<uint16_t>(static_cast<int32_t>(
        static_cast<float>(degrees * 4096) / 360.0f) & 0x1fff);
    soft3d::Object* object = RequireThing(command).GetObject(0);
    if (object == nullptr) {
        return;
    }
    if (axis == 0) object->RotateX(angle);
    if (axis == 1) object->RotateY(angle);
    if (axis == 2) object->RotateZ(angle);
}

void GameSI::RotateThingX(soft3d::ScriptCommand& command) { RotateThing(command, 0); }
void GameSI::RotateThingY(soft3d::ScriptCommand& command) { RotateThing(command, 1); }
void GameSI::RotateThingZ(soft3d::ScriptCommand& command) { RotateThing(command, 2); }

void GameSI::SetThingPosition(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    RequireThing(command).SetPosition({command.GetArgument(1).GetFixed(),
        command.GetArgument(2).GetFixed(), command.GetArgument(3).GetFixed()});
}

void GameSI::MoveThing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    soft3d::Things& thing = RequireThing(command);
    thing.SetPosition(soft3d::Add(thing.GetPosition(),
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()}));
    thing.SetCollisionBounds({soft3d::Add(thing.GetLocalBounds().minimum, thing.GetPosition()),
                              soft3d::Add(thing.GetLocalBounds().maximum, thing.GetPosition())});
}

void GameSI::MoveThingTowards(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 5);
    const bool reached = RequireThing(command).MoveTowards(
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()}, command.GetArgument(4).GetFixed());
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(reached);
}

void GameSI::DestroyThing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).StartDestruction(
        static_cast<uint16_t>(command.GetArgument(1).GetShort()));
}

void GameSI::SetThingDepthBias(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequireThing(command).SetDepthBias(command.GetArgument(1).GetFixed());
}

void GameSI::RemoveThing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->RemoveUnitById(command.GetArgument(0).GetShort(), 2, true);
}

void GameSI::ThingExists(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.unitManager->FindUnit(
            2, command.GetArgument(0).GetShort()) != nullptr);
    }
}

void GameSI::ConsumeBreakableEvent(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(RequireBreakable(command).ConsumeBreakEvent());
    }
}

void GameSI::TestThingBounds(soft3d::ScriptCommand& command, bool outside)
{
    RequireArguments(command, 7);
    const soft3d::ObjectBounds bounds{
        {command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed(),
         command.GetArgument(3).GetFixed()},
        {command.GetArgument(4).GetFixed(), command.GetArgument(5).GetFixed(),
         command.GetArgument(6).GetFixed()}};
    const bool hit = RequireThing(command).QueryBounds(bounds) != nullptr;
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(outside ? !hit : hit);
}

void GameSI::ThingBoundsInside(soft3d::ScriptCommand& command) { TestThingBounds(command, false); }
void GameSI::ThingBoundsOutside(soft3d::ScriptCommand& command) { TestThingBounds(command, true); }

void GameSI::SetThingLighting(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    soft3d::Object* object = RequireThing(command).GetObject(0);
    if (object != nullptr) object->SetLightingEnabled(command.GetArgument(1).GetBool());
}

void GameSI::SetPushableEnabled(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    RequirePushable(command).SetPushEnabled(command.GetArgument(1).GetBool());
}

void GameSI::SetThingTexture(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    soft3d::Things& thing = RequireThing(command);
    const uint32_t material = static_cast<uint16_t>(command.GetArgument(1).GetShort());
    for (std::size_t index = 0; index < thing.GetObjectCount(); ++index) {
        thing.GetObject(index)->LoadTextureOverride(material,
            command.GetArgument(2).GetString().c_str(), 3);
    }
}

void GameSI::GetMagicCubeOrientation(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    soft3d::Object* object = RequireThing(command).GetObject(0);
    int32_t orientation = 0;
    if (object != nullptr) {
        const soft3d::Vec3 axes[] = {object->GetAxisX(), object->GetAxisY(), object->GetAxisZ()};
        for (std::size_t index = 0; index < 3; ++index) {
            if (soft3d::AbsFixed(axes[index].x) < 0x1999 &&
                soft3d::AbsFixed(axes[index].z) < 0x1999) {
                orientation = static_cast<int32_t>(index * 2) + (axes[index].y > 0 ? 0 : 1);
                break;
            }
        }
    }
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(orientation);
}

void GameSI::SetPlayerDirection(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    context.scene->SetPlayerDirection({command.GetArgument(0).GetFixed(),
                                       command.GetArgument(1).GetFixed(),
                                       command.GetArgument(2).GetFixed()});
}

void GameSI::TeleportPlayer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    const soft3d::Vec3 position{command.GetArgument(0).GetFixed(),
                                command.GetArgument(1).GetFixed(),
                                command.GetArgument(2).GetFixed()};
    context.player->SetPosition(position);
    context.player->SnapToGround();
    context.player->SetCollisionBounds({
        soft3d::Add(context.player->GetLocalBounds().minimum, context.player->GetPosition()),
        soft3d::Add(context.player->GetLocalBounds().maximum, context.player->GetPosition())});
}

void GameSI::SetPlayerClipping(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (context.player->GetObject(0) != nullptr) {
        context.player->GetObject(0)->SetRenderMode(command.GetArgument(0).GetBool()
            ? soft3d::ObjectRenderMode::Normal : soft3d::ObjectRenderMode::SecondaryStreams);
    }
}

void GameSI::SetPlayerAnimation(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    const auto* animation = dynamic_cast<const soft3d::Animation*>(context.resources->GetResource(
        soft3d::ResourceAnimation, 3, command.GetArgument(0).GetString().c_str(), u".\\common\\"));
    context.player->SetCustomAnimation(animation, command.GetArgument(1).GetBool());
}

void GameSI::IsPlayerAnimationFinished(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.player->IsAnimationFinished());
    }
}

void GameSI::NavigatePlayer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    const bool reached = context.player->NavigateTo({command.GetArgument(0).GetFixed(),
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed()}, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(reached);
}

void GameSI::PlayerHearSound(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    const bool heard = context.player->HearSound({command.GetArgument(0).GetFixed(),
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed()},
        command.GetArgument(3).GetFixed(), -1);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(heard);
}

void GameSI::PlayerPointInside(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 6);
    const soft3d::ObjectBounds bounds{{command.GetArgument(0).GetFixed(),
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed()},
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()}};
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.player->IsPointInside(bounds.minimum));
    }
}

void GameSI::PlayerPointOutside(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 6);
    const soft3d::ObjectBounds bounds{{command.GetArgument(0).GetFixed(),
        command.GetArgument(1).GetFixed(), command.GetArgument(2).GetFixed()},
        {command.GetArgument(3).GetFixed(), command.GetArgument(4).GetFixed(),
         command.GetArgument(5).GetFixed()}};
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(!context.player->IsPointInside(bounds.minimum));
    }
}

void GameSI::PlayerHearEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    soft3d::GameUnit* unit = context.unitManager->FindUnit(0, command.GetArgument(0).GetShort());
    const bool heard = unit != nullptr && context.player->HearSound(unit->GetPosition(),
        command.GetArgument(1).GetFixed(), -1);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(heard);
}

void GameSI::PlayerHearThing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    const int32_t id = command.GetArgument(0).GetShort();
    soft3d::GameUnit* unit = context.unitManager->FindUnit(2, id);
    const bool heard = unit != nullptr && context.player->HearSound(unit->GetPosition(),
        command.GetArgument(1).GetFixed(), id);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(heard);
}

void GameSI::PlayerAttack(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->StartAttack(command.GetArgument(0).GetInt() != 0);
}
void GameSI::IsPlayerDead(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(context.player->IsDead());
}
void GameSI::GetPlayerHealth(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetHealth());
}
void GameSI::SetPlayerHealth(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetHealth(command.GetArgument(0).GetShort());
}
void GameSI::AddPlayerHealth(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->AddHealth(command.GetArgument(0).GetShort());
}
void GameSI::DamagePlayer(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->Damage(command.GetArgument(0).GetShort());
}
void GameSI::SetPlayerWeapon(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetWeapon(command.GetArgument(0).GetInt());
}
void GameSI::GetPlayerWeapon(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetWeapon());
}
void GameSI::IsPlayerAttacking(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(context.player->IsAttacking());
}
void GameSI::IsPlayerHidden(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(context.player->IsHiddenOrFPS());
}
void GameSI::StartPlayerQTE(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    context.player->BeginQTE(command.GetArgument(0).GetShort(), command.GetArgument(1).GetShort(),
                             command.GetArgument(2).GetShort());
}
void GameSI::GetPlayerQTEResult(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetQteResult());
}
void GameSI::GetPlayerUsedItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetUsedItem());
}
void GameSI::ClearPlayerUsedItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.player->ClearUsedItem();
}
void GameSI::EnterPlayerFPS(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.player->EnterFPS();
}
void GameSI::HasAttackingEnemy(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.player->HasAttackingEnemy());
    }
}
void GameSI::SetPlayerSpeed(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetNavigationSpeed(command.GetArgument(0).GetFixed());
}
void GameSI::IsPlayerPushing(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.player->IsPushingUnit(command.GetArgument(0).GetShort()));
    }
}
void GameSI::SetPlayerYoung(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->SetYoung(command.GetArgument(0).GetBool());
}
void GameSI::IsPlayerYoung(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetBool(context.player->IsYoung());
}
void GameSI::ExitPlayerHide(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.player->ExitHide();
}
void GameSI::AddPlayerItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->AddItem(command.GetArgument(0).GetShort(), true);
}
void GameSI::HasPlayerItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.player->HasItem(command.GetArgument(0).GetShort()));
    }
}
void GameSI::RemovePlayerItem(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.player->RemoveItem(command.GetArgument(0).GetShort());
}

void GameSI::LoadNextDay(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    const soft3d::result status = context.scene->LoadDay(context.scene->GetDay() + 1);
    if (status != soft3d::S3D_OK) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "LoadDay failed with Soft3D result " + std::to_string(status));
    }
}

void GameSI::SetLoadMapParams(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 10);
    const soft3d::Vec3 position{command.GetArgument(1).GetFixed(),
                                command.GetArgument(2).GetFixed(),
                                command.GetArgument(3).GetFixed()};
    const soft3d::Vec3 direction{command.GetArgument(4).GetFixed(),
                                 command.GetArgument(5).GetFixed(),
                                 command.GetArgument(6).GetFixed()};
    const uint16_t color = soft3d::PackRgb565(command.GetArgument(7).GetShort(),
                                      command.GetArgument(8).GetShort(),
                                      command.GetArgument(9).GetShort());
    context.scene->SetLoadMapParams(command.GetArgument(0).GetInt(), position, direction, color);
}

void GameSI::LoadNextLevel(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.scene->LoadNextLevel(command.GetArgument(0).GetShort(),
                                 command.GetArgument(1).GetShort());
}

void GameSI::LoadMap(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->LoadMap(command.GetArgument(0).GetShort());
}

void GameSI::IsMapLoaded(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->IsMapLoaded());
    }
}

void GameSI::SetEventFlag(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.scene->SetEventFlag(command.GetArgument(0).GetShort());
}

void GameSI::GetEventFlag(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(context.scene->GetEventFlag(command.GetArgument(0).GetShort()));
    }
}

void GameSI::CompleteGame(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.gameUi->CompleteGame();
}

void GameSI::OpenSaveMenu(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.gameUi->OpenSaveMenu();
}

void GameSI::SetPaused(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.unitManager->SetPaused(command.GetArgument(0).GetBool());
}

void GameSI::ShowEventMessage(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.gameUi->ShowEventMessage(command.GetArgument(0).GetShort());
}

void GameSI::SetDialogString(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.gameUi->SetDialogString(command.GetArgument(0).GetByte(),
                                    command.GetArgument(1).GetShort());
}

void GameSI::ShowChoiceDialog(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 3);
    context.gameUi->ShowChoiceDialog(command.GetArgument(0).GetShort(),
                                     command.GetArgument(1).GetByte(),
                                     command.GetArgument(2).GetBool());
    context.gameUi->ClearDialogResult();
}

void GameSI::ShowSimpleMessage(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.gameUi->ShowSimpleMessage(command.GetArgument(0).GetInt());
}

void GameSI::ShowTimedMessage(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.gameUi->ShowTimedMessage(command.GetArgument(0).GetInt(),
                                     command.GetArgument(1).GetInt());
}

void GameSI::GetDialogResult(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(context.gameUi->GetDialogResult());
    }
}

void GameSI::CloseDialog(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.gameUi->CloseDialog();
}

void GameSI::IsDialogFinished(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetBool(!context.gameUi->IsDialogActive());
    }
}

void GameSI::ShowMap(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    context.gameUi->OpenMap();
}

void GameSI::ShowTimedResultMessage(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.gameUi->ShowTimedResultMessage(command.GetArgument(0).GetInt(),
                                           command.GetArgument(1).GetInt());
}

void GameSI::SetSubtitle(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 2);
    context.gameUi->SetSubtitle(command.GetArgument(0).GetBool(), command.GetArgument(1).GetShort());
}

void GameSI::SetActionMask(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    context.controlMap->SetActionEnabled(5, !command.GetArgument(0).GetBool());
    context.controlMap->SetActionEnabled(6, !command.GetArgument(1).GetBool());
    context.controlMap->SetActionEnabled(7, !command.GetArgument(2).GetBool());
    for (int action = 1; action <= 4; ++action) {
        context.controlMap->SetActionEnabled(action, !command.GetArgument(3).GetBool());
    }
}

void GameSI::SetMapMarker(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 4);
    context.gameUi->SetMapMarker(command.GetArgument(0).GetByte(),
                                 command.GetArgument(1).GetByte() - 1,
                                 command.GetArgument(2).GetInt(), command.GetArgument(3).GetInt());
}

void GameSI::ClearMapMarker(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.gameUi->ClearMapMarker(command.GetArgument(0).GetByte());
}

void GameSI::SetGunHudEnabled(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 1);
    context.gameUi->SetGunHudEnabled(command.GetArgument(0).GetInt());
}

void GameSI::GetItemUseCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetItemUseCount());
}

void GameSI::GetKillCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetKillCount());
}

void GameSI::GetPlayTime(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetPlayTime());
}

void GameSI::GetDeathCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) command.GetOutput()->SetInt(context.player->GetDeathCount());
}

void GameSI::GetNoteCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(context.scene->GetUiController()->CountNotes());
    }
}

void GameSI::GetSecretCount(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (command.GetOutput() != nullptr) {
        command.GetOutput()->SetInt(context.scene->GetUiController()->CountSecrets());
    }
}

void GameSI::SetSharedWord255(soft3d::ScriptCommand& command)
{
    RequireArguments(command, 0);
    if (script == nullptr) {
        throw GameSIError(command.GetId(), GetArmHandlerAddress(command.GetId()),
                          "script back-pointer is not bound");
    }
    script->SetSharedVariable(255, 1);
}

void GameSI::NullCallback(soft3d::ScriptCommand&)
{
}

} // namespace game

# GameSI callback and episode integration matrix

## Coverage contract

The native table at `0x00140cc0` contains 218 `{handler, thisAdjustment}` records for IDs 43–260. All adjustments are zero. The table below records native offsets, recovered argument/result types, portable implementations, and directed test families. Types are SST types, not C++ ABI types; every native callback has the C++ shape `void handler(GameSI*, ScriptValue** arguments, int argumentCount, ScriptValue* output)`.

`test_game_integration.cpp` independently counts all 217 shipped SST files, 19,251 host-call records (2,261 generic and 16,990 game), 162 used game IDs, and the 53 unused IDs in 43–257. It executes all 53 plus 258–260 as synthetic scripts through the concrete `GameSI`, loads all 151 day maps through the production BSP/entity factory, and runs every shipped episode for 120 real engine frames after its actual day/episode selection. `common/menu.sbp`, the 152nd shipped SBP, is exercised by engine startup. These tests distinguish static call-record coverage from conditional runtime coverage; they do not claim that every branch of every episode is reached in 120 frames.

IDs 69, 82, 258, and 260 are confirmed original empty callbacks. ID 82 is `CalcRank`; no ranking algorithm should be invented. ID 259 writes one to shared word 255 and does not produce a command result. ID 69's callback is empty despite its position in the camera family.

| ID | ARM handler | Arguments -> result | Subsystem | Portable implementation | Directed tests |
|---:|---:|---|---|---|---|
| 43 | `0x000653c0` | `int -> bool` | Input | `GameSI::IsActionFirstDown` | scene, integration |
| 44 | `0x00065428` | `int -> bool` | Input | `GameSI::IsActionDown` | scene, integration |
| 45 | `0x00065490` | `int -> bool` | Input | `GameSI::IsActionPress` | scene, integration |
| 46 | `0x000654f8` | `int -> bool` | Input | `GameSI::IsActionUp` | scene, integration |
| 47 | `0x00065560` | `int -> bool` | Input | `GameSI::IsActionClicked` | scene, integration |
| 48 | `0x000655c8` | `void -> bool` | Input | `GameSI::IsAnyKeyDown` | scene, integration |
| 49 | `0x00065614` | `void -> bool` | Input | `GameSI::IsAnyKeyClick` | scene, integration |
| 50 | `0x00065660` | `string` | Logging | `GameSI::LogMessage` | scene, integration |
| 51 | `0x000656dc` | `int,int,int,int -> bool` | Scene/fade | `GameSI::FadeOut` | scene, integration |
| 52 | `0x00065820` | `int,int,int,int -> bool` | Scene/fade | `GameSI::FadeIn` | scene, integration |
| 53 | `0x00065964` | `void` | Scene/day | `GameSI::LoadNextDay` | scene, integration |
| 54 | `0x000659ac` | `int, fixed×6, short×3` | Scene/transition | `GameSI::SetLoadMapParams` | scene, integration |
| 55 | `0x00065b18` | `short,short` | Scene/transition | `GameSI::LoadNextLevel` | scene, integration |
| 56 | `0x00065b78` | `short` | Scene/map | `GameSI::LoadMap` | scene, integration |
| 57 | `0x00065bc0` | `void -> bool` | Scene/map | `GameSI::IsMapLoaded` | scene, integration |
| 58 | `0x00065c10` | `short` | Scene/events | `GameSI::SetEventFlag` | scene, integration |
| 59 | `0x00065c64` | `short -> bool` | Scene/events | `GameSI::GetEventFlag` | scene, integration |
| 60 | `0x00065cd0` | `void` | Completion/UI | `GameSI::CompleteGame` | UI, integration |
| 61 | `0x00065d08` | `void` | Save/UI | `GameSI::OpenSaveMenu` | UI, integration |
| 62 | `0x00065d3c` | `bool` | Manager pause | `GameSI::SetPaused` | scene, UI, integration |
| 63 | `0x00065d84` | `bool` | Camera | `GameSI::SetCameraFrozen` | camera, integration |
| 64 | `0x00065e3c` | `fixed×6` | Camera | `GameSI::CameraLookAt` | scene, camera, integration |
| 65 | `0x00065f94` | `fixed×7 -> bool` | Camera | `GameSI::MoveCamera` | scene, camera, integration |
| 66 | `0x000660b4` | `int,fixed×7,string,fixed,bool,short` | Camera/SPL | `GameSI::SetCameraMode` | camera, integration |
| 67 | `0x00066444` | `void -> bool` | Camera/SPL | `GameSI::IsCameraPathCompleted` | camera, integration |
| 68 | `0x00066488` | `void` | Camera | `GameSI::BindCameraToPlayer` | scene, camera, integration |
| 69 | `0x00066504` | `void`; original no-op | Camera family | `GameSI::NullCallback` | scene, integration |
| 70 | `0x0006652c` | `void` | Camera | `GameSI::BindCameraToEnemy` | scene, camera, integration |
| 71 | `0x00066664` | `int,fixed` | Camera shake | `GameSI::ShakeCamera` | scene, camera, integration |
| 72 | `0x000666c8` | `bool,fixed` | Camera follow | `GameSI::SetDirectFollow` | scene, camera, integration |
| 73 | `0x0006672c` | `short` | Event UI | `GameSI::ShowEventMessage` | UI, integration |
| 74 | `0x00066774` | `byte,short` | Dialog/UI | `GameSI::SetDialogString` | UI, integration |
| 75 | `0x000667d4` | `short,byte,bool` | Dialog/UI | `GameSI::ShowChoiceDialog` | UI, integration |
| 76 | `0x00066860` | `int` | Dialog/UI | `GameSI::ShowSimpleMessage` | UI, integration |
| 77 | `0x000668a8` | `int,int` | Dialog/UI | `GameSI::ShowTimedMessage` | UI, integration |
| 78 | `0x00066908` | `void -> int` | Dialog/UI | `GameSI::GetDialogResult` | UI, integration |
| 79 | `0x0006694c` | `void` | Dialog/UI | `GameSI::CloseDialog` | UI, integration |
| 80 | `0x00066980` | `void -> bool` | Dialog/UI | `GameSI::IsDialogFinished` | UI, integration |
| 81 | `0x000669c8` | `void` | Map/UI | `GameSI::ShowMap` | UI, integration |
| 82 | `0x000669fc` | `void`; original `CalcRank` no-op | Progression | `GameSI::NullCallback` | UI, integration |
| 83 | `0x00066a24` | `void`; original no-op | Progression | `GameSI::NullCallback` | UI, integration |
| 84 | `0x00066a4c` | `string,bool` | Music | `GameSI::PlayMusic` | scene, integration |
| 85 | `0x00066ab0` | `void` | Music | `GameSI::StopMusic` | scene, integration |
| 86 | `0x00066ae8` | `int,string,int,int` | Environment audio | `GameSI::AddEnvironmentSound` | scene, integration |
| 87 | `0x00066b80` | `int` | Environment audio | `GameSI::ClearEnvironmentChannel` | scene, integration |
| 88 | `0x00066bc8` | `string` | Sound | `GameSI::PlaySound` | scene, integration |
| 89 | `0x00066c14` | `void` | Sound | `GameSI::StopAllSounds` | scene, integration |
| 90 | `0x00066c48` | `string,fixed×3` | Positional sound | `GameSI::CreatePositionalSound` | scene, integration |
| 91 | `0x00066d1c` | `void` | Sound instances | `GameSI::ClearSoundInstances` | scene, integration |
| 92 | `0x00066d50` | `int` | Camera listener | `GameSI::SetUseCameraListener` | scene, integration |
| 93 | `0x00066d94` | `int` | Script timers | `GameSI::StartScriptTimer` | scene, integration |
| 94 | `0x00066ddc` | `int -> int` | Script timers | `GameSI::GetScriptTimerElapsed` | scene, integration |
| 95 | `0x00066e34` | `int` | Script timers | `GameSI::StopScriptTimer` | scene, integration |
| 96 | `0x00066e7c` | `int,int` | Dialog/UI | `GameSI::ShowTimedResultMessage` | UI, integration |
| 97 | `0x00066edc` | `void` | Dialog/UI | `GameSI::CloseDialog` | UI, integration |
| 98 | `0x00066f10` | `short,bool` | BSP lightstyle | `GameSI::SetLightStyle` | scene, camera, integration |
| 99 | `0x00066f70` | `short -> int` | BSP lightstyle | `GameSI::GetLightStyle` | scene, integration |
| 100 | `0x00066fc8` | `short×3,fixed,fixed` | BSP fog | `GameSI::SetFog` | scene, integration |
| 101 | `0x0006714c` | `bool,short` | Subtitle/UI | `GameSI::SetSubtitle` | UI, integration |
| 102 | `0x000671ac` | `bool×4`; suppression mask | Input/UI | `GameSI::SetActionMask` | scene, UI, integration |
| 103 | `0x000672e4` | `int` | Presentation shader | `GameSI::SelectPresentationShader` | scene, integration |
| 104 | `0x00067404` | `byte,byte,int,int` | Map markers | `GameSI::SetMapMarker` | UI, integration |
| 105 | `0x0006749c` | `byte` | Map markers | `GameSI::ClearMapMarker` | UI, integration |
| 106 | `0x000674e4` | `int` | Gun HUD | `GameSI::SetGunHudEnabled` | UI, integration |
| 107 | `0x00067524` | `void -> int` | Statistics | `GameSI::GetItemUseCount` | UI, integration |
| 108 | `0x00067560` | `void -> int` | Statistics | `GameSI::GetKillCount` | UI, integration |
| 109 | `0x0006759c` | `void -> int` | Statistics | `GameSI::GetPlayTime` | UI, integration |
| 110 | `0x000675d8` | `void -> int` | Statistics | `GameSI::GetDeathCount` | UI, integration |
| 111 | `0x00067614` | `void -> int` | Statistics | `GameSI::GetNoteCount` | UI, integration |
| 112 | `0x000676ac` | `void -> int` | Statistics | `GameSI::GetSecretCount` | UI, integration |
| 113 | `0x00067760` | `fixed×6` | BSP vector storage | `GameSI::SetBspScriptVectors` | scene, integration |
| 114 | `0x000678ac` | `bool` | Player presentation | `GameSI::SetPlayerPresentationVisible` | scene, integration |
| 115 | `0x000678f8` | `bool`; nonzero disables | Input | `GameSI::SetControlsDisabled` | scene, integration |
| 116 | `0x00067948` | `bool` | Player visibility | `GameSI::SetPlayerVisible` | scene, integration |
| 117 | `0x000679a4` | `fixed×3` | Player direction | `GameSI::SetPlayerDirection` | player, integration |
| 118 | `0x00067a60` | `bool` | Player clipping | `GameSI::SetPlayerClipping` | player, integration |
| 119 | `0x00067ab8` | `string,bool` | Player animation | `GameSI::SetPlayerAnimation` | player, integration |
| 120 | `0x00067b60` | `void -> bool` | Player animation | `GameSI::IsPlayerAnimationFinished` | player, integration |
| 121 | `0x00067ba4` | `fixed×3` | Player teleport | `GameSI::TeleportPlayer` | scene, player, integration |
| 122 | `0x00067cb0` | `fixed×3 -> bool` | Player navigation | `GameSI::NavigatePlayer` | player, integration |
| 123 | `0x00067da8` | `fixed×4 -> bool` | Player hearing | `GameSI::PlayerHearSound` | player, integration |
| 124 | `0x00067e94` | `fixed×6 -> bool` | Player bounds | `GameSI::PlayerPointInside` | player, integration |
| 125 | `0x00067fa8` | `fixed×6 -> bool` | Player bounds | `GameSI::PlayerPointOutside` | player, integration |
| 126 | `0x000680c0` | `short,fixed -> bool` | Player/enemy hearing | `GameSI::PlayerHearEnemy` | player, integration |
| 127 | `0x00068174` | `short,fixed -> bool` | Player/thing hearing | `GameSI::PlayerHearThing` | player, integration |
| 128 | `0x00068228` | `int` | Player attack | `GameSI::PlayerAttack` | player, integration |
| 129 | `0x00068280` | `void -> bool` | Player state | `GameSI::IsPlayerDead` | player, integration |
| 130 | `0x000682d0` | `void -> int` | Player health | `GameSI::GetPlayerHealth` | player, integration |
| 131 | `0x0006830c` | `short` | Player health | `GameSI::SetPlayerHealth` | player, integration |
| 132 | `0x000683c4` | `short` | Player health | `GameSI::AddPlayerHealth` | player, integration |
| 133 | `0x000684ac` | `short` | Player damage | `GameSI::DamagePlayer` | player, integration |
| 134 | `0x000684f8` | `int` | Player weapon | `GameSI::SetPlayerWeapon` | player, integration |
| 135 | `0x00068540` | `void -> int` | Player weapon | `GameSI::GetPlayerWeapon` | player, integration |
| 136 | `0x00068584` | `void -> bool` | Player state | `GameSI::IsPlayerAttacking` | player, integration |
| 137 | `0x000687d0` | `void -> bool` | Player hide | `GameSI::IsPlayerHidden` | player, integration |
| 138 | `0x00068814` | `short×3` | Player QTE | `GameSI::StartPlayerQTE` | player, integration |
| 139 | `0x00068894` | `void -> int` | Player QTE | `GameSI::GetPlayerQTEResult` | player, integration |
| 140 | `0x000688d0` | `void -> int` | Used item | `GameSI::GetPlayerUsedItem` | player, integration |
| 141 | `0x0006890c` | `void` | Used item | `GameSI::ClearPlayerUsedItem` | player, integration |
| 142 | `0x00068944` | `void` | Player FPS | `GameSI::EnterPlayerFPS` | player, integration |
| 143 | `0x00068978` | `void -> bool` | Player combat | `GameSI::HasAttackingEnemy` | player, integration |
| 144 | `0x000689bc` | `fixed` | Player speed | `GameSI::SetPlayerSpeed` | player, integration |
| 145 | `0x00068a10` | `short -> bool` | Player pushing | `GameSI::IsPlayerPushing` | player, integration |
| 146 | `0x00068a68` | `bool` | Player variant | `GameSI::SetPlayerYoung` | player, integration |
| 147 | `0x00068af4` | `void -> bool` | Player variant | `GameSI::IsPlayerYoung` | player, integration |
| 148 | `0x00068b44` | `void` | Player hide | `GameSI::ExitPlayerHide` | player, integration |
| 149 | `0x00068b78` | `short,int,short,fixed×6` | Event trigger | `GameSI::AddEventTrigger` | triggers, integration |
| 150 | `0x00068cbc` | `short,int` | Event trigger | `GameSI::SetEventTriggerCount` | triggers, integration |
| 151 | `0x00068d24` | `short,short` | Event trigger | `GameSI::SetEventTriggerText` | triggers, integration |
| 152 | `0x00068d8c` | `short -> bool` | Event trigger | `GameSI::IsEventTriggerActive` | triggers, integration |
| 153 | `0x00068df0` | `short×3,fixed×6` | Pick trigger | `GameSI::AddPickTrigger` | triggers, integration |
| 154 | `0x00068f34` | `short,bool` | Pick trigger | `GameSI::EnablePickTrigger` | triggers, integration |
| 155 | `0x00068fa4` | `short×6,fixed×12` | Door trigger | `GameSI::AddDoorTrigger` | triggers, integration |
| 156 | `0x00069200` | `short,short` | Door trigger | `GameSI::SetDoorTriggerItem` | triggers, integration |
| 157 | `0x00069268` | `short,short` | Door trigger | `GameSI::SetDoorTriggerLockedText` | triggers, integration |
| 158 | `0x000692d0` | `short,bool` | Door trigger | `GameSI::EnableDoorTrigger` | triggers, integration |
| 159 | `0x0006934c` | `short,fixed×6` | Act trigger | `GameSI::AddActTrigger` | triggers, integration |
| 160 | `0x00069450` | `short,int` | Act trigger | `GameSI::SetActTriggerOptionCount` | triggers, integration |
| 161 | `0x000694b8` | `short,int,short` | Act trigger | `GameSI::SetActTriggerString` | triggers, integration |
| 162 | `0x00069578` | `short -> int` | Act trigger | `GameSI::GetActTriggerSelection` | triggers, integration |
| 163 | `0x000695f8` | `short` | Act trigger | `GameSI::ClearActTriggerSelection` | triggers, integration |
| 164 | `0x00069660` | `short,fixed×6` | Act camera | `GameSI::SetActTriggerCamera` | triggers, integration |
| 165 | `0x0006979c` | `short,bool` | Act trigger | `GameSI::EnableActTrigger` | triggers, integration |
| 166 | `0x0006980c` | `short,short,fixed×12` | Hide trigger | `GameSI::AddHideTrigger` | triggers, integration |
| 167 | `0x000699f8` | `short,fixed,fixed` | Hide trigger | `GameSI::SetHideTriggerLimits` | triggers, integration |
| 168 | `0x00069a9c` | `short,bool` | Hide trigger | `GameSI::EnableHideTrigger` | triggers, integration |
| 169 | `0x00069b0c` | `short,short,fixed×6` | Item trigger | `GameSI::AddItemTrigger` | triggers, integration |
| 170 | `0x00069c30` | `short,short` | Item trigger | `GameSI::AddItemTriggerItem` | triggers, integration |
| 171 | `0x00069ca0` | `short,bool` | Item trigger | `GameSI::EnableItemTrigger` | triggers, integration |
| 172 | `0x00069d10` | `short,fixed×6,bool` | Talk trigger | `GameSI::AddTalkTrigger` | triggers, integration |
| 173 | `0x00069e34` | `short,fixed×3` | Talk camera | `GameSI::SetTalkTriggerCameraPosition` | triggers, integration |
| 174 | `0x00069eec` | `short,short,fixed×6` | Talk camera | `GameSI::AddTalkTriggerCamera` | triggers, integration |
| 175 | `0x0006a020` | `short×4` | Talk lines | `GameSI::AddTalkTriggerLine` | triggers, integration |
| 176 | `0x0006a0cc` | `short -> bool` | Talk state | `GameSI::IsTalkTriggerActive` | triggers, integration |
| 177 | `0x0006a14c` | `short -> bool` | Talk state | `GameSI::IsTalkTriggerCompleted` | triggers, integration |
| 178 | `0x0006a1d8` | `short,bool` | Talk trigger | `GameSI::EnableTalkTrigger` | triggers, integration |
| 179 | `0x0006a248` | `short` | Trigger removal | `GameSI::RemoveInteractionTrigger` | triggers, integration |
| 180 | `0x0006a298` | `short,int,short,fixed×6` | Enemy spawn | `GameSI::AddEnemy` | enemy, integration |
| 181 | `0x0006a408` | `short` | Enemy persistence | `GameSI::PersistEnemy` | enemy, integration |
| 182 | `0x0006a468` | `short,bool` | Enemy rendering | `GameSI::SetEnemyNormalRendering` | enemy, integration |
| 183 | `0x0006a4f4` | `short,string,bool` | Enemy animation | `GameSI::SetEnemyAnimation` | enemy, integration |
| 184 | `0x0006a5cc` | `short -> bool` | Enemy animation | `GameSI::IsEnemyAnimationFinished` | enemy, integration |
| 185 | `0x0006a64c` | `short -> bool` | Enemy state | `GameSI::IsEnemyRemoved` | enemy, integration |
| 186 | `0x0006a6f8` | `short -> int` | Enemy health | `GameSI::GetEnemyHealth` | enemy, integration |
| 187 | `0x0006a76c` | `short,short` | Enemy health | `GameSI::SetEnemyHealth` | enemy, integration |
| 188 | `0x0006a834` | `short -> int` | Enemy attack | `GameSI::GetEnemyAttackPower` | enemy, integration |
| 189 | `0x0006a8a8` | `short,int` | Enemy attack | `GameSI::SetEnemyAttackPower` | enemy, integration |
| 190 | `0x0006a920` | `short,fixed×3` | Enemy direction | `GameSI::SetEnemyDirection` | enemy, integration |
| 191 | `0x0006aa14` | `short,fixed×3` | Enemy teleport | `GameSI::TeleportEnemy` | enemy, integration |
| 192 | `0x0006ab0c` | `short,fixed×3 -> bool` | Enemy movement | `GameSI::MoveEnemyTo` | enemy, integration |
| 193 | `0x0006ac1c` | `short,fixed×3 -> bool` | Enemy hearing | `GameSI::EnemyHearSound` | enemy, integration |
| 194 | `0x0006ad44` | `short,fixed×6 -> bool` | Enemy bounds | `GameSI::EnemyBoundsInside` | enemy, integration |
| 195 | `0x0006ae9c` | `short` | Enemy follow | `GameSI::FollowEnemy` | enemy, integration |
| 196 | `0x0006af04` | `short` | Enemy attack | `GameSI::AttackEnemy` | enemy, integration |
| 197 | `0x0006afa0` | `short` | Enemy removal | `GameSI::RemoveEnemy` | enemy, integration |
| 198 | `0x0006b00c` | `short` | Enemy removal by type | `GameSI::RemoveEnemiesByType` | enemy, integration |
| 199 | `0x0006b0b4` | `void` | Remove all enemies | `GameSI::RemoveAllEnemies` | enemy, integration |
| 200 | `0x0006b138` | `short,bool` | Enemy pause | `GameSI::PauseEnemy` | enemy, integration |
| 201 | `0x0006b1c8` | `short,bool` | Enemy pause by type | `GameSI::PauseEnemiesByType` | enemy, integration |
| 202 | `0x0006b298` | `bool` | Pause all enemies | `GameSI::PauseAllEnemies` | enemy, integration |
| 203 | `0x0006b33c` | `short,bool` | Enemy stop | `GameSI::StopEnemy` | enemy, integration |
| 204 | `0x0006b3bc` | `short,fixed×6` | StayBox creation | `GameSI::AddStayBox` | enemy, integration |
| 205 | `0x0006b4c0` | `short,fixed×6` | StayBox denied region | `GameSI::AddStayBoxDeniedRegion` | mapunits, enemy, integration |
| 206 | `0x0006b5e8` | `short,fixed×6` | StayBox allowed region | `GameSI::AddStayBoxAllowedRegion` | mapunits, enemy, integration |
| 207 | `0x0006b710` | `short,short` | StayBox enemy link | `GameSI::AddStayBoxEnemy` | mapunits, enemy, integration |
| 208 | `0x0006b7bc` | `short,fixed×3` | StayBox return | `GameSI::SetStayBoxReturnPosition` | enemy, integration |
| 209 | `0x0006b884` | `short,fixed×3,short,fixed×3` | Enemy waypoint | `GameSI::AddEnemyWaypoint` | enemy, integration |
| 210 | `0x0006b9c8` | `short,bool` | Enemy waypoint loop | `GameSI::SetEnemyWaypointLoop` | enemy, integration |
| 211 | `0x0006ba48` | `short` | Enemy waypoints | `GameSI::ClearEnemyWaypoints` | enemy, integration |
| 212 | `0x0006baac` | `short,fixed` | Enemy perception | `GameSI::SetEnemyPerceptionRange` | enemy, integration |
| 213 | `0x0006bb34` | `short -> bool` | Enemy alert | `GameSI::IsEnemyAlerted` | enemy, integration |
| 214 | `0x0006bbb4` | `short,short,string` | Enemy animation resource | `GameSI::SetEnemyAnimationResource` | enemy, integration |
| 215 | `0x0006bc50` | `short,short,string` | Enemy sound resource | `GameSI::SetEnemySoundResource` | enemy, integration |
| 216 | `0x0006bcec` | `short,fixed` | Enemy alert speed | `GameSI::SetEnemyAlertSpeed` | enemy, integration |
| 217 | `0x0006bd78` | `short,fixed×6` | Collision unit creation | `GameSI::AddCollisionUnit` | enemy, integration |
| 218 | `0x0006be7c` | `short` | Collision unit removal | `GameSI::RemoveCollisionUnit` | enemy, integration |
| 219 | `0x0006becc` | `short,fixed×9` | Trap creation | `GameSI::AddTrap` | mapunits, integration |
| 220 | `0x0006c03c` | `short` | Trap removal | `GameSI::RemoveTrap` | mapunits, integration |
| 221 | `0x0006c08c` | `short,int,int,string,fixed×12,bool` | Thing creation | `GameSI::AddThing` | mapunits, integration |
| 222 | `0x0006c31c` | `short,fixed×3` | Thing direction | `GameSI::SetThingDirection` | mapunits, integration |
| 223 | `0x0006c3e8` | `short,short` | Pickable item | `GameSI::SetPickableItem` | mapunits, integration |
| 224 | `0x0006c458` | `short,bool` | Thing collision | `GameSI::SetThingCollision` | mapunits, integration |
| 225 | `0x0006c4c0` | `short,fixed,fixed,short` | Thing lighting/bounce | `GameSI::SetThingIntensityBounce` | mapunits, integration |
| 226 | `0x0006c564` | `short,bool` | Thing visibility | `GameSI::SetThingVisible` | mapunits, integration |
| 227 | `0x0006c5cc` | `short -> bool` | Thing visibility | `GameSI::IsThingVisible` | mapunits, integration |
| 228 | `0x0006c63c` | `short,string,int×3` | Thing animation | `GameSI::SetThingAnimation` | mapunits, integration |
| 229 | `0x0006c718` | `short,bool` | Animation pause | `GameSI::SetThingAnimationPaused` | mapunits, integration |
| 230 | `0x0006c788` | `short -> bool` | Animation pause | `GameSI::IsThingAnimationPaused` | mapunits, integration |
| 231 | `0x0006c7ec` | `short,int` | Animation time | `GameSI::SetThingAnimationTime` | mapunits, integration |
| 232 | `0x0006c85c` | `short -> int` | Animation time | `GameSI::GetThingAnimationTime` | mapunits, integration |
| 233 | `0x0006c8c0` | `short -> bool` | Animation completion | `GameSI::IsThingAnimationFinished` | mapunits, integration |
| 234 | `0x0006c924` | `short,string,string` | Break model | `GameSI::SetThingBreakModel` | mapunits, integration |
| 235 | `0x0006c9e0` | `short,string` | Break animation | `GameSI::SetThingBreakAnimation` | mapunits, integration |
| 236 | `0x0006ca54` | `short,string` | Break sound | `GameSI::SetBreakableSound` | mapunits, integration |
| 237 | `0x0006cac4` | `short,string` | Push sound | `GameSI::SetPushableSound` | mapunits, integration |
| 238 | `0x0006cb34` | `short,short` | Rotate X | `GameSI::RotateThingX` | mapunits, integration |
| 239 | `0x0006cbec` | `short,short` | Rotate Y | `GameSI::RotateThingY` | mapunits, integration |
| 240 | `0x0006cca4` | `short,short` | Rotate Z | `GameSI::RotateThingZ` | mapunits, integration |
| 241 | `0x0006cd5c` | `short,fixed×3` | Thing position | `GameSI::SetThingPosition` | mapunits, integration |
| 242 | `0x0006ce18` | `short,fixed×3` | Relative movement | `GameSI::MoveThing` | mapunits, integration |
| 243 | `0x0006cf08` | `short,fixed×4 -> bool` | Smooth movement | `GameSI::MoveThingTowards` | mapunits, integration |
| 244 | `0x0006cfec` | `short,short` | Thing destruction | `GameSI::DestroyThing` | mapunits, integration |
| 245 | `0x0006d06c` | `short,fixed` | Depth bias | `GameSI::SetThingDepthBias` | mapunits, integration |
| 246 | `0x0006d0dc` | `short` | Thing removal | `GameSI::RemoveThing` | mapunits, integration |
| 247 | `0x0006d12c` | `short -> bool` | Thing existence | `GameSI::ThingExists` | mapunits, integration |
| 248 | `0x0006d198` | `short -> bool` | Break event | `GameSI::ConsumeBreakableEvent` | mapunits, integration |
| 249 | `0x0006d208` | `short,fixed×6 -> bool` | Bounds inside | `GameSI::ThingBoundsInside` | mapunits, integration |
| 250 | `0x0006d350` | `short,fixed×6 -> bool` | Bounds outside | `GameSI::ThingBoundsOutside` | mapunits, integration |
| 251 | `0x0006d498` | `short,bool` | Thing lighting | `GameSI::SetThingLighting` | mapunits, integration |
| 252 | `0x0006d508` | `short,bool` | Pushability | `GameSI::SetPushableEnabled` | mapunits, integration |
| 253 | `0x0006d574` | `short,short,string` | Texture override | `GameSI::SetThingTexture` | mapunits, integration |
| 254 | `0x0006d600` | `short -> int` | Magic-cube orientation | `GameSI::GetMagicCubeOrientation` | mapunits, integration |
| 255 | `0x0006d988` | `short` | Inventory add | `GameSI::AddPlayerItem` | player, integration |
| 256 | `0x0006d9d4` | `short -> bool` | Inventory query | `GameSI::HasPlayerItem` | player, integration |
| 257 | `0x0006da2c` | `short` | Inventory remove | `GameSI::RemovePlayerItem` | player, integration |
| 258 | `0x00064fd0` | `void`; original no-op | Script control | `GameSI::NullCallback` | scene, integration |
| 259 | `0x00064ff8` | `void`; writes shared word 255 = 1 | Script control | `GameSI::SetSharedWord255` | scene, integration |
| 260 | `0x00065034` | `void`; original no-op | Script control | `GameSI::NullCallback` | scene, integration |

## Integration corrections recovered by the sweep

- The manager's 32 saved two-word records are script frame timers, not opaque data. Start writes `{0,0}`, stop writes `{-1,-1}`, elapsed is `current-start`, and only unpaused manager updates increment `current` when `start==0`.
- Control-map global and per-action gates use zero for enabled and nonzero for disabled. Script ID 102 writes suppression flags and ID 115 sets the global disabled flag. Changing global state clears input.
- ID 92 selects the camera as audio listener through `Scene+0x6b00`; it is unrelated to the license countdown.
- ID 114 changes only `Player+0x9dc` presentation visibility. ID 116 calls the full player visibility virtual at primary-vtable offset `0x5c`.
- BSP path edges are `{uint16 nextNode,uint16 weight}`. `0xffff` is a valid null next-node sentinel. Eager validation of `weight` as an index incorrectly rejected map 3004; rejection of the sentinel incorrectly rejected map 1002.
- `CameraTrigger_LoadEntity` ignores SPL load failure. Treating it as fatal incorrectly rejected map 1003.
- Sub-1000 episode paths use `ep0%d`, without an underscore. This is required by resources such as `day4/ep0101.sst`.

# 7 Days game-object recovery inventory

## Scope and Session-34A completion

This inventory covers game-owned objects and non-RTTI helpers that are **not** reusable Soft3D APIs. It is the companion to `soft3d_sdk_inventory.md`; “game-specific” there does not mean unreviewed or unrecoverable.

Session 34A rechecked portable revision `d2a94b88a6a570aacf162d3d2d9077167070b4cd` against the native `out.bin` database. The pass covered all 462 zero-direct-reference function records, the named owner families in the early game/engine range, the previously identified candidates and their callers, and source definitions under `src/game`, `src/soft3d/MenuDialogs.*`, `GameConfig.*`, and `GameUIDlg.*`. No additional unnamed enemy, trigger, player-state, menu, scene, or manager class was found. Constructor/destructor duplicates and internal inlined equivalents were not promoted to new portable classes.

All candidate rows are now **matches**, **corrected**, **original empty**, **duplicate compiled body**, **reserved / intentionally omitted**, **platform-specific**, or **intentional portable safety difference**. No missing game API or unresolved gameplay field remains in this inventory.

### Verification codes

| Code | Verification |
|---|---|
| **G1** | `test_input.cpp`: keys 1–23, all six ControlMap any-key wrappers, primary/secondary action queries, global/action gates, gate bypass, synthetic events, and exact ActionUp edges. |
| **G2** | `test_game_camera.cpp` and `test_game_player.cpp`: mode/saved-state accessors, trigger storage/update lifecycle, unit-manager pointer, orbit arguments/rotation/repetition, facing and bounds calculations, and real engine frames. |
| **G3** | Existing menu/configuration and generic dialog coverage; reserved fields are excluded without invented semantics. |
| **G4** | `test_game_enemy.cpp`: alternate DarkKen descending/equality results and existing ascending region-neighbor checks. |
| **E** | Existing game-focused and real engine-frame coverage. |
| **X** | Explicit exclusion; no consumer-style implementation is warranted. |

## Native ownership and naming corrections

The second pass corrected these earlier misclassifications. Ghidra names, signatures, plate comments, and tags were updated accordingly.

| Native address | Earlier interpretation | Correct native meaning and portable disposition |
|---|---|---|
| `0x00031708` | `ControlMap_SetEngineContext` | **Corrected ownership:** `Bsp_SetEntityFactory`, storing the callback at Bsp `+0x2aae0`. `Scene_BindEngineServices` passes `Bsp_CreateGameEntity`. Portable `Bsp::SetEntityFactory` **matches**, with context stored by the same public call. |
| `0x0003173c` | `ControlMap_SetInput` | **Corrected ownership:** `FadeShader_SetRenderer`, storing renderer at `+0x14`. Public `FadeShader::SetRenderer` was added and tested. |
| `0x00054028` | `ControlMap_IsActionDisabled` | **Corrected:** `ControlMap_GetPrimaryBinding`, reading primary bindings at `+0x48`. Its QTE caller feeds the result to `Input_IsKeyClick`. Existing `ControlMap::GetPrimaryBinding` **matches**. |
| `0x0004e6b8` | `PlayDlg_SetNewGameOptions` | **Corrected:** writes the reserved `S3dPoint` at PlayDlg `+0x248/+0x24c`. No native caller or point consumer exists. **Reserved / intentionally omitted**; weapon/outfit options belong to MainUIDlg `+0x290/+0x294`, not this body. |
| `0x0006e120` | unknown GameUI field `+0x80` | **Corrected:** returns inherited Dialog element-template 1 `value3`. `Dialog::GetElementTemplate` already exposes that storage. No GameUI-specific wrapper is needed; **intentionally omitted**. |
| `0x00078c60` | previous-direction X | **Corrected:** `CameraController_GetPreviousMode`, reading the mode snapshot at `+0x190`. Public `GetPreviousMode` **matches**. |
| `0x00078c88` | fixed follow target | **Corrected:** `CameraController_GetSavedDirection`, copying the direction saved at `+0x194` by mode setup. Public `GetSavedDirection` **matches**. Path `fixedTarget` remains the distinct field at `+0x1fc`. |
| `0x00078cc0` | renderer pointer | **Corrected:** `CameraController_GetSavedFollowDistance`, reading the fixed-point snapshot at `+0x1a0`. Public `GetSavedFollowDistance` **matches**. BSP/screen/control/camera/manager pointers are at `+0x1a4` through `+0x1b4`. |
| `0x000d9ee8` | object bounds translated by player position | **Corrected arithmetic:** object world bounds **minus** player position, followed by local-bounds Y replacement. `Player::GetObjectBounds` implements the subtraction. |

The camera accessors are pinned by mode-transition tests, not by structure-field names alone. After discarding an unsafe shifting edit, the final Ghidra layout uses same-size types: `CameraModeHistoryPrefix` at `+0x190` contains previous mode and saved-direction X/Y, `+0x19c` is saved-direction Z, and `+0x1a0` is saved follow distance. The 0x208-byte structure size and all later service-pointer offsets remain unchanged; typed decompilation was read back.

## ControlMap and input policy

The native object stores global disable at `+0`, input at `+0x04`, eight action-disable words at `+0x08`, eight secondary bindings at `+0x28`, and eight primary bindings at `+0x48`. Zero means enabled. Portable `ControlMap` retains inverse Boolean gates and explicit `Input&` arguments; no stale stored input pointer is introduced.

| Native address / owner | Portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x00053108`, `0x00053138`, `0x00053168`, `0x00053198`, `0x000531c8`, `0x000531f8` | `AnyKeyFirstDown`, `AnyKeyDown`, `AnyKeyPress`, `AnyKeyUp`, `AnyKeyClick`, `AnyKeyDownOrClick` forward to `Input`. Gates do not apply. | **matches** | **G1**. |
| `0x00053754`, `0x00053808`, `0x000538bc` | `ActionKeyDown`, `ActionKeyPress`, `ActionKeyUp` send hardware-event=true to primary then secondary only when global and action gates are enabled. | **matches** | **G1**: enabled, globally disabled, action-disabled, and both key states. |
| `0x00053970`, `0x00053a84`, `0x00053b98`, `0x00053dc0` | `ActionFirstDown`, `ActionDown`, `ActionPress`, `ActionClick` expose action, primaryOnly, and respectActionGate. Primary succeeds first; secondary is allowed only when primaryOnly=false. | **matches** | **G1**, including bypass true/false. |
| `0x00053cac` `ControlMap_IsActionUp` | Requires primary UP, secondary UP, and `!primaryOnly`; primaryOnly=true always returns false. The concrete player caller is gun `AttackC1` cancellation, not QTE. | **corrected** from portable primary-only UP behavior | **G1/G2**: primary-only and both-bindings-up cases through real player/QTE frames and gun cancellation frames. |
| `0x00053ed4` | `ActionDownOrClick` implements the same primary/secondary and gate rules using DOWN_OR_CLICK. | **matches** | **G1**. |
| `0x00054028` | `GetPrimaryBinding`; used by QTE independently of the action-disable gate. | **matches** | **G1/G2**. |
| `0x000536bc` | Uncalled action-disable setter variant returning 1. `SetActionEnabled` represents the called setter. | **duplicate compiled body** | **X / E**. |
| `0x00054068`, `0x000540a0` | Configuration serialization preserves exactly 0x20 bytes of secondary bindings. | **matches** | **E** (`test_menu.cpp`). |
| `0x000ef0f0`, `0x000ef1c8`, `0x000ef234`, `0x000ef30c` | Generic Input FIRST_DOWN/PRESS/UP/DOWN_OR_CLICK scans keys 1–23; recovered in Session 33. DOWN/CLICK and per-key scans were already present. | **matches** | **G1/E** (`test_input.cpp`, `test_sdk_utility.cpp`). |

Portable invalid-action checks remain an intentional safety guard; valid native action behavior is unchanged.

## Player and game camera

| Native address / owner | Portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x0003b920`, `0x0003b97c` | `Player::ConfigureOrbitCamera` and `SetOrbitRotationEnabled` forward to the scene-owned controller. | **matches** | **G2**, including subsequent real frames. |
| `0x00079e38` `CameraController_ConfigureOrbit` | Non-null target selects current/orbit target; non-null center sets `position=target-center` and `orbitVertical=center.y`; angle uses `Vec3AngleXZ`. Rotation is always enabled. `vertical` sets the repeat flag, not rotation. | **corrected** center/target, angle, rotation, and repeat behavior | **G2**: argument order, enabled/disabled trajectories, and repetition. Frames <=8 retain the native fade-branch priority. |
| `0x00077ab8` | `Player::IsFacingPoint` zeros both Y components and returns dot >= 0. | **matches** | **G2**: ahead, behind, perpendicular, vertical-only. |
| `0x000d9ee8` | `Player::GetObjectBounds` subtracts player position from object world X/Y/Z, then overwrites min/max Y from `localBounds`. | **corrected** | **G2** with the real player model. Null object returns local bounds as an **intentional portable safety difference**. |
| `0x00078c60`, `0x00078c88`, `0x00078cc0` | `GetPreviousMode`, `GetSavedDirection`, `GetSavedFollowDistance`. | **corrected names / matches** | **G2** across two mode transitions. |
| `0x00078cf0`, `0x00078d18` | `HasSavedState`, `ClearSavedState`; clear affects only the flag. | **matches** | **G2**: initial, trigger save, clear, nested trigger coverage. |
| `0x00078df4`, `0x00078e7c`, `0x00078ecc` | `GetUp`, on-demand `Length(target-position)`, and `GetDirection`. | **matches** | **G2**. |
| `0x00079178` | `SetOrbitTarget`: null disables; non-null copies to current target and orbitTarget and enables. | **matches** | **G2**. |
| `0x000792d8`, `0x00079300` | Opaque unit-manager pointer get/set. | **matches** | **G2**. |
| `0x00080f68` | `UpdateTriggers` captures count and calls Update in storage order. | **matches** | **G2**: zero/one/multiple and existing real-map trigger frames. |
| `0x0008100c`, `0x00081034` | Trigger count and indexed accessor. | **matches** | **G2**. Out-of-range returns null as an **intentional portable safety difference**. |
| `0x00078d40`, `0x00078f04`, `0x00078f34`, `0x00079238`, `0x00079260` | Collision setter, follow distance/right, bound and input-cancelled queries. | **matches** | **E** (`test_game_camera.cpp`). |

## Menu, GameUI, license, and enemy helpers

| Native address / owner | Portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x0004e6b8` `PlayDlg_SetReservedPoint` | No portable PlayDlg field or setter. MainUIDlg owns the actual new-game weapon/outfit options. | **reserved / intentionally omitted** | **G3 / X**. |
| `0x0006e120` `GameUI_GetDialogTemplate1Value3` | Generic `Dialog::GetElementTemplate(1)->value3`; no GameUI-specific state. | **intentionally omitted duplicate wrapper** | **G3 / E**. |
| `0x0000ea8c` `MyClientSession_SendVerificationRequest` | `License::SendVerification` implements the protocol. Native transport/session object is platform-specific. | **matches / platform-specific transport** | **E** (`test_game_license.cpp`). |
| `0x0001fcd0` `DarkKen_CompareRegionDistanceAscending` | Active direct-distance comparator behavior is represented by ascending region-neighbor construction. | **matches** | **G4/E**. |
| `0x0003fc04` `DarkKen_CompareRegionDistanceDescending` | `CompareDarkKenRegionDistanceDescending(const Enemy* const*, const Enemy* const*)` compares `GetPlayerDistance`; +1 greater, -1 smaller, 0 equal. | **matches as distinct helper** | **G4**. |
| `0x00027138` `Sword_IsAnimationFinished` | Sword completion flag query. | **matches** | **E**. |
| `0x0003cecc` `Player_HasAnyEnemy` | Enemy bucket count > 0. | **matches** | **E**. |
| `0x0003f7fc` `Pushable_GetPushSound` | Push-sound pointer accessor. | **matches** | **E**. |
| `0x00075d88`, `0x0008ab8c`, `0x00093758`, `0x00078064` | Door/Act/Talk/Camera trigger prompt/enabled/outside accessors. | **matches** | **E**. |
| `0x0001edd4` `GameItemRuntime_InitEmpty` | Empty body. | **original empty** | **X**. |

## Reserved, duplicate, empty, internal, and platform exclusions

| Native bodies | Disposition | Test |
|---|---|---|
| `0x0004ac80`, `0x0004b4a4`, `0x0004d2a8`, `0x0004dfa4`, `0x0004e6f8`, `0x0004e77c` | Simple writes to uncalled Option/Load/About/Help/Play fields. The consumer audit found no new evidence. **Reserved / intentionally omitted**. | **X**. |
| `0x00049e5c`, `0x0004e7a8`, `0x0004e7c8`, `0x00050700` | Empty menu callbacks/hooks. | **original empty / X**. |
| Game constructors/destructors listed in `soft3d_sdk_inventory.md` | **duplicate compiled body**; portable ownership already exists. No alternate constructor implies another game class. | **X / E**. |
| `0x0000d5f8` and other license/session/socket/phone methods | **platform-specific** transport or duplicate compiled body. Protocol outcomes remain in `License`. | **X / E**. |
| `0x0003fce4`, `0x0006505c`, `0x000829a0`, `0x00082ae0`, `0x00095a50`, `0x00096030` | MyGameUnitMgr/GameSI/Boss lifecycle and dormant debug update/render already have portable ownership and behavior. | **matches / E**. |
| StayBox region scans, Things leaf predicate, manager/trigger vector parsers, Enemy looping/alert tests, Act camera-vector comparison, PickTrigger link setters | Called native helpers have inlined or consolidated portable equivalents: `OverlapsAny`, existing submission predicate, `ParseVector`, direct sound/state checks, `HasCameraOverride`, and `PickTrigger::Configure`. Session 34A also corrected `0x000498f8` to `PickTrigger_SetPickable` (`+0x64`) and `0x000498cc` to `PickTrigger_SetPlayer` (`+0x68`). They do not require duplicate public APIs. | **matches / E**. |
| `GameItemRuntime`, `EnemyInfoGroup`, `EnemySpawnRecord`, `EnemyWaypoint`, `StayBoxRegion`, `StayBoxSpawnRecord`, `TalkLine`/`TalkLineVector`, DarkKen `RegionDistance` | Non-RTTI support records and their constructor/vector helpers map to `GameData` records, `EnemyWaypoint`, StayBox spawn/bounds storage, TalkTrigger lines, and the local region-neighbor records. Portable containers replace native linked-list/vector bookkeeping without adding another game class. | **matches / E**; DarkKen ordering also **G4**. |
| `CompareIntNotEqual`, `VectorsNearlyEqual`, and similar tiny comparison predicates | Existing direct component/epsilon comparisons preserve their called behavior. They are internal compiled helpers, not additional game-owned policy APIs requiring duplicate public functions. | **matches / intentionally omitted / E**. |
| `0x000530d4` `ComparePointerNotNull` | Uncalled trivial pointer predicate with no game-specific state or owning class. Existing null checks cover behavior; not elevated to a game API. | **intentionally omitted / X**. |

## Handoff

Session 34A closes the game-owned inventory. ControlMap queries/events/gates, player/camera wrappers, camera saved-state and trigger lifecycle, FadeShader renderer binding, and the alternate DarkKen comparator are public and tested. Orbit behavior and ActionUp primary-only behavior were corrected from ARM. Reserved setters and generic dialog storage were explicitly excluded rather than assigned invented gameplay semantics.

Focused input, camera, player, enemy, and presentation-shader tests pass, including real engine frames. The complete serial matrix passes **44/44 default**, **44/44 STDIO**, **44/44 host-math**, **44/44 WAR**, **37/37 core**, **37/37 ASan/UBSan**, and **23/23 applicable English-layout** tests, plus **6/6 SOJ** and **8/8 S3DTEX** Python tests. The English diagnostic reproduced only the same 14 Chinese-archive initialization exclusions. Ghidra is saved with no unsaved changes. The next roadmap work is Session 01's reproducible first-room correctness packet, not further invention of game APIs.

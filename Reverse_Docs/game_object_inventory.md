# 7 Days game-object recovery inventory

## Scope and relationship to the Soft3D SDK inventory

This inventory covers game-owned objects and non-RTTI helpers that are **not** reusable Soft3D APIs. It is the companion to `soft3d_sdk_inventory.md`; “game-specific” there means “does not belong in the `soft3d` library,” not “unreviewed” or “not worth recovering.”

Evidence comes from the same native `out.bin` program and portable revision `9663d705619bdada2e3f1c49300a67a7c91c9f95`. All game RTTI classes and vtables were crosschecked, all zero-direct-reference game functions were dispositioned, and called families were checked where they exposed additional missing methods. No additional unnamed enemy, trigger, player-state, or menu class was found. The remaining work is chiefly uncalled methods, alternate helpers, and non-RTTI support objects.

A dedicated **Session 34A — Game-object API completeness** is added to the roadmap. It must not move these objects into `soft3d` merely because some currently reside there for historical reasons. `ControlMap` is native game input policy despite its current portable header location.

Dispositions are **implemented**, **missing game API**, **corrected behavior required**, **duplicate compiled body**, **original empty**, **platform-specific**, **reserved/unresolved**, or **intentionally omitted**. No unresolved field may be presented as a recovered feature.

### Recovery test codes

| Code | Planned native-derived verification |
|---|---|
| **E** | Existing game-focused and real engine-frame coverage; no additional API is needed. |
| **G1** | ControlMap/Input tests for keys 1–23, primary/secondary bindings, global/action gates, gate-bypass argument, synthetic action events, disabled accessors, and exact binding bytes. |
| **G2** | Player/camera tests for wrapper argument order, facing/bounds calculations, saved-state lifecycle, orbit target null/non-null, manager pointers, and trigger update order through real frames. |
| **G3** | Menu/GameUI tests for two-word option assignment and any resolved native field semantics; no test may assign meaning to reserved data without a consumer. |
| **G4** | DarkKen alternate comparator tests for descending, ascending, and equality without merging it with the active comparator. |
| **X** | Exclusion or already covered behavior; no new consumer-style test. |

## ControlMap and input policy

The native object stores an input pointer, eight secondary bindings at `+0x28`, eight primary bindings at `+0x48`, global/action disable words where zero means enabled, and an engine context pointer. Portable `ControlMap` implements the principal action queries with inverse Boolean gates and explicit `Input&` arguments. That implementation difference is acceptable only where native behavior is preserved.

| Native address / owner | Native evidence and portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x00031708` `ControlMap_SetEngineContext`, `0x0003173c` `ControlMap_SetInput` | Native service-binding setters called by `Scene_BindEngineServices`. Portable services inject input at query time instead of storing it. | **implemented differently / missing game API** for input-backed native-style wrappers | **G1**: choose API shape without duplicating ownership or making stale input pointers. |
| `0x00053108`, `0x00053168`, `0x00053198`, `0x000531f8` | Any-key FIRST_DOWN/PRESS/UP/DOWN_OR_CLICK wrappers through the stored input. DOWN and CLICK wrappers at `0x00053138`/`0x000531c8` already have portable behavior through `Input`. | **missing game API** | **G1**: each query, no keys, each key slot, and mixed key states. |
| `0x00053754`, `0x00053808`, `0x000538bc` | Synthetic action KeyDown/KeyPress/KeyUp sends hardware-event=true to both primary and secondary bindings only when global and action gates are enabled. | **missing game API** | **G1**: enabled, globally disabled, action-disabled, and both resulting key states. |
| `0x00053ed4` `ControlMap_IsActionDownOrClick` | Tests primary, then secondary when primaryOnly=false; honors global gate and optional action gate. | **missing game API** | **G1**: DOWN, CLICK, neither, both bindings, and gate bypass. |
| `0x00053970`, `0x00053a84`, `0x00053b98`, `0x00053cac`, `0x00053dc0` | Native action queries have a fourth `respectActionGate` parameter in addition to action and primaryOnly. Portable methods always honor actionEnabled and expose no bypass. | **missing game API** for gate-bypass semantics | **G1**: respectActionGate true/false for every query. |
| `0x00053cac` `ControlMap_IsActionUp` | Native requires primary UP and, when primaryOnly=false, secondary UP as used by QTE cancellation. When primaryOnly=true the native expression always yields false; portable currently returns primary UP in that case. | **corrected behavior required** for the primaryOnly=true edge; the QTE both-bindings-up path already matches | **G1**: explicitly preserve the native primaryOnly=true result and the QTE false path through real player frames. |
| `0x00054028` `ControlMap_IsActionDisabled` | Returns the raw action-disable word and has a real QTE caller. Portable only has the setter and inline gate checks. | **missing game API** | **G1**: set/clear and independent slots. |
| `0x000536bc` | Uncalled action-disable setter variant returning 1. The called setter and portable `SetActionEnabled` implement the state change. | **duplicate compiled body** | **X / E**. |
| `0x00054068`, `0x000540a0` | Save writes exactly 0x20 bytes of secondary bindings; load copies 0x20 and advances the input pointer. Portable configuration serialization already preserves those bytes. | **implemented**; no second serialization implementation | **E** (`test_menu.cpp` configuration round trips). |
| `0x000ef0f0`, `0x000ef1c8`, `0x000ef234`, `0x000ef30c` | Generic `Input` any-key FIRST_DOWN/PRESS/UP/DOWN_OR_CLICK scans keys 1–23. These are reusable engine APIs and are also listed in the Soft3D session-33 ledger. | **missing useful API** in `soft3d::Input` | **G1** and SDK **33A**. |
| `0x000ef15c`, `0x000ef2a0`, `0x000ef4dc` | Any-key DOWN/CLICK and per-key DOWN_OR_CLICK scans/bit tests. | **implemented** | **E** (`test_input.cpp`). |

## Player and game camera

| Native address / owner | Native evidence and portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x0003b920` `Player_ConfigureOrbitCamera` | Forwards frames, optional center/target pointers, vertical, and clockwise flags to the controller at Player `+0x708`. | **missing game API** | **G2**: exact forwarding and subsequent orbit frames. |
| `0x0003b97c` `Player_SetOrbitRotationEnabled` | Forwards the Boolean to the same controller. The controller-level setter exists. | **missing game API** | **G2**. |
| `0x00077ab8` `Player_IsFacingPoint` | Copies forward and target-position delta, zeros both Y components, and returns dot >= 0. | **missing game API** | **G2**: ahead, behind, perpendicular, vertical-only delta. |
| `0x000d9ee8` `Player_GetObjectBounds` | Starts with primary-object world bounds translated by player position, then overwrites min/max Y from native player fields `+0x1c`/`+0x28`. Distinct from `GetInteractionBounds`. | **missing game API** | **G2**: real model bounds and Y override. |
| `0x00078c60` | Returns previous-direction X only. | **missing game API** | **G2**: direction update then accessor. |
| `0x00078c88` | Returns the fixed follow-target vector at controller `+0x194`. | **missing game API** | **G2**. |
| `0x00078cc0` | Returns the renderer pointer at `+0x1a0`. | **missing game API** | **G2**. |
| `0x00078cf0`, `0x00078d18` | Reads and clears the saved-state byte at `+0x10a`. Portable has `hasSavedState` privately but no public lifecycle. | **missing game API** | **G2**: initial, trigger save, clear, nested restoration. |
| `0x00078df4` | Returns the controller up vector. | **missing game API** | **G2**. |
| `0x00078e7c` | Computes `Length(target-position)` on demand. | **missing game API** | **G2**: exact fixed-point distance. |
| `0x00078ecc` | Returns the controller direction vector. | **missing game API** | **G2**. |
| `0x00079178` `CameraController_SetOrbitTarget` | Null disables orbit-target mode; non-null copies into both current target and orbitTarget and enables it. | **missing game API** | **G2**: null, non-null, subsequent orbit update. |
| `0x000792d8`, `0x00079300` | Gets/sets the opaque unit-manager pointer. Portable already stores `unitManager` privately. | **missing game API** | **G2**: round trip without assuming concrete type. |
| `0x00080f68` `CameraController_UpdateTriggers` | Iterates triggers in storage order and invokes virtual slot `+0x20` (Update) on each. | **missing game API** | **G2**: zero/one/multiple triggers and exact order through real frames. |
| `0x00078d40`, `0x00078f04`, `0x00078f34`, `0x00079238`, `0x00079260` | Collision setter, follow distance/right, bound and input-cancelled queries. | **implemented** | **E** (`test_game_camera.cpp`). |

## Menu, GameUI, license, and enemy helpers

| Native address / owner | Native evidence and portable crosswalk | Disposition | Test |
|---|---|---|---|
| `0x0004e6b8` `PlayDlg_SetNewGameOptions` | Copies two consecutive 32-bit words from the argument to dialog fields `+0x248` and `+0x24c`. It has no native caller and no getter. | **missing game API**, semantics unresolved beyond exact storage | **G3**: recover layout first, then exact two-word assignment; do not claim unlock behavior without a consumer. |
| `0x0006e120` `GameUI_GetNative80` | Returns the 32-bit field at GameUI `+0x80`; no direct caller and no semantic field evidence in this pass. | **reserved/unresolved** | **G3**: enumerate field readers/writers before deciding implementation or exclusion. |
| `0x0000ea8c` `MyClientSession_SendVerificationRequest` | Writes type 2 and operation BE16, MD5(text), and MD5(0x100-byte identity), then encrypts. `License::SendVerification` implements the same protocol. | **implemented**, native MyClientSession transport object is **platform-specific** | **E** (`test_game_license.cpp` protocol fixtures). |
| `0x0003fc04` `DarkKen_CompareRegionDistance` | Uncalled alternate comparator reads enemy `+0x704` and returns +1 when left > right, -1 when left < right, 0 equal. The active comparator at `0x0001fcd0` implements the opposite ordering on direct values. | **missing game API**, not a duplicate | **G4**: descending/equality and contrast with active ascending behavior. |
| `0x00027138` `Sword_IsAnimationFinished` | Returns native completion flag == 1. | **implemented** | **E**. |
| `0x0003cecc` `Player_HasAnyEnemy` | Portable checks enemy bucket count > 0. | **implemented** | **E**. |
| `0x0003f7fc` `Pushable_GetPushSound` | Returns the push-sound pointer. | **implemented** | **E**. |
| `0x00075d88`, `0x0008ab8c`, `0x00093758`, `0x00078064` | Door/Act/Talk/Camera trigger prompt/enabled/outside accessors. | **implemented** | **E**. |
| `0x0001edd4` `GameItemRuntime_InitEmpty` | Empty body. | **original empty** | **X**. |

## Reserved, duplicate, empty, and platform exclusions

| Native bodies | Disposition | Test |
|---|---|---|
| `0x0004ac80`, `0x0004b4a4`, `0x0004d2a8`, `0x0004dfa4`, `0x0004e6f8`, `0x0004e77c` | Simple writes to uncalled Option/Load/About/Help/Play dialog fields. **Reserved/unresolved**: no consumer-backed meaning or useful behavior was established. Session 34A may revisit them only with new field evidence. | **X / G3** if new evidence appears. |
| `0x00049e5c`, `0x0004e7a8`, `0x0004e7c8`, `0x00050700` | Empty menu callbacks/hooks: **original empty**. | **X**. |
| Game constructors/destructors listed in `soft3d_sdk_inventory.md` | **duplicate compiled body**; portable ownership already exists. No new game object is implied by an alternate constructor alone. | **X / E**. |
| `0x0000d5f8` and other license/session/socket/phone methods not listed above | **platform-specific** transport or **duplicate compiled body**. Protocol outcomes belong to `License`; Symbian session objects are not recreated as game APIs. | **X / E**. |
| `0x0003fc04` active-comparator confusion | The alternate is deliberately retained as a separate candidate. The active `0x0001fcd0` is **implemented** and must not be replaced by the alternate. | **G4 / E**. |
| Remaining game zero-direct-reference methods not explicitly listed as missing in this document | **implemented**, **duplicate compiled body**, **original empty**, or **intentionally omitted reserved behavior**, as crosschecked against `src/game`, `src/soft3d/MenuDialogs.*`, and existing stage ledgers. | **X / E**. |

## Session 34A handoff

1. Recover ControlMap's missing query/event/gate API and add generic Input any-key scans. Resolve the native ActionUp primary-only edge with directed and real QTE tests.
2. Recover Player/CameraController wrappers and accessors, especially trigger update ordering and saved-state lifecycle.
3. Resolve `GameUI_GetNative80` and the two PlayDlg option fields from all native readers/writers before implementing them. Leave reserved setters out if no meaning emerges.
4. Recover the alternate DarkKen comparator as a distinct helper only if its native ownership can be represented without confusing it with the active comparator.
5. Run focused input/player/camera/menu/enemy suites, real engine-frame tests, and the complete serial validation matrix. Update this ledger with matches/corrections and do not move game policy into reusable Soft3D APIs.

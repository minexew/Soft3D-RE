# Symbian3rd player, sword, and 20 player states

## Scope and evidence

This document records the stage-4 recovery of `Player`, `Sword`, and `PlayerStateSet`. The authority is the ARMv5 Symbian 3rd binary, not the leaked early script sources. The loaded Ghidra memory block begins at `0x8000`; all addresses below are loaded addresses and already include that bias. Do not add `0x8000` a second time.

The portable implementation is in `Reversed_Game/src/game/Player.*`, `PlayerStates.cpp`, and `Sword.*`. It uses generic `Creature`, `StateMachine`, `CameraSpot`, `Object`, `GunFire`, physics, render queues, and resource managers. Concrete enemy AI, concrete `Pushable`, and final HUD/QTE drawing remain stages 5, 8, and 9; narrow `CombatTarget` and `PushableTarget` interfaces carry the recovered calls without inventing those implementations.

## Native object layout

`Player_Ctor` is `0x0003867c` (alternate constructor `0x00038234`). Its arguments are resources, renderer, BSP, scene, audio, and state set. The native player is **0xf44 bytes**:

```text
0x0000  Creature                         0x6f0 bytes
0x06f0  StateMachine<Player>             0x14 bytes
0x0704  CameraSpot                       0x08 bytes
0x070c  player-specific fields
0x0aa4  embedded GunFire                 0x44 bytes
0x0f44  end
```

The player creates a **0x27c-byte** `Sword` at `0x00027420`. `Sword` derives from `GameUnit` and has a secondary `Renderable` base at `0x54`, unit type 7, five model slots and five shadow slots at `0xa0`/`0xb4`, current object pointers at `0xc8`/`0xcc`, signed 16-bit type at `0xd0`, current animation at `0xd4`, completion at `0xd8`, four-entry morph history at `0xdc`/`0x10c`, and 20 animation pointers per weapon type at `0x13c`.

Important player fields are:

| Offset | Recovered meaning |
|---:|---|
| `0x6b4` | Inherited navigation/movement speed; adult `0xcccc`, young `0x8000`. |
| `0x6c4` | Inherited navigation-enabled flag. |
| `0x6d0` | Active script navigation flag; suppresses direct movement input. |
| `0x70c`, `0x710` | Item-use and secondary statistic counters. |
| `0x714`, `0x718` | Play time and death count. |
| `0x724`, `0x728` | Sixteen-frame counter and heal-to-full request. |
| `0x72c` | Young-Ken model flag. |
| `0x730`, `0x734`, `0x738` | HP, maximum HP, and initial HP/ten. These also alias unarmed damage phases 0–2. |
| `0x790`, `0x794` | Gun experience and level; level-up thresholds at `0x798`/`0x79c` are 50/100. |
| `0x7a0`–`0x7a8` | Current action, animation direction, and loop flag. |
| `0x7ac`, `0x7b0`, `0x7b4` | Control map, scene, and sword. |
| `0x7b8`–`0x7c4` | Hurt 1/2/3/4 requests. |
| `0x7c8`, `0x7cc` | Animated-death selector and death state: 0 alive, 1 dying, 2 complete. |
| `0x7d0`–`0x7d8` | Attacker, knockback speed, and initial knockback velocity. |
| `0x7e4`, `0x7e8` | Shared state timer and counter. |
| `0x7ec` | Automatically selected melee target. |
| `0x7f8`, `0x7fc`, `0x800` | Attack phase, gun-attack flag, and attack-input disable word. |
| `0x804`, `0x808`–`0x820` | Target range and per-weapon attack blend values. |
| `0x824` | Eighty animation pointers: 20 actions for each of five player/weapon types. |
| `0x964`–`0x96c` | Default stand/run/hurt-2 resources used when restoring adult mode. |
| `0x970`, `0x97c` | Cached camera forward and right vectors. |
| `0x988`–`0x9bc` | Push flag, original/combined bounds, and push target. |
| `0x9c0`–`0x9d4` | QTE delay, duration, required hits, current hits, key, and result. |
| `0x9dc`, `0x9e0`, `0x9e4` | Presentation visibility, FPS, and hidden flags. |
| `0x9e8`–`0xa0c` | Hide mask, return position, camera direction, return-to-FPS flag, and angular limits. |
| `0xa10` | External velocity added after state movement. |
| `0xaa0` | Last used item returned by script. |
| `0xae8`, `0xb0c` | Nine current and nine day-start inventory slots. |
| `0xb30` | Day-start health-pack count. |
| `0xb34`, `0xd34` | 128 current and 128 day-start note slots. |
| `0xf34`, `0xf38` | Selected note and inventory slots. |
| `0xf3c`, `0xf40` | Primary and secondary gun targets. |

The Ghidra `Player` and `Sword` structures contain these fields and explicit padding. The attack arrays overlap other fields in native code; the documentation below gives the actual consumers rather than pretending the object has a modern separated layout.

## State registration and common transitions

`PlayerStateSet` is 0x4f4 bytes: a vptr, 256 registration pointers, and 20 embedded 12-byte states. `PlayerStateSet_InitializeStates` (`0x00032c50`) registers:

| ID | Class | Enter | Exit | Update | Register |
|---:|---|---:|---:|---:|---:|
| 0 | `CStand` | `0x32dfc` | `0x32e30` | `0x32e50` | shared `State_Player_Register` `0x36f14` |
| 1 | `CRun` | `0x330c8` | `0x33120` | `0x3315c` | `0x36ec4` |
| 2 | `CAttackA1` | `0x33660` | `0x336f4` | `0x33728` | `0x36e74` |
| 3 | `CAttackA2` | `0x33868` | `0x33990` | `0x339fc` | `0x36e24` |
| 4 | `CAttackA3` | `0x33bd0` | `0x33c4c` | `0x33ca8` | `0x36dd4` |
| 5 | `CAttackB1` | `0x33de8` | `0x33e80` | `0x33eb4` | `0x36d84` |
| 6 | `CAttackB2` | `0x33ff4` | `0x340fc` | `0x34168` | `0x36d34` |
| 7 | `CAttackB3` | `0x3433c` | `0x343b8` | `0x34414` | `0x36ce4` |
| 8 | `CAttackC1` | `0x34554` | `0x34620` | `0x34660` | `0x36c94` |
| 9 | `CAttackC2` | `0x347f4` | `0x348a8` | `0x348fc` | `0x36c44` |
| 10 | `CAttackC3` | `0x34abc` | `0x34b78` | `0x34be0` | `0x36bf4` |
| 11 | `CHurt` | `0x34d20` | `0x34de8` | `0x34e30` | `0x36ba4` |
| 12 | `CHurt2` | `0x34f70` | `0x34ffc` | `0x35064` | `0x36b54` |
| 13 | `CHurt3` | `0x350c8` | `0x351f8` | `0x3524c` | `0x36b04` |
| 14 | `CHurt4` | `0x35380` | `0x35408` | `0x35470` | `0x36ab4` |
| 15 | `CDie` | `0x355ac` | `0x3566c` | `0x356d8` | `0x36a64` |
| 16 | `CPush` | `0x357c8` | `0x35e14` | `0x35e88` | `0x36a14` |
| 17 | `CQTE` | `0x36090` | `0x36110` | `0x36158` | `0x369c4` |
| 18 | `CHide` | `0x3647c` | `0x3659c` | `0x36670` | `0x36974` |
| 19 | `CFPS` | `0x36734` | `0x3684c` | `0x368c4` | `0x36924` |

Every state has separate portable enter/exit/update/register bodies. Registration only stores the borrowed pointer and sets enabled. The generic machine preserves child-first enter/transition order and outer-first exit order.

The usual interrupt priority is death, hurt 1, hurt 2, hurt 3, hurt 4. `CHurt2` and `CHurt3` wait for animation completion without accepting ordinary attack/run interrupts. `CHurt4` has its own 24-frame countdown and omits a redundant self-transition. `CDie` never leaves through its update callback.

## Movement and ordinary states

`Player_Update` (`0x0003b4a4`) handles healing and play time first. The play-time word advances every 16 player updates. Navigation-only mode zeros velocity and skips normal state/physics updates. Otherwise it polls the state machine, adds external velocity, runs `PhysicUnit_Update`, synchronizes the sword's position/right/up/forward, updates hit history, and refreshes gun targets.

`Player_UpdateMovement` (`0x00039860`) uses camera forward/right and the real control map:

- Actions 1/2 move forward/backward; actions 3/4 strafe in non-FPS modes.
- In first-person mode, actions 3/4 stop movement and leave camera look to the controller.
- Forward is flattened in Y. If camera alignment with world up exceeds `0xe666`, the original cross/blend calculation uses `(alignment - 0xe666) * 0xa0000`.
- If both X and Z are `<= 0x4000` using signed comparisons, both are shifted left eight bits before normalization.
- The vector is scaled by navigation speed; X/Z below absolute `0x28f` become zero.

`CStand` selects action 0, looped. `CRun` selects action 1, sets physics step-offset bit `0x10`, and loops sound 21 (`c_ken_run`); exit clears the bit and stops the sound. Either state enters run on horizontal velocity or nonzero yaw velocity. With a weapon and attacks enabled, action 5 click selects `A1`, except type 4 selects `C1`; action 5 press selects `B1`, except type 3 selects `C1`.

`CRun` also queries thing bucket 2. Exactly one hit of type `0x1d`, with push enabled, X or Z displacement below `0x20000`, and normalized forward dot product greater than `0x9999`, selects `CPush` and stores the target.

## Attacks and damage

Actions map directly into the per-type animation table:

| State | Action | Next state and special behavior |
|---|---:|---|
| `A1` | 2 | Select/face target, non-looping; finished → `A2`. |
| `A2` | 3 | Phase 1, hit active/history -1, timer 1, counter 0; third finished poll → `A3`. |
| `A3` | 4 | Finished → stand. |
| `B1` | 5 | Same target selection; finished → `B2`. |
| `B2` | 6 | Same hit-window timing as `A2`; third finished poll → `B3`. |
| `B3` | 7 | Finished → stand. |
| `C1` | 5 | Type 3 exits to stand when action 5 is up on both bindings; otherwise finished → `C2`. |
| `C2` | 6 | Phase 2 on entry, gunfire one frame, sound 24; third finished poll → `C3`. |
| `C3` | 7 | Finished → stand. |

`A2`/`B2` disable simple movement and enable keep-direction/step-offset. A positive counter disables sword hits and clears the attack phase. The first completed update with timer 1 clears timer and counter; subsequent completed updates increment the counter. This is intentionally three update calls after completion, not a direct transition.

Type-3 level-2 attacks set player and weapon animation speed to `0x1c2`. Sounds are weapon 1 → 22, weapons 2/3 in the shared second-attack path → 23, gun `C2` → 24, hurt → 25, death → 26. The original decompiler sometimes shows a sound's `this` as the weapon index; disassembly confirms that the player is the receiver.

Ordinary melee damage is read from `0x730 + 4 * (type * 2 + attackPhase)`:

| Type | Phase 0 | Phase 1 | Phase 2 |
|---|---:|---:|---:|
| 0 unarmed | current HP | maximum HP | initial HP/10 |
| 1 knife | 10 | 20 | 20 |
| 2 sword | 40 | 20 | 50 |
| 3 gun fallback | 27 | 27 | 15 |
| 4 special | 15 | 40 | 40 |

The unarmed alias is original behavior, not a documentation mistake. Melee deliberately invokes the target's **point-query** virtual with `sword.hitBounds.minimum`; it is not replaced by a more plausible bounds query.

Target selection (`Player_SelectAttackTarget`, `0x0003ba0c`) uses range `0xf0000`, or `0x270f0000` for a leveled type-3 weapon, rejects BSP occlusion and targets outside the forward cone, and faces the nearest accepted enemy. Enemy states 16/17 are excluded through the combat interface.

Gun phase 2 maintains two targets. Damage uses target distance against thresholds 0, `0x280000`, and `0x3c0000`, with a fourth fallback tier:

| Level | Tier 0 | Tier 1 | Tier 2 | Fallback |
|---|---:|---:|---:|---:|
| 0 | 27 | 27 | 15 | 15 |
| 1 | 40 | 40 | 20 | 20 |
| 2 | 60 | 60 | 30 | 30 |

Special type 4 phase 2 instead returns 125 after its range and BSP checks. The original virtual selection and unusual radius arithmetic are preserved in the portable combat interface rather than replaced by generic hitscan behavior.

`Player_AddGunExperience` (`0x0003dd1c`) skips progression when the extra-weapon unlock disables it. At 50 and 100 it plays sound 142 and presents tutorial IDs 41 and 42 for 64 frames. Two upgrades can be observed through the same entry point; the second check follows the first immediately.

## Sword and gunfire

`Sword_SetAnimation` (`0x000288c0`) indexes `(type * 16 + action)` in the 80-entry table, applies direction/loop/time to model and shadow, copies primary bounds, and clears completion. `Sword_Update` updates the primary transform, copies its matrix to the shadow, and rebuilds hit bounds. `Sword_PostUpdate` advances both animations and records primary completion.

Hit reconstruction (`0x000289c8`) samples morph targets 0 and 1 in model space and transforms them by the model matrix. On a changed first point it computes normalized `(first - previousFirst) * 10`, expands current bounds by both points and the indexed history pair, advances the signed history index, clamps it to 3, shifts history, and stores the new pair. Inactive hits decrement the index and clamp at -1.

Player submission suppresses the model in FPS/hide, suppresses sword animation/submission during push and hurt 2/4, copies object intensity and shadow transform, and submits gunfire in queue 5 when frames remain. The muzzle position is sword morph target 0. `C2` sets one remaining frame.

The exact animation-table consumers are preserved. For example, sword type 1 actions 0–9 use fixed names 44–53, type 2 uses 54–63, type 3 uses 68–77, and type 4 uses 79/80, 81–84, and 86 in its populated slots. Player type 1 uses stand/run/hurt/death/push from the base set and knife attacks 5–10; type 2 uses sword resources 11–20; type 3 uses gun resources 25–35. The separate special loader installs base action 12 from name 34, type-3 action 12 from name 36 or type-4 action 12 from name 40, and then selects the corresponding sword stand resource for the special consumer.

## Hurt, death, push, QTE, hide, and FPS

- `CHurt`: faces the attacker if non-null, action 8, sound 25, clears its request on entry, and returns to stand when finished.
- `CHurt2`: action 11, sound 25, restore-X/Y/Z flags; exit clears the request. Its update only waits for completion.
- `CHurt3`: action 12, initial velocity `-forward * knockbackSpeed`. For the first four updates it adds `forward * 0x4000`; negative dot with initial knockback stops movement early. Completion returns to stand.
- `CHurt4`: looping action 12, 24-frame timer, movement still accepted. Death and hurt 1/2/3 interrupts retain their priority.
- `CDie`: with animation enabled chooses action 11 when hurt 2 is pending and action 9 otherwise, plays sound 26. Completion sets death state 2 and clears the presentation shader. The hurt-2 path instead waits for exact animation time `0x1518`, then pauses all player objects and completes. Nonanimated death completes immediately. Scene post-update turns completed death into Ink/game-over stage 2.
- `CPush`: action 10, keep-direction, computes a side position and expands player bounds to include the target, plays the target push sound, attaches the pusher, and disables target collision. Exit detaches and re-enables collision but **does not restore the enlarged player bounds**. While completed, no movement/turning exits to stand; movement away from forward exits to run; aligned movement restores the push animation and uses forward * speed * `0x4000`.
- `CQTE`: clears slide and sets restore-X/Y/Z. Entry copies delay/duration/required hits and sets result -1. After delay, key is `Random()%4+1`; single-hit mode uses UI mode 0 and mash mode 2. A wrong click fails single-hit immediately; the correct click succeeds it. Mash mode compares hits with the requirement at timeout. Result remains 0/1 and does not automatically select stand.
- `CHide`: freezes the camera, enters limited first-person mode, activates the optional mask, hides the model, and clears slide while setting restore-X/Y/Z. Action 5 or 7 click clears input and returns to stand or FPS. Exit restores position and visibility, unfreezes the camera, and disables the mask.
- `CFPS`: enters locked first-person mode with limits `0x9999`/`0xcccc`, hides the player, and changes local Y bounds to `[-0x10000, +0x10000]`. Movement speed is multiplied by `0x4ccc`. Exit restores the model's original primary bounds and visibility.

## Inventory and saved state

Nine ordinary slots hold IDs below 1001; 128 note slots hold larger IDs. Item 1 is special: it occupies slot zero, increments the manager's health-pack count, and recursively reinserts displaced slot-zero content. Other items use the first empty slot. Pickup sound is index 141. Adding items updates model/document/photo unlocks through the existing configuration owner.

`Player_UseItem` (`0x0003c76c`) uses a health pack only below max HP, adds `maxHP >> 1` with clamping, decrements the pack count, and removes/compacts the slot when no packs remain. Other item-use groups return result 2 and set `usedItem`; ordinary health-pack success returns 1. Directional combination consumes both inputs and adds the result. Compaction preserves slot zero and reinserts occupied later slots without sound/selection side effects.

Day snapshots copy current and initial inventory, both note arrays, health-pack count, and the two statistic counters. Game-over restart restores these before reloading the day.

`Player_SaveState` (`0x0003d4f4`) writes `VisUnit` state followed by:

```text
navigationEnabled, health, maxHealth, hidden, hideMask,
hideReturnPosition (three words), weapon, gunExperience, gunLevel, fpsActive,
inventory[9], inventoryDayStart[9], healthPackCount,
notes[128], notesDayStart[128]
```

`Player_LoadState` (`0x0003d278`) resets first, skips the consumed four-byte type word, restores those fields in the same order, reapplies weapon resources, and resets to stand. This is preserved for stage 13 rather than replacing it with the header-only save path.

## `GameSI` player commands

All handlers use the standard `(arguments, argumentCount, output)` convention. IDs and loaded ARM addresses:

| ID | Handler | Behavior |
|---:|---|---|
| 117 | `0x679a4` `GameSI_SetPlayerDirection` | Three fixed direction components. |
| 118 | `0x67a60` `GameSI_SetPlayerClipping` | Bool true selects normal render mode; false selects secondary streams. |
| 119 | `0x67ab8` `GameSI_SetPlayerAnimation` | Loads an animation from `common`, direction 1, scripted loop flag. |
| 120 | `0x67b60` `GameSI_IsPlayerAnimationFinished` | Returns the visual completion flag. |
| 121 | `0x67ba4` `GameSI_TeleportPlayer` | Sets XYZ, snaps to ground, translates collision bounds. |
| 122 | `0x67cb0` `GameSI_NavigatePlayer` | Three fixed XYZ; returns navigation arrival. |
| 123 | `0x67da8` `GameSI_PlayerHearSound` | XYZ plus fixed range; position-based hearing ID -1. |
| 124 | `0x67e94` `GameSI_PlayerPointInside` | Six fixed values form bounds, but original virtual selection tests only the minimum point. |
| 125 | `0x67fa8` `GameSI_PlayerPointOutside` | Same six arguments and point test, negated. |
| 126 | `0x680c0` `GameSI_PlayerHearEnemy` | Enemy ID and range; missing unit returns false. |
| 127 | `0x68174` `GameSI_PlayerHearThing` | Thing ID and range; uses the ID-based hearing path. |
| 128 | `0x68228` `GameSI_PlayerAttack` | Zero directly enters `A1`; nonzero directly enters `C1`. |
| 129 | `0x68280` `GameSI_IsPlayerDead` | Returns deathState != 0. |
| 130 | `0x682d0` `GameSI_GetPlayerHealth` | Returns HP. |
| 131 | `0x6830c` `GameSI_SetPlayerHealth` | Negative or above max selects max; zero requests nonanimated death. |
| 132 | `0x683c4` `GameSI_AddPlayerHealth` | Adds signed-short delta and clamps to 0..max; zero requests nonanimated death. |
| 133 | `0x684ac` `GameSI_DamagePlayer` | Applies signed-short damage through the normal hurt/death method. |
| 134 | `0x684f8` `GameSI_SetPlayerWeapon` | Selects weapon type and reapplies current action/transforms. |
| 135 | `0x68540` `GameSI_GetPlayerWeapon` | Returns sword type. |
| 136 | `0x68584` `GameSI_IsPlayerAttacking` | Compares the current state against each of the nine attack states. |
| 137 | `0x687d0` `GameSI_IsPlayerHidden` | Returns hidden || FPS. |
| 138 | `0x68814` `GameSI_StartPlayerQTE` | Three signed shorts: delay, duration, required hits. |
| 139 | `0x68894` `GameSI_GetPlayerQTEResult` | Returns -1, 0, or 1. |
| 140 | `0x688d0` `GameSI_GetPlayerUsedItem` | Returns last used item. |
| 141 | `0x6890c` `GameSI_ClearPlayerUsedItem` | Sets last used item to zero. |
| 142 | `0x68944` `GameSI_EnterPlayerFPS` | Directly enters `CFPS` when alive. |
| 143 | `0x68978` `GameSI_HasAttackingEnemy` | Checks enemy bucket 0 for an attacking state. |
| 144 | `0x689bc` `GameSI_SetPlayerSpeed` | Writes fixed navigation/movement speed. |
| 145 | `0x68a10` `GameSI_IsPlayerPushing` | Returns whether pushing and target ID matches. |
| 146 | `0x68a68` `GameSI_SetPlayerYoung` | Bool selects young or adult resources and stand state. |
| 147 | `0x68af4` `GameSI_IsPlayerYoung` | Returns young flag. |
| 148 | `0x68b44` `GameSI_ExitPlayerHide` | Restores hidden state and enters stand/FPS. |
| 255 | `0x6d988` `GameSI_PlayerAddItem` | Adds signed-short ID with pickup sound. |
| 256 | `0x6d9d4` `GameSI_PlayerHasItem` | Returns ordinary/note membership. |
| 257 | `0x6da2c` `GameSI_PlayerRemoveItem` | Removes the signed-short ID. |

## Generic corrections and integration

- `Input::IsAnyKeyClick` now scans all 23 keys for `CLICK`, matching `0x000ef2a0`; the former member-only implementation could never report QTE input.
- `Object` exposes model-space morph sampling transformed by the model matrix, plus named texture override loading used by adult outfit variants.
- `StateMachine` exposes direct enter-chain and current-state comparison needed by player commands.
- `CameraController` exposes its recovered right vector and direct first-person setup, including lock and angular limits.
- `Scene` owns concrete `Player`, loads day weapon resources, snapshots inventory, enters FPS at map finalization, and connects completed death to the existing Ink/restart flow. QTE/hide/tutorial state is held for the stage-8 UI consumer rather than rendered by a duplicate UI system.

## Verification

`Reversed_Game/tests/test_game_player.cpp` covers registration and every virtual enter/update/exit path, all attack chains, hit windows and melee/gun damage, hurt/death variants, QTE success/failure/timeout, hide/FPS, push attach/detach, inventory/use/combine/snapshot/save round trips, every command above, and real `GameEngine::RunFrame` movement/combat input on map 2008.

Exact default dummy-presented FNV-1a hashes over 320×240 XRGB pixels are:

```text
stand  0x9da9ea25
run    0x6d5c862d
attack 0x4ac44b45
hurt   0xff7f3005
QTE    0x3f72c8dd
```

All **32 default** tests and all **25 SDL-independent core**, logging, host-math, WAR, and ASan/UBSan tests pass. The real `day1/ep0` application path still reaches the explicit later-stage command 221; no later-stage command was made a silent no-op.

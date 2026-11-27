# Symbian 3rd game camera and scene presentation

## Scope and evidence

This document records the game-specific camera controller and the stage-3 scene consumers recovered from the ARMv5 Symbian binary. It supplements `symbian3rd_game.md`, `spl_format.md`, `sbp_format.md`, `symbian3rd_audio.md`, and `symbian3rd_shaders.md`.

The mode names below are descriptive labels, not RTTI names. ARM addresses, field offsets, argument order, frame counts, fixed-point constants, and state transitions are the behavioral authority. The portable implementation is in `Reversed_Game/src/game/CameraController.*`, `Triggers.*`, `Scene.*`, `GameSI.*`, and `MyGameUnitMgr.*`.

## Native layouts

`CameraController_Ctor` (`0x0007984c`, duplicate `0x00079384`) constructs a **0x208-byte** object. It is not the menu-only `soft3d::CameraPathController`, and it is not itself a `CameraSpot`. It owns a 32-entry trigger-pointer list and consumes an engine `Camera`, `CameraSpot`, `Bsp`, `ControlMap`, renderer, and manager random state.

Ghidra contains the saved `GameCameraController` structure with 87 fields. Important offsets are:

| Offset | Purpose |
|---:|---|
| `0x00`, `0x04` | Trigger-resume and freeze words. |
| `0x08` | Embedded trigger-pointer list. |
| `0x18` | Path loop byte. |
| `0x1c`, `0x28`, `0x34` | Current position, target, and up vector. |
| `0x40`, `0x4c`, `0x58` | Desired position, right, and forward vectors. |
| `0x64`–`0xa0` | Previous position/target/binding/desired/direction state. |
| `0xac`, `0xb8` | Desired target and direction. |
| `0xc4`, `0xc8` | Follow-height delta and nearest-path time. |
| `0xcc` | Path direction-selection byte, distinct from the loop byte. |
| `0xd0`–`0xf0` | Position/target velocity and acceleration vectors. |
| `0xf4` | Follow distance or time parameter. |
| `0xf8`, `0xfc` | Active update callback and adjustment. |
| `0x100`, `0x104` | Previous mode and bound `CameraSpot *`. |
| `0x108`–`0x10a` | Input-cancel, instant, and saved-state bytes. |
| `0x10c`–`0x150` | Saved camera state used by triggers. |
| `0x154`, `0x158` | Shake frames and amplitude. |
| `0x15c` | Manager/random context. |
| `0x160`–`0x18f` | Orbit timing, angle, radius, target, and position. |
| `0x190`, `0x194`, `0x1a0` | Previous mode, saved direction, and saved follow distance. Ghidra represents these with same-size `CameraModeHistoryPrefix`, saved-direction Z, and fixed-point follow-distance fields. |
| `0x1a4`–`0x1b4` | BSP, screen blitter, controls, camera, and manager pointers. |
| `0x1b8`–`0x1bc` | Binding flag, direct-follow flag, and direct-follow speed. |
| `0x1c0`–`0x1e4` | First-person/free-look axes, lock, reference, and angular limits. |
| `0x1e8`–`0x1f4` | SPL collection, path time, path speed, and target mode. |
| `0x1f8`, `0x1f9` | Collision-enabled and path-completed bytes. |
| `0x1fc` | Fixed target used by path target mode 3. |

`CameraTrigger` is 0x90 bytes: 0x54-byte `GameUnit`, controller/player pointers, mode, position, direction, time, owned SPL collection, snap byte, path speed, active/outside bytes, and target mode. Ghidra names this layout `CameraTriggerLayout`. `LightTrigger` is 0x68 bytes and has a fixed-direction word/vector plus player pointer (`LightTriggerLayout`).

Each randomized environment channel is 0x4c bytes (`EnvironmentAudioChannel`):

| Offset | Purpose |
|---:|---|
| `0x00` | Resource count, zero through four. |
| `0x08` | Four `AudioData *` slots. |
| `0x28`, `0x30` | Base delay and random-delay range in scene updates. |
| `0x38`, `0x40` | Elapsed count and next delay, with `-1` selecting a new delay. |
| `0x48` | Active voice, zero when idle. |

## Eight camera modes

`CameraController_SetMode` (`0x0007b930`) dispatches these modes. The public script command takes position and target vectors and computes `direction = target - position`; it normalizes that delta except for path target mode 2.

| Mode | Setup callback | Update callback | Behavior |
|---:|---|---|---|
| 1 | `CameraController_SetFixedMode` `0x0007bb24` | `CameraController_UpdateFixed` `0x0007db20` | Fixed position; relative direction or bound target. |
| 2 | `CameraController_SetFrontFollowMode` `0x0007bdbc` | `CameraController_UpdateFrontFollow` `0x0007dcac` | Position behind the bound spot along direction, with projection-aware smoothing and clipping. |
| 3 | `CameraController_SetOrbitFollowMode` `0x0007c06c` | `CameraController_UpdateOrbitFollow` `0x0007dd9c` | Third-person follow using follow distance, vertical delta, horizontal radius, and collision recovery. |
| 4 | `CameraController_SetOffsetFollowMode` `0x0007c34c` | `CameraController_UpdateOffsetFollow` `0x0007e040` | Fixed offset position with bound target. |
| 5 | `CameraController_StartPath` `0x0007cc44` | `CameraController_UpdatePath` `0x0007ec08` | SPL playback or zero-speed closest-point projection. |
| 6 | `CameraController_SetFirstPersonMode` `0x0007c5c0` | `CameraController_UpdateFirstPerson` `0x0007e2f0` | Bound first-person view, input rotation, spot-axis updates, and pitch clamp. |
| 7 | `CameraController_SetSideFollowMode` `0x0007c904` | `CameraController_UpdateSideFollow` `0x0007e144` | Side-follow using the spot right vector and follow distance. |
| 8 | `CameraController_SetFreeLookMode` `0x0007cb84` | `CameraController_UpdateFreeLook` `0x0007e6c8` | Free-look rotation with optional lock and angular limits. |

`CameraController_Update` (`0x0007a1ac`) refreshes the bound position before invoking the active callback. Modes 2, 3, 4, and 7, plus fixed/path modes with a bound target, use bound position + `(0,2,0)` as the look target. `previousDesiredTarget` preserves the desired camera pose for fixed/orbit updates, while `previousDirection` receives the current desired look target; they are not interchangeable. `previousDesiredPosition` receives the bound position at the end of an update. `CameraController_Reset` (`0x0007b608`) clears triggers and restores the default mode-3 camera from the bound spot, normalized `{0,-0x6666,-0x10000}` direction, and `0x1a8000` follow distance. Its horizontal offset uses the negated spot-right vector and its height delta is `previousDesiredTarget.y - previousDesiredPosition.y`.

The original snap polarity is important: **snap zero selects the immediate pose** (also when the separate instant byte is set), while nonzero snap interpolates. Normal interpolation uses `0x1999`, approximately 0.1. Direct-follow mode can instead cap position motion by its configured fixed speed and scale target movement by `speed / distance`. `CameraController_MoveCamera` (`0x00080978`) performs independent component-wise position/target steps and returns true only after all position components arrive.

`CameraController_Bind` (`0x00078fa4`) changes the spot and binding flag. Player and enemy/dummy-spot binding are separate `GameSI` commands. `CameraController_SetFrozen` (`0x00079d14`) records freeze state; unfreezing with a nonempty trigger stack resumes the last trigger. `SetInstant` controls the two-frame warmup used by scene startup.

### Session-34A orbit and accessor corrections

The uncalled player orbit wrappers at `0x0003b920`/`0x0003b97c` are now public. Their controller target required several ARM corrections. For `CameraController_ConfigureOrbit` (`0x00079e38`), non-null **target** selects current target and orbitTarget; non-null **center** sets `position=target-center` and `orbitVertical=center.y`. The initial angle is `Vec3AngleXZ(Normalize(flatTarget-flatPosition))`. Configuration always enables rotation; `vertical` sets the repeat flag instead. The previous portable code swapped center/target roles, used a partial arc tangent, and incorrectly mapped vertical to rotation enable. `CameraController_SetOrbitTarget` (`0x00079178`) independently implements null disable and non-null copy-to-both-targets behavior.

The update branch also retains a native edge: with `orbitFrames <= 8`, the terminal interpolation branch has priority and the rotation phase is never reached. Longer sequences reach rotation at `orbitFrames == orbitEndFrames`. `test_game_camera.cpp` uses 16-frame orbits to verify enabled/disabled rotation and vertical repetition, plus immediate center/target forwarding through the real Player wrapper.

The simple accessors at `0x00078c60`, `0x00078c88`, and `0x00078cc0` respectively return previous mode (`+0x190`), saved direction (`+0x194`), and saved follow distance (`+0x1a0`); earlier names describing previous-direction-X, follow target, and renderer were wrong. `GetUp`, `GetDirection`, target distance, saved-state query/clear, opaque unit-manager access, indexed trigger access, and `UpdateTriggers` (`0x00080f68`) complete the recovered public surface. Trigger updates capture count and use storage order. `Player_IsFacingPoint` and `Player_GetObjectBounds` are covered in `symbian3rd_player.md`; the latter subtracts rather than adds player position.

Shake consumes three random values per update, each mapped from `0,1,2` to `-1,0,1`, multiplied independently by amplitude and added to target XYZ. `CameraController_ClipCamera` (`0x0007fad4`) traces target↔desired position with ±1 bounds, handles opposing planes and a sideways recovery candidate, and applies a follow-distance vertical correction.

## SPL playback and target modes

For nonzero speed, `CameraController_StartPath` initializes the desired position from global spline time zero; only zero-speed paths perform closest-point projection. `CameraController_UpdatePath` then evaluates position at current time and advances time by:

```text
step = 1 / (16 * speed)
```

At speed 30 the exact fixed-point increment is 136; the 482nd update crosses 1.0. Position uses pre-increment time while derivative targeting uses advanced time. Completion sets the completed byte and clamps to 1.0 or wraps to zero according to the independent loop byte. Derivative scale is `0xa0000` (10.0).

Zero speed uses the closest projection of bound position + `(0,2,0)` onto the endpoint line. The ARM code computes projection/endpoint cases, perpendicular offset, a quadratic discriminant, clamped root times, and direction-specific root selection. It does not degenerate into simple timed playback.

Path target modes are:

| Target mode | Look target |
|---:|---|
| 0 | Position + SPL derivative × 10. |
| 1 | Bound spot + `(0,2,0)`. |
| 2 | Position + the supplied unnormalized direction. |
| 3 | Fixed target + `(0,2,0)`, retained separately. |

Endpoint distance selects the root-selection direction; this byte is not the loop flag. The original start also snaps desired position when `snap == 0` or instant is set.

## Map camera and light triggers

`MyGameUnitMgr_EntityFactory` (`0x00043ed8`) handles camera type 10, light type 11, type-102 six-word records, and lensflare type 202. Types 10, 11, and 102 return no renderable `BspEntity`, so null entity slots remain valid.

A type-10 record contains eight 32-byte entity-pair fields, each split on disk into two 16-byte key/value arrays and treated as one logical string:

```text
mode, position, direction, time, snap, SPL name, path speed, target mode
```

Position/direction use parenthesized comma-separated fixed-point text. Mode 5 allocates a 64-entry SPL collection and loads `.\common\<name>`. Bounds come from the entity brush's six signed 16-bit values shifted to 16.16 fixed point. A scan of the 152 shipped day maps finds **802 camera triggers**, **29 light triggers**, **124 type-102 records**, and **4 lensflare records**.

`CameraTrigger_Update` (`0x000782d4`) uses inclusive AABB overlap with player collision bounds. Entry applies the mode and pushes the trigger. If this is the first active trigger, the controller snapshots state **after applying the outer trigger's mode**, not before it. Nested exit resumes the previous trigger; outer exit restores the saved state. Non-top exits only remove their pointer. Frozen controllers ignore trigger updates.

A type-11 record contains:

```text
use-player-direction flag, position, fixed light direction
```

`LightTrigger_Update` (`0x00052d04`) applies fixed direction when the flag is zero, otherwise normalized `playerPosition - triggerPosition`, while the player overlaps. It preserves ambient intensity and does not restore the old direction on exit. `LightTrigger_SaveState`/`LoadState` (`0x00052fb0`/`0x00053000`) append the 32-bit direction-mode word and light-direction vector to `GameUnit` state; position and bounds already come from the base record. Type-102 brush values are stored as six 16.16 words at manager offsets `0x24ff4`–`0x25008`. Their higher-level meaning is not established by this consumer alone; they must not be renamed as proven fog planes or an AABB. The six floats in the SBP header also remain intentionally unnamed in `sbp_format.md`.

## Scene transitions and presentation

`Scene_BeginMapLoad` (`0x0002ca50`) sets the transition camera, renderer direction `Normalize({0xa666,-0x10000,0xcccc})`, ambient `0xb333`, fog off, global controls to native zero (enabled), and an eight-frame fade-in when no fade is active. Door index zero has special initial framing; other door indices start from the player position. A high `0x10000` index selects the GameUI special-door path, whose actual dialog rendering belongs to stage 8.

The 22 model/sound records from `Scene_InitializeDoorResources` (`0x0002b554`) are already tabulated in `ResourceNames.cpp` and `game_data_tables.md`. Standard transitions use these exact update numbers:

| Timer | Recovered action |
|---:|---|
| 8 | Load door model, **materials**, and animation entries 238+index; position model at target + forward×20; fade out over 4; stop sounds; choose door-specific camera; play open sound; enter stage 2. |
| 24 | Apply spawn position/direction when not restoring; clear special-door UI state; fade in over 4; enter stage 3. |
| 28 | Hide player and rebind the player spot. |
| 31 | Fade out over 8; enter stage 4; play close sound when positive. |
| 36 | Clear transition and restore-saved words and reset timer. |

Camera speed before stage 2 is `distance / 8`; after door creation it is `distance / 20`. Door model update occurs in stages 2–3 and post-update only before timer 29. Door submission is restricted to stages 2–3 before timer 29; stage 1 with a negative door index instead submits the player. Map-chunk loading and finalization continue in the same update, so `Player_ResetForMap` can make the player visible again after frame 28. Tests must not treat intermediate visibility as a standalone transition rule.

`Scene_FinishMapLoad` (`0x0002d018`) resets the player/camera, forces an immediate update, finalizes manager content, clears input, and starts script tasks. It calls `Player_EnterFPS` only when the post-reset `nFpsActive` flag is nonzero; ordinary map starts must not force first-person mode or hide the player. `Scene_LoadNextLevel` clears triggers and unfreezes before queuing script/map state.

Day titles load `riqi_<day>` and `<day>nd` from the active UI directory. On day 7 with event flag 699 clear, ARM appends `_a` to the formatted name, selecting **`7nd_a`** instead of `7nd`. These are the two shipped ending-specific textures; replacing the whole name with `_a` was incorrect. The portable dummy-presented hashes for these two frames are `0x84f3b875` and `0x506a0bad` respectively.

## Environment audio and listener selection

`Scene_AddEnvironmentSound` (`0x0002f3d0`) appends one resource to either channel, assigns base/random delay, forces `nextDelay = -1`, and clears voice. `Scene_ClearEnvironmentChannel` (`0x0002f590`) clears only resources/count/voice; it does not stop the active voice or reset timing fields. Full unload resets both channels completely and sets both next delays to -1.

When resources exist, scene update performs:

```text
if nextDelay == -1:
    nextDelay = baseDelay + (randomDelay ? Random() % (randomDelay << 7) >> 7 : 0)
    elapsed = 0
else:
    if voice != 0:
        if voice is not finished:
            skip this channel
        voice = 0
    ++elapsed
    if elapsed >= nextDelay:
        nextDelay = -1
        voice = PlaySound(resources[Random() % (resourceCount << 7) >> 7])
```

The RNG is `state = state * 0x343fd + 0x269ec3` with wrapping 32-bit arithmetic, returning `(state >> 16) & 0x7fff` (`Random_Next` `0x0009a29c`). `MyGameUnitMgr_ResetRandomSeed` (`0x00043e38`) seeds from the platform clock. Tests inject the seed for deterministic scheduling. Environment scheduling runs before BSP/player/unit/camera update and only when transition processing reaches world update.

Listener selection uses player position by default and controller position when enabled. Music commands operate on voice 0; environment/effect voices use 1–15. Game over stops sounds, plays `m_gameover`, unloads script, pauses the manager, selects Ink, and disables submit/prepare. Ink timer above 16, completion, or input after frame 8 disables pre-update and enters the restart/menu stage. Accept/number 7 restarts the current day from the snapshot; soft-right/number 9 returns to menu.

## Stage-3 `GameSI` commands

All callbacks below are implemented in the portable dispatch and named in Ghidra:

| IDs | Behavior |
|---|---|
| 51–52 | Fade-out/in with mode-0 completion polling. |
| 63–68 | Freeze, direct look-at, component move, eight-mode selection, path completion, player bind. |
| 70–72 | Enemy/dummy-spot bind, shake, direct-follow speed. |
| 84–85 | Play/stop music. |
| 86–87 | Add environment resource / clear channel. |
| 88–91 | Play sound, stop all, create positional instance, clear instances. |
| 98–100 | Lightstyle set/get and fog configuration/disable. |
| 103 | Complete fade and select Gray, MotionBlur+reset, Blind, Twist, or default. |
| 117, 121 | Basic player direction and position/ground/bounds synchronization used by the real startup script. |

`GameSI_SetCameraMode` (`0x000660b4`) takes 12 arguments and loads SPL for mode 5, appending `.spl` when missing. `GameSI_SetFog` (`0x00066fc8`) disables fog for nonpositive range; otherwise it packs RGB565, sets start/end, and enables fog. The real `day1/ep0` now proceeds beyond stage-2's ID 121 stop and reaches the still-later command **221** (`0x0006c08c`).

## Portable integration and verification

The reusable menu `CameraPathController` remains unchanged. The game controller and triggers are separate game classes. `SceneCameraSpot` adapts the current generic `Creature` until the concrete stage-4 `Player` replaces it; this is not a claim that player movement/states are complete. Map triggers are destroyed through native-equivalent `ClearBucket(..., true)` calls before `ResetBuckets`, preventing leaks and stale trigger back-pointers.

`tests/test_game_camera.cpp` covers every mode, snap/interpolation, binding/freeze, timed and projected SPL playback, completion/wrapping, deterministic shake, and nested restoration. `tests/test_game_scene.cpp` adds VM commands, real map 2008 trigger counts and type-102 words, fade polling, all shader selections, music/environment/listener behavior, exact door frame stages, ending-title hashes, and game-over restart.

In the stage-8 verification, the default, stdio-logging, host-math, and WAR suites each pass all 35 tests. The SDL-independent/core and ASan+UBSan suites each pass all 28 tests after restoring native bucket destruction. Earlier concurrent variant runs could collide in the shared `/tmp/soft3d_stage6_menu_tests` directory; isolated reruns pass, so this is a test-orchestration issue rather than a game-code defect.

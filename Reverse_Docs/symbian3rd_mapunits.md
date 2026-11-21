# Symbian3rd things, map units, traps, and stay boxes

## Scope and portable status

Stage 5 recovers the game-specific `Things` descendants and map-unit consumers. `Reversed_Game/src/game/MapUnits.*` implements `Breakable`, `Pushable`, `Pickable`, `BspUnit`, `ElectricSaw`, `Trap`, and `StayBox`; `MyGameUnitMgr` owns construction, lensflares, and update/submission integration. `GameSI` implements commands 219–254 from their individual ARM bodies.

The reusable `Things` base in `src/soft3d/Units.*` now implements the shared save/load extension used by all three descendants. The concrete interaction triggers are still stages 6–7, and StayBox enemy-pool operations use `StayBoxHost` until the concrete enemy implementation in stage 9; no generic enemy substitute is installed. Full scene reconstruction of these records remains stage 13.

## Native layouts

| Class | Native size | Recovered extension fields |
|---|---:|---|
| `Breakable` | `0x694` | `0x684` break-sound `AudioData*`; `0x688` player; `0x68c` required weapon; `0x690` signed-short event; `0x692` signed-short hit latch. |
| `Pushable` | `0x690` | `0x684` push-enabled byte; `0x688` push-sound `AudioData*`; `0x68c` player/pusher. |
| `Pickable` | `0x690` | `0x684` pickup-sound `AudioData*`; `0x688` item ID, initially -1; `0x68c` pointer to the originating gameitem record. |
| `BspUnit` | `0x68` | `0x54` entity state initialized to zero; `0x58` model `Object*`; `0x5c` map `Animation*`; `0x60` secondary vertex-light buffer. |
| `Trap` | `0x64` | `0x54` player; `0x58` fixed XYZ impulse. |
| `StayBox` | `0x6c` | `0x54` enemy; `0x58` enemy-outside flag, initially 1; `0x5c` spawned flag; `0x60` allowed-region list; `0x64` denied-region list; `0x68` spawn-record list. |

`Breakable`, `Pushable`, and `Pickable` use unit types `0x1c`, `0x1d`, and `0x1e`; `Trap` uses `0x1f`. `BspUnit` sets type 3, while `StayBox` sets `0x26` as a manager-internal record. `ElectricSaw` retains the `VisUnit` type and has no recovered extension behavior beyond its update method.

A `StayBoxRegion` is `0x1c` bytes: 24-byte bounds and a next pointer at `0x18`. A spawn record is `0x14` bytes: position at 0, unit ID/type/active/spawn-state bytes at `0x0c`–`0x0f`, and next pointer at `0x10`. Region and record insertion is head-first; portable vectors insert at the beginning to preserve that ordering.

## Construction and `Things` behavior

`MyGameUnitMgr_AddThing` (`0x00044b74`) selects the class from signed-byte variant 0–3:

- 0: ordinary `Things`;
- 1: `Breakable`, receiving the scene player and required-weapon argument;
- 2: `Pushable`, with physics flags `0x100`;
- 3: `Pickable`, receiving the manager and restoring physics flags `0x1e0`.

All variants load one model, materials, position and three axes, update the initial transform and BSP leaf, set the manager back-pointer, and enter bucket 2. Invalid portable class IDs fail explicitly rather than reproducing the original uninitialized-pointer path. Pickables receive material-intensity bounce 0 to `0xe666` over period 10 and then disable collision.

The model-name special cases `r_ken_a` and `r_kenh_a` select material-0 replacements from new-game options: weapon-selected `dj_ken_ab`/`dj_ken_bb`, or outfit-selected `dj_ken_ac`/`dj_ken_bc`. This is a texture override, not a model replacement.

`Pushable_Update` (`0x0003246c`) derives velocity from `player.position - player.previousPosition` while enabled, zeroes squared velocity below `0x28f`, calls `Things_Update`, and then clears the native current-leaf pointer. `Pickable_Update` (`0x0003f97c`) calls the base update and forces object intensity to zero.

`Pickable_PickUp` (`0x0003f9c8`) calls `Player_AddItem(itemId, true)` and zeroes the originating gameitem record's item ID when linked. This suppresses table-driven respawn; the pointer is reconstructed from map content during full loading rather than serialized.

## Breakable hit and destruction state

`Breakable_Update` (`0x00082688`) checks the player's native attack-phase word at `0x7f8`:

1. Attack phase zero clears both the hit latch and event.
2. A nonzero-phase, not-yet-latched hit is tested against `QueryBounds(Player_GetSwordHitBounds())`. A hit sets the latch and event state 2. The sword's separate hit-active byte is not this gate.
3. If the required weapon is nonnegative, event state is 2, and the player's weapon is at least that value, the optional sound plays, the alternate object's animation speed becomes 300, event state becomes 1, and destruction starts with duration 32.

`Breakable_ConsumeBreakEvent` (`0x000827fc`) returns whether the event is nonzero and clears it. The query is intentionally consuming. A required weapon of -1 leaves the event available without starting destruction.

The shared destruction timer and blend thresholds remain in `Things_Update`; stage 5 does not replace that implementation.

## `Things` persistence correction

`Things_SaveState`/`LoadState` (`0x0003261c`/`0x0003276c`) append to `VisUnit`:

1. presence and file/directory strings for the alternate model;
2. presence and file/directory strings for the alternate animation;
3. destroying as a 32-bit word;
4. signed 16-bit destruction duration and timer;
5. the native 32-bit word at `0x67c`.

The `0x67c` word is separate from the transient BSP leaf pointer at `0x680`. Earlier portable code incorrectly serialized the leaf index. The native constructor does not initialize `0x67c`; portable construction initializes it to zero to avoid indeterminate output, then preserves it on round trips.

`Pushable` appends its push-sound file name. `Pickable` appends item ID and pickup-sound file name. `Breakable` appends required weapon and break-sound file name. Null audio resources use the existing empty wide-string representation. Derived hit/event and pusher/spawn pointers are not serialized by these methods.

## Map entities

`Bsp_CreateGameEntity` (`0x000528ec`, containing address previously identified as `0x52948`) allocates `BspUnit` only for entity types -1 and 0. Other records dispatch to `MyGameUnitMgr_EntityFactory`; null entries are valid for non-renderable game-owned entities.

`BspUnit_LoadEntity` (`0x000fd5c0`) stores the entity type as the unit ID, converts six signed brush shorts to fixed-point local bounds, and borrows the model's object, animation, and secondary vertices. `BspUnit_Update` and `BspUnit_PostUpdate` (`0x000fd920`/`0x000fd904`) are original no-ops.

`BspUnit_Submit` (`0x000fd790`) updates an externally driven transform, otherwise evaluates the object against the renderer frustum. Culled objects are skipped. A visible object advances its map animation, installs secondary vertices, and enters queue 3 with negated camera depth when opaque or queue 4 with positive depth when transparent. `Object_UpdateFrustumVisibility` (`0x0001e110`) bypasses culling for non-normal render modes and otherwise tests sign-selected bounds points against the near and four side planes in native order 4, 0, 1, 2, 3.

Portable `Bsp` retains model and vertex storage while entities borrow it. `FreeLastLevel` destroys entities before clearing models, preventing dangling submissions during repeated loads.

Type 202 creates at most 16 manager-owned `Lensflare` objects. Each loads `guangyun_a`, `guangyun_b`, and `guangyun_c` at offsets 0, `0x4ccc`, and `0x9999`, then parses the source vector from the first entity pair. `MyGameUnitMgr_SubmitRenderables` (`0x00045414`) submits active lensflares to queue 6 after ordinary units. Reset drops the active count; map teardown therefore leaves no active lensflare submissions.

`ElectricSaw_Update` (`0x00022c04`) rebuilds collision bounds from local bounds plus position, expands X and Z by `0x20000`, and expands Y by `0xa0000`.

## Trap

`MyGameUnitMgr_AddTrap` (`0x00044aa8`) creates a bucket-5 `Trap` with ID, collision bounds, impulse, and the scene player. `Trap_Update` (`0x00026a80`) copies the impulse into the player's external velocity while their collision bounds overlap. After exit it clears external velocity only when it still equals this trap's impulse, preserving another trap's value. Save/load appends the impulse vector to `GameUnit`.

## StayBox

`StayBox_Update` (`0x000295ac`) computes `enemyOutside` by testing the enemy against the StayBox's local bounds. An allowed-region overlap spawns records when the spawned flag is clear; a denied-region overlap despawns them when it is set.

`StayBox_SpawnEnemies` (`0x00029784`) processes only records whose active byte is nonzero. It requests enemy construction with ID/type/active and a zero direction, restores spawn state, and binds the StayBox plus return position. `StayBox_DespawnEnemies` (`0x000298c8`) snapshots spawn position, type, active state, and spawn state before removal. A missing unit or enemy state above 14 instead clears the record's active byte. Save/load includes only the outside and spawned words after `GameUnit`; lists are reconstructed by commands and map setup.

## Script commands

All handlers have the standard `(arguments, argumentCount, output)` script dispatch signature. Portable calls validate the bucket-2 class where ARM would otherwise dereference an invalid pointer.

| ID | ARM handler | Arguments and behavior |
|---:|---:|---|
| 219 | `0x6becc` `GameSI_AddTrap` | Short ID, six fixed bounds values, fixed impulse XYZ; calls `MyGameUnitMgr_AddTrap`. |
| 220 | `0x6c03c` `GameSI_RemoveTrap` | Short ID; deleting removal from bucket 5. |
| 221 | `0x6c08c` `GameSI_AddThing` | Short ID, signed-byte type/variant, model, position/three fixed axes, normal-render boolean. Calls `AddThing` with collision enabled, then selects render mode 0 (normal) when true or 1 (secondary streams) when false. |
| 222 | `0x6c31c` `GameSI_SetThingDirection` | ID and fixed direction XYZ through the visual direction slot. |
| 223 | `0x6c3e8` `GameSI_SetPickableItem` | ID and short item ID. |
| 224 | `0x6c458` `GameSI_SetThingCollision` | ID and collision-enabled boolean. |
| 225 | `0x6c4c0` `GameSI_SetThingIntensityBounce` | ID, fixed minimum/maximum, short period. |
| 226 | `0x6c564` `GameSI_SetThingVisible` | ID and visibility boolean. |
| 227 | `0x6c5cc` `GameSI_IsThingVisible` | ID; returns visibility. |
| 228 | `0x6c63c` `GameSI_SetThingAnimation` | ID, animation name, integer speed, signed-byte direction and loop flag; loads the resource and applies playback/speed to every LOD and optional shadow object. |
| 229 | `0x6c718` `GameSI_SetThingAnimationPaused` | ID and paused boolean for every LOD object. |
| 230 | `0x6c788` `GameSI_IsThingAnimationPaused` | ID; returns object zero's paused state. |
| 231 | `0x6c7ec` `GameSI_SetThingAnimationTime` | ID and integer time for every LOD and optional shadow object. |
| 232 | `0x6c85c` `GameSI_GetThingAnimationTime` | ID; returns object zero's animation time. |
| 233 | `0x6c8c0` `GameSI_IsThingAnimationFinished` | ID; returns the visual completion flag. |
| 234 | `0x6c924` `GameSI_SetThingBreakModel` | ID, alternate model and animation names; loads the model, reloads materials, then sets the animation. |
| 235 | `0x6c9e0` `GameSI_SetThingBreakAnimation` | ID and alternate animation name only. ARM register flow corrects the decompiler's lost `this`. |
| 236 | `0x6ca54` `GameSI_SetBreakableSound` | ID and audio resource name. |
| 237 | `0x6cac4` `GameSI_SetPushableSound` | ID and audio resource name. |
| 238 | `0x6cb34` `GameSI_RotateThingX` | ID and signed-short degrees; converts `degrees * 4096 / 360.0f`, truncates, masks to `0x1fff`, and rotates object zero around X. |
| 239 | `0x6cbec` `GameSI_RotateThingY` | Same conversion around Y. |
| 240 | `0x6cca4` `GameSI_RotateThingZ` | Same conversion around Z. |
| 241 | `0x6cd5c` `GameSI_SetThingPosition` | ID and fixed XYZ; changes position without rebuilding collision bounds. |
| 242 | `0x6ce18` `GameSI_MoveThing` | ID and fixed delta XYZ; adds the delta and rebuilds collision bounds from local bounds plus position. |
| 243 | `0x6cf08` `GameSI_MoveThingTowards` | ID, target XYZ, fixed step; returns `Things_MoveTowards` arrival. |
| 244 | `0x6cfec` `GameSI_DestroyThing` | ID and signed-short duration through the destruction virtual slot. |
| 245 | `0x6d06c` `GameSI_SetThingDepthBias` | ID and fixed depth bias. |
| 246 | `0x6d0dc` `GameSI_RemoveThing` | ID; deleting removal from bucket 2. |
| 247 | `0x6d12c` `GameSI_ThingExists` | ID; returns whether bucket-2 lookup succeeds. |
| 248 | `0x6d198` `GameSI_ConsumeBreakableEvent` | ID; returns and clears the Breakable event. |
| 249 | `0x6d208` `GameSI_ThingBoundsInside` | ID and six fixed bounds values; returns whether `QueryBounds` hits. |
| 250 | `0x6d350` `GameSI_ThingBoundsOutside` | Same test, negated. |
| 251 | `0x6d498` `GameSI_SetThingLighting` | ID and boolean; writes object zero's lighting-enabled byte at native `0x69` through the shared visual helper. |
| 252 | `0x6d508` `GameSI_SetPushableEnabled` | ID and boolean written to the push-enabled byte. |
| 253 | `0x6d574` `GameSI_SetThingTexture` | ID, short material index, texture name; loads the override into every LOD object. |
| 254 | `0x6d600` `GameSI_GetMagicCubeOrientation` | ID; first axis with `abs(X)<0x1999 && abs(Z)<0x1999` returns X±/Y±/Z± as 0–5 according to Y sign. Non-aligned transforms return 0. |

## Verification

`Reversed_Game/tests/test_game_mapunits.cpp` covers every command above, all four AddThing variants, break hit latching and weapon gates, push velocity and threshold behavior, pickup and spawn-record clearing, trap entry/exit/removal, destruction/removal, animation state, rotations and all magic-cube orientations, movement/bounds asymmetry, lighting/depth/texture state, ElectricSaw expansion, StayBox spawn/despawn, and persistence second-save equality.

Real map 2008 creates 15 `BspUnit` instances and exercises opaque/transparent submission. Real map 2009 creates and submits its shipped lensflare. Both paths run through incremental loading and the real engine frame loop, followed by teardown. The real `day1/ep0` startup now advances past stage 5 and stops explicitly at stage-6 command 159 (`GameSI_AddActTrigger`, `0x0006934c`).

All 33 default tests pass sequentially. Core, STDIO logging, host-math, WAR-nibble, and ASan/UBSan builds each pass all 26 SDL-independent tests in separate directories. Ghidra contains the recovered names, typed signatures, native layouts, comments, 90 `stage5` function tags, a completion property, and a saved bookmark.

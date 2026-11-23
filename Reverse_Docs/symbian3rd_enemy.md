# Symbian3rd common enemy AI, pools, and script commands

## Scope and portable status

Stage 9 recovers the common `Enemy` implementation shared by the ordinary enemy classes, the manager's pools and spawn records, StayBox integration, blood-splat rendering, and `GameSI` commands 180–204 and 208–218. The authority is the ARM Symbian 3rd binary; addresses below are the Ghidra addresses used by the existing notes.

The portable common implementation is `Reversed_Game/src/game/Enemy.*`, with pool/day/spawn integration in `MyGameUnitMgr.*`, commands in `GameSI.cpp`, kill/gun-experience notification in `Player.*`, and lightmap modification in `src/soft3d/BspEffects.cpp`. Stage 10 adds concrete Maid/Bully classes in `MaidBully.*`; stage 11 adds Worm, Ghost, Lamper, and Weed in `SpecialEnemies.*`. See `symbian3rd_maidbully.md` and `symbian3rd_specialenemies.md`.

The pool factory uses concrete Maid/Bully classes for types 0, 1, 2, 6, 7, 9, and 13, and the stage-11 classes for types 3, 4, 5, and 10. An internal common-behavior adapter remains only for stage 12 and is not a claim that those variants are complete. Their native overrides, including Kate's linked-Bully pointer and variant-specific resources, still need individual recovery.

## Native `Enemy` layout

`Enemy` is 0x83c bytes and derives from the 0x6f0-byte `Creature`. Its primary vtable is at `0x00141b40`. Ghidra contains the full `Enemy` type and a separate `EnemyWaypointNode` type.

| Offset | Type/meaning |
|---:|---|
| `0x6f0` | Enemy type. |
| `0x6f4`, `0x6f8` | Maximum and current health. |
| `0x6fc` | Attack power. |
| `0x700` | State. |
| `0x704` | Horizontal player distance. |
| `0x708` | `Player*`. |
| `0x70c`, `0x710` | Hit-segment distance and attack range. |
| `0x714`, `0x718` | Knockback chance and speed. |
| `0x71c`–`0x730` | Six `Animation*`, indexed by state/resource slot. |
| `0x734`–`0x748` | Six `AudioData*`. |
| `0x74c`, `0x74e` | Signed-short attack hit-window start/end. |
| `0x750`, `0x752` | Hit cooldown and death countdown. |
| `0x754`–`0x7c3` | Embedded blood-splat state. |
| `0x7c8`, `0x7ca` | State time and an unused signed-short word. |
| `0x7cc` | Waypoint count. |
| `0x7d0`, `0x7d4` | Waypoint-list head and current node. |
| `0x7d8`, `0x7da` | Waypoint loop flag and direction. |
| `0x7dc`, `0x7de`, `0x7e0` | Alert-recovery delay, idle-alert delay, and wait timer. |
| `0x7e2` | Attack-hit latch. |
| `0x7e4`, `0x7e8` | Alert and chase speeds. |
| `0x7ec` | Player-visible latch. |
| `0x7f0` | Perception range. |
| `0x7f4` | Knockback-pending flag. |
| `0x7f8`, `0x7fa`, `0x7fc` | Attack cooldown, attack interval, and stopped/spawn state. |
| `0x800`, `0x802`, `0x804` | StayBox-attached, StayBox-outside, and `StayBox*`. |
| `0x808` | Configured StayBox return position. |
| `0x814` | `EnemySpawnRecord*`. |
| `0x818`, `0x824` | Three default animation and sound pointers. |
| `0x830`, `0x834` | Spawn-time copies of alert speed and perception range. |
| `0x838` | Gun-experience reward. |

Relevant inherited fields are navigation-enabled at `0x6c4`, saved animation/time at `0x6c8`/`0x6cc`, movement mode at `0x6d0`, and movement target at `0x6d4`.

The serialized `EnemyWaypoint` is 0x1c bytes: position, wait time, and direction. The runtime linked node is instead 0x24 bytes, adding previous/next pointers at `0x1c`/`0x20`. These are distinct Ghidra types; portable code uses an owned vector and a current index rather than exposing native allocation pointers.

## Common virtual slots

| Slot | Address | Ghidra name | Behavior |
|---:|---:|---|---|
| 28 | `0x00072bd8` | `Enemy_LoadResources` | Load model/materials, initialize animations/sounds, enable shadow, load blood textures, snapshot default resources. |
| 29 | `0x00072cf0` | `Enemy_Reset` | Mark removed/free, restore maximum health, clear pause/spawn/waypoint/animation state, stop sound. |
| 30 | `0x00074300` | `Enemy_UpdatePerception` | Range, facing, healing/hidden/FPS, and BSP visibility checks. |
| 31 | `0x000752f0` | `Enemy_Damage` | Apply damage, select hurt/knockback, trigger blood, set hit cooldown. |
| 32 | `0x0007561c` | `Enemy_Die` | Common death transition, player statistics/reward, death resources, spawn-record clearing. |
| 33 | `0x00072328` | `Enemy_InitializeAnimations` | Empty base hook; concrete classes supply resource slots. |
| 34 | `0x00072364` | `Enemy_InitializeSounds` | Empty base hook; concrete classes supply resource slots. |
| 35 | `0x00072304` | `Enemy_StopSound` | Common sound stop. |
| 36 | `0x00072dc4` | `Enemy_Spawn` | Initialize a free slot for active play. |
| 37 | `0x00074644` | `Enemy_UpdateIdle` | Player attack input, stopped state, movement, perception, and waypoint delay. |
| 38 | `0x00074784` | `Enemy_UpdateChase` | Target/follow navigation and attack selection. |
| 39 | `0x00074090` | `Enemy_UpdateAlertDelay` | Enter waypoint alert movement after the randomized delay. |
| 40 | `0x00074a68` | `Enemy_UpdateAlert` | Waypoint navigation, waiting, perception, and attack input. |
| 41 | `0x00074bc8` | `Enemy_SelectAttack` | Return 0 for unavailable, -1 for cooldown, or 1 for an allowed attack. |
| 42 | `0x00074cd0` | `Enemy_BeginAttack` | Enter state 3, face player, select nonlooping attack animation/sound, clear hit latch. |
| 43 | `0x00074dc8` | `Enemy_TestAttackHit` | Test the forward segment's AABB once inside the active frame window and damage the player. |
| 44 | `0x00074f64` | `Enemy_UpdateAttack` | Advance attack time, test hit, return to idle and start cooldown on completion. |
| 45 | `0x0007507c` | `Enemy_ReceivePlayerAttack` | Query player damage when hit cooldown is zero and apply a nonzero result. |
| 46 | `0x00075110` | `Enemy_UpdateHurt` | Hurt timing and recovery. |
| 47 | `0x00075200` | `Enemy_UpdateKnockback` | Knockback timing and recovery. |
| 48 | `0x0007570c` | `Enemy_UpdateDeath` | Secondary-stream rendering, motion stop, countdown, collision disable, final dead state. |
| 49 | `0x000759a4` | `Enemy_SetAnimationByIndex` | Select one of the six animation resources. |

Other recovered helpers include constructors/destructors at `0x000723a0`/`0x000728d0`, `Enemy_MoveTo` `0x00072a8c`, `Enemy_AddWaypoint` `0x0007324c`, `Enemy_ClearWaypoints` `0x00073374`, `Enemy_AdvanceWaypoint` `0x00073400`, `Enemy_Update` `0x0007388c`, `Enemy_BeginChase` `0x00073dd8`, `Enemy_StopMotion` `0x00074004`, `Enemy_SetNavigationEnabled` `0x00075840`, and `Enemy_SaveState`/`Enemy_LoadState` `0x000759fc`/`0x00075a68`.

Point, segment, and bounds queries at `0x000721b8`–`0x00072260` reject states 15 and above. Submission skips only state 17. Enemy collision queries additionally append the player to the unit-query result when their bounds overlap.

## State machine and perception

The common state values are:

| Value | State |
|---:|---|
| 0 | Idle |
| 1 | Alert/waypoint movement |
| 2 | Chase |
| 3 | Attack |
| 4 | Hurt |
| 5 | Knockback |
| 15 | Dying |
| 16 | Dead |
| 17 | Removed/free pool slot |

Spawn restores default resources, faces the player, performs the ground trace, enters idle, enables collision, resets timers/movement/StayBox flags, restores snapshotted alert/perception configuration, and selects normal rendering. Reset makes the slot available without destroying the pooled object.

Perception is latched once acquired. Healing item use clears the latch; hidden/FPS and dying states reject new acquisition. Range is tested first. Otherwise the direction must have `dot(direction, enemyForward) >= 0xb333`, with the native dark-object rejection and eye-height BSP fallback preserved. Player distance is horizontal: the enemy Y is replaced by player Y before measuring length.

Waypoints ping-pong by default; loop mode wraps instead. Arrival uses the native squared-distance threshold of `fixedOne`. Wait values and the alert/idle randomized delays use the manager RNG. Outside an attached StayBox, the enemy returns to the configured return position; ordinary player pursuit is suppressed until it is back inside.

The common attack selector distinguishes cooldown (`-1`) from other rejection (`0`). `RequestAttack` treats both `-1` and `1` as a request to enter the attack, matching command 196's nonzero test. Damage selects knockback when `Random() % 250 <= knockbackChance`, sets an eight-frame hit cooldown, and creates a nine-update blood effect. Death increments kill count and adds the type's gun-experience reward once, clears the linked spawn record's active value, and starts countdown 12.

## Pools, day preparation, and map spawning

`MyGameUnitMgr_InitializeEnemyPools` (`0x000419b0`) defines 17 type rows with eight-slot capacity. Singleton types 1, 2, 3, 8, 12, 13, 14, 15, and 16 construct only slot 0; every other type constructs all eight slots. Portable allocation mirrors that distinction. The hierarchy has 18 named concrete classes including `Boss`, but the ordinary pool has 17 rows and excludes `Boss`.

| Type | Class/constructor | Singleton |
|---:|---|:---:|
| 0 | `Maid`, `0x0005138c` | no |
| 1 | `Bully`, `0x00081ac8` | yes |
| 2 | `BullyDay3`, `0x0008b7c4` | yes |
| 3 | `Ghost`, `0x0008f70c` | yes |
| 4 | `Lamper`, `0x000901b8` | no |
| 5 | `Weed`, `0x000910e0` | no |
| 6 | `FatMaid`, `0x00091c84` | no |
| 7 | `Maid2`, `0x00051a28` | no |
| 8 | `Lord`, `0x000154f0` | yes |
| 9 | `Maid3`, `0x000520dc` | no |
| 10 | `Worm`, `0x0000fbe4` | no |
| 11 | `Prisoner`, `0x0001499c` | no |
| 12 | `Victor`, `0x00011d40` | yes |
| 13 | `BullyDay5`, `0x000230e4` | yes |
| 14 | `DarkKen`, `0x0001f190` | yes |
| 15 | `Sam`, `0x0001403c` | yes |
| 16 | `Kate`, `0x0001e410` | yes |

All 17 constructor configurations were compared with their ARM bodies. `EnemyCommonConfig` records unit type, HP/ATK, hit/attack ranges, attack interval/window, both delays, both speeds, knockback chance/speed, perception, reward, and positional audio. Maid3 (type 9), Worm (10), Victor (12), BullyDay5 (13), and Sam (15) assign nonzero gun-experience rewards: 2, 1, 8, 30, and 10 respectively; every other ordinary pool type retains base zero. The Maid3 assignment at `0x000520dc` was missed by the initial stage-9 audit and corrected in stage 10; Worm's assignment at `0x0000fbe4` was corrected in stage 11. Bully/BullyDay3/Victor/BullyDay5 use recovery/idle delays 64/32, Victor has knockback chance 800, and Sam uses interval 255, attack window 0/0, recovery/idle 9999/1. These corrections are covered field-by-field by the `constructor defaults` test group rather than only by spot-checking health.

`MyGameUnitMgr_AddEnemy` (`0x000454a4`) rejects zero spawn state, invalid types, duplicate script IDs already present in the map's spawn table, and exhausted pools. It searches X/Z offsets in 2.0 increments over the native -4.0 to just below 4.0 range when the requested position collides. Removal resets and unlinks without deletion.

`MyGameUnitMgr_PrepareDayEnemies` (`0x00042ba0`) loads resources by day:

- Day 2: Bully, Maid, Weed.
- Day 3: Ghost, Maid, Lamper with shadows disabled, Weed.
- Day 4: Maid, Weed, FatMaid, Maid2.
- Day 5: Victor, Sam, Maid3, Worm, Prisoner.
- Day 6: Bully and Kate; native code links Kate to that Bully for variant behavior.
- Day 7: Maid and Ghost, after removing player item 1.

`Scene::LoadDay` snapshots player state before `PrepareDayEnemies`, then loads UI and day content in native order. Weapon preparation lives in `PrepareDayEnemies`, including the unlock-dependent selection and day-5 additional weapon; the scene no longer duplicates those calls.

Ghost deliberately receives an empty model-name string on days 3 and 7. Its slot-34 body loads common sound indices 45–49 and a separate index-50 proximity sound at `Ghost+0x83c`; the older identification of that extra resource as index 49 was wrong. Kate's slot 34 loads sound index 109 into slots 1 and 2. The reviewed common animation/sound tables match native 0x80-byte table strides. Stage 10 adds all Maid/Bully extra resources and corrects the Victor/Day3 sound-block order; stage 11 supplies the four special variants, leaving the remaining extra resources to stage 12.

`MyGameUnitMgr_SetMapId` (`0x00043c34`) snapshots old-map enemies before changing the ID and resets all pool slots. `MyGameUnitMgr_FinishMapLoad` (`0x00043e80`) spawns items before enemies. `MyGameUnitMgr_SpawnMapEnemies` (`0x00047470`) uses stored IDs or descending IDs from 9999, restores active/health, appends waypoints, and links the originating record. `MyGameUnitMgr_UpdateEnemySpawnRecords` (`0x00047984`) writes health truncated to the active word, clamping negative values to zero, plus current position, direction, and unit ID.

## Script commands

All handlers use the standard script dispatch signature. Portable missing-unit paths return safely; native null-dereference bugs are called out where relevant.

| ID | Address / Ghidra name | Behavior |
|---:|---|---|
| 180 | `0x0006a298` `GameSI_AddEnemy` | ID, type, signed-short spawn/health, fixed position and direction; add through the pool. |
| 181 | `0x0006a408` `GameSI_PersistEnemy` | Ensure the enemy has a current-map spawn record. |
| 182 | `0x0006a468` `GameSI_SetEnemyNormalRendering` | Select normal rendering when true, secondary streams otherwise. |
| 183 | `0x0006a4f4` `GameSI_SetEnemyAnimation` | Load named animation and apply direction/loop selection. |
| 184 | `0x0006a5cc` `GameSI_IsEnemyAnimationFinished` | Return animation completion; missing enemy returns false. |
| 185 | `0x0006a64c` `GameSI_IsEnemyRemoved` | Return whether state is dead/removed; missing enemy returns true. |
| 186 | `0x0006a6f8` `GameSI_GetEnemyHealth` | Return health. Native missing-enemy path writes zero then dereferences null; portable code returns zero. |
| 187 | `0x0006a76c` `GameSI_SetEnemyHealth` | Negative restores maximum; zero enters death; positive assigns directly. |
| 188 | `0x0006a834` `GameSI_GetEnemyAttackPower` | Return attack power, with the same native missing-enemy null dereference as 186. |
| 189 | `0x0006a8a8` `GameSI_SetEnemyAttackPower` | Assign attack power. |
| 190 | `0x0006a920` `GameSI_SetEnemyDirection` | Set fixed XYZ facing. |
| 191 | `0x0006aa14` `GameSI_TeleportEnemy` | Set fixed XYZ position. |
| 192 | `0x0006ab0c` `GameSI_MoveEnemyTo` | Request movement to fixed XYZ and return its result; missing enemy returns false. |
| 193 | `0x0006ac1c` `GameSI_EnemyHearSound` | Test hearing at fixed XYZ through `Creature`; missing enemy returns false. |
| 194 | `0x0006ad44` `GameSI_EnemyBoundsInside` | Test six fixed bounds values; missing enemy returns false. |
| 195 | `0x0006ae9c` `GameSI_FollowEnemy` | Select player-follow movement. |
| 196 | `0x0006af04` `GameSI_AttackEnemy` | Enter attack for any nonzero selection, including cooldown result -1. |
| 197 | `0x0006afa0` `GameSI_RemoveEnemy` | Reset/unlink one enemy without deleting its pool object. |
| 198 | `0x0006b00c` `GameSI_RemoveEnemiesByType` | Remove all active enemies of one type. |
| 199 | `0x0006b0b4` `GameSI_RemoveAllEnemies` | Remove every active enemy. |
| 200 | `0x0006b138` `GameSI_PauseEnemy` | Set one enemy's navigation/pause flag. |
| 201 | `0x0006b1c8` `GameSI_PauseEnemiesByType` | Set the flag for all active enemies of one type. |
| 202 | `0x0006b298` `GameSI_PauseAllEnemies` | Set the flag for all active enemies. |
| 203 | `0x0006b33c` `GameSI_StopEnemy` | Set stopped/spawn state; stopping clears movement and visibility. |
| 204 | `0x0006b3bc` `GameSI_AddStayBox` | Construct an embedded StayBox with ID and six fixed bounds values. |
| 208 | `0x0006b7bc` `GameSI_SetStayBoxReturnPosition` | Set the enemy's configured fixed XYZ return position. |
| 209 | `0x0006b884` `GameSI_AddEnemyWaypoint` | Append fixed position, signed-short wait, and fixed direction. |
| 210 | `0x0006b9c8` `GameSI_SetEnemyWaypointLoop` | Set loop/wrap versus ping-pong behavior. |
| 211 | `0x0006ba48` `GameSI_ClearEnemyWaypoints` | Destroy/clear the route. |
| 212 | `0x0006baac` `GameSI_SetEnemyPerceptionRange` | Assign fixed perception range. |
| 213 | `0x0006bb34` `GameSI_IsEnemyAlerted` | Return the common alerted/attacking query; missing enemy returns false. |
| 214 | `0x0006bbb4` `GameSI_SetEnemyAnimationResource` | Replace one indexed animation from its resource name. |
| 215 | `0x0006bc50` `GameSI_SetEnemySoundResource` | Replace one indexed sound from `.\audio\`. |
| 216 | `0x0006bcec` `GameSI_SetEnemyAlertSpeed` | Assign fixed alert speed. |
| 217 | `0x0006bd78` `GameSI_AddCollisionUnit` | Add a base collision unit in bucket 1 with ID and six fixed bounds values. |
| 218 | `0x0006be7c` `GameSI_RemoveCollisionUnit` | Delete a bucket-1 collision unit by ID. |

StayBox commands 205–207 remain documented with the other map-unit commands in `symbian3rd_mapunits.md`.

## Persistence corrections

`Enemy_SaveState` first writes the inherited `VisUnit` record, then exactly four big-endian 32-bit words:

```text
enemyType
navigationEnabled
health
state
```

`Enemy_LoadState` reads that same extension. The portable loader restores the raw navigation flag through `Creature::RestoreNavigationEnabled`, avoiding pause side effects during reconstruction.

The earlier identification of manager record `0x26` as an enemy was wrong. In `MyGameUnitMgr_LoadUnitState` (`0x00048314`), `0x26` consumes the next embedded 0x6c-byte StayBox at manager `+0xa28`, loads it, and binds the player. There is no pooled-enemy case in this switch. Full-scene writing/reading remains stage 13; any enemy persistence outside that switch must be established separately rather than assigned to `0x26`.

StayBox despawn preserves the configured return position, type, active-byte health, and stopped/spawn state, not the enemy's current world position. Missing units and states above 14 clear the record's active byte.

## Blood-splat lightmap update

`Enemy::BloodSplat` loads `zv_jianxue_a` and `zv_jianxue_b` from `.\common\`. A hit initializes life 9 and randomized horizontal spread. Updates damp X/Z by `0xe147` and subtract `0x428f` from Y velocity. At life 1, the effect selects texture B with probability 556/1000, chooses independent U/V flips with threshold 501/1000, and traces from `position + Y(8.0)` to `position + velocity - Y(6.0)`.

`Bsp_SetLightmapSplatTexture` (`0x00054684`) stores texture/flip state; `Bsp_BlendLightmapSplat` (`0x00054640`) enters `Bsp_TraverseLightmapSplat` (`0x00105f5c`). Traversal visits the near child, then node-owned faces at the crossing, then the far child. For each selected face it projects the intersection through texinfo, clips a half-width/half-height splat against the face, skips the transparent index, and blends each covered lightmap pixel.

The source color is `Texture_GetPaletteEntry(texture, index, 15)`, equivalent to palette byte offset `(index * 32 + 15) * 2` for the extended palette. The blend is exactly:

```c
destination = ((destination & 0xf7de) >> 1) + ((source & 0xf7de) >> 1);
```

This is not rounded per-channel averaging. Native axis-plane intersection writes `Y = -plane.distance`; general planes interpolate only Y from the crossing fraction. That fraction uses `Fixed_ReciprocalSignedNormalized` (`0x00106d64`) and `Fixed_ReciprocalMagnitudeNormalized` (`0x00106b90`), not exact host division. `Fixed_ReadReciprocalTable` (`0x00124214`) returns `0x40000000/index` (or `0x7fffffff` at zero), with magnitude shifts at the native `<0x20000`, `<0x80000`, `<0x200000`, and `<0x800000` boundaries. The portable code preserves these truncation/wrapping rules. Portable bounds checks prevent malformed textures/faces from accessing invalid memory.

## Verification and Ghidra status

`Reversed_Game/tests/test_game_enemy.cpp` has eighteen groups: the five common groups, six stage-10 groups, and seven stage-11 groups. They cover every common config field, common virtual behavior and state transitions, perception, attack/damage/death, statistics, waypoints, pause/stop, exact save/second-save bytes, pool exhaustion/reuse, all commands above, real `einfo2` spawning and snapshotting, StayBox return state, all eleven concrete enemy classes, and real map combat. `test_bsprender.cpp` adds directed blood-splat pixels for palette/blend arithmetic, transparency, flips, clipping, the axis-plane quirk, and an asymmetric crossing where native reciprocal arithmetic produces Y=0 while exact division would produce Y=1. The map-unit weapon tests prepare a real sword and use its computed hit bounds rather than relying on the former invalid zero-initialized box.

At stage 9, all 36 default, STDIO-logging, host-math, and WAR tests passed serially, and core-only and ASan/UBSan each passed 29. Stage 11 reruns all 36 default tests and all 29 host-math and ASan/UBSan tests successfully. Tests in one build directory must run serially, and separate build directories must not run concurrently, because some tests share fixed temporary filenames.

Ghidra contains the saved 0x83c `Enemy` and 0x24 `EnemyWaypointNode` layouts, typed common signatures, common/manager/command/pool-constructor names, behavior comments, and `game,enemy,stage9` tags. The corrected `0x26` comment and blood-splat prototypes are saved with them. Stage 10 adds the seven Maid/Bully layouts and stage-10 tags; stage 11 adds the four special-class layouts, the blood-splat prefix layout, 45 typed/commented functions, stage-11 tags, and completion properties. See `symbian3rd_maidbully.md` and `symbian3rd_specialenemies.md`.

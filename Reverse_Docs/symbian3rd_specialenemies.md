# Symbian3rd Worm, Ghost, Lamper, and Weed

## Scope and portable status

Stage 11 recovers the four special enemy classes from the ARM Symbian 3rd binary. The loaded memory block begins at `0x8000`; addresses below are loaded addresses and already include that bias. Do not add it again.

The portable implementation is `Reversed_Game/src/game/SpecialEnemies.*`. `MyGameUnitMgr::Initialize` selects these concrete classes after the Maid/Bully factory and before the completed stage-12 classes. The former common adapter has been removed. Common behavior remains in `Enemy.*`; these classes retain their native overrides rather than receiving a shared approximation.

| Type | Class | Native size | Pool capacity | Day/resource preparation |
|---:|---|---:|---:|---|
| 3 | `Ghost` | `0x844` | 1 | Days 3 and 7, empty model name |
| 4 | `Lamper` | `0x83c` | 8 | Day 3, `r_lamper_a`, shadows disabled |
| 5 | `Weed` | `0x83c` | 8 | Days 2, 3, and 4, `r_weed_a` |
| 10 | `Worm` | `0x840` | 8 | Day 5, `r_bug_a` |

Common animation slots are Worm 190–195, Lamper 110–115, and Weed 116–121. Worm slot 4 is deliberately empty. Common sound indices are Worm 83–87, Ghost 45–49, Lamper 52–56, and Weed 58–62. Native missing resources remain null.

## Worm

`Worm` adds only the attack-path-clear byte at `0x83c`, padded to a 0x840-byte native object. Its constructor also assigns **1** gun experience at `Enemy+0x838`; the stage-9 common configuration incorrectly retained the base zero and is corrected.

### Spawn variation

`Worm_Spawn` (`0x00010080`) calls common spawn and then performs three manager-RNG draws:

1. Chase speed becomes `0x3333 + Random() % 0x3333`.
2. The primary object transform is reset to identity and scaled by `1.0 + Random() % 80000` on X and Z, with Y unchanged at 1.0.
3. `Random() % 30000` selects a material-0 texture override on every object: `dk_bug_a` for 0–10999, `dk_bug_b` for 11000–18999, and `dk_bug_c` for 19000–29999.

The colours are therefore per-spawn cosmetic variants of the same type-10 class, not separate enemy classes and not a common-class parameter. Attack behavior, health, and type are shared. The portable `Object::ResetTransform` implements the native reset at `0x000ea7a0`.

### Attack path and movement

`Worm_Update` (`0x000102d8`) runs the common update, then, only while unpaused, recomputes the path byte. The path is rejected when:

- absolute player/enemy Y difference exceeds `0x50000`;
- the BSP trace to the player using the worm's local bounds is blocked; or
- bucket 0 or 2 contains another unit in the expanded bounds.

Those bounds combine current collision bounds with the player's local bounds behind the player along worm forward. The offset is `enemyHalfWidth + playerHalfWidth - fixedOne`. Queries exclude the worm itself.

`Worm_UpdateAlert` (`0x000105f0`) clears attack cooldown when the player is within `hitDistance` and this path is clear. `Worm_SelectAttack` (`0x00010658`) additionally requires normal stopped/movement/alert state and `playerDistance <= attackRange`; it returns -1 for cooldown and does not perform the common BSP visibility check. `Worm_IsAttackPathClear` (`0x00010c74`) simply reads the byte.

### Jump attack and death

`Worm_BeginAttack` (`0x0001073c`) uses common entry, then sets `PhysicKeepDirection` and clears `PhysicStepOffset` and `PhysicSimpleMovement`.

`Worm_TestAttackHit` (`0x0001079c`) checks whether the last unit-query result is the player and sets the latch; it does not damage directly. `Worm_UpdateAttack` (`0x00010864`) implements the jump:

- Player damage during the attack clears velocity, snaps to ground, clears KeepDirection/SeparateAxisCollision, and restores StepOffset/SimpleMovement.
- At state time 14, velocity becomes `forward * 0x20000` with Y `0xcccc`, SeparateAxisCollision is enabled, and the ground collision flag is cleared. No other attack work runs on that update.
- Before the first landing, animation time is clamped to `0x12c0`.
- A latched collision damages the player and changes only X/Z velocity to `-forward * 0x1999`.
- After time 14, nonpositive Y velocity plus ground collision clears velocity and SeparateAxisCollision. The first landing sets animation time `0x13ec`; every landing increments the counter at common `0x7ca`.
- Finished animation clears KeepDirection, restores StepOffset/SimpleMovement, selects idle animation, enters idle, and starts the attack interval. State time is not reset here.

`Worm_UpdateDeath` (`0x00010c9c`) follows common death, sinks by `0x1999` on each finished animation, clears SlideMovement, and removes the enemy through its manager at countdown zero.

## Ghost

Ghost adds `AudioData* proximitySound` at `0x83c` and `int proximityVoice` at `0x840`, initialized to -1. `Ghost_InitializeSounds` (`0x0008f93c`) loads common sound indices 45–49 and the **separate index-50** proximity resource `c_ghost_walk01`. Earlier notes incorrectly identified the proximity resource as index 49.

`Ghost_SubmitRenderables` (`0x0008f49c`) is an original empty method. `Ghost_ReceivePlayerAttack` (`0x0008f4bc`) always returns false, so the ordinary player attack path cannot damage Ghost.

`Ghost_Update` (`0x0008fabc`) first copies the player's local bounds and forces X/Z extents to ±1.0, then runs common update. While unpaused and no fade is active, it selects presentation by horizontal player distance:

- `<= 0xa0000`: Twist shader with amplitude 2; the existing proximity voice is left alone.
- `> 0xa0000` and `<= 0x140000`: Twist amplitude 1 and start the independent looping proximity sound if its voice is -1.
- `> 0x140000`: select the default renderer shader and stop proximity audio.

The disassembly uses native Scene offset `0x928`, which is the Twist owner, not MotionBlur. Portable `TwistShader::SetAmplitude` exposes the native amplitude field. Renderer deselection selects the basic/fog shader, so a null `Renderer::SelectShader` argument does not imply a null active-shader pointer.

`Ghost_UpdateAttack` (`0x0008fd60`) advances state time and performs the common hit test without checking player attacks. At exactly 20 it resets state time, starts the interval, and enters idle without selecting the idle animation. `Ghost_StopSound` stops both the independent proximity voice and the common voice. `Ghost_SetNavigationEnabled` stops proximity audio on a pause rising edge before common pause handling; `Ghost_Spawn` resets the voice after common spawn. States above 15 stop only proximity audio in the custom update.

## Lamper

Lamper has no fields beyond `Enemy`.

`Lamper_Spawn` (`0x00090634`) is not common spawn. It copies position/previous position, selects stand twice, resets core health/timers/movement/waypoint state, computes horizontal player distance, disables collision, clears the two navigation path-node pointers, selects normal rendering, stops the base visual sound, sets physics flags `0xb`, and clears ground collision. It does not perform common facing, ground placement, default-resource restoration, spawn-record clearing, or all of the common path/configuration resets.

`Lamper_UpdatePerception` (`0x00090ce0`) recomputes on every call: living state, player not hidden/FPS, and distance below range. Every rejection clears visibility; there is no latch, facing, healing, darkness, or BSP test. `Lamper_UpdateIdle` (`0x000908d8`) updates perception, checks player attack, and attacks when the common selector returns positive.

`Lamper_BeginAttack` (`0x00090978`) resembles common entry but does not face the player. `Lamper_TestAttackHit` (`0x00090b5c`) tests the enemy position raised by half its local height with `Player::IsPointInside` inside the attack window, then latches and damages once.

`Lamper_ReceivePlayerAttack` (`0x00090a40`) temporarily enables collision only around `Player_GetAttackDamage`, then disables it again. Any nonzero hit subtracts health and calls `Die` unconditionally, even if health remains positive. It does not use common hurt recovery or set the common eight-frame cooldown. `Lamper_Damage` (`0x00090da4`) instead calls common damage and then clears blood-splat life at native `Enemy+0x758`.

## Weed

Weed also has no fields beyond `Enemy`.

`Weed_UpdatePerception` (`0x00091558`) is range-only but latched: once acquired, visibility remains set. New acquisition requires a living enemy, player not hidden/FPS, and distance below range. It omits common healing, facing, darkness, and BSP tests.

`Weed_ReceivePlayerAttack` (`0x00091614`) is a separate inline damage path. It subtracts health, dies below one, otherwise selects hurt versus knockback using `Random() % 1000` against `knockbackChance`, sets direction/velocity and hit cooldown 8. This differs from common damage's modulus 250 and does not trigger blood. `Weed_Damage` (`0x00091910`) calls common damage and then clears blood life at `0x758`.

The Ghidra `EnemyBloodSplatLayout` records the confirmed resource-manager and life fields at relative offsets 0 and 4; the remaining 108 bytes stay opaque. Its placement makes life native `Enemy+0x758`, distinct from `knockbackPending` at `0x7f4`.

## Verification and Ghidra status

`test_game_enemy.cpp` adds seven stage-11 groups: concrete types/resources, Worm colour boundaries and jump/death, Ghost effects/immunity/attack, Lamper perception/point hit/one-hit death, Weed perception/custom damage, real content, and engine-frame encounters. Tests check every colour threshold, X/Z scaling, chase speed, landing and damage, proximity voice/shader transitions, collision toggling, blood-life clearing, and Worm's corrected reward.

Real-content checks execute every day-3 `einfo` group through scene updates, including Lamper and Weed. The shipped `day5/ep1053` type-10 `AddEnemy` command objects run through production `GameSI` on native map 1053, creating worms 2, 3, and 5. The shipped `day3/ep1020` Ghost command runs through the same dispatcher and updates on known-good map 2008. All four classes also run and attack through the real map-2008 engine frame loop. Full condition/task progression is intentionally left to stage 15; these tests execute the actual SST host commands rather than synthesized replacements.

Two unrelated existing BSP-loader limitations were encountered and are not hidden by the tests: day-1 map 1002 stalls at progress 56, and day-3 map 1020 fails with `S3D_ACCESS`. Worm's native map 1053 loads and runs successfully; Ghost's SST behavior is verified on map 2008 instead.

All 36 default tests pass serially, and the host-math and ASan/UBSan suites each pass all 29 tests. Ghidra contains the four concrete class layouts, the blood-splat prefix layout, 45 typed/commented native functions including constructors and destructors, `game,enemy,stage11` tags, and saved `Stage11Types`/`Stage11Status` properties. Worm's reward and Ghost's index-50 proximity resource corrections are included.

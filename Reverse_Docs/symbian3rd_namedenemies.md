# Symbian3rd Victor, Sam, Prisoner, Kate, DarkKen, Lord, and Boss

## Scope and portable status

Game stage 12 recovers the six remaining ordinary enemy classes and the separately constructed `Boss`. The authority is the ARM Symbian 3rd binary. Addresses below are loaded Ghidra addresses and already include the file-to-memory `0x8000` bias; do not add it again.

The portable implementation is split into:

- `Reversed_Game/src/game/NamedEnemies.*`: Victor, Sam, Prisoner, Kate, and DarkKen;
- `Reversed_Game/src/game/Lord.*`: Lord's combat and complete scripted encounter;
- `Reversed_Game/src/game/Boss.*`: the generic native Boss action-sequence controller.

`MyGameUnitMgr::Initialize` now constructs a concrete class for every one of the 17 pool rows. The stage-9 common-behavior adapter has been removed. Boss remains outside the pool and has no construction caller in this binary; no new pool type or script command was invented for it.

| Type | Class | Native size | Pool capacity | Model/resource preparation |
|---:|---|---:|---:|---|
| 8 | `Lord` | `0x958` | 1 | Spawn loads `r_nanjue_b`, sword, shield, and encounter objects |
| 11 | `Prisoner` | `0x848` | 8 | Day 5, `r_qiutu_a` |
| 12 | `Victor` | `0x86c` | 1 | Day 5, `r_victor_a` |
| 14 | `DarkKen` | `0xb04` | 1 | Spawn loads `r_ken_a`, `dj_evilken_b`, and two guns |
| 15 | `Sam` | `0x83c` | 1 | Day 5, `r_sam_b` |
| 16 | `Kate` | `0x840` | 1 | Day 6, `r_kate_a`, linked to the day-6 Bully |
| — | `Boss` | `0x870` | not pooled | Generic; no native construction call or fixed resource table |

## Victor

Victor's common animation slots are 204–209. Extra leap animations 210–212 are `r_victor_leapattack_a/b/c`. Common sounds are 110–115; extra sounds are 116 (`c_victor_shrink`), 117 (`c_victor_jump`), and 117 again. Fields after `Enemy` are leap phase at `0x83c`, animation pointers at `0x840`, sound pointers at `0x84c`, target at `0x858`, and minimum/maximum leap distances `0x140000`/`0x280000` at `0x864`/`0x868`.

`Victor_BeginAttack` (`0x00012560`) uses common entry and window 7–9. A `Random()%10000` result above 6000 selects speed 600 and halves the unsigned window; a second result above 7000 halves attack interval. `Victor_TestAttackHit` (`0x000134ec`) uses the full-width forward volume.

Idle and chase (`0x00013050`, `0x000132d0`) add a leap when distance is within 20–40 world units inclusive and the random selection exceeds 7000. Idle uses short random delays below 5001 and chase otherwise; chase can stop for a random delay above 9800 when ordinary attack selection is unavailable.

`Victor_UpdateLeap` (`0x00012988`) has three phases:

1. Track the player for up to 18 frames, adjusting the target by `0x28000` along a blocking BSP normal. On animation completion select leap B and clear SimpleMovement/StepOffset.
2. Add forward velocity `0x6666` per update. Test expanded player bounds, invoke hurt2, and test a second impact volume after a 150-unit forward trace. Ground/unit collision or a valid impact enters recovery with leap C and the shrink sound.
3. On completion stop motion, apply attack cooldown, and set the ordinary delay to 24.

Player collision is not appended during airborne phase 1. `Victor_ReceivePlayerAttack` (`0x000126d8`) preserves a nonzero airborne phase after subtracting health/triggering blood; other hits use custom hurt and knockback. Death uses common behavior and the confirmed eight-point gun reward.

## Sam

Sam has no fields beyond `Enemy`. Animation slots 0 and 1 both use index 234 (`r_sam_walk_a`); the remaining animation slots are null. Sounds are index 107 (`c_sam_help`) in slot 1 and 108 (`c_sam_fade`) in slot 5.

Perception (`0x00013dd0`) and attack selection (`0x00013df0`) are original constant-zero methods. Spawn resets material-0 blend mode to zero. Damage calls common damage and clears blood life.

`Sam_Die` (`0x000143f0`) increments kill count, plays the fade sound, starts countdown 16, enters dying, and adds the confirmed 10-point gun reward. It does not select common death animation. `Sam_UpdateDeath` (`0x000144b0`) selects secondary streams and decrements the countdown each update. Blend modes are 0 through 13–15, 3 through 9–12, 2 through 5–8, and 1 through 1–4. At zero it disables collision and removes the pooled slot through the manager.

## Prisoner

Prisoner adds three idle animation pointers at `0x83c`–`0x844`. Common slots are 196, 199, 200, 201, 202, and 203; the extra idle resources are 196, 197, and 198 (`r_qiutu_stand_a/b/c`). Sounds are 89–93.

The constructor assigns **1** gun-experience point. The stage-9 table incorrectly retained zero; both duplicate constructor bodies (`0x00014770` and `0x0001499c` plus its continuation at `0x000149f8`) confirm the correction.

Perception and attack selection (`0x00014730`, `0x00014750`) always return zero. After common update (`0x00014ec4`), entering idle or completing an idle animation draws `Random()%30000`: 0–10999 selects stand A, 11000–18999 stand B, and 19000–29999 stand C. Death (`0x00015024`) sinks by `0x1999` on completed animation, clears SlideMovement, and removes the slot at countdown zero.

## Kate

Kate adds `Bully* linkedBully` at `0x83c`. `MyGameUnitMgr_PrepareDayEnemies` day 6 assigns the pooled Bully after loading both models. Common animations are 235, 236, and 237; sound 109 is loaded into slots 1 and 2.

`Kate_UpdatePerception` (`0x0001e1a0`) always returns true, while `Kate_SelectAttack` (`0x0001e1c0`) always returns -1. Common `RequestAttack` consequently treats a request as nonzero and enters common attack. Idle (`0x0001ea84`) calls that always-true perception override and begins chase beyond `0x50000`; scripted movement modes enter chase directly. It does not perform the common idle player-attack check.

`Kate_Update` (`0x0001e784`) faces the player before common update. Unless the player used a healing item or Bully is already alerted, Kate relays detection to Bully:

- Kate/Bully distance below Bully's perception range immediately sets Bully's `playerVisible` latch.
- Otherwise Kate's object intensity must be at least `-0x1999` and the normalized Bully-to-Kate direction must have forward dot product at least `0xb333`.
- A trace from Bully's eye to Kate can succeed directly, with a second trace to Kate's eye as fallback.

`Kate_AlertLinkedBully` (`0x0001eb34`) writes Bully's native latch at `Enemy+0x7ec`; it does not call common perception.

## DarkKen

DarkKen uses body animations 25, 26, 26, 30, 33, and 35, plus 31/32 for the additional attack sequence. Equipment animations are 68, 69, 69, 73, 76, 78, 74, and 75. All animation resources use cache level 2, although the model and sounds use level 3. Common sounds are 21, 21, 29, 30, and 31, with extra sounds 24 and 28.

The extra layout contains:

| Offset | Meaning |
|---:|---|
| `0x83c`–`0x844` | Two extra attack animations and two sounds |
| `0x84c`–`0x9cb` | Sixteen 0x18-byte world-space region bounds |
| `0x9cc`–`0xabb` | Sixteen rows of 15 nearest-neighbor indices |
| `0xabc`, `0xac0`–`0xac8` | Current region and three-history entries |
| `0xacc` | Current region target |
| `0xad8`, `0xadc` | Primary and secondary gun `Object*` |
| `0xae0`–`0xafc` | Eight equipment animations |
| `0xb00` | Taunt cooldown |

Spawn (`0x0001fd80`) loads only the player's type-1 weapon animation set, the `r_ken_shadow_a` alternate player object, DarkKen's model/texture, and two `ow_qiang_a` objects. It creates 16 fixed arena rectangles, computes all pairwise center distances, and sorts the other 15 indices for each region. Region history is reset to -1 and target to the spawn position.

`DarkKen_ChooseNextRegion` (`0x00021c24`) locates the enemy and player at half enemy height. It walks the player region's nearest-neighbor row, skips the three recent regions, and updates target/history. Targets use region-center X/Z and current enemy Y (`0x00021f04`). Chase navigates this graph rather than directly pursuing the player.

Attack selection (`0x000210ec`) rejects distance below attack range or nonzero cooldown, then requires a clear same-height BSP line. Attack update (`0x00021220`) chains common attack → extra attack B → extra attack C, synchronizing gun animations 6 then 7. The hit test (`0x00021504`) runs only in the middle stage before its substep latch: it tests a near segment at `playerDistance/8`, then a far segment at `playerDistance+0x50000`, and finally requires a clear BSP trace before one damage event.

Dual guns inherit body transforms and animation indices. Running index 2 selects speed 450 on body and guns. Submission copies depth/transform state and submits the primary gun in queue 3 and secondary gun in queue 2.

## Lord

### Resources and equipment stages

Lord owns sword and shield objects, 19 extra body animations, 16 sword animations, 13 shield animations, and seven extra sounds. Base animation indices are 136, null, 138, 142, 145, and null. Extra body indices are 137, 139, 140, 141, 143, 144, 146–158 in recovered slot order. Base sounds are 95–99 and extras are 100–106.

Spawn (`0x000167b0`) loads `r_nanjue_b`, `ow_dajian_atk_a`, and `ow_dun_atk_a`. Initial descriptor scroll is V=`0x60000` for body/sword and U=`-0x60000` for shield. These dereferenced descriptor writes are not blend-mode changes. The transformation phase loads `dj_nanjue_a` and clears runtime/descriptor scrolling.

Spawn also creates four `oszm_shuijing_aa` Things with IDs 1–4, the event-dependent `ow_qiang_a`/`ow_shaying_a` Thing 0, and five ActTriggers. The original fixed positions, axes, and bounds are preserved in `Lord.cpp`.

Equipment stages are:

| Stage | Meaning |
|---:|---|
| 0 | Introduction; player attacks rejected |
| 1 | Combat with sword and shield |
| 2 | Shield removed; body stand/walk B, hurt B, adjusted leap/landing/equipment resources, hit distance 10, range 9, primary impulse `0x20000`, HP 160 |
| 3 | Sword removed; body walk C and attack D, shared walk sound, hit distance 7.5, range 8.5, impulses `0x8000`/`0x10000`, HP 80 |
| 4 | Encounter complete |

### Combat

Ordinary attacks (`0x0001be70`) choose kind 0 below 5001/10000 and kind 1 otherwise. Attack power is 25/20 for kind 0 before/after stage 2 and 20/15 for kind 1. Kind 0 uses the common window/full-width volume; kind 1 uses native window 11–13 and half width. Haste selects speed 600 and halves windows. Both use `Player_DamageHurt3` with stage-specific impulses.

Player-hit thresholds (`0x0001c8b0`) are stage 1 HP≤160 → encounter phase 12, stage 2 HP≤80 → phase 24, and stage 3 HP<1 → phase 36. These paths enter state 7 and return false after applying the hit. Other hits use custom hurt/knockback behavior and can set haste. Direct `Lord_Die` increments kill count without gun reward and immediately enters dead state with secondary streams and collision disabled.

Leap is available before stage 3 at inclusive distance `0x100000`–`0x140000`. `Lord_UpdateLeap` (`0x0001ce80`) tracks the target, computes vertical velocity from `playerDistance/4+1`, uses separate-axis collision, lands through the shadow trace, tests recovery hits in frames 2–4, and finishes with a randomized delay.

### Encounter state machine

`Lord_UpdateEncounter` (`0x00017b04`) is the complete original cinematic/boss lifecycle, not an empty hook. Its phases cover:

- initial weapon selection and event-1002-dependent weapon 3/4;
- timed messages 100/101/102/104/105/106 and the native dialog-result checks;
- four crystal replacement/break sequences using `oszm_shuijing_ab`, player actions 6/7, sound 24, `t_crystalbreak`, fixed cameras, fades, and control-map gating;
- transformation to `dj_nanjue_a`, body/equipment rendering restoration, and shield/sword broken models;
- cancellation paths restoring 200, 120, and 40 HP at the three break dialogs;
- final `r_nanjue_die_b` corpse Thing, camera shake 64/`0x8000`, `e_landquake`, map/menu action gates, and `m_oddity_01` music.

The native control map uses 0 for enabled and 1 for disabled; portable Boolean conversions are inverted at this boundary. Post-crystal camera waits are 22 frames, while the initial transformation uses 38. Lord's body remains hidden after the final corpse spawn.

## Boss

Boss is present in RTTI and has a complete native vtable, but no constructor xref exists in this binary. It is not an ordinary pool row and no shipped SST command constructs it. Its native fields are four attack animation pointers, sequence count/current index/action index, delay timer/current action, and three arrays of per-sequence integer arrays for actions, delays, and random-delay ranges.

`Boss_Update` (`0x00083364`) calls the original empty custom slot-50 `Boss_SelectNextSequence` (`0x0008384c`) when common Enemy movement mode is zero. Configured actions are:

| Action | Behavior |
|---:|---|
| 0 | Stop motion |
| 2 | Wait until player distance is within attack range, then stop and advance |
| 3–6 | Enter attack using animation slots 0–3 respectively |
| 7 | Stop, reset action index, and clear current sequence |

Delay -1 means immediate advancement. Nonnegative delays count down before the next action; a positive random range adds `(Random()%(range<<8))>>8`. Attack entry does not perform common sound playback. Idle uses the same movement-mode word: no movement faces the player, while a movement mode enters common chase.

Portable `Boss::ConfigureSequences` owns equivalent arrays and validates equal lengths. `StartSequence` is an explicit host/test operation, not a newly invented game command. Base `SelectNextSequence` remains an original no-op.

## Verification and Ghidra status

`test_game_enemy.cpp` now has 25 groups: five common, six stage-10, seven stage-11, and seven stage-12 groups. Stage-12 tests cover every dynamic type/resource table, Victor leap boundaries/phases, Sam blend/death/reward behavior, all Prisoner idle ranges and corrected reward, Kate's linked-Bully relay, DarkKen's region graph and three-stage attack, Lord equipment/combat/final corpse phases, Boss delays/actions/death/statistics, and all six pooled variants through the real engine frame loop.

Shipped SST `AddEnemy` commands run through production `GameSI` for all six pooled variants:

- Native maps 1051, 1060, and 2066 successfully run Lord (`ep1051`), Victor (`ep1060`), and DarkKen (`ep2066`).
- The directed class tests historically ran Prisoner (`ep1056`) and Sam (`ep2052`) on known-good map 1051 and Kate (`ep6004`) on day-6 map 2008 because their native maps then failed to load. Stage 15's corrected path/sentinel and nonfatal-SPL behavior now loads every shipped day map, and the episode integration test runs all three encounters in their actual episode contexts.
- Lord's real map-1051 encounter creates its native objects/triggers. Boss uses real model/animation resources and the real engine frame loop but has no shipped construction site.

All 36 default tests, all 29 host-math tests, and all 29 ASan/UBSan tests pass serially. Variant suites were not run concurrently.

Ghidra contains the seven exact native structures, 114 renamed/prototyped functions with `game,enemy,stage12` tags, 46 behavior comments, three Stage12 bookmarks, and saved `Stage12Types`/`Stage12Status` properties. The saved comments distinguish runtime blend selector `ObjectMaterial+0x28` from descriptor U/V scroll, record Prisoner's corrected reward, and preserve Boss's original empty sequence-selection hook.

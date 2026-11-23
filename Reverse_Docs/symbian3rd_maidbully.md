# Symbian3rd Maid and Bully enemy families

## Scope and portable status

Stage 10 recovers `Maid`, `Maid2`, `Maid3`, `FatMaid`, `Bully`, `BullyDay3`, and `BullyDay5` from the ARM Symbian 3rd binary. The loaded memory block begins at `0x8000`; addresses below are loaded addresses and already include that bias. Do not add it again.

The portable implementation is `Reversed_Game/src/game/MaidBully.*`. `MyGameUnitMgr::Initialize` selects these concrete classes, then the stage-11 classes in `SpecialEnemies.*` and the stage-12 classes in `NamedEnemies.*`/`Lord.*`; no common fallback remains. Common `Enemy` behavior remains in `Enemy.*`; the variants do not share an invented substitute for native overrides.

| Type | Class | Native size | Pool capacity |
|---:|---|---:|---:|
| 0 | `Maid` | `0x83c` | 8 |
| 1 | `Bully` | `0x844` | 1 |
| 2 | `BullyDay3` | `0x8d0` | 1 |
| 6 | `FatMaid` | `0x880` | 8 |
| 7 | `Maid2` | `0x83c` | 8 |
| 9 | `Maid3` | `0x83c` | 8 |
| 13 | `BullyDay5` | `0x8b4` | 1 |

Day 2 prepares Bully and Maid, day 4 prepares FatMaid and Maid2, and day 5 prepares Maid3. Day3 and Day5 load their own models and cache-3 resources during `Spawn`; they are not additional `PrepareDayEnemies` entries in the native manager.

## Gun-experience correction

The stage-9 audit missed `Maid3_Ctor` (`0x000520dc`) assigning **2** at native `Enemy+0x838`. Stage 11 also corrected Worm's native **1** at the same field. The complete nonzero ordinary-pool reward list is therefore:

| Type | Class | Reward |
|---:|---|---:|
| 9 | Maid3 | 2 |
| 10 | Worm | 1 |
| 12 | Victor | 8 |
| 13 | BullyDay5 | 30 |
| 15 | Sam | 10 |

Maid, Maid2, FatMaid, Bully, and BullyDay3 retain base zero. Day3's custom `Die` increments kill count directly without calling the common reward helper; this is equivalent for its native zero reward, but its other death differences are significant. Day5 calls common death after terminating a thrown saw, so its 30-point reward is retained. Tests kill Maid3 and Day5 through their actual damage/death paths and check experience, kill count, and reward uniqueness.

## Resource tables

All entries below are indices into the fixed 260-name animation and 143-slot sound tables.

| Class | Six common animation slots | Extra animations |
|---|---|---|
| Maid | 87–92 | none |
| Maid2 | 130–135 | none |
| Maid3 | 184–189 | none |
| FatMaid | 122,123,124,125,128,129 | throw 126 (`r_pangnvyong_atk_b`), projectile 127 (`ow_shuiguo_a`) |
| Bully | 93,94,95,96,97,98 | none |
| BullyDay3 | same as Bully | 99–109 |
| BullyDay5 | 213,214,214,216,217,218 | 219,220,222,221 in native field order |

The Bully common slots deliberately use `r_bully_atk_b` and `r_bully_hurt_a` in slots 4 and 5. FatMaid's slots 4 and 5 are the native empty entries 128 and 129, not animation indices 126 and 127.

Common sounds are Maid 0/33–37, Maid2 0/71–75, Maid3 0/77–81, FatMaid 0/64–68 plus projectile 69, and Bully 0/39–43. Day3 adds 119–127. Day5 uses 128–133 in its common slots and 134,135,121,122,136,137,138,139,140 in native extra-field order.

The sound-table audit corrected a stage-1 ordering error: **Victor occupies 110–118 and the Day3 Bully block occupies 119–127**. The former portable table had those blocks reversed. The native missing assets `c_bully_attack_a` and `ow_dianju_run_a` remain null resources; no replacement is invented.

BullyDay5 also owns an `ElectricSaw` with animation indices 223,224,224,225,226,227,228,229,230,231,232. These correspond to native saw offsets `0x90`–`0xb8`; the portable `BullySaw` subclass retains the recovered `ElectricSaw_Update` collision expansion.

## Maid variants

Maid and Maid2 have no behavioral override beyond resource initialization. Maid3 adds:

- `Maid3_Spawn` (`0x00052570`): call common spawn, then set chase speed to `0x1999 + Random()%0x199a` every spawn. This is chase speed at `0x7e8`, not perception range.
- `Maid3_UpdateDeath` (`0x00052644`): after common death, each finished animation subtracts `0x1999` from position Y and clears `PhysicSlideMovement`; countdown zero removes the enemy through its manager immediately.

## Bully melee volume

`Bully` adds signed 32-bit second-window start/end fields at `0x83c`/`0x840`, initialized to 23/25. `Bully_TestAttackHit` (`0x00081f68`) accepts frames 11–14 and 23–25. Entering 23 clears the hit latch, allowing a second hit in the same attack.

Unlike the common forward segment, Bully-family hit tests construct a bounds box from the enemy position and a forward endpoint at `hitDistance`. They expand by half the local height and half the local X width along the object's X axis. Bully adds `fixedOne` to that half-width; Day3 and Day5 do not. The portable `Enemy::TestAttackHitVolume` preserves this geometry and the latch reset.

## FatMaid layout and behavior

| Offset | Field |
|---:|---|
| `0x83c` | Throw cooldown; reused as state-6 elapsed time. |
| `0x840` | Projectile state: inactive 0, flying 1, ending 2. |
| `0x844` | Maximum throwing distance, `0x140000` (20.0). |
| `0x848` | Projectile start. |
| `0x854` | Projectile target. |
| `0x860` | Projectile velocity. |
| `0x86c` | Projectile animation-finished result. |
| `0x870` | Owned projectile `Object*`. |
| `0x874` | Projectile animation 127. |
| `0x878` | Throw animation 126. |
| `0x87c` | Projectile/impact sound 69. |

`FatMaid_LoadResources` (`0x00092454`) creates the independent `ow_shuiguo_a` object at cache level 2, loads materials, selects transform mode 0, and disables lighting. It is submitted to queue 3 only while active and advances in `PostUpdate`.

`FatMaid_UpdateChase` (`0x000925b0`) preserves the common attack/cooldown and navigation behavior but adds a ranged throw. Throwing requires distance at least `hitDistance*1.5` and at most 20.0, plus a clear trace from the projectile start to the player. A random value above 400/1000 starts state 6; otherwise the cooldown is `12 + ((Random()%3072)>>8)`. Stopping or StayBox attachment stops motion, as in the native combined word test.

`FatMaid_UpdateThrow` (`0x0009292c`) tracks the player through elapsed 16 and releases at 22. The start is enemy position minus projectile Z times `0x23333`, plus Y times `0x1cccc`, plus X times `fixedOne`. Initial velocity is `normalize(target-start)*0x2cccc`, with Y cleared before normalization.

`FatMaid_UpdateProjectile` (`0x00092f4c`) moves before collision testing, multiplies all velocity axes by `0xf333`, then subtracts `0x1999` from Y. A player hit uses attack power 15 for that call and restores 25. BSP impacts choose either blood texture with the native `<=555` threshold and independent `<=500` flips, then blend over a four-unit velocity segment. Bucket-0 collision excludes the owner; bucket 2 is also tested. Impact animation speed becomes 300, and state 2 ends only after the projectile animation finishes.

`FatMaid_Damage` (`0x000934c4`) is an original empty method and `FatMaid_ReceivePlayerAttack` (`0x000934a4`) always returns false. These are intentional native behavior, not unfinished hooks.

## BullyDay3 layout and encounter

| Offset | Field |
|---:|---|
| `0x83c`–`0x864` | Eleven extra animations, indices 99–109. |
| `0x868`–`0x888` | Nine extra sounds, indices 119–127. |
| `0x88c` | Encounter mode. |
| `0x890` | Attack kind. |
| `0x894` | Charge phase. |
| `0x898` | Charge target. |
| `0x8a4` | Owned QTE `VisUnit`. |
| `0x8a8` | QTE phase. |
| `0x8ac` | Successful QTE round count. |
| `0x8b0` | Saved player position. |
| `0x8bc` | QTE outcome. |
| `0x8c0`/`0x8c4` | Ordinary second hit window, 23/25. |
| `0x8c8`/`0x8cc` | Alternate hit window, 10/14. |

Spawn (`0x0008c37c`) creates a radius-2 fake shadow, loads cache-3 `r_bully_a`, applies `dj_bully_b`, prepares the player QTE resources, and creates `oszv_anjian_a` with intensity `0x2666` and two material intensity bounces. The native resource helper `Player_LoadQteAnimations` (`0x0003af98`) fills all four type-2 player actions 12–15 from `r_ken_qte_a`–`d` and their sword counterparts; portable `Player::LoadBullyQteAnimations` implements this separately from Day5's special helper.

`BullyDay3_UpdateIdle` (`0x0008c854`) uses random thresholds 6500/4500/1000 in mode 0 and 3000/0/0 in mode 1 to select charge, attack, chase/delay, or idle delay. `BeginAttack` (`0x0008cc00`) uses `Random()%10000 < 5001`: ordinary slot 3 with 20 ATK, or extra animation 99 with 25 ATK. Ordinary attacks use both native windows; the alternate attack uses only 10–14.

`BullyDay3_ReceivePlayerAttack` (`0x0008d230`) applies real player damage and blood first. Without event 1002, QTE starts at health at most 100 in mode 0 or below 1 in mode 1. With event 1002, it starts the terminal QTE phase at health below 1. Early charge and QTE suppress the ordinary hurt transition. Other hits select hurt versus knockback using `Random()%1000` against the current knockback chance; the per-frame update sets 700 in mode 0 and 1000 in mode 1.

State 6 (`0x0008d86c`) is a four-phase charge: `rush_a` windup, looping `rush_b` charge, `rush_c` impact, and `rush_d` recovery. The target is traced and adjusted by plane normal times `0x28000` on a partial collision. Charge adds forward velocity `0x4ccc`, tests an expanded player bounds, and calls `Player_DamageHurt2`. Impact uses sound 122 and camera shake 12/`0x8000`. Mode 1 uses faster initial velocity, animation speed 600, and 20 recovery frames; mode 0 uses 40.

State 7 (`0x0008e084`) implements QTE phases 0–7: initial `yundao_b`, looping `qte_a`, proximity acceptance within four units, fades, `qte_b` relocation, looping `qte_c`, player QTE input, and `qte_d` result animation. Two successful rounds are required. Failure restores health to 150 in mode 0 or 50 in mode 1; the first success changes to mode 1, and a later terminal success calls the custom death method.

`BullyDay3_Die` (`0x0008ed8c`) increments kill count, stops sound, selects secondary-stream rendering, clears the animation, and enters dead state 16 immediately. It does not run the common countdown or clear the linked spawn record. Player collision is excluded only during charge phase 1. Pause/navigation calls below state 15 go directly to `Creature`, bypassing common Enemy side effects.

## BullyDay5 layout and saw encounter

| Offset | Field |
|---:|---|
| `0x83c`–`0x848` | Four extra animations in native order 219,220,222,221. |
| `0x84c`–`0x86c` | Nine extra sounds in the order listed above. |
| `0x870` | Charge phase. |
| `0x874` | Slam phase. |
| `0x878` | Throw phase. |
| `0x87c`, `0x880` | Charge distance 15.0 and slam range 40.0. |
| `0x884` | Saw state, initially 2. |
| `0x888` | Owned `ElectricSaw*`. |
| `0x88c` | Pending saw-impact sound flag. |
| `0x890` | Throw target. |
| `0x89c` | Throw speed, `0x38000` (3.5). |
| `0x8a0` | Throw velocity. |
| `0x8ac` | Throw timer. |
| `0x8b0` | Slam cooldown, initialized/reset to 220. |

Spawn (`0x00023d8c`) loads cache-3 `r_bullyx_a`, creates a radius-2 fake shadow, resets charge phase, positions the saw, selects special player resources by event 1002, and sets state time 16 and slam cooldown 220. It deliberately does not reset saw state or throw phase. `Player_SelectSpecialAnimation` (`0x0003b1dc`) loads animation index 36 in both event branches and copies each sword type's stand resource into special action 12; the portable alternate branch was corrected accordingly.

Idle (`0x000240b4`) and chase (`0x000244e8`) select ordinary attacks, throws beyond 15.0, and slams within 40.0 under their native random, hurt4, cooldown, and `SelectAttack` conditions. A missing saw forces charge state 7. The charge-distance random threshold is 8000 in ordinary idle, 6500 when hurt4 is requested, and 8500 during chase.

State 6 (`0x0002558c`) starts `rengdianju`, releases the saw after its own throw animation finishes, selects the saw's second throwing loop, and computes velocity toward the captured player position. The timer begins at 10 but is 9 on return because the same entry performs a thrown-saw update. State 7 (`0x00024b08`) charges toward the saw and tests player impact, saw/player overlap, squared velocity versus squared remaining distance, ground collision, and unit collision in native order. Ground and unit collision are independent branches and can both present their effects in one update.

State 8 (`0x000251c0`) plays saw-start and selects `jiandianju` on enemy and saw, then restores saw state 2 after the enemy animation finishes. State 9 (`0x000252e0`) selects `zhuang` and the saw's death resource; animation time `0xf3c` triggers sound, camera shake 12/`0x28f`, and player hurt4.

`BullyDay5_IsSegmentInside` (`0x0002544c`) applies during throw phase 2 when player weapon is above 2 and attack phase is 2. A segment hit on the saw sets the pending-impact flag. The next thrown-saw update consumes it, plays the fall sound, and starts the ending animation. The common segment query is still evaluated afterward.

`BullyDay5_ReceivePlayerAttack` (`0x00024838`) terminates a throw before common death at zero health. Nonfatal damage triggers blood and enters hurt state 4 with `knockbackPending=1`, except during charge phase 0 or slam. Player collision is excluded during charge phase 2, and pause/navigation below state 15 goes directly to `Creature`.

## Verification and Ghidra status

`test_game_enemy.cpp` retains the common stage-9 coverage and adds six stage-10 groups: dynamic types/native capacities and real resources; Maid3 random speed/death and Bully's two hits; FatMaid throw/immunity/projectile impact; Day3 attack/charge/QTE success/failure/death; Day5 throw/charge/pickup/slam/segment/death; and all seven classes through the real map-2008 engine frame loop. `test_game_data.cpp` checks the corrected Victor/Bully sound-block anchors.

The default and host-math suites each pass all 36 tests, and the isolated ASan/UBSan suite passes all 29 tests. Stack probes stop positional sounds before destruction; the production scene already stops sound during level/scene teardown.

Ghidra contains the seven exact structures, 61 typed method prototypes plus seven typed constructors, 96 functions tagged `game,enemy,stage10`, behavior comments, and the corrected common reward comment. The saved `ElectricSawLayout`, `Resource`, and `AudioData` types make the saw and sound-pointer fields concrete. Function names replace the former `*_vfunc_*` and `FUN_*` placeholders with the recovered virtual/helper names listed above.

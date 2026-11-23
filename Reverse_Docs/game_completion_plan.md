# 7 Days game completion inventory and staged plan

## Purpose and completion contract

This plan covers the game-specific work deliberately postponed by `engine_completion_plan.md`, plus the additional game, content, persistence, UI, and license paths found during the post-engine inventory. The detailed ARM-derived evidence is in `symbian3rd_game.md`.

The goal is a portable, feature-complete 7 Days: Salvation implementation using the reconstructed Soft3D engine—not merely a menu shell, a map viewer, or a collection of parsers. Completion requires all seven days to be playable through their shipped maps and scripts, with concrete gameplay behavior, full save/load, progression, UI, and a documented platform policy for the original license services.

Every stage must include:

- ARM analysis with saved Ghidra function/variable names, prototypes, structures, comments, and tags where behavior is recovered;
- updates to `symbian3rd_game.md` and any more specific affected format document;
- directed synthetic tests and real-resource tests in `Reversed_Game/tests`, using the existing `test_*.cpp` style;
- the normal CTest suite, plus relevant affected variant/sanitizer suites;
- an end-to-end check through the real engine frame loop rather than only construction or parsing.

A stage is not complete if its new behavior remains behind an empty hook, silently returns a default, or is covered only by a test double. Test doubles may isolate a stage, but the stage must also wire the real implementation into the application path it will use.

## Evidence and preservation rules

1. **Use ARM reconstruction as the sole behavioral authority.** `Leaked_Source/commandlist.txt`, leaked episode sources, and `Tools/SimpleScriptDecompiler/bin/Debug/names/*` are stale early-build material. Do not use them to resolve command IDs, signatures, state layouts, or behavior. Preserve them unchanged, but do not cite them as implementation evidence.
2. Use shipped SST/SBP/SDT/SOJ/SAI/STX/SBN/SAU resources as fixtures and coverage evidence, not as a substitute for ARM semantics.
3. Preserve original update order, fixed-point arithmetic, random/timing rules, resource lifetimes, state transitions, and observable bugs. Add safety checks for malformed data without changing valid behavior.
4. Distinguish missing portable behavior from an original empty method. `CalcRank` (`0x000669fc`) and `BspUnit`'s empty update methods must remain no-ops. Do not fill original stubs with invented features.
5. Do not make unimplemented `GameSI` commands silent no-ops. During development, unknown handlers must fail loudly in tests and logs; only ARM-confirmed no-ops may do nothing.
6. Do not turn license/verification checks into unconditional success and call that parity. Recover the state machine and protocol, then make any offline/test policy explicit and injectable.
7. Keep new game code out of the reusable engine. Engine changes must be consumer-driven and generic.

## Source and integration layout

Create a separate `seven_days_game` static library under `Reversed_Game/src/game`, linked against `soft3d`. The application and game tests link that library; `soft3d` must not depend back on it.

Suggested responsibilities, split further when a file becomes unwieldy:

| Path | Contents |
|---|---|
| `src/game/Scene.*` | Concrete `SceneHost`, episode lifecycle, map/day transitions, environment audio, rendering order. |
| `src/game/GameSI.*` | Callback registry/dispatch and game script commands, grouped by subsystem. |
| `src/game/GameData.*` | `itemdata`, `gameitem`, `einfo`, resource-name tables, content ownership. |
| `src/game/CameraController.*` | Camera modes, binding, interpolation, SPL playback, shake, trigger integration. |
| `src/game/Player.*`, `PlayerStates.*`, `Sword.*` | Player, 20 states, combat, QTE, sword/gunfire bindings. |
| `src/game/Things.*`, `MapUnits.*` | Thing descendants, `BspUnit`, `ElectricSaw`, map entity factory behavior. |
| `src/game/Triggers.*` | Interaction/camera/light triggers, `StayBox`, and `Trap`. |
| `src/game/Enemy.*`, `Enemies/` | Common enemy AI, 17 ordinary pool variants, and the separately constructed `Boss`. |
| `src/game/GameUiController.*` | Concrete HUD/map/about/item operations behind existing `GameUIDlg`/`AboutDlg` hooks. |
| `src/game/SaveGame.*` | Full slot and sidecar schemas, typed unit reconstruction, validation. |
| `src/game/License.*` | Portable license state machine and message/domain consumers. |
| `src/platform/` | License/network/device/vibration interfaces and Linux/SDL/dummy backends; no episode logic. |

Keep the existing `GameEngine`, `SceneHost`, `GameplayHooks`, and `ScriptInterface` boundaries. `src/app/Application.cpp` must construct and install the concrete `game::Scene`, `game::GameSI`, and UI hooks. Narrow generic accessors may be added to `soft3d` when needed, but concrete classes must not be moved there. Existing game-facing files may remain in place to avoid churn; all new gameplay implementations go to `src/game`.

## Inventory summary

The remaining surface includes:

- `Scene`, its day/map/door/game-over/authorization state machines, environment audio, and `CameraController`;
- `GameSI` with 218 callback records for IDs 43–260;
- `Player`, `Sword`, and all 20 player states;
- `MyGameUnitMgr`, `BspUnit`, `ElectricSaw`, `StayBox`, and `Trap`;
- `Breakable`, `Pushable`, and `Pickable`;
- nine trigger classes: `EventTrigger`, `PickTrigger`, `DoorTrigger`, `ActTrigger`, `HideTrigger`, `ItemTrigger`, `TalkTrigger`, `CameraTrigger`, and `LightTrigger`;
- `Enemy`, 17 ordinary pool classes, and the separate `Boss`: `Bully`, `BullyDay3`, `BullyDay5`, `Boss`, `DarkKen`, `FatMaid`, `Ghost`, `Kate`, `Lamper`, `Lord`, `Maid`, `Maid2`, `Maid3`, `Prisoner`, `Sam`, `Victor`, `Weed`, and `Worm`;
- complete GameUI/HUD/map/dialog/QTE/inventory/about-model behavior, resource tables, statistics, and unlock progression;
- `itemdata.sdt`, seven `gameitem` files, five `einfo` files, and license `message.sdt`/`domain.sdt` consumers;
- full `slot%d.sav` and `spt\\slot%d.sav` state, including every typed unit record;
- license/network/device services and vibration;
- all currently empty application, scene, entity-factory, HUD/map/about/item hooks.

The shipped 217 SST files exercise 162 game callback IDs. The other 53 game callbacks and IDs 258–260 need synthetic coverage; “all scripts load” is not enough.

## Stage 1 — Game data, content ownership, and resource tables

**Scope**

- Implement `itemdata.sdt` in `src/game/GameData.*`: 67 52-byte item records and 65 12-byte text records, with checked ID mapping and the 0x264-byte runtime consumers identified from ARM.
- Implement all seven `gameitem` tables with eight-byte `{int32 id,int32 count}` group headers (truncated to 16 bits at runtime) and 0x60-byte records; implement all five `einfo` tables with nested 0x1c-byte records and the 0x24-byte runtime entries.
- Recover record field meanings from consumers rather than labeling raw words by position alone.
- Populate the existing item/model/document/photo storage and connect unlock flags without duplicating ownership.
- Recover `InitializeAnimationResourceNames` (`0x00089238`), `InitializeSoundResourceNames` (`0x00088594`, formerly `InitializeUiResourceNames`), and the additional sound/model/UI tables needed by later stages.
- Keep `message.sdt` and `domain.sdt` out of the item parser; stage 14 handles their license consumers.

**Integration**

`MyGameUnitMgr_LoadDayContent` (`0x00043de4`) selects `einfo` and `gameitem` by day. Implement that day lifecycle and valid missing-file behavior for days 6–7 `einfo`. Connect loaded content to `GameContent`, `MyGameUnitMgr`, `GameUIDlg`, and `AboutDlg`.

**Verification**

- Check exact shipped counts: 67 items, 65 text records, gameitem groups 9/13/13/18/14/4/3, and einfo groups 0/4/9/1/5.
- Test truncation, bad counts/IDs, nested allocation overflow, missing files, and reload/destruction.
- Prove item descriptions, model names, enemy/thing spawn records, and unlock arrays contain real data in the application path.
- Run all existing tests and add `test_game_data.cpp` with real and malformed resources.

**Status: complete.** `seven_days_game` implements the checked parsers, all fixed resource tables, and `MyGameUnitMgr` ownership; the SDL new-game path loads day 1. Ghidra contains the recovered structures, names, prototypes, locals, comments, and tags. All 28 default tests pass, and core/logging/math/WAR/ASan-UBSan variants each pass 22 tests. Detailed layouts and any intentional malformed-input checks are in `game_data_tables.md`.

## Stage 2 — Concrete scene, `GameSI` binding, and application startup

**Scope**

- Implement `game::Scene` as the concrete `SceneHost`, owning the BSP, player, `MyGameUnitMgr`, camera, shaders, transition state, and environment channels.
- Implement `game::GameSI` and its 218-entry dispatch table from `GameSI_Ctor`/`GameSI_ExecuteCommand`; bind the full context as `GameEngine_BindSceneInterface` (`0x00070464`) does.
- Implement scene construction/destruction, engine propagation, day/SBN initialization, SST selection, incremental SBP loading, day-1 fallback, and render/update ordering.
- Install the real scene, script interface, and UI hooks in `Application::InitializeVideo`.
- Add explicit development errors for unimplemented commands; do not let scripts advance silently through missing behavior.

**Integration**

Use existing `SetSceneHost`, `SetScriptInterface`, `StartNewGame`, `LoadSaveSlot`, `ReturnToMainMenu`, and `RunFrame`. Implement `Scene_LoadDay`, `Scene_LoadMap`, `Scene_LoadNextLevel`, `Scene_BeginMapLoad`, `Scene_FinishMapLoad`, and `Scene_UnloadLevel` before adding higher-level commands.

**Verification**

- Start a real new game through the SDL/dummy application, select `day1/ep0`, and incrementally reach map progress 100 with the real SBP factory bound.
- Verify frame order: script/scene pre-update, unit update, post-update, BSP/player/unit submission, UI overlay, presentation.
- Verify return to menu and repeated scene creation/destruction without stale pointers or cache corruption.
- Add `test_game_scene.cpp` and application smoke coverage; all prior menu tests must remain unchanged.

**Status: complete.** `game::Scene` and `game::GameSI` are installed by the real application. The scene implements the day intro, day/SBN/content setup, episode selection, incremental SBP loading and day-1 fallback, load finalization, update/render ordering, and menu teardown. `GameSI` preserves all 218 ARM addresses/context pointers, implements IDs 53–59 plus the ARM-confirmed empty/shared-word callbacks, and throws `GameSIError` for later-stage handlers. At stage-2 completion the player used the generic `Creature` base; stage 4 has since replaced it with the concrete `Player`. Camera behavior was completed in stage 3, while the remaining game entity behaviors stay assigned to stages 5–7.

The real `day1/ep0` path reaches map 2008/progress 100 in 26 SBP calls, including through the SDL `--new-game` application test. Tests also cover the 100→103→-1 finalization, day-2 map-1001 fallback, real null entity records, repeated menu/new-game teardown, scene commands, and the explicit error at later-stage ID 121. Stage 3 subsequently advances the same real script to ID 221; the current counts and sanitizer results are recorded there. Earlier unrelated existing UBSan findings remain documented rather than hidden.

## Stage 3 — Camera, transitions, environment audio, and scene presentation

**Scope**

- Recover the game `CameraController`: all eight modes, fixed/curve/follow targets, position/look-at, binding to player/enemy, unbinding, interpolation speed, shake, freeze, and completion.
- Implement `CameraTrigger` interaction with the controller and SPL paths.
- Implement spawn position/direction, next-level parameters, the 22 door/cinematic model and sound records, fades, camera restoration, day titles, and game-over/Ink/restart/menu flows.
- Implement two randomized environment-audio channels, music changes, listener selection, and all six presentation-shader selections used by scene commands.
- Implement the corresponding camera/audio/scene `GameSI` commands and SBP fog/light/header consumers.

**Integration**

Use existing `CameraPathCollection`, `Camera`, `CameraSpot`, `Bsp` lightstyles/fog, `PresentationShaders`, and `AudioManager`; do not add a second camera or music system.

**Verification**

- Directed tests for every camera mode, binding/freeze/interpolation, path completion, shake, and restoration.
- Exact transition-frame tests for door models, fades, titles, game-over, and listener/environment scheduling with deterministic random/time sources.
- Real map/SPL/audio fixtures and dummy-video output; no invented transition timings.

**Status: complete.** `game::CameraController` implements all eight modes, binding, interpolation, direct-follow, shake, freeze, trigger restoration, collision recovery, and SPL completion/projection. `CameraTrigger` and `LightTrigger` consume real SBP records, while type-102 words are preserved without inventing a semantic name. The scene implements the 22 door resources, exact transition frames, spawn/direction, fades, ending-specific day titles, game-over/Ink/restart/menu behavior, both environment channels, listener selection, and all six presentation-shader paths. Corresponding `GameSI` handlers are implemented, including IDs 117 and 121 needed by the real startup script.

At stage-3 completion, the real `day1/ep0` reached later-stage command 221; after stage 5 it reaches stage-6 command 159. `test_game_camera.cpp` covers every mode, binding/freeze/interpolation, timed/projected paths, deterministic shake, and nested trigger restoration; `test_game_scene.cpp` covers commands, real map trigger counts/type-102 values, door frames, environment/listener scheduling, ending-title hashes, and game-over restart. The stage-3 run passed all 31 default, logging, and math tests and all 24 core, WAR, and ASan-UBSan tests. Recovered layouts, 96 function names, 100 typed signatures, comments, and `stage3` tags are saved in Ghidra; details are in `symbian3rd_camera.md`.

## Stage 4 — Player, sword, and all 20 player states

**Scope**

- Implement `Player` over generic `Creature`/`StateMachine`/`CameraSpot`, including resources, movement, visibility/clipping, direction, pause/input masks, HP/ATK, animation, teleport/navigation, attack, hiding, pushing, QTE, used-item state, and death.
- Implement `CStand`, `CRun`, nine attack states, four hurt states, `CDie`, `CPush`, `CQTE`, `CHide`, and `CFPS`, including all 80 enter/exit/update/registration virtual bodies and chain transitions.
- Implement `Sword` and player-owned `GunFire`, model/type selection, attack windows, hit tests, sounds, and enemy interaction.
- Implement all player `GameSI` commands, including the player-related IDs outside the stale command lists.
- Connect player state to GameUI, enemy AI, triggers, and camera listener/spot behavior.

**Integration**

Reuse `PhysicUnit` movement/collision, `VisUnit` animation/audio, generic state templates, `Effects`, and renderer queues. Preserve `Player_SaveState`/`Player_LoadState` fields for stage 13.

**Verification**

- Directed transition tests for every state and every virtual slot, attack timing/hit windows, QTE input/results, pushing/hiding/FPS, and death.
- Real player SOJ/SAI/audio resources and exact dummy frames for representative stand/run/attack/hurt/QTE states.
- Exercise player-controlled movement and combat through engine input, not only direct method calls.

**Status: complete.** `game::Player` replaces the generic scene creature, and `PlayerStateSet` implements all 20 states and all 80 enter/exit/update/registration virtual bodies. `Sword`, player-owned `GunFire`, melee/gun target selection, damage and hurt/death variants, pushing, QTE, hide/FPS, inventory snapshots, and player save fields are implemented. All player commands in IDs 117–148 and 255–257 are bound to their own recovered handlers. `test_game_player.cpp` covers every state, attack chains and hit windows, QTE results, real resources, save round trips, and real map-2008 movement/combat through engine input; exact stand/run/attack/hurt/QTE frame hashes are recorded in `symbian3rd_player.md`.

At stage-4 completion, all 32 default tests and all 25 core, logging, math, WAR, and ASan-UBSan tests passed. Ghidra contains 187 recovered function renames, typed `Player` (0xf44) and `Sword` (0x27c) layouts, typed signatures, field names, comments, `stage4` tags, and a saved completion property. Concrete enemies, final UI rendering, and full scene persistence remain with their assigned later stages. Stage 5 has since completed pushable and thing/map commands; the real startup script now reaches stage-6 command 159.

## Stage 5 — Things and map-unit implementations

**Scope**

- Implement `Breakable`, `Pushable`, and `Pickable`: setup, collision/query policy, model/material/animation, hit/destruction, pushing, pickup/drop, sound, transform/rotation, movement, lighting, and depth bias.
- Implement `BspUnit_LoadEntity`/`BspUnit_Submit` and complete the remaining concrete SBP factory work: map models and lensflare entities. Stage-3 camera/light triggers and type-102 storage are already implemented; add only their missing command/persistence consumers.
- Implement `ElectricSaw`, `StayBox`, and `Trap`, including update and persistence behavior.
- Implement thing/map `GameSI` commands, including the separate later thing-command family in IDs 229–257, each from its own ARM body.

**Integration**

Connect the real `BspEntityFactory` to `MyGameUnitMgr_EntityFactory` (`0x00043ed8`) and preserve object ownership across `Bsp::FreeLastLevel`. Reuse `Things` destruction, `PhysicUnit`, `Lensflare`, and render queues.

**Verification**

- Every AddThing variant and every later thing command, including break/push/pickup, movement, collision, animation, rotation, magic-cube orientation, and inventory drop.
- Real SBP entity creation, model animation/transparency queues, light/fog entities, and repeated map unload/reload.
- Synthetic collision/push/pickup fixtures plus real map models.

**Status: complete.** `src/game/MapUnits.*` implements all three `Things` descendants, `BspUnit`, `ElectricSaw`, `Trap`, and `StayBox`; the manager constructs every AddThing variant, traps, and up to 16 queue-6 lensflares. `GameSI` implements IDs 219–254 from their individual ARM bodies. The shared `Things` persistence extension now preserves alternate resources, destruction state, and the native `0x67c` word separately from the transient leaf pointer. Stage 9 supplies the concrete StayBox enemy-pool host; table-driven pickup-trigger creation belongs to stage 6 rather than a generic substitute.

`test_game_mapunits.cpp` covers every command, all AddThing variants, break/push/pickup behavior, movement/rotation/orientation, destruction, trap and StayBox updates, persistence, and real maps 2008/2009. Map 2008 creates 15 concrete map-model units; map 2009 submits its real lensflare. The real `day1/ep0` path now reaches stage-6 command 159. All 33 default tests pass, and core/logging/math/WAR/ASan-UBSan variants each pass 26 tests. Native layouts, 53 class/helper prototypes, all 36 command prototypes, comments, 90 stage-5 tags, and a completion property are saved in Ghidra. Detailed semantics are in `symbian3rd_mapunits.md`.

## Stage 6 — Event, pick, door, and act triggers

**Scope**

- Implement `EventTrigger`, `PickTrigger`, `DoorTrigger`, and `ActTrigger`: enable/count state, proximity, interaction prompts, dialogs/choices, item requirements, selection results, map/door transitions, and persistence.
- Implement their `MyGameUnitMgr_Add*` helpers and `GameSI` configuration/query/enable/remove commands.
- Connect prompt and choice UI to the real GameUI implementation in stage 8, using a narrow interface until then rather than bypassing interaction state.

**Verification**

- Boundary entry/exit, repeated/infinite/single triggering, prompt timing, accept/cancel, selection get/clear, and removal.
- Door item present/missing/locked cases, next map/episode/spawn/direction, and special door state transitions.
- ARM-confirmed IDs such as AddEvent 149, AddPick 153, AddDoor 155, and AddAct 159, with synthetic tests for unused related commands.

**Status: complete.** `src/game/Triggers.*` implements all four classes and their persistence; `MyGameUnitMgr` implements their Add helpers, the 1024-entry door-state block, and new-map gameitem spawning; `GameSI` implements IDs 149–165. `TriggerUiHost`/`InteractionState` preserves prompt, dialog, pause, and selection state without inventing stage-8 rendering. The recovered pick argument is a dialog title text ID, not a sound. Act has three option strings, a separate title at `0x6c`, and a command/persistence-only word at `0x5c`.

`test_game_triggers.cpp` covers every command, entry/exit and finite-event latching, pickup accept/cancel/full/disable flows, door requirements/tracked states/eight-frame unlock/authorization request, act simple/choice/combat/camera behavior, and second-save bytes. Real map 2008 creates its item-1005 pair with title 26 and clears the spawn record on pickup; restored saves skip spawning. The real `day1/ep0` passes all stage-6 commands; after stages 7–8 it reaches later-stage input command 47 (`0x00065560`). All 34 default tests pass; core/WAR/ASan-UBSan pass 27, and logging/math pass 34. Four corrected native layouts, typed signatures, comments, 69 `stage6` tags, a completion property, and a bookmark are saved in Ghidra. Details are in `symbian3rd_triggers.md`.

## Stage 7 — Hide, item, talk, camera, and light triggers

**Scope**

- Implement `HideTrigger`, `ItemTrigger`, and `TalkTrigger`, including mask/hide points, linked items, talker/camera positions, timed dialogue, completion, enable/disable, and save fields. Complete `LightTrigger` command/save integration; the stage-3 regional consumer already exists. Extend the stage-3 `CameraTrigger` only where script commands or persistence require more work—do not replace its working controller, mode, SPL, or restoration logic.
- Complete `StayBox` and `Trap` command/integration details left from stage 5.
- Implement all corresponding `GameSI` handlers and removal paths.

**Verification**

- Hide enter/exit and player visibility/mask state; item-link capacity and acquisition; multi-line talk timing/completion/camera restoration.
- Preserve the stage-3 tests for every camera mode and SPL completion/freeze path; add only new command/persistence cases. Verify light region/style state.
- Save-field fixtures are prepared here but only end-to-end verified after stage 13.

**Status: complete.** `src/game/Triggers.*` implements Hide, Item, and Talk, including confirmation/combat gates, eight-item use zones, timed multi-line dialogue, actor/camera setup, both completion paths, and native persistence. `GameSI` implements IDs 166–179 and StayBox IDs 205–207; Trap commands 219–220 were already complete. Light save/load is implemented, while CameraTrigger retains its stage-3 controller and adds regression persistence coverage.

The manager preserves Talk's pre-pass/double-update behavior and paused Item registration. Typed Ghidra review corrected Hide's serialized fields and identified Act/Talk camera restoration as a camera-warmup reset, not listener selection. StayBox constructors now use `enemyOutside=0` and `spawned=1`. Tests cover every new command, interaction transitions, item capacity/use, both Talk completion modes, second-save bytes, and real map-2008 item use through `GameEngine::RunFrame`. All 34 default tests pass serially; core, WAR, and ASan/UBSan each pass 27, while STDIO logging and host-math each pass 34. Detailed semantics are in `symbian3rd_triggers.md` and `symbian3rd_mapunits.md`.

## Stage 8 — GameUI, HUD, dialogs, QTE, inventory, maps, and extras

**Scope**

- Replace every remaining empty `renderGameHud`, `renderMap`, `renderAboutModel`, `useItem`, and `combineItems` hook with concrete game code.
- Implement normal HUD/status, interaction/tutorial/subtitle/story/door displays, portrait/simple/choice dialogs, selection results, QTE rendering/input/results, and game-over UI.
- Implement item acquisition/use/combination/descriptions/effects, nine-slot inventory, notes/documents, map floors/markers, and item-driven unlocks.
- Implement about-model rendering and model/document/photo unlock progression.
- Recover statistics counters and direct-map opening; preserve `CalcRank` as the original no-op.
- Complete fixed resource-name and sound-index consumers from stage 1.

**Integration**

Extend the existing `GameUIDlg`, `AboutDlg`, and `HelpDlg` through their current hooks and `GameContent`; do not build a parallel UI system or duplicate SDT rendering.

**Verification**

- Exact dummy pixels for each HUD/dialog/QTE/map/inventory/extras mode and orientation.
- Every item use/combine result, empty/invalid slot, close/reopen behavior, unlock persistence, and map marker.
- End-to-end item pickup → inventory → use/combine → player/world effect → save/unlock update.

**Status: complete.** `game::GameUiController` connects the existing `GameUIDlg`, `AboutDlg`, and `HelpDlg` to the concrete player, manager, scene, and content. HUD/prompt/QTE/hide/subtitle/dialog rendering, nine-slot inventory and files, item use/combination, maps/markers, model previews, statistics, and completion unlocks are implemented. The stage-8 save menu calls the real slot operation, whose full scene writer remains explicitly assigned to stage 13.

`test_game_ui.cpp` covers all recovered commands, item flows, real map pickup, progression, map arithmetic, and exact dummy-presented frames. The real startup now reaches later-stage input command 47. All 35 default, logging, math, and WAR tests pass serially; core and ASan/UBSan each pass 28. The native `GameUIDlgLayout` (0x524), eight-record custom-marker layout, map tables, function names, prototypes, comments, statistics fields, tags, bookmarks, and completion property are saved in Ghidra. Detailed behavior and corrections are in `symbian3rd_gameui.md`.

## Stage 9 — Common enemy AI, pooling, and enemy script commands

**Scope**

- Recover `Enemy` slots 28–49: resource setup, spawn/reset, perception, state updates, attack selection/hit tests, damage, death, and save/load.
- Implement shared idle/alert/chase/attack/hurt/death behavior, hearing/sight, navigation/stay boxes/waypoints, player notification, and statistics.
- Implement `MyGameUnitMgr` enemy pools, day resource preparation, type/ID lookup, pause/remove/update/submit behavior, and common enemy state serialization.
- Implement all enemy `GameSI` commands: add/configure/animate/HP/ATK/direction/move/follow/attack/remove/pause/stop/stay-box/waypoint/alert operations.

**Integration**

Use `Creature` navigation/hearing and the real player/BSP. All concrete enemies must share only ARM-shared behavior; no generic substitution for variant methods. The stage-9 pool adapter supplies common defaults/resources only and must be replaced by the real variant classes in stages 10–12.

**Verification**

- Directed tests for every common virtual slot and state transition, sight/hearing/path cases, damage/death, and pooled reuse.
- Every enemy command with valid/invalid IDs and types, plus common state round trips. Verify from the native loader that `0x26` reconstructs StayBox, not enemy; stage 13 subsequently completes full scene loading and concrete-pool enemy restoration.
- Real enemy resources, exact blood-splat lightmap pixels, and player interaction through the scene frame loop.

**Status: complete for common behavior.** `src/game/Enemy.*` implements slots 28–49, common helpers, AI states, combat, waypoints, StayBox return behavior, blood splats, and the four-word save extension. `MyGameUnitMgr` implements 17 pool rows, singleton selection, collision search, nondeleting reuse, day preparation, map spawning/snapshotting, and collision-unit commands. `GameSI` implements IDs 180–204 and 208–218. Stages 10–12 have since completed every concrete variant; the temporary common adapter has been removed.

`test_game_enemy.cpp` covers common AI/persistence, every new command, pools, real `einfo2` data, StayBox integration, and real map-2008 combat. `test_bsprender.cpp` verifies exact blood-splat palette/blend pixels, transparency, flips, clipping, and the axis-plane quirk. All 36 default, logging, math, and WAR tests pass serially; core and ASan/UBSan each pass 29. The 0x83c `Enemy` and 0x24 `EnemyWaypointNode` layouts, typed common signatures, native names/comments, and stage-9 tags are saved in Ghidra. See `symbian3rd_enemy.md`.

## Stage 10 — Maid and Bully families

**Scope**

Implement the seven related variants and their overrides:

- `Maid`, `Maid2`, `Maid3`, `FatMaid`;
- `Bully`, `BullyDay3`, `BullyDay5`.

Recover their constructor layouts, animations, perception, attacks, movement, damage/death, special state overrides, and day/resource differences.

**Verification**

- Per-class directed tests for each override, not only inherited-base tests.
- Real model/animation/audio checks for all seven and day-specific preparation.
- Scripted encounters covering each family’s distinct behavior and pooled capacity.

**Status: complete.** `src/game/MaidBully.*` implements all seven concrete classes and every recovered override, including FatMaid's fruit projectile, Bully's second hit window, Day3's charge/two-round QTE encounter and custom death, and Day5's owned saw, throw/charge/pickup/slam states, and segment-hit handling. Maid3's missed gun reward is corrected to 2; Day5's 30-point path is exercised through real player damage. The shared sound table now places Victor at 110–118 and Day3 Bully sounds at 119–127. Native missing resources remain null rather than receiving substitutes.

`test_game_enemy.cpp` adds six stage-10 groups covering dynamic types/capacities, all real model/animation/audio tables, exact state transitions, QTE success/failure, rewards, and all seven classes through the real map-2008 engine frame loop. All 36 default and host-math tests pass serially, and all 29 ASan/UBSan tests pass. Exact layouts, 61 typed method prototypes plus seven constructors, recovered helper names, behavior comments, 96 stage-10 functions, and a saved completion property are in Ghidra. Details are in `symbian3rd_maidbully.md`.

## Stage 11 — Worm, Weed, Lamper, and Ghost

**Scope**

Implement the four special creature classes, including their nonstandard movement, visibility/perception, attack/projectile/effect, animation, hurt/death, and custom virtual overrides.

**Verification**

- Per-class special-state and boundary tests with real assets and collision/path geometry.
- Verify `Worm`/`Weed` overrides such as slots 30/31/33/36 and `Ghost`/`Lamper` movement/state behavior from ARM rather than forcing the maid/bully model.
- Exercise their real episode encounters through SST and scene updates.

**Status: complete.** `src/game/SpecialEnemies.*` implements all four concrete classes and their native overrides. Worm has per-spawn random colour/scale/speed, attack-path checks, jump/landing combat, sinking death, and the corrected 1-point gun reward; Ghost has no rendering, player-attack immunity, Twist/proximity-audio behavior, and its separate index-50 sound; Lamper has custom spawn/perception, point hits, and unconditional death on any successful player attack; Weed has latched range perception and its modulus-1000, blood-free hit path. Stage 12 subsequently removed the common adapter by completing all remaining variants.

`test_game_enemy.cpp` adds seven groups covering types/resources, colour thresholds, special states and boundaries, damage/death, real `einfo` encounters, shipped SST `AddEnemy` commands, and all four classes through the real engine frame loop. Native map 1053 runs the three Worm spawns from `day5/ep1053`; Ghost's shipped `day3/ep1020` command runs on known-good map 2008. Day-1 map 1002 stalls at progress 56 and day-3 map 1020 fails with `S3D_ACCESS` in the existing BSP loader; these unrelated limitations are documented rather than hidden. All 36 default and 29 host-math tests pass serially, and all 29 ASan/UBSan tests pass. Four class layouts, the blood-life-aware layout, 45 typed/commented functions, stage-11 tags, and completion properties are saved in Ghidra. Details are in `symbian3rd_specialenemies.md`.

## Stage 12 — Victor, Sam, Prisoner, Lord, Kate, DarkKen, and Boss

**Scope**

Implement the six remaining named enemies and `Boss`, including unique construction, linked actor references, scripted/special attacks, animation/state overrides, and boss lifecycle. Do not omit `Boss` because it is absent from the ordinary pool-constructor loop.

**Verification**

- Per-class override and resource tests for all seven.
- Boss spawn/update/death and any linked-enemy behavior, with exact event/statistics effects.
- Full real encounters and transitions for these classes; no fallback to base `Enemy` where ARM provides an override.

**Status: complete.** `src/game/NamedEnemies.*`, `Lord.*`, and `Boss.*` implement every recovered override. Victor has randomized attacks and three-phase leap combat; Sam has constant perception/attack stubs and blend-mode disappearance; Prisoner has randomized idle resources, sinking death, and the corrected 1-point gun reward; Kate relays detection to the linked day-6 Bully. DarkKen implements the 16-region navigation graph, dual guns, and three-stage segment attack. Lord implements the complete four-crystal encounter, equipment stages, dialogs/cancellations, cameras/fades/control gates, broken weapons, corpse, quake, and final music. Boss implements the separate generic action/delay/random-delay controller and original empty slot-50 hook; it has no native constructor caller and was not added to the pool.

The common pool adapter is removed and every pool row constructs its concrete class. Seven stage-12 test groups cover all seven classes, real resources, Boss lifecycle/statistics, shipped SST commands, and real engine frames. Native maps 1051, 1060, and 2066 run Lord, Victor, and DarkKen. Existing BSP-loader failures on 1056, 2052, and 6004 are documented; those SST encounters run on known-good maps 1051/2008 rather than hiding the failures. All 36 default, 29 host-math, and 29 ASan/UBSan tests pass serially. Seven exact layouts, 114 typed/tagged functions, 46 behavior comments, three bookmarks, and completion properties are saved in Ghidra. Details are in `symbian3rd_namedenemies.md`.

## Stage 13 — Full save/load and deterministic persistence

**Scope**

- Implement the complete `slot%d.sav` writer and `Scene_LoadSave` reader after the 44-byte header: day/map, music, environment channels, both 0x2800 progression blocks, GameUI, player, manager/content, and typed units.
- Implement every unit extension and reconstruction switch type: 0, 4, `0x1b`–`0x1f`, `0x21`–`0x23`, `0x25`–`0x28`.
- Implement PickTrigger’s linked-thing ID, map-entity reconstruction, enemy spawn-table restoration, and `spt\\slot%d.sav` script archive integration. Do not invent a pooled-enemy case for record `0x26`, which reconstructs StayBox.
- Preserve stream endianness, wide strings, EOF handling, cache lifetimes, and original state-reset order. Add malformed-input checks without changing valid bytes.

**Integration**

Replace the header-only `LoadSaveSlot` path with the real scene load while retaining header display in `LoadDlg`. Ensure config unlocks and slot state remain distinct.

**Verification**

- Exact byte fixtures for every record and a full scene, truncation at every section, invalid type/link, missing sidecar, and reload/destruction.
- Save → load → second-save byte determinism, including task cursors/local values and environment state.
- Resume each concrete unit class and real maps, then continue gameplay rather than only comparing structures.

**Status: complete.** `src/game/SaveGame.cpp` implements the complete scene writer/reader, 44-byte headers, both progression blocks, environment/music/GameUI/player state, manager data tables, typed units, concrete pooled enemies, embedded StayBoxes, pickup links, and script sidecars. Deterministic reconstruction preserves bucket order and replays player visual state after native resets. The original writer/reader asymmetries for enemy tags `0x0a`–`0x1a` and StayBox `0x26` are documented as portable completion fixes rather than misattributed to the native switch. The slot-header word at `0x14` is a save number; play time is at `0x24`. `test_game_save.cpp` covers every reconstruction family, exact second-save body/sidecar bytes, malformed/truncated sections, invalid types/links, missing sidecars, and resumed real engine frames. At stage-13 completion, all 37 default tests passed; 30 SDL-independent tests passed in each ASan/UBSan, host-math, STDIO-logging, and WAR build. See `symbian3rd_savegame.md`.

## Stage 14 — License, network/device services, and vibration

**Scope**

- Recover `ClientSession`, `MyClientSession`, `SessionDevice`, `HttpSession`, `TCPSession`, `CSocketReader`, `CSocketWriter`, `CIMEI`, `CPhoneObserver`, and `CSmsSessionObserver` behavior needed by the game.
- Recover verification protocol, stream records, encryption/obfuscation, `com.dingoo.server.VerifyService`, `message.sdt`/`domain.sdt` consumers, and authorization success/failure/retry/gating.
- Implement `MainUIDlg` mode 4 and `Scene_DrawAuthorization`/`Scene_BeginAuthorization` behavior and user-visible text/progress states.
- Implement vibration behind a platform interface. Confirm SDL3 haptic capability before selecting the SDL backend; provide deterministic dummy behavior and an explicit unsupported-device result rather than pretending vibration occurred.
- Keep Symbian application/document/UI classes out of portable scope except lifecycle/service ownership behavior relevant to these features.

**Integration**

Put protocol/game state in `src/game/License.*`; put device identity, network, SMS, and vibration behind `src/platform` interfaces with dummy backends. Live network access is not required for routine tests.

**Verification**

- Recorded protocol fixtures for success, malformed response, failure, timeout, retry, cancel, and absence of device identity/network/SMS support.
- Authorization-gated scene update/render tests and exact user-visible states.
- No unconditional-success shortcut; any explicitly supported offline policy must be a separate, documented host choice with tests showing when it differs from original behavior.

**Status: complete.** `src/game/License.*` implements the recovered DES/MD5 protocol, verification bitmap, initial/session schedules, service and response records, SMS retry/timeout behavior, and default-cracked gate semantics. Platform interfaces provide deterministic identity/network/SMS/vibration backends, explicit unsupported results, Linux test identity/SMS services, and SDL3 haptic rumble. The day-1 door and all four scene phases use their recovered gate slots; menu mode 4 and scene authorization screens implement their native text/retry/cancel flows. `OfflineAuthorized` is an explicit test/host-only policy, not the application default. The suspected crack at native `0x0000d590` and the unreachable zero-result response branches are documented rather than silently repaired.

`test_game_license.cpp` covers independent DES/MD5 fixtures, real message/domain resources, protocol success/malformed/failure/pending/cancel cases, all policy differences, missing capabilities, vibration, later-day gating, and real engine-frame authorization flows with exact hashes. All 38 default tests pass serially; SDL-independent core, STDIO-logging, host-math, WAR, and ASan/UBSan suites each pass 31 tests. Detailed protocol, native addresses, suspected crack analysis, host policy, and verification are in `symbian3rd_license.md`. Recovered names, typed signatures, comments, stage-14 tags, bookmarks, and a completion property are saved in Ghidra.

## Stage 15 — Complete `GameSI` and episode integration matrix

**Scope**

- Finish every callback not naturally covered by earlier stages: input/log/fades, progression/events/save/pause, rank, timers/prompt/light/fog/subtitles/key masks/screen shaders/map markers, and any remaining player/enemy/thing commands.
- Produce an ARM-derived table for IDs 43–260 containing address, signature, owning subsystem, implementation, and test. Preserve IDs 258/260 as empty and implement ID 259 exactly.
- Run all 217 shipped scripts through the real VM and concrete scene, not a mocked command host. Cover all 53 unused game IDs and three final slots with synthetic scripts.
- Exercise every shipped SBP entity type and every concrete enemy/trigger/thing in its actual episode context.

**Verification**

- A callback matrix proving all 218 slots and all 19,251 shipped host calls execute with their ARM-derived semantics.
- Full day/map/script progression, next-level/next-day/door transitions, script suspension/runTask/shared variables, and script sidecar reload.
- No unknown-command fallthrough, empty `GameplayHooks`, empty scene methods, or game-specific `FUN_*` recovery gap left without a documented original no-op.

## Stage 16 — Final feature-parity audit and full-game validation

**Scope**

- Re-audit every game RTTI class and vtable against `src/game`: Player/states, all 17 enemies, all triggers/things/map units, `Scene`, `GameSI`, `MyGameUnitMgr`, GameUI consumers, and license services.
- Audit original unused/empty methods separately from missing portable work. Keep `CalcRank` and other confirmed empty methods empty.
- Verify no concrete game logic was added to `src/soft3d`, no platform service contains episode rules, and all new game code is under `src/game`.
- Run fresh-start, continue, return-to-menu, game-over/restart, configuration/unlock, and shutdown scenarios with real SDL input and dummy video/audio.

**Verification**

- Full CTest, ASan/UBSan, math, stdio-logging, WAR-decoder, and applicable English-layout matrices, plus the Python converter suites.
- Headless playthrough coverage for all seven days and every map/script family, including at least one save/load continuation in each day and a final new-game/exit cycle.
- Exact directed frame/audio/state checks where ARM evidence supports them; record any remaining unverified visual discrepancy explicitly rather than declaring blanket pixel parity.
- Final Ghidra save, updated `symbian3rd_game.md`/format docs/README status, and no generated artifacts or empty placeholders left in the deliverable.

## Dependency order and session guidance

Stages 1–2 establish data and the real application path. Stage 3 supplies scene/camera services; stage 4 supplies the player; stages 5–7 supply world interaction; stage 8 connects their UI. Stage 9 establishes common AI before the three enemy-family stages. Stage 13 intentionally follows all unit implementations so persistence is not repeatedly redesigned. Stage 14 may be researched earlier if license gating blocks test runs, but its final behavior and platform backends remain a required stage. Stages 15–16 are completion gates, not optional cleanup.

Each stage is sized as one related implementation session. If a stage expands, split along the listed class families rather than dropping unused methods or leaving a hook in place.

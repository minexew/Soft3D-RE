# Symbian3rd game-specific architecture and remaining inventory

## Scope and evidence rules

This document inventories the 7 Days game layer that remains after the reusable Soft3D engine work in `engine_completion_plan.md`. Evidence comes from the ARMv5 Symbian 3rd binary, its RTTI/vtables, and the shipped resources. Portable implementations of these classes belong in `Reversed_Game/src/game`; reusable engine and platform facilities remain in `src/soft3d` and `src/platform`.

**Do not use `Leaked_Source/commandlist.txt`, the leaked episode sources, or `Tools/SimpleScriptDecompiler/bin/Debug/names/*` as command-ID, signature, or behavior authorities.** They describe an earlier build and disagree with the ARM callback table and shipped SST arguments. In particular, `commands_mobile.txt` assigns IDs that are absent from shipped scripts and contradicts the binary. Preserve those files unchanged as historical material only. `GameSI` names must be recovered from ARM code; shipped SST files are execution fixtures and coverage evidence, not naming evidence.

`game_completion_plan.md` turns this inventory into implementation stages.

## Current portable boundary

Stages 1–10 are implemented in `seven_days_game`. Stage 9 supplies common enemy AI/pools and commands; stage 10 adds the concrete Maid/Bully families in `MaidBully.*`, including their native projectile, charge, QTE, saw, death, and reward behavior. See `symbian3rd_enemy.md` and `symbian3rd_maidbully.md`.

- `src/app/Application.cpp` constructs and installs concrete `game::Scene` and `game::GameSI` objects, the full script context, and the new-game/load/return hooks. `GameEngine` owns the scene; the application owns the non-owning script interface and clears it before engine destruction.
- Starting a new game loads day 1, reloads the player model after the cache-level-2 clear, displays the 56-frame day intro, selects `day1/ep0`, and incrementally loads map 2008 through the real SBP factory. The SDL new-game smoke test proves that this path reaches progress 100 and executes the stage-3 camera/audio commands.
- `Scene` owns the BSP, manager, concrete `Player`, game `CameraController`, camera-path resources, six presentation shaders, transition state, event/snapshot arrays, and two environment-channel states. Stage 4 adds the complete 20-state player, sword/gunfire, combat, inventory, and hide/FPS lifecycle; `symbian3rd_player.md` is the detailed reference.
- `GameSI` contains all 218 ARM handler addresses and zero adjustments. Stage-2 scene commands, stage-3 camera/audio/presentation commands, stage-4 player commands, stage-5 thing/map/trap commands 219–254, stage-6 interaction commands 149–165, stage-7 Hide/Item/Talk commands 166–179, StayBox commands 205–207, and stage-8 UI/progression commands are implemented alongside the confirmed empty/shared-word callbacks. Every remaining later-stage command throws `GameSIError` with its ID and ARM address. The real `ep0` test confirms the map handshake and reaches later-stage input command 47 (`0x00065560`).
- The SBP factory creates concrete `BspUnit` renderable entities for types -1/0, real camera/light triggers for types 10/11, type-102 header state, and manager-owned type-202 lensflares. Other game-owned non-renderable records return null and are skipped by the generic loader, as in ARM. See `symbian3rd_mapunits.md` for the completed stage-5 classes and command matrix.
- `game::GameUiController` completes the GameUI HUD/map/about-model/item operations, including high-`0x10000` special-door subtitles, dialogs, inventory/files, progression, and statistics; see `symbian3rd_gameui.md`. Full scene save/load remains stage 13 and reports an explicit error rather than treating the 44-byte header as a scene load. License gating remains stage 14.

These boundaries leave substantial gameplay work, but the application no longer uses an empty scene or an unbound script host.

## Scene and engine wiring

`Scene` is a 0x6b68-byte `s3dBase`-derived game object constructed at `Scene_Ctor` (`0x0002a0f4`). Important native members identified from consumers are:

| Scene offset | Purpose |
|---:|---|
| `0x40` | Owned `Bsp`. |
| `0x4c` | Embedded game `CameraController`, not the menu-only camera path helper. |
| `0x254` | Owned concrete `Player`. |
| `0x770` | Owned `MyGameUnitMgr`. |
| `0x90c` | `GameUIDlg` pointer. |
| `0x914`–`0x928` | Ink, Fade, Gray, MotionBlur, Blind, and Twist presentation-shader owners. |
| `0x92c`–`0x978` | Two randomized environment-audio channels, each with four resources, interval state, countdown, and voice. |
| `0x98c`–`0x9f4` | Day-intro, next-script, next-map, save-request, spawn, door-transition, and incremental-load state. |
| `0x9f8`–`0x14f7` | Twenty-two 64-character door/cinematic model-name records. |
| `0x19f8`–`0x1acf` | Two 22-entry `int32_t` door open/close sound-index arrays. |
| `0x1afc`, `0x42fc` | Two 0x2800-byte progression/snapshot blocks. |
| `0x6b04`, `0x6b08` | Authorization/registration state. |

Construction is split rather than performed entirely by `Scene_Ctor`. `BindGameStateSubsystems` (`0x0006f908`) fills the service pointers at `0x754`–`0x774` and calls `Scene_BindEngineServices` (`0x0002badc`), which creates the 0x26018-byte manager, creates `ControlMap`, installs the entity factory, clears the event block, and sets minimum intensity to `0x0ccc`. `GameEngine_LoadPlayerAndLicenseData` (`0x0006f9b8`) calls `Scene_CreatePlayer` (`0x0002bd6c`) to allocate the 0xf44-byte player, initialize its state set, and load `r_ken_a`/`r_ken_shadow_a`. `Scene_InitRendering` then binds the player/camera and loads `default.s3dsty` plus the 64-entry `1000.spl` collection. `GameEngine_StartNewGame` (`0x00071250`) resets state and calls `Scene_LoadDay(1)`.

`Scene_LoadDay` ignores the SBN loader result. In particular, the default `uien/day1.sbn` is absent and is a valid nonfatal case. The portable implementation preserves this while checking required content loading. Cache-level-2 clearing must be followed by player-resource reload; otherwise the retained `ObjectRes` pointers dangle. Portable `GameEngine` also destroys its scene before `UiManager`, because scene teardown resets `GameUIDlg`.

The incremental loader uses 26 calls and progress values 0, 4, …, 96, 98, 100. A failed current-day map restarts at zero under `day1`; failure of the fallback frees the BSP. Once 100 is reached, an active initializer selects 101, while script state 3 selects 102. A newly assigned 102 immediately increments to 103 in the same update; 103 on the following update calls `Scene_FinishMapLoad`, then stores -1. `Scene_FinishMapLoad` uses `SimpleScript_StartTasks` (`0x00031bd0`) to set state 4 and update immediately. The real `day2/ep0` map 1001 exercises the day-1 fallback in the portable tests.

The principal methods are `Scene_Update` (`0x0002d100`), `Scene_PostUpdate` (`0x0002e634`), `Scene_PrepareRender` (`0x0002ee0c`), and `Scene_SubmitRenderables` (`0x0002f088`). Named game-lifecycle methods now recorded in Ghidra include:

- `Scene_LoadDay` `0x0002c3e0`;
- `Scene_UnloadLevel` `0x0002c700`;
- `Scene_SetLoadMapParams` `0x0002c87c`;
- `Scene_LoadNextLevel` `0x0002c900`;
- `Scene_LoadMap` `0x0002c984`;
- `Scene_BeginMapLoad` `0x0002ca50`;
- `Scene_FinishMapLoad` `0x0002d018`;
- `Scene_ReturnToMainMenu` `0x0002ec28`;
- `Scene_LoadSave` `0x0002f864`;
- `Scene_ResetEnvironmentAudio` `0x0002f6bc`;
- `Scene_DrawAuthorization` `0x000302fc` and `Scene_BeginAuthorization` `0x00030dd0`.

`GameEngine_BindSceneInterface` (`0x00070464`) initializes scene rendering and populates the `GameSI` context with the scene, unit manager, camera controller, player, GameUI, audio/resource managers, BSP, and control map before calling `SimpleScript_SetScriptInterface`. This is the required integration model for the portable `game::Scene` and `game::GameSI`.

Stage 3 adds the 0x208-byte `GameCameraController`, all eight mode callbacks, `CameraTrigger`/`LightTrigger` layouts, randomized environment channels, exact door transitions, ending-specific titles, and game-over presentation. The detailed field tables, mode semantics, environment algorithm, command list, and verification are in `symbian3rd_camera.md`.

The scene includes the following behavior that was not covered by menu work:

- new-game/day initialization and day-specific SBN loading;
- SST episode selection and incremental SBP loading, including the day-1 fallback;
- next-level/next-day transitions, door playback, fades, spawn position/direction, and camera restoration;
- map submission and game-unit update/post-update ordering;
- listener selection between player and camera;
- randomized environment audio and music transitions;
- day-title and game-over rendering, including Ink progression and restart/menu actions;
- authorization/registration flow and its gating of scene processing;
- full save/load and the script sidecar.

## `GameSI` and script commands

`GameSI_Ctor` is `0x000636e0` (duplicate constructor `0x00061df0`), and `GameSI_ExecuteCommand` is `0x0006528c`. The executor indexes a table at `0x00140cc0` by `commandId - 43`. It contains **218 eight-byte records** for IDs **43–260**:

```c
struct GameSI_Callback {       // 0x08 bytes
    void *handler;             // +0x00
    int32_t thisAdjustment;    // +0x04; all observed entries are zero
};
```

Each callback receives the argument-pointer array, argument count, and output `ScriptValue` as selected by the generic script interface. The native `GameSI` is 0x738 bytes: `ScriptInterface` at 0, context pointers at `0x40`–`0x60`, reserved word at `0x64`, and the callback array at `0x68`. The context order is scene, manager, camera controller, player, GameUI, audio, resources, BSP, and ControlMap.

The following scene commands are recovered and implemented in stage 2:

| ID | Handler | Behavior |
|---:|---:|---|
| 53 | `GameSI_LoadNextDay` `0x00065964` | Calls `Scene_LoadDay(day + 1)`. |
| 54 | `GameSI_SetLoadMapParams` `0x000659ac` | Takes door/resource word, two fixed XYZ vectors, and an RGB short triple. |
| 55 | `GameSI_LoadNextLevel` `0x00065b18` | Takes signed-short map and episode IDs. |
| 56 | `GameSI_LoadMap` `0x00065b78` | Takes a signed-short map ID; `day1/ep0` passes 2008. |
| 57 | `GameSI_IsMapLoaded` `0x00065bc0` | Writes `mapProgress == 100` to the output. |
| 58 | `GameSI_SetEventFlag` `0x00065c10` | Sets the selected byte at scene `0x1afc` to one. |
| 59 | `GameSI_GetEventFlag` `0x00065c64` | Returns whether that event byte is nonzero. |

ID 69 is also an original empty callback at `0x00066504`, in addition to IDs 258 (`0x00064fd0`) and 260 (`0x00065034`). ID 259 (`0x00064ff8`) writes one to shared script word 255. These callbacks have no shipped-script use and must not be assigned invented meaning. The complete ARM address table is preserved in `Reversed_Game/src/game/GameSI.cpp`; unimplemented entries fail explicitly rather than becoming aliases or no-ops.

A direct parse of all 217 shipped Mobile SST files found:

- 19,251 host-command calls in total;
- 2,261 calls to generic IDs 37–42; ID 36 is unused;
- 16,990 calls to game IDs 43–257;
- 162 of the 215 game IDs used, with 53 game IDs unused;
- no use of IDs 258–260.

Every callback still needs a directed test; the 53 unused game IDs and three final slots cannot be covered merely by loading all shipped scripts.

Confirmed ARM-derived examples illustrate why the stale tables must not be used:

| ID | Handler | ARM-confirmed behavior |
|---:|---:|---|
| 149 | `GameSI_AddEventTrigger` `0x00068b78` | Constructs an `EventTrigger` with ID, count, string, and bounds. |
| 153 | `GameSI_AddPickTrigger` `0x00068df0` | Constructs a `PickTrigger`. |
| 154 | `GameSI_EnablePickTrigger` `0x00068f34` | Sets pick-trigger enable state. |
| 155 | `GameSI_AddDoorTrigger` `0x00068fa4` | Constructs a `DoorTrigger` with map/spawn/direction/bounds data. |
| 158 | `GameSI_EnableDoorTrigger` `0x000692d0` | Sets door-trigger enable state. |
| 159 | `GameSI_AddActTrigger` `0x0006934c` | Constructs an `ActTrigger` from ID and six fixed bounds values. |
| 160–163 | `0x00069450`, `0x000694b8`, `0x00069578`, `0x000695f8` | Set option count/string, get selection, clear selection. |
| 165 | `GameSI_EnableActTrigger` `0x0006979c` | Sets act-trigger enable state. |
| 166 | `GameSI_AddHideTrigger` `0x0006980c` | Constructs a `HideTrigger`. |
| 169 | `GameSI_AddItemTrigger` `0x00069b0c` | Constructs an `ItemTrigger`. |

IDs 229–257 form a further thing/item/player command family with distinct handler addresses. They include animation pause/frame/completion, break model/animation/sound, push sound, rotation, absolute/relative/smooth movement, destruction, depth bias, removal/existence/hit/bounds tests, lighting/pushability, magic-cube orientation, and player inventory operations. Related earlier handlers are not automatically interchangeable aliases; implement each slot from its own ARM body.

`CalcRank` at `0x000669fc` is intentionally empty in this build. Preserve that no-op instead of inventing a ranking algorithm.

## Player and player states

`Player` multiply inherits `Creature`, `StateMachine<Player>` at 0x6f0, and `CameraSpot` at 0x704. `Player_Ctor` is `0x0003867c` (duplicate `0x00038234`). ARM call evidence gives its parameters as `(resources, renderer, bsp, scene, audio, states)` after `this`; the BSP and audio arguments were previously swapped by the untyped decompiler. Its embedded `GunFire`, sword model, physics, combat, QTE, inventory, note/event state, and camera bindings are all game work.

The primary vtable's slots 28–34 implement the `CameraSpot` getters/setters: `Player_GetPosition`, `Player_GetRight`, `Player_GetUp`, `Player_GetForward`, `Player_SetRight`, `Player_SetUp`, and `Player_SetForward` (`0x000380bc`–`0x000381f8`). `Player_SaveState` and `Player_LoadState` are `0x0003d4f4` and `0x0003d278`.

`PlayerStateSet` contains all 20 concrete states and 80 state-specific virtual implementations:

- `CStand`, `CRun`;
- `CAttackA1/A2/A3`, `CAttackB1/B2/B3`, `CAttackC1/C2/C3`;
- `CHurt`, `CHurt2`, `CHurt3`, `CHurt4`, `CDie`;
- `CPush`, `CQTE`, `CHide`, `CFPS`.

`PlayerStateSet_InitializeStates` (`0x00032c50`) registers them in slots 0–19. Stage 4 implements every enter/exit/update/registration body and its chain transitions. The concrete `Player` owns `Sword` and `GunFire`, uses the real control map and camera, and connects death to scene game-over. Player commands 117–148 and 255–257 are implemented. Native layouts, animation actions, combat tables, QTE/hide rules, save order, and exact frame tests are in `symbian3rd_player.md`. Concrete enemy and pushable descendants remain in their assigned stages behind the documented narrow interfaces.

## Unit manager, things, and map entities

`MyGameUnitMgr_Ctor` (`0x0003fce4`) extends the generic manager with content tables, enemy pools, lensflares, map data, player/trigger links, statistics, and persistence. Important recovered methods are:

- `MyGameUnitMgr_InitializeEnemyPools` `0x000419b0`;
- `MyGameUnitMgr_PrepareDayEnemies` `0x00042ba0`;
- `MyGameUnitMgr_Reset` `0x00043b40`;
- `MyGameUnitMgr_LoadDayContent` `0x00043de4`;
- `MyGameUnitMgr_EntityFactory` `0x00043ed8`;
- `MyGameUnitMgr_Update` `0x0004519c`;
- `MyGameUnitMgr_SubmitRenderables` `0x00045414`;
- `MyGameUnitMgr_LoadUnitState` `0x00048314`.

The SBP factory handles camera triggers (type 10), light triggers (type 11), six fog/header words (type 102), and lensflare entities (type 202), alongside the ordinary map-model path. `BspUnit_LoadEntity` (`0x000fd5c0`) and `BspUnit_Submit` (`0x000fd790`) implement concrete map-model entity configuration, animation, transparency selection, and queue submission. The portable scene now uses these methods through `game::BspUnit`, not the former generic `BspEntity` submission path.

Stages 5–7 implement `Breakable`, `Pushable`, `Pickable`, `ElectricSaw`, `StayBox`, `Trap`, and all seven script-created interaction triggers, including state records and commands. `Sword`, `CameraTrigger`, and `LightTrigger` were completed in stages 3–4; stage 7 adds Light persistence without replacing the working map-trigger/controller logic. `symbian3rd_mapunits.md` and `symbian3rd_triggers.md` record native layouts and exact behavior. Stage 9 supplies the concrete StayBox enemy-pool host.

The trigger unit-type words confirmed by constructors are Event `0x1b`, Camera `0x20`, Act `0x21`, Door `0x22`, Hide `0x23`, Light `0x24`, Pick `0x25`, Item `0x27`, and Talk `0x28`. `Breakable`, `Pushable`, and `Pickable` use `0x1c`, `0x1d`, and `0x1e`; `Trap` uses `0x1f`.

## Enemy hierarchy

`Enemy` extends `Creature` with 22 additional virtual slots (28–49). They cover model/animation setup, spawn/reset, perception, idle/alert/chase/attack updates, attack selection and hit tests, damage, death, and save/load. `Enemy_Die` (`0x0007561c`) is shared by many classes and performs the common death transition, statistics update, sound/animation, and player notification. Enemy save adds four big-endian 32-bit state words to `VisUnit`: type, navigation-enabled, health, and state. Stage 9 implements this common layer; see `symbian3rd_enemy.md` for the layout, states, commands, and tests.

The hierarchy has **18 named concrete classes**: 17 ordinary pool classes plus `Boss`. The pool variants are:

`Bully`, `BullyDay3`, `BullyDay5`, `DarkKen`, `FatMaid`, `Ghost`, `Kate`, `Lamper`, `Lord`, `Maid`, `Maid2`, `Maid3`, `Prisoner`, `Sam`, `Victor`, `Weed`, and `Worm`.

`MyGameUnitMgr_InitializeEnemyPools` constructs only slot 0 for singleton types and all eight slots for non-singleton types, assigns the player and manager, and initializes their shadows. `MyGameUnitMgr_PrepareDayEnemies` selects resources and outfit variants by day. `Boss` has separate construction/behavior from the ordinary pool and must not be omitted merely because it is not in that pool's constructor loop.

Stage 10 replaces the common adapter for all Maid/Bully pool rows with the native classes documented in `symbian3rd_maidbully.md`. The adapter remains only for stages 11–12. Their special perception, movement, attacks, projectiles, transformations, death, linked actors, and boss scripting still require individual ARM recovery; none may be approximated by the generic enemy.

## Full save format and unit reconstruction

`Scene_LoadSave` seeks to `0x2c`, after the 44-byte `slot%d.sav` header, then reads:

1. day and map IDs;
2. a wide music name and two environment-audio configurations/resources;
3. both 0x2800-byte progression blocks;
4. day content and GameUI state;
5. player state through the custom player loader;
6. manager content and a sequence of typed unit records;
7. script state from `spt\\slot%d.sav`.

`MyGameUnitMgr_LoadUnitState` handles unit type words 0, 4, `0x1b`, `0x1c`, `0x1d`, `0x1e`, `0x1f`, `0x21`, `0x22`, `0x23`, `0x25`, `0x26`, `0x27`, and `0x28`. These represent base collision units, Things, Event/Breakable/Pushable/Pickable/Trap/Act/Door/Hide/Pick/StayBox/Item/Talk records. In particular, `0x26` consumes the next embedded 0x6c-byte StayBox at manager `+0xa28`; it does not reconstruct a pooled enemy, and this switch has no enemy case. PickTrigger stores an additional linked-thing ID. Map-driven camera/light entities are rebuilt from map data rather than emitted by this switch.

The save writer, exact field order, per-class extensions, EOF rules, script archive consistency, and second-save byte determinism are all still required. Reading only the header cannot produce a valid load.

## Game-data tables

### `itemdata.sdt`

`LoadItemDataSdt` (`0x00045d98`) parses 67 52-byte item records and 65 12-byte text records, exactly 4,272 bytes. Item fields are now confirmed as name string, 32-byte model name, directional combination input/result, and description string. Text IDs 1000–1999 populate documents and 2000–2099 populate photos; there is no separate populated model-text section. The 0x264-byte runtime item includes its 0x20c-byte `Object`, 64-character model name, those four words, saved runtime state, and model unlock. About-model rendering shares this same item array. See `game_data_tables.md` for complete layouts and consumers.

### `gameitem1.sdt`–`gameitem7.sdt`

`LoadGameItemSdt` (`0x00046288`, formerly `LoadNumberedSdtResourceA`) reads a 32-bit group count and **two 32-bit header words per group**, not a packed `{int16 id, int16 count}` pair. Each 0x60 record contains item ID, pickup dialog title text ID, 16-byte packed model name, position, three orientation axes, and pickup bounds. The title field was formerly mislabeled as a sound index; stage-6 `PickTrigger_Update` proves its actual consumer. Shipped group counts are 9, 13, 13, 18, 14, 4, and 3. `MyGameUnitMgr_SpawnMapItems` (`0x00047198`) selects the first matching map group and creates `Pickable`/`PickTrigger` pairs.

### `einfo1.sdt`–`einfo5.sdt`

`LoadEnemyInfoSdt` (`0x00046750`, formerly `LoadNumberedSdtResourceB`) uses the same eight-byte group-header rule. A 0x20-byte spawn contains enemy type, position, direction, and waypoint count, followed by 0x1c-byte `{position, waitTime, direction}` waypoints. Runtime entries are 0x24 bytes with active/type/unit-ID/count words and a nested pointer; groups hold 32 entries in 0x484 bytes. Shipped group counts are 0, 4, 9, 1, and 5. Missing day-6/day-7 files are valid. Spawn and persistence consumers are documented in `game_data_tables.md`.

### `message.sdt` and `domain.sdt`

These are not UI string hierarchies. `GameEngine_Init` reads 512 bytes from `message.sdt` into two 256-byte license/message buffers, with an external-file fallback. `domain.sdt` is a 36-byte opaque service record. Their consumers belong to the license/network/verification code and must be analyzed there rather than decoded with `ParseStateSdtTable` or treated as item text.

### Resource-name and statistics tables

`InitializeAnimationResourceNames` (`0x00089238`) fills 260 names. `InitializeSoundResourceNames` (`0x00088594`, formerly `InitializeUiResourceNames`) fills 142 named slots following a reserved empty index 0, producing a 143-slot indexed table now present in `src/game/ResourceNames.cpp`. `Scene_InitializeDoorResources` (`0x0002b554`) has 22 model records and two 22-entry sound-index arrays; animation entries 238–259 mirror the door models. Stage 8 implements the HUD/inventory/map/QTE/story, item-effect, statistics, and runtime-unlock consumers; `symbian3rd_gameui.md` records them. Full scene save records remain stage 13.

## Game UI and progression

`GameUIDlg_Update` (`0x000566cc`) and its helpers include much more than the portable menu shell:

- normal HUD and player status;
- interaction prompts, tutorial text, subtitles, story/door displays;
- portrait/simple/choice dialogs and selection results;
- QTE display/input/result handling;
- inventory acquisition, use, combination, descriptions, and item effects;
- map/floors/markers and files/documents;
- game-over, options, controls, and return/exit flows.

`GameUI` resource pointers connect it to the player, scene, unit manager, content records, and map state. Stage 8 implements these consumers in `game::GameUiController` and `GameUIDlg`, including the native statistics and unlock updates. The former `ClearRank` label was incorrect: ID 81 opens the map, `CalcRank` remains the original no-op, and ID 83 is another confirmed empty callback. Full layouts, map tables, dialog quirks, command IDs, and verification are in `symbian3rd_gameui.md`.

## License and platform services

The remaining RTTI includes `ClientSession`, `MyClientSession`, `SessionDevice`, `HttpSession`, `TCPSession`, `CSocketReader`, `CSocketWriter`, `CIMEI`, `CPhoneObserver`, and `CSmsSessionObserver`. The service identifier `com.dingoo.server.VerifyService` is present, and scene rendering/update calls query the verification state. These are not unused boilerplate if a portable run is expected to reproduce the original gating behavior.

A portable implementation needs a platform interface for license state, device identity, messaging/network operations, and vibration, with deterministic test backends. The original protocol, encryption/obfuscation, stream records, `domain.sdt`/`message.sdt` consumers, success/failure/retry states, and user-visible screens must be recovered before choosing an offline policy. Do not silently turn the checks into unconditional success and call that feature parity.

`CSymbian3rdApplication`, `CSymbian3rdDocument`, `CSymbian3rdAppUi`, and `CSymbian3rdAppView` are Symbian shell classes rather than new game logic. They matter only for lifecycle, focus/suspend, vibration, and service ownership; the SDL app should not reproduce Symbian UI boilerplate.

## Ghidra status

Confirmed scene, player, manager, trigger, data-loader, spawn, item-operation, resource-table, and `GameSI` names have been applied and saved. Stage 1 added typed `SdtItemRecord`, `SdtGameTextRecord`, `GameItemSpawnRecord`, `EnemyWaypoint`, `SdtEnemySpawnRecord`, `EnemySpawnRecord`, `GameItemGroup`, `EnemyInfoGroup`, and `GameItemRuntime` structures, loader locals, prototypes, plate comments, and `game,data,stage1` tags. The callback table has a typed `GameSI_Callback` first record, a full-table bookmark, and a `game_completion_research` property; existing pointer labels remain preserved.

Stage 2 adds saved, explicitly padded native `Scene` (0x6b68) and `GameSI` (0x738) layouts, `SbpEntityRecord` (0x1c), and the 0x26018-byte manager type. The scene layout includes the engine services, transition state, shaders, environment state, event/snapshot arrays, and authorization words; Scene-local areas not consumed by that stage stayed opaque; the separate native player/state layouts have since been recovered in stage 4. Function prototypes, plate comments, and `game,scene/script/map,stage2` tags cover the startup and load paths. The corrected `Player_Ctor` and manager-constructor argument orders are applied. `GameSI_LoadNextDay`, `GameSI_SetLoadMapParams`, `GameSI_LoadNextLevel`, `GameSI_LoadMap`, `GameSI_IsMapLoaded`, event callbacks, `SimpleScript_StartTasks`, and the three confirmed empty callbacks are named. Remaining unnamed methods belong to their later implementation stages.

Stage 3 adds saved `GameCameraController` (0x208), `CameraTriggerLayout` (0x90), `LightTriggerLayout` (0x68), and `EnvironmentAudioChannel` (0x4c) structures, 96 recovered function names, 100 typed signatures, local/parameter names, plate comments, and `game,camera/trigger/scene/script/audio,stage3` tags. All eight update callbacks, trigger stack methods, environment handlers, scene presentation commands, and both basic player positioning commands are included. Two stage-3 bookmarks and the corrected `7nd_a` ending-title rule are saved. The 110-function stage-3 tag set was verified after splitting the bulk-tag operation, and the program was saved afterward.

Stage 4 adds saved `Player` (0xf44) and `Sword` (0x27c) structures with recovered field names, 187 function renames covering player/sword/states/commands, typed state/player/sword/GameSI signatures, plate comments, `game,player/states/sword/script,stage4` tags, a player-constructor bookmark, and a completion property. The former `<State>_vfunc_<slot>` names are now explicit `Enter`/`Exit`/`Update`/`Register` names. The program was saved after verifying typed player construction, attack, QTE, and sword decompilation.

Stage 5 adds native layouts for all seven recovered map/thing classes plus StayBox region/spawn records, named class/helper/command functions, 53 typed class/helper signatures and 36 typed command signatures, behavioral plate comments, 90 verified `stage5` tags, a completion property, and a bookmark at `GameSI_AddThing`. The stale `Things.nUnknown67c` label is now `nSerialized67c`, distinct from `pCurrentLeaf` at `0x680`. Typed decompilation was checked after applying the layouts, and the program was saved. Exact implementation and verification details are in `symbian3rd_mapunits.md`.

Stage 6 adds `EventTriggerLayout` (0x64), `PickTriggerLayout` (0x74), `DoorTriggerLayout` (0xa4), and `ActTriggerLayout` (0xa0), typed class/manager/GameSI signatures, behavioral plate comments, 69 verified `stage6` tags, a completion property, and a bookmark at `ActTrigger_Update`. Typed decompilation corrected the initial consumer assumptions: gameitem `+0x04` and PickTrigger `+0x58` are dialog title text IDs, while Act has three option strings, a separate title at `0x6c`, and a command/persistence-only word at `0x5c`. The corrected structures, prototypes, and persistence rules were verified again after applying them. Details are in `symbian3rd_triggers.md`.

Stage 7 adds `HideTriggerLayout` (0x90), `ItemTriggerLayout` (0x7c), `TalkTriggerLayout` (0xb8), camera/line records and native vector layouts, 61 typed class/manager/GameSI signatures, behavioral plates, 67 verified `stage7` tags, a completion property, and a bookmark at `TalkTrigger_Update`. Typed review proved that Hide saves prompt/dialog/mask rather than angular limits, and that Act/Talk reset camera warmup rather than listener selection. StayBox constructor defaults and its three commands were corrected and saved. The program was saved after verifying the typed decompilation.

Stage 8 adds `GameUIDlgLayout` (0x524) and `GameUIMapMarkerRecord` (0x0c), 59 function renames, typed UI/command signatures, corrected kill-count fields, comments, `stage8` tags, bookmarks, and a completion property. The native marker area has eight records. `GameUI_CloseDialog` preserves its result, dialog string slot 4 aliases the secondary label, and ID 81 is direct-map opening rather than `ClearRank`. Details are in `symbian3rd_gameui.md`.

Stage 9 adds the saved 0x83c `Enemy` layout and distinct 0x24 `EnemyWaypointNode`, typed common `Enemy` signatures, common AI/helper names, pool constructors, all 36 enemy/collision command names, BSP blood-splat and normalized-reciprocal functions, behavioral plates, and 140 verified `game,enemy,stage9` function-tag assignments. Comments correct the `0x26` StayBox meaning, native missing-enemy command behavior, Ghost's empty model input, Kate's sound mapping, constructor defaults/rewards, and the sword constructor's inverted empty hit bounds. The program was saved after checking the layouts and typed decompilation. See `symbian3rd_enemy.md`.

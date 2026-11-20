# Symbian3rd game-specific architecture and remaining inventory

## Scope and evidence rules

This document inventories the 7 Days game layer that remains after the reusable Soft3D engine work in `engine_completion_plan.md`. Evidence comes from the ARMv5 Symbian 3rd binary, its RTTI/vtables, and the shipped resources. Portable implementations of these classes belong in `Reversed_Game/src/game`; reusable engine and platform facilities remain in `src/soft3d` and `src/platform`.

**Do not use `Leaked_Source/commandlist.txt`, the leaked episode sources, or `Tools/SimpleScriptDecompiler/bin/Debug/names/*` as command-ID, signature, or behavior authorities.** They describe an earlier build and disagree with the ARM callback table and shipped SST arguments. In particular, `commands_mobile.txt` assigns IDs that are absent from shipped scripts and contradicts the binary. Preserve those files unchanged as historical material only. `GameSI` names must be recovered from ARM code; shipped SST files are execution fixtures and coverage evidence, not naming evidence.

`game_completion_plan.md` turns this inventory into implementation stages.

## Current portable boundary

The reconstructed engine has generic hooks but no game implementation behind them:

- `GameEngine` owns an empty `SceneHost`; `src/app/Application.cpp` installs neither a concrete scene nor `GameplayHooks`.
- Starting a new game only flips `inGame` and initializes `GameUIDlg`. No episode, map, player, unit, or script interface is installed.
- Stage 1 adds `seven_days_game` with `GameData`, fixed resource tables, and a concrete `MyGameUnitMgr` data owner. `itemdata.sdt`, all seven `gameitem` files, and all five `einfo` files now populate `GameContent`; day 1 loads through the real new-game path, and the full sound table reaches `AudioManager`. Concrete scene/map spawning and later-day transitions remain stage 2+ work.
- `GameUIDlg` delegates the actual HUD, map, about-model rendering, item use, and item combination to empty hooks.
- `BspEntityFactory` creates no game entities. `BspEntity` is not the concrete `BspUnit` implementation.
- Save-slot support reads the 44-byte header only. It does not load the scene, units, or script state.
- The original `GameSI` is not constructed or connected to `SimpleScript::SetScriptInterface`.

These are real missing code paths, not interfaces that can remain empty in a feature-complete build.

## Scene and engine wiring

`Scene` is a 0x6b68-byte `s3dBase`-derived game object constructed at `Scene_Ctor` (`0x0002a0f4`). Important native members identified from consumers are:

| Scene offset | Purpose |
|---:|---|
| `0x40` | Owned `Bsp`. |
| `0x4c` | Embedded game `CameraController`, not the menu-only camera path helper. |
| `0x254` | Owned concrete `Player`. |
| `0x770` | Owned `MyGameUnitMgr`. |
| `0x90c` | `GameUIDlg` pointer. |
| `0x914`–`0x91c` | Ink, Fade, and other presentation-shader owners. |
| `0x92c`–`0x978` | Two randomized environment-audio channels, each with four resources, interval state, countdown, and voice. |
| `0x98c`–`0x9f4` | Day-intro, next-script, next-map, save-request, spawn, door-transition, and incremental-load state. |
| `0x9f8`–`0x14f7` | Twenty-two 64-character door/cinematic model-name records. |
| `0x19f8`–`0x1acf` | Two 22-entry `int32_t` door open/close sound-index arrays. |
| `0x1afc`, `0x42fc` | Two 0x2800-byte progression/snapshot blocks. |
| `0x6b04`, `0x6b08` | Authorization/registration state. |

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

Each callback receives the argument-pointer array, argument count, and output `ScriptValue` as selected by the generic script interface. IDs 258 and 260 are empty callbacks; ID 259 writes one to shared script word 255. These three have no shipped-script use and must not be assigned invented meaning.

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

`Player` multiply inherits `Creature`, `StateMachine<Player>` at 0x6f0, and `CameraSpot` at 0x704. `Player_Ctor` is `0x0003867c` (duplicate `0x00038234`). Its embedded `GunFire`, sword model, physics, combat, QTE, inventory, note/event state, and camera bindings are all game work.

The primary vtable's slots 28–34 implement the `CameraSpot` getters/setters: `Player_GetPosition`, `Player_GetRight`, `Player_GetUp`, `Player_GetForward`, `Player_SetRight`, `Player_SetUp`, and `Player_SetForward` (`0x000380bc`–`0x000381f8`). `Player_SaveState` and `Player_LoadState` are `0x0003d4f4` and `0x0003d278`.

`PlayerStateSet` contains all 20 concrete states and 80 state-specific virtual implementations, already named `<State>_vfunc_<slot>` in Ghidra:

- `CStand`, `CRun`;
- `CAttackA1/A2/A3`, `CAttackB1/B2/B3`, `CAttackC1/C2/C3`;
- `CHurt`, `CHurt2`, `CHurt3`, `CHurt4`, `CDie`;
- `CPush`, `CQTE`, `CHide`, `CFPS`.

`PlayerStateSet_InitializeStates` (`0x00032c50`) registers all of them. The concrete enter/exit/update chains, transitions, animation/attack windows, movement, hiding, QTE, pushing, death, and first-person behavior remain unimplemented in the portable build. `Sword` (`GameUnit` plus `Renderable` at 0x54) and the player-owned `GunFire` must be wired to those states.

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

The SBP factory handles camera triggers (type 10), light triggers (type 11), six fog/header words (type 102), and lensflare entities (type 202), alongside the ordinary map-model path. `BspUnit_LoadEntity` (`0x000fd5c0`) and `BspUnit_Submit` (`0x000fd790`) implement concrete map-model entity configuration, animation, transparency selection, and queue submission. The portable generic `BspEntity` does not replace these methods.

Remaining concrete classes are:

| Family | Classes and required behavior |
|---|---|
| Things | `Breakable`, `Pushable`, `Pickable`: collision/query policy, pushing, pickup/item drops, hit/destruction, animations/sounds, transforms, movement, lighting, state records. |
| Other units | `ElectricSaw` motion/animation; `Sword` model/combat rendering; `StayBox` enemy restriction/return regions; `Trap` velocity effects. |
| Camera/map | `CameraTrigger` eight camera modes, SPL paths, interpolation/freeze/binding; `LightTrigger` lightstyle/color regions; `BspUnit` map models. |
| Interaction triggers | `EventTrigger`, `PickTrigger`, `DoorTrigger`, `ActTrigger`, `HideTrigger`, `ItemTrigger`, `TalkTrigger`. |

The trigger unit-type words confirmed by constructors are Event `0x1b`, Camera `0x20`, Act `0x21`, Door `0x22`, Hide `0x23`, Light `0x24`, Pick `0x25`, Item `0x27`, and Talk `0x28`. `Breakable`, `Pushable`, and `Pickable` use `0x1c`, `0x1d`, and `0x1e`; `Trap` uses `0x1f`.

## Enemy hierarchy

`Enemy` extends `Creature` with 22 additional virtual slots (28–49). They cover model/animation setup, spawn/reset, perception, idle/alert/chase/attack updates, attack selection and hit tests, damage, death, and save/load. `Enemy_vfunc_32` (`0x0007561c`) is shared by many classes and performs the common death transition, statistics update, sound/animation, and player notification. Enemy save adds four 32-bit state words to `VisUnit`.

All **17 concrete enemy classes** remain:

`Bully`, `BullyDay3`, `BullyDay5`, `Boss`, `DarkKen`, `FatMaid`, `Ghost`, `Kate`, `Lamper`, `Lord`, `Maid`, `Maid2`, `Maid3`, `Prisoner`, `Sam`, `Victor`, `Weed`, and `Worm`.

`MyGameUnitMgr_InitializeEnemyPools` creates one instance of singleton types and eight instances of pooled types, assigns the player and manager, and initializes their shadows. `MyGameUnitMgr_PrepareDayEnemies` selects resources and outfit variants by day. `Boss` has separate construction/behavior from the ordinary pool and must not be omitted merely because it is not in that pool's constructor loop.

Implementation should first recover the common `Enemy` state machine, then share code only where ARM shows it. Special perception, movement, attacks, projectiles, transformations, death, and boss scripting cannot be approximated by one generic enemy.

## Full save format and unit reconstruction

`Scene_LoadSave` seeks to `0x2c`, after the 44-byte `slot%d.sav` header, then reads:

1. day and map IDs;
2. a wide music name and two environment-audio configurations/resources;
3. both 0x2800-byte progression blocks;
4. day content and GameUI state;
5. player state through the custom player loader;
6. manager content and a sequence of typed unit records;
7. script state from `spt\\slot%d.sav`.

`MyGameUnitMgr_LoadUnitState` handles unit type words 0, 4, `0x1b`, `0x1c`, `0x1d`, `0x1e`, `0x1f`, `0x21`, `0x22`, `0x23`, `0x25`, `0x26`, `0x27`, and `0x28`. These represent base collision units, Things, Event/Breakable/Pushable/Pickable/Trap/Act/Door/Hide/Pick/Enemy/Item/Talk records. Enemy records reuse a pooled instance instead of allocating a new class. PickTrigger stores an additional linked-thing ID. Map-driven camera/light entities are rebuilt from map data rather than emitted by this switch.

The save writer, exact field order, per-class extensions, EOF rules, script archive consistency, and second-save byte determinism are all still required. Reading only the header cannot produce a valid load.

## Game-data tables

### `itemdata.sdt`

`LoadItemDataSdt` (`0x00045d98`) parses 67 52-byte item records and 65 12-byte text records, exactly 4,272 bytes. Item fields are now confirmed as name string, 32-byte model name, directional combination input/result, and description string. Text IDs 1000–1999 populate documents and 2000–2099 populate photos; there is no separate populated model-text section. The 0x264-byte runtime item includes its 0x20c-byte `Object`, 64-character model name, those four words, saved runtime state, and model unlock. About-model rendering shares this same item array. See `game_data_tables.md` for complete layouts and consumers.

### `gameitem1.sdt`–`gameitem7.sdt`

`LoadGameItemSdt` (`0x00046288`, formerly `LoadNumberedSdtResourceA`) reads a 32-bit group count and **two 32-bit header words per group**, not a packed `{int16 id, int16 count}` pair. Each 0x60 record contains item ID, pickup sound, 16-byte packed model name, position, three orientation axes, and pickup bounds. Shipped group counts are 9, 13, 13, 18, 14, 4, and 3. `MyGameUnitMgr_SpawnMapItems` (`0x00047198`) selects the first matching map group and creates `Pickable`/`PickTrigger` pairs.

### `einfo1.sdt`–`einfo5.sdt`

`LoadEnemyInfoSdt` (`0x00046750`, formerly `LoadNumberedSdtResourceB`) uses the same eight-byte group-header rule. A 0x20-byte spawn contains enemy type, position, direction, and waypoint count, followed by 0x1c-byte `{position, waitTime, direction}` waypoints. Runtime entries are 0x24 bytes with active/type/unit-ID/count words and a nested pointer; groups hold 32 entries in 0x484 bytes. Shipped group counts are 0, 4, 9, 1, and 5. Missing day-6/day-7 files are valid. Spawn and persistence consumers are documented in `game_data_tables.md`.

### `message.sdt` and `domain.sdt`

These are not UI string hierarchies. `GameEngine_Init` reads 512 bytes from `message.sdt` into two 256-byte license/message buffers, with an external-file fallback. `domain.sdt` is a 36-byte opaque service record. Their consumers belong to the license/network/verification code and must be analyzed there rather than decoded with `ParseStateSdtTable` or treated as item text.

### Resource-name and statistics tables

`InitializeAnimationResourceNames` (`0x00089238`) fills 260 names and `InitializeSoundResourceNames` (`0x00088594`, formerly `InitializeUiResourceNames`) fills 142 sound names, all now present in `src/game/ResourceNames.cpp`. `Scene_InitializeDoorResources` (`0x0002b554`) has 22 model records and two 22-entry sound-index arrays; animation entries 238–259 mirror the door models. HUD/inventory/map/QTE/story behavior, item effects, statistics counters, save headers, and runtime unlock updates remain later-stage consumers.

## Game UI and progression

`GameUIDlg_Update` (`0x000566cc`) and its helpers include much more than the portable menu shell:

- normal HUD and player status;
- interaction prompts, tutorial text, subtitles, story/door displays;
- portrait/simple/choice dialogs and selection results;
- QTE display/input/result handling;
- inventory acquisition, use, combination, descriptions, and item effects;
- map/floors/markers and files/documents;
- game-over, options, controls, and return/exit flows.

`GameUI` resource pointers connect it to the player, scene, unit manager, content records, and map state. `ClearRank` and the statistics counters have real state behavior; `CalcRank` remains the original no-op noted above. Unlock arrays in `config.sdt` must be updated by actual model/document/photo/item progression rather than only round-tripped.

## License and platform services

The remaining RTTI includes `ClientSession`, `MyClientSession`, `SessionDevice`, `HttpSession`, `TCPSession`, `CSocketReader`, `CSocketWriter`, `CIMEI`, `CPhoneObserver`, and `CSmsSessionObserver`. The service identifier `com.dingoo.server.VerifyService` is present, and scene rendering/update calls query the verification state. These are not unused boilerplate if a portable run is expected to reproduce the original gating behavior.

A portable implementation needs a platform interface for license state, device identity, messaging/network operations, and vibration, with deterministic test backends. The original protocol, encryption/obfuscation, stream records, `domain.sdt`/`message.sdt` consumers, success/failure/retry states, and user-visible screens must be recovered before choosing an offline policy. Do not silently turn the checks into unconditional success and call that feature parity.

`CSymbian3rdApplication`, `CSymbian3rdDocument`, `CSymbian3rdAppUi`, and `CSymbian3rdAppView` are Symbian shell classes rather than new game logic. They matter only for lifecycle, focus/suspend, vibration, and service ownership; the SDL app should not reproduce Symbian UI boilerplate.

## Ghidra status

Confirmed scene, player, manager, trigger, data-loader, spawn, item-operation, resource-table, and selected `GameSI` names have been applied and saved. Stage 1 also adds typed `SdtItemRecord`, `SdtGameTextRecord`, `GameItemSpawnRecord`, `EnemyWaypoint`, `SdtEnemySpawnRecord`, `EnemySpawnRecord`, `GameItemGroup`, `EnemyInfoGroup`, and `GameItemRuntime` structures, loader locals, prototypes, plate comments, and `game,data,stage1` tags. The callback table has a typed `GameSI_Callback` first record, a full-table bookmark, and a `game_completion_research` property. Existing pointer labels inside the table were deliberately preserved rather than forcibly replacing the entire range with one array. Remaining `FUN_*` methods and vague `*_vfunc_*` names are recovery work for later stages.

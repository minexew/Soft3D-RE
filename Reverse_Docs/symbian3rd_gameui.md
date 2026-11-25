# Symbian 3rd GameUI, HUD, maps, inventory, and extras

## Evidence and portable ownership

This document records game-completion stage 8 from the ARMv5 Symbian binary. Function addresses are the Ghidra display addresses used throughout the project. When checking those addresses against the extracted image, account for the **0x8000 memory-base displacement**.

The implementation is split without creating a second UI system:

- `src/soft3d/GameUIDlg.*` retains the existing SDT dialog, menu/input state, native overlay state, and resource acquisition.
- `src/game/GameUiController.*` supplies concrete gameplay drawing, player/content synchronization, item operations, map projection, and item-model submission.
- `GameUiController` implements the existing `TriggerUiHost`; `Scene` owns it and installs it as the manager's real interaction UI. The old `InteractionState` remains useful as a standalone test/fallback host, but is not the application's production substitute.
- `GameEngine::PrepareRender` preserves the existing scene/About ordering and calls the GameUI item-model preparation hook. Inventory and About use the same controller-owned `Object` array.
- Stage 13 implements full scene save records and script sidecars. The stage-8 save menu invokes the real slot callback and `Scene::SaveSaveSlot` writes the complete scene; `LoadDlg` continues to use the 44-byte header only for display. See `symbian3rd_savegame.md`.

## Native `GameUIDlg` layout

The recovered native object is **0x524 bytes**. Ghidra stores `GameUIDlgLayout` with named fields and explicit padding, plus `GameUIMapMarkerRecord` (0x0c). The main regions are:

| Offset | Native contents |
|---:|---|
| `0x238` | Temporary photo texture. |
| `0x244` | Small digit texture. |
| `0x248` | Menu mode: -1 game, 0/11 transitions, 1 main, 2 inventory, 3 map, 4 files, 5 options, 6 examine, 7 bindings, 8/9 confirmations, 10 item actions, 12 completion, 13 save, 14 combination. |
| `0x254`–`0x268` | Six interaction-prompt textures. |
| `0x26c`, `0x270` | Prompt ID and animation frame. |
| `0x274`–`0x284` | QTE time, zero-based key, blink/mash mode, failure animation, and wait-hint mode. |
| `0x288`–`0x2d4` | QTE key, alpha, result, guide, and waiting-hint textures. |
| `0x2dc`–`0x2e4` | HUD bar and left/right help textures. |
| `0x2f8`–`0x318` | Current map texture, four floor textures, map ID texture, offsets, and floor. |
| `0x324`–`0x338` | Three story panels, portrait, and directional player-marker textures. |
| `0x340`–`0x374` | Dialog kind/active/text, four options, secondary label, line/result/count/portrait/timer state. |
| `0x378`, `0x37c` | Hide-mask active and resource index. |
| `0x380` | Item error-message timer. |
| `0x390` | Item action; reused as map blink direction. |
| `0x3bc` | Combination source; reused as map blink frame. |
| `0x3c4`, `0x3c8` | Portrait and simple-dialog text widths. |
| `0x3d0` | Item-preview view matrix. |
| `0x410` | Menu/modal-open flag. |
| `0x430`, `0x434` | Subtitle active and text ID; special doors use the same path. |
| `0x43c`–`0x444` | Shared selection, document scroll, and document-view flag. |
| `0x448` | Eight 12-byte custom marker descriptors `{x,y,type}`. |
| `0x4b0`–`0x4c0` | Gun HUD enable and four gun textures. |
| `0x4c4`, `0x4c8` | Two door-marker textures. |
| `0x4e4` | Eight `{sin,cos}` map-direction vectors. |

`GameUI_ResetLevelState` (`0x0005fc10`) clears transient level/modal state but does not clear the custom marker descriptors. Day resource loading at `0x000562dc` acquires `qiang_01`–`qiang_04` at cache level 2 only for day 5 and resets the gun-HUD enable word. `GameUI_ClearDayResources` (`0x000563fc`) is an original empty function.

## HUD and overlay order

`GameUIDlg_Update` (`0x000566cc`) is the native dispatcher. Outside the menu, the portable controller preserves this order:

1. A completed player death draws SDT state 7 elements 1 and 2 and suppresses ordinary HUD processing.
2. Interaction prompt, QTE, hide mask, modal dialog, health, gun HUD.
3. Subtitle/special-door bars and text last.

The prompt uses one quarter of its texture per frame and advances every four frames. State 7 element 0 advances only while `0 < health <= initialHealthTenths` (10 at the default 100 HP), so the danger animation stays stopped at full health. The inventory-only health selector at `0x0005a3f4` chooses SDT elements 8/9/10 from current/max health.

QTE functions are `GameUI_ShowQte` (`0x0005ac74`) and `GameUI_FailQte` (`0x0005acdc`). Keys are stored zero-based. Single-hit mode follows the mapped phone key; mash mode alternates from the remaining frame count. Failure displays four result textures at two frames each. Waiting hints use the player's state counter and original QTE duration to select blend modes 3/4/5 at the two endpoints. The existing `PlayerStateSet::CQTE` remains the sole input/result state machine.

Hide masks use `fengmian_bj` and `fengmian_bj2` from `ui`, selected by the hide index. Subtitles fill the top and bottom 32-pixel strips black and draw anchored base-font text in the lower strip. A high-`0x10000` door resource index selects the same subtitle path, with the low word as its text ID.

## Dialogs

`GameUIDlg_RenderDialog` (`0x0005e570`) implements five kinds:

| Kind | Background | Behavior |
|---:|---|---|
| 0 | `juqing_01` plus `touxiang_02` | Event text; action 5 advances one wrapped line and closes after the final line. |
| 1 | `juqing_02` | Choice; left/right clamp at the ends, action 5 records a zero-based result. Portrait mode closes and clears the four options. |
| 2 | `juqing_03` | Timed tutorial/text; setup does not change manager pause. Closes at timer zero and unpauses only when player navigation is enabled. |
| 3 | `juqing_01` | Simple message; action 5 advances lines, stores result 0, and closes. |
| 4 | `juqing_01` | Timed result; at zero stores result 0 and becomes inactive without an extra implicit close. |

`GameUI_CloseDialog` (`0x0005ded4`) clears active and secondary-label state and unpauses the manager. **It does not clear the result.** `GameUI_ClearDialogResult` (`0x0005e380`) is the explicit -1 reset.

Dialog string slots 0–3 are choices. Slot 4 aliases the secondary text label at `0x35c`. Pickup writes the item name to that slot before calling `GameUI_ShowChoiceDialog`; choice setup resets it, so the original ordering clears the label. This observable native quirk is preserved rather than fixed.

## Inventory, documents, and unlocks

The existing native `Player` functions remain authoritative:

- `Player_AddItem` (`0x0003c230`), `Player_UseItem` (`0x0003c76c`), and `Player_CombineItems` (`0x0003c940`);
- nine ordinary slots and 128 note slots;
- health packs occupy slot 0, increment the manager-owned count, heal half maximum health, and compact only when the count reaches zero;
- item-trigger use returns 2 and publishes the used item ID;
- combination is directional and takes **slot indices**, consumes both records, adds the configured result, and preserves its newly selected slot.

The previous hook calls passed item IDs. The portable hooks now pass the selected native slot, and the controller forwards to these player functions. Inventory/action/combination screens draw `nameString`; examine mode draws `descriptionString`. Error text uses string 14 in red, and selecting the same combination slot uses string 26.

Item IDs below 1001 update model unlock progression. Notes 1001–2000 update documents, and 2001 upward update photos. The portable `GameConfiguration` remains the persistent unlock owner, while `GameContent` mirrors current player inventory/notes for the existing dialogs. The files screen lists documents and photos, supports scrolling and close/reopen, and honors player-selected note/document slots.

`HelpDlg` consumes item uses, kills, elapsed seconds, deaths, files, and secrets. The former `nPlayerCounter710` is now `nKillCount` in Ghidra and `killCount` in portable code. Secret count is the number of set scene events in this 22-entry table at `0x001414e8`:

```text
51 55 60 72 74 8 189 234 245 248 250
282 306 307 309 314 301 322 323 508 522 515
```

Game completion sets `extraWeapon0` from event 998, `extraWeapon1` from event 999, and `newGameOptions` to one before the normal configuration save. `CalcRank` (`0x000669fc`) remains empty.

Gun upgrades are guarded by scene event **1002**, not by the persistent `extraWeapon1` field. Tutorials 41 and 42 are 64-frame kind-2 timed GameUI messages.

## Maps

`GameUIDlg_RenderMap` (`0x0005af24`), `GameUIDlg_ProjectMapPosition` (`0x0005fda0`), and `GameUIDlg_LoadMapTexture` (`0x0006149c`) contain the native behavior.

Unavailable maps clear black, draw string 29 and the standard help textures. This includes the explicit native ID set and the 5000-series group. Day 6 uses `day6` from the Chinese `ui` directory and fixed screen positions for its maps. Other days select floor resources as follows:

- day 5: `_1f_day5`, `1f_day5`, `2f_day5`, `3f_day5`, including floor zero;
- other days: no floor-zero image, then `1f_00`, `2f_00`, `3f_00`.

Maps pan by ten pixels per held direction and clamp to the image/help-bar bounds. Accept/Number6 cycles floors, wrapping to zero on day 5 and one otherwise. Returning to the current floor re-centers on the player. Maps 1030 and 1052 select floor 2 above the native -10.0 Y threshold and project through maps 2030 and 2048 respectively.

### Calibration

The table at `0x0013f594` contains **700** `{width,height,offsetX,offsetY}` int16 records. For ordinary maps:

```c
group = mapId / 1000;
slot = mapId % 100 - 1;
record = table[group * 100 + slot];
xRatio = (position[xAxis] - minimum[xAxis]) / (maximum[xAxis] - minimum[xAxis]);
zRatio = (position[zAxis] - minimum[zAxis]) / (maximum[zAxis] - minimum[zAxis]);
x = record.width - FixedToInt(zRatio * record.width) + record.offsetX;
y = FixedToInt(xRatio * record.height) + record.offsetY;
```

The manager's six type-102 values are maximum/minimum XYZ triples, not an opaque header. Seven records at `0x00140230` remap axes and optionally invert each ratio for map IDs `2023, 2003, 2024, 2010, 2006, 2009, 2011` from `0x00140214`.

The eight-direction marker uses the recovered fixed-point arc cosine (`FixedArcCosine`, `0x00125454`; arc sine at `0x00125050`). Its truncated polynomial is intentionally approximate: outputs for inputs 0, +1, and -1 are `0x800`, `0x0ea`, and `0x0f17`. Do not replace these with host `acos` in the faithful configuration.

### Door markers

The table at `0x00140284` has **256** `{x,y,floor,day,icon}` int16 records. A record displays when its floor matches and the manager's corresponding door-state entry is positive. Day 6 additionally requires native record day 5. The door-state value minus one selects one of three horizontal animation frames in `door01`/`door02`. This is the existing 1024-entry manager block at `0x2500c`; it is not a second marker array.

Commands 104/105 also write/clear the eight custom descriptor records in GameUI. The native map function does not consume those descriptors as substitutes for the fixed table, so portable code does not invent such rendering.

## Item and About model rendering

`GameUIDlg_PrepareItemModels` (`0x0005f388`) and `AboutDlg_PrepareModelRenderer` (`0x0004b78c`) share item objects:

- inventory uses cache level 2, an 18-unit distant look-at camera, and a nine-cell grid;
- examine uses cache level 3, distance 8, and translation `(1.5,1,0)`;
- About uses cache level 1 and its existing zoom/16-frame direction transition;
- selected inventory items rotate by `0xcc` in the relevant modes;
- all paths apply the recovered `0xfc00` X rotation and submit to render queue 3.

## Recovered commands

| IDs | Behavior |
|---|---|
| 60 | Completion/unlock/save/return flow. |
| 61 | Open in-game save menu. |
| 62 | Pause/unpause manager. |
| 73–80 | Event text, dialog strings, choice, simple/timed text, result, close, and finished query. |
| 81 | Open map directly. This was incorrectly called `ClearRank` in the plan. |
| 82 | Empty `CalcRank`. |
| 83 | Another confirmed empty callback (`0x00066a24`). |
| 96–97 | Timed result dialog and close. |
| 101 | Subtitle/special-door text state. |
| 102 | Action enables: 5, 6, 7 individually and 1–4 together. |
| 104–105 | Set/clear custom map-marker descriptor. |
| 106 | Gun HUD enable. |
| 107–110 | Item-use, kill, play-time, and death counters. |
| 111–112 | Note and secret counts. |

The real `day1/ep0` path passes command 102 and executes input query ID 47 (`0x00065560`) through the completed dispatcher. ID 102's native Boolean arguments are suppression values: nonzero disables the selected actions.

## Verification and Ghidra state

`test_game_ui.cpp` covers every command above, dialog state/result behavior, empty/invalid item operations, health and directional combination through engine input, real map-2008 pickup and unlocks, statistics/secrets, map bounds/door state, arc-cosine boundaries, and configuration output in a temporary directory. Live soft-right and asterisk clicks open/close the gameplay menu and direct map through `RunFrame`; opening requires the native idle condition `mapProgress < 0`, not the former portable `>= 0`. `test_game_triggers.cpp` also drives shipped `day1/ep0` through the introductory messages and timed text, verifies ground movement, and presses Enter to acquire the real map-2008 papers.

Exact FNV-1a dummy-presented frame baselines are:

```text
HUD            f0cc850d    event          6bbfe04d
simple         14c8dd85    choice         dd20f75d
timed          a4f908dd    QTE            d787fb25
QTE failure    6c7337dd    subtitle       bfa62de5
hide mask      0fcefc15    menu           bd0cd14d
inventory      f0801085    item actions   403fa5cd
item examine   375e0015    map            ac605735
map/other hand 96792195     About model    d21cc8dd
```

The event baseline includes visible dialogue text after correcting localized day-font loading and the accept/number-6 action bindings. HUD checks also assert that the danger animation remains at time zero at full health and advances at 10 HP.

At stage-8 completion, all 35 default tests passed serially. STDIO logging and host-math variants each passed 35; core and combined ASan/UBSan builds passed 28; the WAR-nibble variant passed 35. After stage 9, the corresponding current counts are 36 for default/logging/math/WAR and 29 for core/ASan-UBSan. These builds use separate directories and serial CTest execution.

Ghidra contains 59 recovered function renames, the typed 0x524-byte GameUI layout and marker record, corrected player statistics fields, typed GameUI/GameSI signatures, behavioral comments, `stage8` tags, two bookmarks, and a completion property. The program was saved after verifying typed decompilation.

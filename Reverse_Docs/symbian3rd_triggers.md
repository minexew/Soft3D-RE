# Symbian3rd interaction triggers

## Scope and evidence

Stages 6–7 of `game_completion_plan.md` recover `EventTrigger`, `PickTrigger`, `DoorTrigger`, `ActTrigger`, `HideTrigger`, `ItemTrigger`, and `TalkTrigger` from the ARMv5 Symbian 3rd binary. The loaded Ghidra memory block starts at `0x8000`; addresses below are loaded addresses and already include that bias. Do not add it again. Leaked early command tables are not evidence for IDs, signatures, or behavior.

The portable implementation is in `Reversed_Game/src/game/Triggers.*`, with construction and map-item integration in `MyGameUnitMgr.*` and commands in `GameSI.*`. `TriggerUiHost` remains the narrow interaction boundary. In stage 8, `Scene` installs its concrete `GameUiController` as the production host, forwarding to the real `GameUIDlg`; the manager-owned `InteractionState` is a standalone fallback/test host, not the application substitute. Dialog input, rendering, pause, and result behavior are documented in `symbian3rd_gameui.md`.

## Shared proximity and interaction rules

All seven classes derive from `GameUnit` and enter manager bucket 4. Their types are Event `0x1b`, Act `0x21`, Door `0x22`, Hide `0x23`, Pick `0x25`, Item `0x27`, and Talk `0x28`.

These triggers query the player's ordinary collision bounds when not pushing. While pushing, `ObjectBounds_Translate` (`0x00013b30`) instead adds the saved push-local bounds to the current player position. Pick, Door, Act, Hide, and Talk require a living player; Event and Item do not perform the death-state check. Hide also suppresses interaction while the player is already hidden or in FPS mode.

The interaction prompt IDs written by `GameUI_SetInteractionPrompt` (`0x0005a6a0`) are:

- `-1`: no prompt;
- `0`: pickup;
- `1`: door;
- `3`: act/interaction;
- `4`: hide;
- `5`: talk.

Pick, Door, Act, Hide, and Talk also set the player's attack-input disable word at native `0x800` while their proximity prompt is active. Each trigger clears it independently when its own prompt ends, including when another trigger may still be active. `GameUI_ShowEventMessage` (`0x0005df50`), `GameUI_ShowSimpleMessage` (`0x0005e0d0`), and `GameUI_ShowChoiceDialog` (`0x0005e2ac`) pause the manager. `GameUI_GetDialogResult` (`0x0005e358`) returns -1 while pending. Choice results are zero-based; Act stores result + 1. `GameUI_IsDialogActive` (`0x00054fd0`) reads the active word at GameUI `0x344`; `GameUI_CloseDialog` (`0x0005ded4`) clears it and unpauses the manager while preserving the result. Slot 4 aliases the secondary label at `0x35c`; choice setup clears that label after Pickup writes the item name, preserving the original ordering quirk.

## Native layouts

### `EventTriggerLayout` — 0x64 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54` | Player pointer |
| `0x58` | GameUI pointer |
| `0x5c` | Text/string ID |
| `0x60` | Signed 16-bit remaining count |
| `0x62` | Signed 16-bit outside flag, initially 1 |

`EventTrigger_Update` (`0x00071dd8`) returns immediately when count is zero. On entry it shows kind-0 event text, decrements only a positive count, and sets outside to zero. Exit sets outside to one only while count remains nonzero. Consequently, a finite event whose last activation changes the count to zero stays latched as inside/completed even after the player exits. Count -1 repeats after every exit/reentry; other negative counts also avoid decrementing. `EventTrigger_IsInside` (`0x00071c9c`) returns `outside == 0`.

Persistence at `0x00071f34`/`0x00071f98` appends `int16 count`, `int32 textId`, `int16 outside` to `GameUnit` state.

### `PickTriggerLayout` — 0x74 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54` | Enabled word, initially 1 |
| `0x58` | Dialog title text ID, initially -1 |
| `0x5c` | Prompt-active word |
| `0x60` | Dialog-active word |
| `0x64` | Linked `Pickable *` |
| `0x68` | Player pointer |
| `0x6c` | Control map pointer |
| `0x70` | GameUI pointer |

`PickTrigger_IsAvailable` (`0x0003f514`) requires enabled, a linked Pickable, and item ID != -1. `PickTrigger_Update` (`0x0003ef90`) displays prompt 0 while available and nearby. Action 5 behaves as follows:

1. For item IDs below 1000, `Player_IsInventoryFull` (`0x0003c168`) rejects pickup with simple message 5 when all nine slots are occupied. An empty slot or an existing health-pack slot permits pickup.
2. Otherwise it opens a two-choice dialog. Its title is the trigger's text ID, option strings 0 and 1 are text IDs 3 and 4, and dialog string slot 4 receives the item/document/photo name.
3. Result 0 invokes `PickTrigger_TryPickUp` (`0x0003f3a4`); result 1 clears dialog-active state without pickup.

A successful pickup calls `Pickable_PickUp`, destroys the linked bucket-2 unit, nulls the trigger pointer, and hides the prompt. Disabling with `PickTrigger_SetEnabled` (`0x0003ef34`) hides an active prompt immediately; the next update clears the remaining proximity/attack state.

The `+0x04` gameitem word is therefore a **dialog title text ID**, not a sound index. The actual generic item-acquisition sound is separately played by `Player_AddItem` at index 141. This corrects the earlier stage-1 consumer label.

Save (`0x0003f440`) appends enabled, title text ID, and linked thing ID. Load (`0x0003f4c0`) reads only enabled and title text ID; `MyGameUnitMgr_LoadUnitState` reads the following linked thing ID and restores the pointer.

### `DoorTriggerLayout` — 0xa4 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54`, `0x58` | Unlocking flag and eight-frame timer |
| `0x5c` | Enabled word, initially 1 |
| `0x60` | Door/cinematic resource index |
| `0x64`, `0x68` | Next map and episode IDs |
| `0x6c`, `0x78` | Spawn position and direction |
| `0x84` | Required item ID |
| `0x88` | Locked-message text ID |
| `0x8c`, `0x90` | Prompt-active and dialog-active words |
| `0x94`, `0x98`, `0x9c`, `0xa0` | Player, controls, GameUI, and manager pointers |

`DoorTrigger_Update` (`0x00075fbc`) shows prompt 1 while enabled and nearby. On action 5:

- Required item zero, or a negative item ID after a previously stored open state, opens immediately through `DoorTrigger_OpenDoor` (`0x00076484`).
- A present positive item plays sound 8, removes the item, and sets unlocking. Eight subsequent enabled updates request the transition.
- A missing item plays sound 9 and shows the locked text. A negative required item is always locked.
- Door IDs 10000 and above use the manager's door-state array. State 1 opens without another item check; state 2 records a missing positive item; state 3 records a negative-item lock; successful access stores state 1.
- Door ID 10000 additionally checks registration and calls `Scene_BeginAuthorization` (`0x00030dd0`) plus input clearing when registration is incomplete. The portable scene records the same two authorization words; the network/license protocol remains stage 14.

`DoorTrigger_OpenDoor` calls `Scene_SetLoadMapParams` with black, then `Scene_LoadNextLevel`, and hides the prompt. Disabling a prompted door hides the prompt and resets unlocking/timer (`0x00075f48`).

The manager owns 1024 32-bit door states at native `0x2500c` (0x1000 bytes). `MyGameUnitMgr_LoadDayContent` (`0x00043de4`) clears this block on every day load. It is distinct from the scene's two 0x2800-byte event/snapshot blocks.

Door persistence (`0x00076518`/`0x000765cc`) includes resource index, next map/episode, spawn position/direction, required item, and locked text. It excludes enabled, unlocking, timer, prompt, and dialog state.

### `ActTriggerLayout` — 0xa0 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54` | Enabled word |
| `0x58` | Option count |
| `0x5c` | Reserved word written by command index 1 and persistence |
| `0x60`–`0x68` | Three option text IDs |
| `0x6c` | Dialog title text ID |
| `0x70` | Prompt-active word |
| `0x74` | Dialog-active word |
| `0x78` | Selection/result + 1 |
| `0x7c`, `0x80`, `0x84` | Player, controls, and GameUI pointers |
| `0x88`, `0x94` | Optional camera position and target |

There are **three** option strings followed by a separate dialog title. `GameSI_SetActTriggerString` indices have the native meanings 0 = option 0 alias, 1 = reserved `0x5c`, 2–4 = options 0–2, 5 = title, and 6 = prompt-active. The `0x5c` word has no other recovered consumer, so its purpose is left unnamed rather than invented.

`ActTrigger_Update` (`0x0008ae18`) shows prompt 3 unless `ActTrigger_IsSuppressed` (`0x0008b308`) finds an attacking enemy while option count is zero. A nonzero option count permits interaction during that combat condition. Action 5 opens a kind-3 simple message for zero options or a choice dialog otherwise.

Different camera position/target components select fixed mode, normalize target-minus-position, freeze and immediately update the camera, and hide the player. On a non-pending dialog result, the trigger unfreezes the camera, resets the scene's camera-warmup counter, requests instant mode, and restores player visibility when not in FPS mode, stores result + 1, and clears dialog-active state. This field is not the audio-listener selector. `ActTrigger_SetEnabled` (`0x0008ada4`) clears both camera vectors on every enable or disable call.

Act persistence (`0x0008b35c`/`0x0008b3f8`) stores enabled, option count, reserved `0x5c`, all three option strings, and title in that order. Selection, prompt/dialog state, and camera vectors are transient.

### `HideTriggerLayout` — 0x90 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54` | Enabled word, initially 1 |
| `0x58`, `0x64` | Hide position and camera-direction vectors |
| `0x70`, `0x74` | Prompt-active and dialog-active words |
| `0x78` | Mask index, initially 0 |
| `0x7c`, `0x80` | Horizontal/vertical angular limits, initially `0x9999`/`0xcccc` |
| `0x84`, `0x88`, `0x8c` | Player, controls, and GameUI pointers |

`MyGameUnitMgr_AddHideTrigger` (`0x00044708`) stores `maskIndex = scriptMask - 1`. `HideTrigger_Update` (`0x00054ad8`) shows prompt 4 while nearby. Mask -1 enters hiding directly when no enemy is attacking. Other masks open choice title 24 with option text IDs 3 and 4; result 0 enters and result 1 clears dialog state. An attacking enemy shows simple text 25 for a masked trigger, while the direct mask--1 path simply does nothing. `HideTrigger_EnterHide` (`0x00054de8`) calls `Player_BeginHide`, hides the prompt, and clears only prompt-active; it does not clear dialog-active. Disabling through `0x00054a7c` hides an active prompt immediately, and the next update clears the remaining proximity/attack state.

Hide persistence (`0x00054e98`/`0x00054f2c`) appends enabled, hide position, camera direction, prompt-active, dialog-active, and mask index. **The two angular limits are not serialized.** Typed decompilation corrected the earlier assumption that the final three words were mask plus both limits.

### `ItemTriggerLayout` — 0x7c bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54` | Enabled word, initially 1 |
| `0x58` | Player pointer |
| `0x5c`–`0x78` | Eight item IDs, initially -1 |

`ItemTrigger_AddItem` (`0x0008f410`) fills the first negative slot and ignores additions once all eight are occupied. `ItemTrigger_Update` (`0x0008f13c`) does not pick up or grant anything: while enabled and overlapping, it appends itself to the player's transient item-trigger list. `ItemTrigger_AcceptsItem` (`0x0008f38c`) scans all eight IDs. `Player_UseItem` returns 2 and sets `usedItem` for a match without removing the item. The player clears the registration list in both update branches; the persistent item-use statistic is a different field.

Item persistence (`0x0008f284`/`0x0008f308`) appends enabled and all eight signed 32-bit item IDs.

### `TalkTriggerLayout` — 0xb8 bytes

| Offset | Field |
|---:|---|
| `0x00` | `GameUnit base` |
| `0x54`, `0x58` | Enabled and fade-at-end words |
| `0x5c` | State timer |
| `0x60` | Fixed camera position |
| `0x6c`, `0x7c` | Native growable camera and line vectors |
| `0x8c` | Prompt-active word |
| `0x90`, `0x94` | Running and active words |
| `0x98` | Phase: 0 dialogue, 1 final fade-out, 2 final fade-in |
| `0x9c` | Completed latch |
| `0xa0`, `0xa4`, `0xa8` | Current-line pointer, line index, and write-only line-advance state |
| `0xac`, `0xb0`, `0xb4` | Player, controls, and GameUI pointers |

A camera record is 0x1c bytes: `{int32 actorId; Vec3 target; Vec3 axisX}`. Actor -1 selects the player; other IDs select bucket-2 things. A line record is four bytes: `{int16 textId; uint8 duration; int8 cameraIndex}`.

`TalkTrigger_Update` (`0x00093b70`) shows prompt 5 only nearby with no attacking enemy. Action 5 also requires navigation to be disabled. Interaction writes native suppression value 1 to controls 6, 7, and 1–4, sets running, timer 6, and starts a black five-frame fade-out. At timer zero, `TalkTrigger_StartDialogue` (`0x000941ac`):

1. Sets active, clears line index/phase/completed, snapshots every actor position into its camera target, applies the record's axis X through the visual virtual slot, and updates the actor.
2. Freezes the camera and selects fixed mode at the trigger's camera position, looking at the selected camera target plus Y `0x10000`.
3. Closes any old dialog, opens the first line as a simple message, clears input, sets timer 6, and starts a five-frame fade-in.
4. Adds five to a nonzero first-line duration with byte wrapping. No other Talk consumer of duration exists in this build.

While active in phase 0, the timer first counts down to zero, then the trigger waits for the dialog to become inactive. `TalkTrigger_AdvanceDialogue` (`0x00094544`) selects and opens the next line, returning false, or returns true after the final line. Finalization enters phase 1, sets timer 6, starts fade-out, and decrements to 5 in that same update.

At phase-1 zero, `TalkTrigger_FinishDialogue` (`0x000946d8`) unfreezes the camera, resets the scene's **camera-warmup counter**, requests an immediate camera update, closes the dialog, and clears input. Without fade-at-end it writes native zero to restore the dialogue controls and immediately sets completed. Otherwise phase 2 sets timer 6, starts fade-in, and completes at zero. The active word is set at dialogue start and cleared at completion; the completed word is a separate latch. `TalkTrigger_SetEnabled` (`0x00093afc`) hides an active prompt on disable and always resets line index and timer, but does not reset active/completed state.

Talk persistence (`0x000948c0`/`0x00094a68`) stores enabled, fade-at-end, camera position, camera count and each `{actorId,target,axisX}`, then line count and each line as three 32-bit values in the order `{duration,textId,cameraIndex}`. Runtime prompt/running/active/phase/completion/current-line state is excluded.

`MyGameUnitMgr_Update` (`0x0004519c`) updates every Talk trigger before its pause gate. An unpaused normal unit pass updates Talk a second time; a paused pass updates only bucket 1 and Item triggers after that Talk pre-pass. This ordering is required for dialogue progress while simple messages pause the manager.

## Manager helpers and real map items

`MyGameUnitMgr_AddEventTrigger` (`0x000442bc`), `AddPickTrigger` (`0x000443a8`), `AddDoorTrigger` (`0x000444bc`), and `AddActTrigger` (`0x0004460c`) allocate the layouts above and insert bucket 4. Stage-7 helpers are `AddHideTrigger` (`0x00044708`), `AddItemTrigger` (`0x00044820`), and `AddTalkTrigger` (`0x000448e8`). Talk construction snapshots the current controller position as its default fixed-camera position.

`MyGameUnitMgr_FinishMapLoad` (`0x00043e80`) calls `MyGameUnitMgr_SpawnMapItems` (`0x00047198`) only for a new map, not when restoring saved state. `SpawnMapItems` selects the first gameitem group matching the current map, skips zero item IDs, and creates descending-ID Pickable/PickTrigger pairs beginning at 9999. Each Pickable retains a mutable pointer to its originating record; pickup zeroes that record's item ID and suppresses later respawn. Duplicate map groups remain valid and the first wins.

Real day-1 map 2008 creates exactly one pair: unit ID 9999, item 1005, and dialog title text ID 26.

## `GameSI` commands

All handlers use the standard argument array/count/output convention.

| ID | Handler | Arguments and behavior |
|---:|---|---|
| 149 | `GameSI_AddEventTrigger` `0x68b78` | Short ID, integer count truncated to 16 bits, short text ID, six fixed bounds values. |
| 150 | `GameSI_SetEventTriggerCount` `0x68cbc` | Sets the signed 16-bit count. |
| 151 | `GameSI_SetEventTriggerText` `0x68d24` | Sets the text ID. |
| 152 | `GameSI_IsEventTriggerInside` `0x68d8c` | Returns `outside == 0`, including the finite completion latch. |
| 153 | `GameSI_AddPickTrigger` `0x68df0` | Short trigger ID, linked thing ID, dialog title text ID, six bounds values. |
| 154 | `GameSI_EnablePickTrigger` `0x68f34` | Sets enabled state. |
| 155 | `GameSI_AddDoorTrigger` `0x68fa4` | ID, required item, locked text, door resource, next map/episode, spawn XYZ, direction XYZ, six bounds values. |
| 156 | `GameSI_SetDoorTriggerItem` `0x69200` | Sets required item. |
| 157 | `GameSI_SetDoorTriggerLockedText` `0x69268` | Sets locked-message text ID. |
| 158 | `GameSI_EnableDoorTrigger` `0x692d0` | Sets enabled state; missing IDs are ignored. |
| 159 | `GameSI_AddActTrigger` `0x6934c` | Short ID and six bounds values. |
| 160 | `GameSI_SetActTriggerOptionCount` `0x69450` | Sets option count. |
| 161 | `GameSI_SetActTriggerString` `0x694b8` | Index/text mapping described above. |
| 162 | `GameSI_GetActTriggerSelection` `0x69578` | Returns selection; missing IDs return zero. |
| 163 | `GameSI_ClearActTriggerSelection` `0x695f8` | Stores -1; missing IDs are ignored. |
| 164 | `GameSI_SetActTriggerCamera` `0x69660` | Sets camera position/target; missing IDs are ignored. |
| 165 | `GameSI_EnableActTrigger` `0x6979c` | Sets enabled state and clears camera vectors. |
| 166 | `GameSI_AddHideTrigger` `0x6980c` | Short ID, one-based mask, hide XYZ, camera-direction XYZ, six fixed bounds values. |
| 167 | `GameSI_SetHideTriggerLimits` `0x699f8` | Sets horizontal and vertical fixed angular limits. |
| 168 | `GameSI_EnableHideTrigger` `0x69a9c` | Sets enabled state. |
| 169 | `GameSI_AddItemTrigger` `0x69b0c` | Short ID, initial item ID, six fixed bounds values. |
| 170 | `GameSI_AddItemTriggerItem` `0x69c30` | Adds a short item ID to the first free slot. |
| 171 | `GameSI_EnableItemTrigger` `0x69ca0` | Sets enabled state. |
| 172 | `GameSI_AddTalkTrigger` `0x69d10` | Short ID, six fixed bounds values, fade-at-end boolean. |
| 173 | `GameSI_SetTalkTriggerCameraPosition` `0x69e34` | Sets fixed camera XYZ. |
| 174 | `GameSI_AddTalkTriggerCamera` `0x69eec` | ID, short actor ID, target XYZ, axis-X XYZ. |
| 175 | `GameSI_AddTalkTriggerLine` `0x6a020` | ID, short text ID, unsigned-byte duration, signed-byte camera index. |
| 176 | `GameSI_IsTalkTriggerActive` `0x6a0cc` | Returns the active word; missing IDs return false. |
| 177 | `GameSI_IsTalkTriggerCompleted` `0x6a14c` | Returns the completed latch; missing IDs return false. |
| 178 | `GameSI_EnableTalkTrigger` `0x6a1d8` | Sets enabled and resets line index/timer. |
| 179 | `GameSI_RemoveInteractionTrigger` `0x6a248` | Deleting removal from bucket 4. |

Portable lookups validate class/ID where the original would dereference an invalid pointer; the ARM-defined missing-ID behavior above is preserved explicitly.

## Verification

`Reversed_Game/tests/test_game_triggers.cpp` covers every stage-6/7 command, finite/infinite event entry and exit, pickup accept/cancel/full-inventory/disable behavior, door item/lock/timer/tracking/authorization states, act choices and camera-warmup restoration, hide confirmation/direct masks and player state, item capacity and paused registration, navigation/combat gates, multi-line Talk camera/dialog timing and both completion paths, removal, and second-save byte equality. Camera/Light fixtures also verify their native persistence fields without changing stage-3 camera behavior.

Real-resource tests verify map-2008 item creation, spawn-record clearing and non-respawn, the no-spawn path for restored saves, and item-trigger use through `GameEngine::RunFrame`. The real `day1/ep0` engine-frame-loop path passes every stage-6 command. After stages 7–8 it reached then-unimplemented input query 47 (`0x00065560`); stage 15 implements the query and continues execution.

All 35 default CTest tests pass serially in stage 8. Core and ASan/UBSan each pass 28 SDL-independent tests; WAR, STDIO logging, and host-math builds pass all 35. Stage-7 Ghidra state includes Hide/Item/Talk layouts, camera/line record and vector types, typed class/manager/command signatures, behavioral plates, 67 verified `stage7` tags, a completion property, and a saved bookmark at `TalkTrigger_Update`.

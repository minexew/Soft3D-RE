# Symbian3rd event, pick, door, and act triggers

## Scope and evidence

Stage 6 of `game_completion_plan.md` recovers `EventTrigger`, `PickTrigger`, `DoorTrigger`, and `ActTrigger` from the ARMv5 Symbian 3rd binary. The loaded Ghidra memory block starts at `0x8000`; addresses below are loaded addresses and already include that bias. Do not add it again. Leaked early command tables are not evidence for IDs, signatures, or behavior.

The portable implementation is in `Reversed_Game/src/game/Triggers.*`, with construction and map-item integration in `MyGameUnitMgr.*` and commands in `GameSI.*`. `TriggerUiHost` is the narrow interaction-state boundary until the final GameUI dialogs are implemented in stage 8. `MyGameUnitMgr` owns the real `InteractionState`; it does not auto-accept a dialog or substitute a test double in the application path.

## Shared proximity and interaction rules

All four classes derive from `GameUnit` and enter manager bucket 4. Their types are Event `0x1b`, Act `0x21`, Door `0x22`, and Pick `0x25`.

Event, Pick, Door, and Act query the player's ordinary collision bounds when not pushing. While pushing, `ObjectBounds_Translate` (`0x00013b30`) instead adds the saved push-local bounds to the current player position. Pick, Door, and Act require a living player; Event does not perform the death-state check.

The interaction prompt IDs written by `GameUI_SetInteractionPrompt` (`0x0005a6a0`) are:

- `-1`: no prompt;
- `0`: pickup;
- `1`: door;
- `3`: act/interaction.

Pick, Door, and Act also set the player's attack-input disable word at native `0x800` while their proximity prompt is active. Each trigger clears it independently when its own prompt ends, including when another trigger may still be active. `GameUI_ShowEventMessage` (`0x0005df50`), `GameUI_ShowSimpleMessage` (`0x0005e0d0`), and `GameUI_ShowChoiceDialog` (`0x0005e2ac`) pause the manager. `GameUI_GetDialogResult` (`0x0005e358`) returns -1 while pending. Choice results are zero-based; Act stores result + 1.

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

Different camera position/target components select fixed mode, normalize target-minus-position, freeze and immediately update the camera, and hide the player. On a non-pending dialog result, the trigger restores camera freeze/listener/instant state and player visibility when not in FPS mode, stores result + 1, and clears dialog-active state. `ActTrigger_SetEnabled` (`0x0008ada4`) clears both camera vectors on every enable or disable call.

Act persistence (`0x0008b35c`/`0x0008b3f8`) stores enabled, option count, reserved `0x5c`, all three option strings, and title in that order. Selection, prompt/dialog state, and camera vectors are transient.

## Manager helpers and real map items

`MyGameUnitMgr_AddEventTrigger` (`0x000442bc`), `AddPickTrigger` (`0x000443a8`), `AddDoorTrigger` (`0x000444bc`), and `AddActTrigger` (`0x0004460c`) allocate the layouts above and insert bucket 4.

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

Portable lookups validate class/ID where the original would dereference an invalid pointer; the ARM-defined missing-ID behavior above is preserved explicitly.

## Verification

`Reversed_Game/tests/test_game_triggers.cpp` covers every command, finite/infinite event entry and exit, the finite completion latch, pickup accept/cancel/full-inventory/disable behavior, linked-unit removal, item present/missing/negative-item doors, the eight-frame unlock delay, tracked and authorization door states, act simple/choice results and clearing, combat suppression, camera override/restoration, and second-save byte equality.

Real-resource tests verify map-2008 item creation, spawn-record clearing and non-respawn, and the no-spawn path for restored saves. The real `day1/ep0` engine-frame-loop path now passes every stage-6 command and stops at the unrelated later-stage command 102 (`0x000671ac`), rather than stage-6 command 159.

All 34 default CTest tests pass serially. Core, WAR-nibble, and ASan/UBSan builds each pass 27 SDL-independent tests; STDIO logging and host-math builds each pass all 34. Ghidra contains the four corrected native layouts, typed class/manager/command signatures, behavioral comments, 69 `stage6` tags, a completion property, and a saved bookmark at `ActTrigger_Update`.

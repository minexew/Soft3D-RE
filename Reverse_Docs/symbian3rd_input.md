# Symbian 3rd input

## Object layout and key numbers

`Input_Ctor` (`0x000eec68`) allocates no storage itself; `S3dEngine_ctor` allocates the 49-byte (`0x31`) object at `Engine + 0x8c`. The exact layout is:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `uint8_t` | Legacy any-key flag, initialized to zero but never changed by the recovered event methods. |
| `+0x01` | `uint8_t[24]` | Key-state slots. Slot zero is unused; valid engine keys are 1–23. |
| `+0x19` | `uint8_t[24]` | Repeat counters, indexed identically. |

`TranslateKey` (`0x000ad658`) maps Symbian scan codes as follows. The enum names in the portable implementation describe the phone keys rather than reusing Dingoo button names.

| Engine key | Portable name | Symbian scan codes |
|---:|---|---|
| 1 | `Asterisk` | `0x2a` |
| 2 | `Hash` | `0x7f` |
| 3–12 | `Number1`–`Number0` | ASCII `1`–`0` (`0x30`–`0x39`) |
| 3–12 | Same number-key aliases | `M,R,T,Y,F,G,H,V,B,N` (`0x4d,0x52,0x54,0x59,0x46,0x47,0x48,0x56,0x42,0x4e`) |
| 13 | `Left` | `0x0e` |
| 14 | `Right` | `0x0f` |
| 15 | `Up` | `0x10` |
| 16 | `Down` | `0x11` |
| 17 | `SoftLeft` | `0xa4` (`EStdKeyDevice0`) |
| 18 | `SoftRight` | `0xa5` (`EStdKeyDevice1`) |
| 19 | `Accept` | `0xa7` (`EStdKeyDevice3`) |
| 0 | `None` | Any other scan code |

## State machine

State bits are `DOWN=0x01`, `FIRSTDOWN=0x02`, `PRESS=0x04`, and `CLICK=0x08`; zero is `UP`.

- `Input_Update` (`0x000eed30`) applies `state &= 0xf5` to slots 1–23. It clears only `FIRSTDOWN` and `CLICK`; `DOWN` and `PRESS` persist.
- `Input_KeyDown` (`0x000eeda4`) changes a key without `DOWN` to `DOWN|FIRSTDOWN`. While held, it increments the repeat counter only if `PRESS` is clear. When the incremented count exceeds five, it sets `PRESS` and resets the counter. Once `PRESS` is set, further held-down events do not increment the counter.
- `Input_KeyPress` (`0x000eef64`) ORs `PRESS` into the existing state and does not alter the repeat counter.
- `Input_KeyUp` (`0x000ef004`) changes `DOWN` to exactly `CLICK`, or a non-down key to exactly `UP`, then clears the repeat counter.
- A new down after release replaces the old state with `DOWN|FIRSTDOWN`.

All three event methods ignore key zero. Hardware events are admitted only when the legacy any-key byte is zero or the key is 17–19; programmatic events bypass this gate. Since none of these methods sets the any-key byte after construction, it remains zero in the examined build. That byte gates hardware events; it does not determine the any-key query results. `Input_IsAnyKeyClick` and the other any-key queries scan the 23 key-state bytes independently. The portable implementation rejects indices above 23 instead of reproducing the original out-of-bounds write.

The query helpers match the S2D inline definitions: `IsKeyDown` and `IsKeyUp` test `DOWN`; `IsKeyFirstDown`, `IsKeyPress`, and `IsKeyClick` test their respective bits; `IsKeyDownOrClick` accepts either `DOWN` or `CLICK`.

## Platform key-event wrapper

`GameView_OfferKeyEvent` (`0x00137fcc`, formerly unnamed) returns zero if the engine pointer is null. Otherwise it translates `keyEvent->iScanCode` and passes a true hardware-event flag:

- event type 2 (`EKeyUp`) → `Input_KeyUp`, return 1;
- event type 1 (`EKeyDown`) with `iRepeats == 1` → `Input_KeyPress`, return 1;
- event type 3 (`EEventKey`) → `Input_KeyDown`, return 1;
- other events return 0.

`Engine_RunFrame` calls `Input_Update` after video presentation, so consumers see transient flags for one complete frame before they are cleared.

## Menu control map

The scene control object has eight secondary bindings at `+0x28` and eight primary bindings at `+0x48`. `ControlMap_SetOrientation` (`0x000534a4`) installs both sets; changing handedness therefore resets customized secondary bindings as well as the primary directions.

| Action | Up | Down | Left | Right |
|---|---:|---:|---:|---:|
| Action number | 1 | 2 | 3 | 4 |
| Orientation 0 primary | Up (15) | Down (16) | Left (13) | Right (14) |
| Orientation 1 primary | Left (13) | Right (14) | Down (16) | Up (15) |
| Orientation 2 primary | Right (14) | Left (13) | Up (15) | Down (16) |
| Orientation 0 secondary | Number4 (5) | Number0 (11) | Number7 (7) | Number9 (9) |
| Orientation 1 secondary | Number7 (7) | Number9 (9) | Number0 (11) | Number4 (5) |
| Orientation 2 secondary | Number9 (9) | Number7 (7) | Number4 (5) | Number0 (11) |

Action 5 defaults to Accept (19) with Number6 (8) as its secondary binding, action 6 to SoftLeft (17) with Number1 (1), and action 7 to SoftRight (18) with Number3 (3). These fixed primary bindings are restored by the native constructor and are not changed by handedness; the secondary action-5 binding is Number6, not an empty slot. The eight words serialized at the start of `config.sdt` are the secondary bindings, including the unused action-zero slot. `ControlMap_IsActionFirstDown` (`0x00053970`), `ControlMap_IsActionDown` (`0x00053a84`), `ControlMap_IsActionPress` (`0x00053b98`), `ControlMap_IsActionUp` (`0x00053cac`), and `ControlMap_IsActionClicked` (`0x00053dc0`) test their corresponding input bits. Their mode argument selects primary-only or primary-plus-secondary matching, and the action-gate argument honors the corresponding action-disable word. Native global/action gate words use zero for enabled and nonzero for disabled; portable `ControlMap` exposes inverse Boolean values, and native callers are converted at their boundaries. `ControlMap_SetDisabled` (`0x00053704`) clears input when the global state changes. GameSI IDs 43–47 expose the five action queries, IDs 48/49 expose any-key DOWN/CLICK, ID 102 writes the four suppression-mask groups, and ID 115 sets global disabled state. `Input_IsAnyKeyDown` (`0x000ef15c`) and `Input_IsAnyKeyClick` (`0x000ef2a0`) scan keys 1–23. Stage 4's QTE code also uses the original primary-binding lookup and the action-5 both-bindings-up cancellation test.

The native action-query signatures include a fourth `respectActionGate` argument in addition to action and primaryOnly; the current portable public methods expose only the first two semantic choices and always honor the action gate. `ControlMap_IsActionUp` requires primary UP and, when primaryOnly=false, secondary UP, matching QTE cancellation. Its primaryOnly=true expression always returns false, whereas the current portable method returns primary UP in that case. This unresolved portable edge is assigned to Session 34A in `game_object_inventory.md`, with directed and real-QTE tests required before changing behavior.

Additional native any-key scans are FIRST_DOWN (`0x000ef0f0`), PRESS (`0x000ef1c8`), UP (`0x000ef234`), and DOWN_OR_CLICK (`0x000ef30c`). ControlMap wraps them at `0x00053108`, `0x00053168`, `0x00053198`, and `0x000531f8`. Synthetic action KeyDown/KeyPress/KeyUp methods at `0x00053754`–`0x000538bc` target both bindings only while global/action gates are enabled. These uncalled methods, `ControlMap_IsActionDownOrClick` (`0x00053ed4`), and `ControlMap_IsActionDisabled` (`0x00054028`) are also Session-34A recovery candidates.

`GameEngine_SetControlOrientation` (`0x000719ac`) stores the orientation, updates the renderer, and calls `ControlMap_SetOrientation`. Options can reassign secondary bindings while excluding duplicates. See `sdt_format.md` for the configuration file and concrete dialogs.

## Portable and SDL implementation

`Reversed_Game/src/soft3d/Input.*` implements the state machine and exact Symbian translation table. `Engine::GetInput` replaces `InputStub`; the initial menu now reads this state directly rather than accepting app-specific booleans.

`src/platform/sdl/SdlInput.*` is the only SDL event-mapping layer. Arrows and WASD map to engine directions, digits to number keys, Q/E to soft keys, Space/Enter/keypad Enter to `Accept`, and M, `SDLK_ASTERISK` (including Shift+8), and keypad multiply to `Asterisk`. The asterisk therefore opens the native action-6 map view when gameplay UI allows it. In landscape display, including the default automatic mode after handedness acceptance, directional keys receive the inverse of the current control-orientation rotation: for orientation 1, screen up/right/down/left become phone left/up/right/down; orientation 2 uses the opposite rotation. This cancels the original control map's rotation and keeps keyboard directions screen-relative. The automatic mode's initial portrait handedness screen and forced portrait mode pass phone directions through unchanged. Non-directional bindings are unaffected. Every SDL key-down, including repeats, calls `KeyDown`, preserving the engine repeat counter. Escape remains an application quit command and maps to no engine key.

`GameConfig.*` implements the recovered control map and exact config serialization. `GameEngine::SetControlOrientation` now updates both the renderer and control map, matching `0x000719ac`; the handedness and options dialogs use this combined entry point. `tests/test_input.cpp` covers all 23 keys and aliases, first-down/click clearing, six-call repeat threshold, persistent press state, simultaneous keys, lifecycle clearing, deterministic 17 FPS timing, and opt-in FPS text. `tests/test_sdlinput.cpp` covers both landscape compensation rotations, SDL aliases, digits, soft/accept keys, repeat events, simultaneous keys, and ignored non-key events. `tests/test_menu.cpp` exercises orientation-dependent navigation and configuration round trips.

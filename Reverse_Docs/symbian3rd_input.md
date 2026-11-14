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

All three event methods ignore key zero. Hardware events are admitted only when the legacy any-key byte is zero or the key is 17–19; programmatic events bypass this gate. Since none of these methods sets the any-key byte after construction, it remains zero in the examined build, and `IsAnyKeyClick` always returns false. The portable implementation rejects indices above 23 instead of reproducing the original out-of-bounds write.

The query helpers match the S2D inline definitions: `IsKeyDown` and `IsKeyUp` test `DOWN`; `IsKeyFirstDown`, `IsKeyPress`, and `IsKeyClick` test their respective bits; `IsKeyDownOrClick` accepts either `DOWN` or `CLICK`.

## Platform key-event wrapper

`GameView_OfferKeyEvent` (`0x00137fcc`, formerly unnamed) returns zero if the engine pointer is null. Otherwise it translates `keyEvent->iScanCode` and passes a true hardware-event flag:

- event type 2 (`EKeyUp`) → `Input_KeyUp`, return 1;
- event type 1 (`EKeyDown`) with `iRepeats == 1` → `Input_KeyPress`, return 1;
- event type 3 (`EEventKey`) → `Input_KeyDown`, return 1;
- other events return 0.

`Engine_RunFrame` calls `Input_Update` after video presentation, so consumers see transient flags for one complete frame before they are cleared.

## Portable and SDL implementation

`Reversed_Game/src/soft3d/Input.*` implements the state machine and exact Symbian translation table. `Engine::GetInput` replaces `InputStub`; the initial menu now reads this state directly rather than accepting app-specific booleans.

`src/platform/sdl/SdlInput.*` is the only SDL event-mapping layer. Arrows and WASD map to engine directions, digits to number keys, Q/E to soft keys, and Space/Enter/keypad Enter to `Accept`. Every SDL key-down, including repeats, calls `KeyDown`, preserving the engine repeat counter. Escape remains an application quit command and maps to no engine key.

`tests/test_input.cpp` covers all 23 keys and aliases, first-down/click clearing, six-call repeat threshold, persistent press state, simultaneous keys, lifecycle clearing, deterministic 17 FPS timing, and opt-in FPS text. `tests/test_sdlinput.cpp` covers SDL orientation aliases, digits, soft/accept keys, repeat events, simultaneous keys, and ignored non-key events.

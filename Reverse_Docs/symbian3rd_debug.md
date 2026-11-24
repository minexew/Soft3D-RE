# Symbian 3rd debug and diagnostic helpers

## `s3dMemInfo` is an empty class shell

RTTI identifies `s3dMemInfo` with typeinfo `0x0015d958`, name `0x0015d988`, and vtable function area `0x0015d918`. The vtable contains only its destructors and inherited `s3dBase::SetEngine`. It has no fields beyond `s3dBase` and no memory-query method.

`S3dMemInfo_Ctor` (`0x000f3640`) constructs the eight-byte object at `Engine + 0x58`; `S3dMemInfo_Dtor` (`0x000f3348`) and `S3dMemInfo_DeletingDtor` (`0x000f3a1c`) only tear down the base. All references are construction, destruction, vtable, and engine-link operations. Earlier notes that called this member `s3dTimer` were incorrect. The actual `s3dTimer` time source is at `Engine + 0x2c`, constructed by `S3dTimer_Ctor` (`0x000e47e8`) with vtable function area `0x0013e3d0`.

The portable `s3dMemInfo.hpp` deliberately reflects this empty class shell. It should not gain invented query methods.

## Standalone RAM queries

Useful diagnostic logic exists as two standalone S2D-compatible functions, not as `s3dMemInfo` methods:

| Address | Function | Behavior |
|---:|---|---|
| `0x000a77ec` | `MemTotal` | Return `TMemoryInfoV1::iTotalRamInBytes`, or `-1` if `UserHal::MemoryInfo` fails. |
| `0x000a7840` | `MemRemain` | Return `TMemoryInfoV1::iFreeRamInBytes` at structure offset `+0x0c`, or `-1` on failure. |

Both construct a `TMemoryInfoV1Buf` with `TMemoryInfoV1Buf_Ctor` (`0x000a79e4`) and access its payload with `TMemoryInfoV1Buf_Data` (`0x000a7a28`). The SDK declaration in `s2dbase_memory.h` confirms the corresponding `memtotal()`/`memremain()` roles and `-1` error result. Neither function has a caller in this game build, so there is no recovered game-facing memory display to enable. `s3dMemory.hpp` and `platform/linux/Memory.cpp` implement standalone wrappers using `sysinfo`, returning total/free bytes or `-1`. The portable API uses 64-bit values to avoid overflow on modern hosts and remains separate from the empty class. `tests/test_stream.cpp` verifies successful queries and their ordering.

## FPS text and stale real-FPS field

`Engine_DrawFPS` (`0x000f2c30`) formats the signed 32-bit value at `Engine + 0x18` with `"%d"` into a ten-character UTF-16 buffer and calls `S3dVideo_DrawText(text, width - 20, 10)`. `GameEngine_DrawFPS` (`0x00070ab0`) is empty.

The examined constructor does not initialize `Engine + 0x18`, and neither `Engine_LimitFrameRate` nor any other identified engine routine updates it. `Engine_RenderFrame` has no `DrawFPS` call. Thus the base helper is dormant debug code, not a functioning real-FPS measurement path in this release.

The reconstruction initializes the field to zero for deterministic behavior, keeps display disabled by default, and adds `Engine::SetFpsDisplayEnabled` plus the application's `--show-fps` option as an explicit restoration path. `tests/test_input.cpp` verifies identical dummy-video pixels with and without restoration and checks text `"0"` at `(300, 10)` on a 320-wide frame. This opt-in host overlay does not claim to restore the missing original FPS calculation.

## Dormant debug console

Stage 16 recovered a separate, compiled but unreachable debug console at `0x00095730`–`0x000966c7`. Its constructors, destructors, update, and renderer have no native callers, and it is not embedded in the native `Scene`; this is dead debug tooling, not an originally accessible cheat menu. The two constructor bodies (`0x00095730`, `0x00095860`) and two destructor bodies (`0x00095990`, `0x000959c0`) are duplicate compiled implementations and remain separately named in Ghidra.

The native `DebugConsoleLayout` is 0x68 bytes:

| Offset | Type | Purpose |
|---:|---|---|
| `0x00` | `Engine *` | Engine owner; the update reaches `GameEngine`/`Scene` through it. |
| `0x04` | `int32_t` | Active flag. |
| `0x08` | `int32_t` | Current choice, 0–10. |
| `0x0c` | `char16_t[11]` | Space followed by digits `0`–`9`. |
| `0x22` | `uint16_t` | Alignment padding. |
| `0x24` | `int32_t` | Entered-character count. |
| `0x28`, `0x2c` | `int32_t` | Text X/Y; X is 32 and Y vertically centers one font row. |
| `0x30` | `int32_t` | Renderer frame counter. |
| `0x34` | `VisString` | Display string. |
| `0x40` | `char16_t[8]` | Seven entered characters plus terminator. |
| `0x50`, `0x5c` | `Vec3` | Fixed-point map spawn position and direction. |

`DebugConsole_Update` (`0x00095a50`) uses key 2 (Hash) to toggle the console, keys 15/16 (Up/Down) to select a character, key 13 (Left) to delete, and key 14 (Right) to append. Input is capped at seven characters and Right resets the choice to space. Accept (key 19) or 7 (key 8) executes the current string, clears the key, closes the console, and returns zero; otherwise an active console returns one. The command grammar is:

| Entered text | Effect |
|---|---|
| one space | Restore player health to maximum. |
| two spaces | Set player health to one. |
| three spaces | Toggle the player's used-item healing flag. |
| `0` plus one digit | Select weapon 0–9. |
| space plus decimal digits | Add the parsed item with notification enabled. |
| two spaces plus decimal day 1–7 | Unload and load that day. A same-day reload clears the event block but preserves flags 1000–1002. |
| four decimal digits, greater than 100 and less than 7000 | Read that map's SBP header and start the corresponding map/episode transition with its spawn position/direction. |

`DebugConsole_LoadMapSpawn` (`0x0009639c`, formerly `LoadSbpMapHeader`) reads the 0xf8-byte SBP header from the current day and falls back to `day1`. It converts header floats 0–5 to 16.16 fixed point. Native file selection/read/version failures are unchecked and its return type is void. The portable implementation returns false for those failures rather than proceeding with invalid data. The one-space health command also performs the native `AddItem(0)` call; portable code preserves that call rather than special-casing it away.

`DebugConsole_Render` (`0x00096030`) draws a black strip, red entered text plus the current choice, and a five-pixel-wide, two-pixel-high white cursor on alternating render calls. The portable `game::DebugConsole` is **disabled by default** and is enabled explicitly with `Scene::SetDebugConsoleEnabled`; no normal application path enables it. When enabled, its update return pauses the scene and `SceneHost::PostRender` draws it after the UI overlay. `ResetTransitionForDebugJump` clears any active door transition before a debug map jump, preventing stale door resources after cache teardown without changing normal transitions. This host integration makes the recovered body usable without pretending the original binary called it. `test_game_scene.cpp` exercises real key input and engine frames for navigation, reopen/reset, the seven-character limit, every cheat family, map/day transitions (including a mid-transition jump under ASan), event preservation, rendering, and the default-disabled state.

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

Both construct a `TMemoryInfoV1Buf` with `TMemoryInfoV1Buf_Ctor` (`0x000a79e4`) and access its payload with `TMemoryInfoV1Buf_Data` (`0x000a7a28`). The SDK declaration in `s2dbase_memory.h` confirms the corresponding `memtotal()`/`memremain()` roles and `-1` error result. Neither function has a caller in this game build, so there is no recovered game-facing memory display to enable. Portable wrappers remain in the stage 12 diagnostics work rather than being presented as methods of the empty class.

## FPS text and stale real-FPS field

`Engine_DrawFPS` (`0x000f2c30`) formats the signed 32-bit value at `Engine + 0x18` with `"%d"` into a ten-character UTF-16 buffer and calls `S3dVideo_DrawText(text, width - 20, 10)`. `GameEngine_DrawFPS` (`0x00070ab0`) is empty.

The examined constructor does not initialize `Engine + 0x18`, and neither `Engine_LimitFrameRate` nor any other identified engine routine updates it. `Engine_RenderFrame` has no `DrawFPS` call. Thus the base helper is dormant debug code, not a functioning real-FPS measurement path in this release.

The reconstruction initializes the field to zero for deterministic behavior, keeps display disabled by default, and adds `Engine::SetFpsDisplayEnabled` plus the application's `--show-fps` option as an explicit restoration path. `tests/test_input.cpp` verifies identical dummy-video pixels with and without restoration and checks text `"0"` at `(300, 10)` on a 320-wide frame. This opt-in host overlay does not claim to restore the missing original FPS calculation.

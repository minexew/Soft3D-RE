# Symbian 3rd video and software rendering

## Scope and reliability

This document covers `SymbianBin/out.bin` (`out.bin` in Ghidra) at image base zero. Names listed as recovered have been applied to the program. Platform behavior is established from local control flow, RTTI, the corrected E32 import names, and the S60 3rd FP2 headers in `SDK/devices/`. The `CDirectScreenAccess` and `CDirectScreenBitmap` calls in this path now match those SDK declarations.

## Application and engine initialization

`AppUi_ConstructL` (`0x00138198`) creates the game view. `GameView_ConstructL` (`0x00137704`) initializes a fixed 320×240 descriptor with `0x0009a044` and creates the `0x1125c`-byte `GameEngine`.

The engine constructor (`S3dEngine_ctor`, `0x000f1584`) establishes the relevant members:

| Engine offset | Object | Evidence |
|---:|---|---|
| `+0x58` | embedded `s3dTimer` | Constructor `S3dTimer_Ctor` `0x000f3640`; RTTI vtable. |
| `+0x60` | embedded `s3dExit` | Constructor `S3dExit_Ctor` `0x000f3604`; RTTI vtable. |
| `+0x74` | embedded `s3dVideo` | Constructor `S3dVideo_Ctor` `0x000f35c8`; RTTI vtable. |
| `+0x88` | `ResManager *` | `0x70`-byte allocation and `ResManager_Ctor` `0x000f3450`. |
| `+0x8c` | audio manager | `0x9c`-byte allocation and `AudioManager_Ctor` `0x00112fe0`. |
| `+0x94` | `Renderer *` | `0x1a388`-byte allocation and `Renderer_ctor` `0x000b0454`. |
| `+0x98` | screen blitter | `0x134`-byte allocation and `ScreenBlitter_Ctor` `0x000d4568`. |

`CreateSymbianVideoDevice` (`0x000a6eb8`) allocates a `0xa8`-byte `VID_impl` at `s3dVideo + 0x08`. Its three platform arguments are a window-server session, screen device, and window. The constructor at `0x000a65b4` is named `VidImpl_Ctor`; `0x000a6378` is the byte-for-byte-equivalent `VidImpl_CtorDuplicate`.

`GameEngine_Init` passes the renderer and screen-blitter pointers to the UI through `SetUiRenderer` (`0x0001dbbc`) and `SetUiScreenDescriptor` (`0x00049728`). Those names are accurate as simple field setters, but they do not themselves render anything.

## Platform video lifecycle

RTTI and the constructor identify `CActive` as the primary base and `MDirectScreenAccess` as a secondary interface at object offset `+0x1c`. The vtables start at `0x0014dc90` and `0x0014dcb8`. The complete object size is `0xa8` bytes:

| Offset | Type | Recovered field |
|---:|---|---|
| `+0x00` | `CActive`, `0x1c` bytes | Active-object base; `iStatus` at `+0x04` tracks bitmap presentation. |
| `+0x1c` | vptr | `MDirectScreenAccess` secondary vtable. |
| `+0x20` | `s3dLog`, `0x14` bytes | Embedded logger. |
| `+0x34` / `+0x38` / `+0x3c` | pointers | Window-server session, screen device, and window. |
| `+0x40` | `int32_t` | Started flag. |
| `+0x44` | `CDirectScreenAccess *` | Direct-screen-access owner. |
| `+0x48` | `CFbsBitGc *` | Graphics context returned by `CDirectScreenAccess::Gc()`. |
| `+0x4c` | `uint8_t *` | Cached back-buffer address. |
| `+0x50` | `uint32_t` | Zero-initialized handle; no subsequent use established. |
| `+0x54` | `0x1c` bytes | Unidentified state. |
| `+0x70` | `CDirectScreenBitmap *` | Double-buffered direct-screen bitmap. |
| `+0x74` | `TAcceleratedBitmapInfo`, `0x1c` bytes | Current bitmap description. |
| `+0x90` | `uint8_t` | Buffer-ready flag, initialized and cleared to zero. |
| `+0x94` | `TRect` | Window rectangle used to create the bitmap. |
| `+0xa4` | font pointer | `LatinPlain12()` result used by the graphics context. |

`TAcceleratedBitmapInfo` contains `iDisplayMode` at `+0x00`, `iAddress` at `+0x04`, a `TSize` at `+0x08`, `iLinePitch` at `+0x10`, `iPixelShift` at `+0x14`, and `iPhysicalAddress` at `+0x18`. Ghidra now has explicit `CActive`, `TSize`, `TRect`, `TAcceleratedBitmapInfo`, `VID_impl`, and `s3dVideo` structures.

| Address | Function | Observed behavior |
|---:|---|---|
| `0x000a65b4` | `VidImpl_Ctor` | Creates direct-screen access and bitmap, starts access, installs clipping, and creates the bitmap with `EDoubleBuffer`. It leaves `started` at zero. |
| `0x000a6904` / `0x000a6a1c` | `VidImpl_Dtor` / deleting destructor | Releases `CDirectScreenAccess` and embedded state. Bitmap shutdown is performed by `Stop`, not this destructor. |
| `0x000a6b34` | `VidImpl_Start` | If stopped, repeats `StartL`, refreshes the GC and clipping region, calls `Create(screenRect, EDoubleBuffer)`, then sets `started = 1`. |
| `0x000a6c88` | `VidImpl_Stop` | If started, calls `CDirectScreenBitmap::Close()` and clears `started`. |
| `0x000a6cf4` | `VidImpl_Update` | Calls `CDirectScreenBitmap::EndUpdate(iStatus)`, marks the active object active, and clears `bufferReady`. |
| `0x000a6de0` | `VidImpl_GetBackBufferPtr` | If `bufferReady` is zero, calls `BeginUpdate(bitmapInfo)` and logs `GetBackBufferPtr Error Code: %d` on failure. Returns `bitmapInfo.iAddress`, cached at `+0x4c`, even after an error. |
| `0x000a6d6c` | `VidImpl_DrawText` | Wraps UTF-16 text in `TPtrC16` and invokes the GC text-drawing virtual at `+0xac` with an `(x, y)` position. |
| `0x000a6340` | `VidImpl_RunError` | Empty active-object error handler. |
| `0x000a635c` | `VidImpl_RunL` | Empty completion handler. No local code sets `bufferReady`, so `GetBackBufferPtr` attempts `BeginUpdate` on every call. |
| `0x000a6e78` / `0x000a6e98` | `VidImpl_RestartDirectScreenAccess` / `VidImpl_AbortNow` | Empty direct-screen callbacks. |
| `0x000a729c` / `0x000a72a4` | `VidImpl_RestartThunk` / `VidImpl_AbortNowThunk` | Secondary-vtable thunks subtract `0x1c` from the interface pointer and tail-call the corresponding callback. |

The `CDirectScreenBitmap` vtable slots are established directly from `cdsb.h`: `+0x0c` is `Create`, `+0x10` is `BeginUpdate`, `+0x14` is the whole-bitmap `EndUpdate(TRequestStatus&)`, and `+0x1c` is `Close`. Local inline helpers `CDirectScreenAccess_Gc` (`0x000a7124`) and `CDirectScreenAccess_DrawingRegion` (`0x000a70fc`) return the fields at direct-access offsets `+0x1c` and `+0x24`. `CActive_IsActive` is `0x000a2594`.

Thin `s3dVideo` forwarders are `S3dVideo_Start` (`0x000a6f68`), `S3dVideo_Stop` (`0x000a6f98`), `S3dVideo_GetBackBufferPtr` (`0x000a702c`), `S3dVideo_Update` (`0x000a705c`), and `S3dVideo_DrawText` (`0x000a708c`). `S3dVideo_SetTextColor` (`0x000a70d4`) is empty; its name and RGB parameters follow the matching S2D API. `S3dVideo_Dtor` (`0x000a6fc8`) deletes the implementation.

The `VID_impl` name and start/stop/update lifecycle also exist in the S2D SDK. The Symbian backend replaces Dingoo LCD presentation rather than sharing its platform code; see `s2dsdk_correlation.md`.

The start/resume path passes the returned pixel address to `Renderer_SetBackBufferPtr` (`0x000f36bc`), formerly misnamed `Renderer_SetScreenDescriptor`, which writes it to the embedded raster object. It also passes the renderer's current texture to `ScreenBlitter_SetTexture` (`0x000f367c`), which caches both the texture and its pixel pointer.

## Renderer, raster, and texture objects

The `Renderer` RTTI vtable starts at `0x0015b600`; its destructors are `Renderer_Dtor` `0x000b0cac` and `Renderer_DeletingDtor` `0x000b0e8c`. `Renderer` inherits `s3dBase`.

`Renderer_ctor` creates:

- a `0x1e6c`-byte `Raster` at renderer offset `+0x1a374`, using `Raster_Ctor` `0x000c98c8`;
- a `0x58`-byte `Texture` at `+0x1a37c`, using `Texture_Ctor` `0x000d7bb0`;
- a `0x378`-byte function-selection/rasterization helper at `+0x1a378`.

The `Raster` RTTI vtable starts at `0x0015b970`; destructors are `0x000c9dd0`/`0x000c9f74`. It inherits `s3dBase`.

The `Texture` RTTI vtable starts at `0x0015bcd0`; destructors are `0x000d7dc8`/`0x000d7ee0`. It also inherits `s3dBase`.

### Texture layout

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `s3dBase` | Eight-byte base. |
| `+0x08` | `s3dIO` | Embedded IO member. |
| `+0x14` | `s3dLog` | Embedded log member. |
| `+0x28` | `int32_t` | Width. |
| `+0x2c` | `int32_t` | Height. |
| `+0x30` / `+0x34` | fixed-point `int32_t` | Width and height in 16.16 form. |
| `+0x38` | `int32_t` | Base-2 width shift when width is a power of two; otherwise `-1`. |
| `+0x40` | `uint32_t` | Pixel allocation byte size, excluding four padding bytes. |
| `+0x44` | `uint8_t` | Bytes per pixel. |
| `+0x48` | pointer | Allocation base. |
| `+0x4c` | pointer | Four-byte-aligned pixel pointer. |
| `+0x54` | pointer | Secondary allocation released by the destructor; purpose not yet established. |

`Texture_Resize` (`0x000d7ff8`) calls `Texture_Reset`, computes:

```text
bytesPerPixel = (mode & 0x30) >> 4
if bytesPerPixel == 0: bytesPerPixel = 4
allocationSize = width * height * bytesPerPixel
allocationBase = Allocate(allocationSize + 4)
pixels = (allocationBase + 3) & ~3
```

Engine startup calls it with mode `2`, which therefore selects four bytes per pixel. This is concrete evidence for a CPU-owned 32-bit framebuffer, independent of SDL or any GPU texture.

Accessors now named in Ghidra are `GetTextureWidth` `0x00031f10`, `GetTextureHeight` `0x00031ee8`, `GetTexturePixels` `0x00071a5c`, and `GetTextureWidthShift` `0x000cf194`.

### Software rasterization

`Raster_InitializeEdgeFunctions` (`0x000c8310`) fills tables of edge-ordering and function-descriptor records. `Raster_PrepareTriangle` (`0x000ca29c`) selects a rasterization path from feature flags and winding. `Raster_RasterizeTriangle` (`0x000ca800`) computes destination addresses as:

```text
destination = texturePixels + 4 * (y * textureWidth + x)
```

It builds scanline state and invokes selected inner loops through function descriptors. This is a fixed-point, CPU triangle rasterizer; no GPU command submission appears in this path.

`Raster_ComputeAttributeDeltas` (`0x000cdcb8`) computes affine UV deltas from screen-space edge geometry; no depth reciprocal appears in that calculation. `Raster_SetupEdge` (`0x000ce170`) and `Raster_EmitScanline` (`0x000ce27c`) maintain the scanline state.

The indexed-texture inner loop `Raster_InnerLoopIndexedTexture` (`0x000b60c8`) addresses source pixels as:

```text
maxU = (textureWidth - 1) << 16
maxV = (textureHeight - 1) << 16
sourceOffset = ((v & maxV) << textureWidthShift) + ((u & maxU) >> 16)
```

Thus V selects the serialized texture row directly, with no vertical inversion. `Texture_GetPaletteEntry` (`0x000c8024`) reads the indexed color. `Tools/stx.py` likewise writes PNG rows in serialized top-to-bottom order. The portable `Raster.cpp` sampler also uses direct texture rows and must not flip V internally. Blender's OBJ texture origin differs, so the required `1 - v` conversion belongs in `sojconv.py`, not in the engine rasterizer.

`BlitTexture` (`0x000d49c8`) clips source/destination rectangles, computes byte addresses using four bytes per pixel, and dispatches through a selected blitter. `SelectTextureBlitter` (`0x000d53c0`) chooses that function from format/state flags.

`BasicShader_Render` (`0x000aec98`, formerly `ConvertRgb565To32Bit`) expands packed pixels with:

```c
rgba = ((pixel & 0xf800) << 8) |
       ((pixel & 0x07e0) << 5) |
       ((pixel & 0x001f) << 3);
```

Its enabled path loops over `y` outside and `x` inside. For a 320×240 source it starts at destination word `240 * 319`, steps backward by 240 words for each source `x`, and advances one word after each source row. Thus destination index is `240 * (319 - x) + y`, producing a 240×320 portrait result. Alpha is not set by this routine.

## Presentation path

`Raster` fields relevant to presentation are:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x1a60` | `uint8_t *` | Current platform back-buffer address returned by `VID_impl`. |
| `+0x1a64` | `Texture *` | Previous texture. |
| `+0x1a68` | `Texture *` | Current texture. |
| `+0x1a7c` | `BasicShader *` | Active presenter selected by `Raster_SelectShader`. |
| `+0x1a80` | `BasicShader *` | Default 0x14-byte shader. |
| `+0x1a84` | `FogShader *` | Optional 0x38-byte depth-fog shader. |

`Renderer_Present` (`0x000f3794`) calls `Raster_Present` (`0x000ca118`). The latter obtains `currentTexture->pixels` and invokes the active shader's virtual slot `+0x08` with the source texture pixels and destination back-buffer address. `VidImpl_Update` then presents that destination through `EndUpdate`. The original path therefore has two distinct stages: software texture-to-back-buffer composition, followed by platform presentation.

`Raster_SelectShader` (`0x00031dc4`) chooses the default or fog shader when passed null, or an explicit `BasicShader` otherwise. `BasicShader_Render` (`0x000aec98`) is the default virtual implementation. Internal pixels carry RGB565 in the low 16 bits and depth in the high 16 bits. The presenter writes `0x00RRGGBB` without channel-bit replication. See `gameengine_api.md` for the complete queue-to-raster path.

## Software effects

RTTI identifies `BasicShader` and derived `FogShader`, `FadeShader`, `GrayShader`, `MotionBlurShader`, `BlindShader`, `InkShader`, and `TwistShader`. Their virtual functions are local ARM code and operate in the same software-rendered environment.

The `FogShader` construction path at `0x000aeff4` allocates `0x20000` bytes and builds lookup data. `0x000af658` constructs attenuation data, while `0x000af940` fills a 16×16×16×16 two-byte color-interpolation table. This is direct evidence of table-driven software color processing.

`Lensflare` is a `Renderable`, not a `BasicShader`. `Lensflare_Render` (`0x000ee7f8`) projects a source into screen coordinates, checks visibility/depth, and draws up to five positioned sprite elements using the normal texture-blitting path. Its vtable starts at `0x0015d280`; `Lensflare_Dtor` is `0x000eebd8`.

## Frame loop and timing

`Engine_RunFrame` (`0x000f2ec4`) is called by the application timer path. Its observed order is:

1. Acquire the current back-buffer address with `S3dVideo_GetBackBufferPtr` and bind it with `Renderer_SetBackBufferPtr`.
2. Run enabled pre-update virtual handlers.
3. Call `Engine_UpdateFrame` (`0x000f30e8`), clearing render queues and invoking the game update.
4. Run enabled game-specific handlers.
5. Call `Engine_RenderFrame` (`0x000f3138`), including `Renderer_Present` when rendering is enabled.
6. Update the audio manager.
7. Complete platform video with `S3dVideo_Update` and clear transient input flags.
8. Call `Engine_LimitFrameRate` (`0x000f31bc`).

`Engine_Start` (`0x000f2ce0`), `Engine_Suspend` (`0x000f2dac`), and `Engine_Resume` (`0x000f2e20`) coordinate video, audio, and exit state. `StartGameEngineTimer` (`0x000f2c8c`) receives target FPS `0x11` during game initialization and computes `1000000 / 17 = 58823` microseconds. The disassembly's numerator is `0xf6240`, not the mislabeled imported function shown by the decompiler. `Engine_DrawFPS` formats the real-FPS field at engine offset `+0x18` and draws at `(width - 20, 10)`; `GameEngine_DrawFPS` is empty, so a reconstruction should expose the base implementation as an explicit opt-in without changing default behavior.

The frame limiter reads the current platform tick, computes elapsed time against the configured microsecond interval, sleeps for the remainder when positive, and records a new tick. SDL's monotonic `SDL_GetTicksNS` plus a microsecond sleep wrapper is the direct replacement for this responsibility. The current portable code incorrectly treats the argument as milliseconds and the app adds a second 60 Hz limiter; both are scheduled for correction in `engine_completion_plan.md`.

## Portable and SDL implementations

`Reversed_Game/src/soft3d/VID_impl.hpp` and `.cpp` reproduce the recovered lifecycle behind a platform-neutral `VideoBackend` interface. The class owns a CPU `uint32_t` back buffer and a `VideoBufferInfo` containing its address, dimensions, byte pitch, and pixel shift. Construction performs initial backend acquisition without setting `started`; the first public `Start` repeats acquisition and sets the flag, matching the original constructor's unusual state transition. `Update` has no additional started-state guard, and failed `BeginUpdate` calls preserve the cached address as in the original.

`src/platform/dummy/DummyVideoBackend.*` is SDL-independent and serves as the reference backend. It records lifecycle calls and copies presented pixels for `tests/test_vidimpl.cpp`. Tests cover construction and first-start behavior, idempotent start/stop, restart, repeated `BeginUpdate`, error propagation and cached-address behavior, buffer metadata, and exact presented pixel contents.

`src/platform/sdl/SdlVideoBackend.*` implements the host presentation stage. `BeginUpdate` exposes the CPU buffer, while `EndUpdate` uploads `0x00RRGGBB` words to a 240×320 `SDL_PIXELFORMAT_XRGB8888` streaming texture with the portrait row pitch, renders with nearest-neighbor aspect-preserving logical presentation, and calls `SDL_RenderPresent`. Resource creation is idempotent, and renderer creation retains the explicit fallback to SDL's `software` renderer.

The app uses `GameEngine::RunFrame` with real SBN/SDT/STX resources from `res.spk`; the initial handedness screen replaces the former host placeholder. `BasicShader` expands RGB565/depth words into the `VID_impl` back buffer using the original portrait mapping. The SDL window/renderer/texture objects remain encapsulated in the backend. `tests/test_engine.cpp` verifies the complete engine-to-dummy-video path with exact pixels. Audio is implemented in `AudioDevice`/`AudioManager` with SDL and dummy backends; scene/gameplay, later menu states, and other higher-level facilities remain stubs. The portable rasterizer does not yet reproduce every specialized original inner loop; see `gameengine_api.md` and `engine_completion_plan.md`.

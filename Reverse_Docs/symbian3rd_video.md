# Symbian 3rd video and software rendering

## Scope and reliability

This document covers `SymbianBin/out.bin` (`out.bin` in Ghidra) at image base zero. Names listed as recovered have been applied to the program. Platform-specific behavior is inferred from local control flow, RTTI, and direct-screen-access interfaces. The current import-derived Symbian names are not consistently trustworthy: several calls in `VID_impl` and engine setup are labeled as unrelated APIs. Conclusions below therefore rely on the ARM instructions and class/data relationships, not those labels.

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

`CreateSymbianVideoDevice` (`0x000a6eb8`) allocates a `0xa8`-byte `VID_impl` at `s3dVideo + 0x08`. The constructor at `0x000a65b4` is now named `VidImpl_Ctor`; `0x000a6378` is a duplicate-code variant.

`GameEngine_Init` passes the renderer and screen-blitter pointers to the UI through `SetUiRenderer` (`0x0001dbbc`) and `SetUiScreenDescriptor` (`0x00049728`). Those names are accurate as simple field setters, but they do not themselves render anything.

## Platform video lifecycle

RTTI identifies `VID_impl` as having an unresolved primary base and `MDirectScreenAccess` at object offset `+0x1c`. Its vtables start at `0x0014dc90` and `0x0014dcb8`.

| Address | Function | Observed behavior |
|---:|---|---|
| `0x000a6904` / `0x000a6a1c` | `VidImpl_Dtor` / deleting destructor | Releases the screen object and destroys embedded state. |
| `0x000a6b34` | `VidImpl_Start` | Acquires/initializes the screen and starts direct-screen access. |
| `0x000a6c88` | `VidImpl_Stop` | Stops direct-screen access and marks the device stopped. |
| `0x000a6cf4` | `VidImpl_CompleteFrame` | Completes direct-screen access for the frame. |
| `0x000a6de0` | `VidImpl_GetScreenDescriptor` | Queries screen state when needed and returns the descriptor cached at `+0x4c`. |
| `0x000a6340` / `0x000a635c` / `0x000a6e78` / `0x000a6e98` | direct-screen callbacks/no-ops | Empty bodies; exact interface slot mapping remains incomplete. |

Thin `s3dVideo` forwarders are:

- `S3dVideo_Start` `0x000a6f68`;
- `S3dVideo_Stop` `0x000a6f98`;
- `S3dVideo_GetScreenDescriptor` `0x000a702c`;
- `S3dVideo_CompleteFrame` `0x000a705c`.

The start/resume path passes the descriptor to `Renderer_SetScreenDescriptor` (`0x000f36bc`), which writes it to the embedded raster object. It also passes the renderer's current texture to `ScreenBlitter_SetTexture` (`0x000f367c`), which caches both the texture and its pixel pointer.

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

`BlitTexture` (`0x000d49c8`) clips source/destination rectangles, computes byte addresses using four bytes per pixel, and dispatches through a selected blitter. `SelectTextureBlitter` (`0x000d53c0`) chooses that function from format/state flags.

`ConvertRgb565To32Bit` (`0x000aec98`) expands packed pixels with:

```c
rgba = ((pixel & 0xf800) << 8) |
       ((pixel & 0x07e0) << 5) |
       ((pixel & 0x001f) << 3);
```

Its two paths differ in traversal direction. Alpha is not set by this routine.

## Presentation path

`Raster` fields relevant to presentation are:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x1a60` | pointer | Current platform screen descriptor. |
| `+0x1a64` | `Texture *` | Previous texture. |
| `+0x1a68` | `Texture *` | Current texture. |
| `+0x1a7c` | pointer | Presenter object; concrete type and initialization remain unresolved. |

`Renderer_Present` (`0x000f3794`) calls `Raster_Present` (`0x000ca118`). The latter obtains `currentTexture->pixels` and invokes the presenter's virtual slot `+0x08` with the pixel pointer and screen descriptor. This is the preferred future SDL hook: upload/copy these CPU pixels to the SDL streaming texture, then present the window.

The presenter's concrete class is not yet proven. Do not equate it with `VID_impl` without a constructor/write chain for `Raster + 0x1a7c`.

## Software effects

RTTI identifies `BasicShader` and derived `FogShader`, `FadeShader`, `GrayShader`, `MotionBlurShader`, `BlindShader`, `InkShader`, and `TwistShader`. Their virtual functions are local ARM code and operate in the same software-rendered environment.

The `FogShader` construction path at `0x000aeff4` allocates `0x20000` bytes and builds lookup data. `0x000af658` constructs attenuation data, while `0x000af940` fills a 16×16×16×16 two-byte color-interpolation table. This is direct evidence of table-driven software color processing.

`Lensflare` is a `Renderable`, not a `BasicShader`. `Lensflare_Render` (`0x000ee7f8`) projects a source into screen coordinates, checks visibility/depth, and draws up to five positioned sprite elements using the normal texture-blitting path. Its vtable starts at `0x0015d280`; `Lensflare_Dtor` is `0x000eebd8`.

## Frame loop and timing

`Engine_RunFrame` (`0x000f2ec4`) is called by the application timer path. Its observed order is:

1. Get the current screen descriptor and bind it to the renderer.
2. Run enabled pre-update virtual handlers.
3. Call `Engine_UpdateFrame` (`0x000f30e8`), clearing render queues and invoking the game update.
4. Run enabled game-specific handlers.
5. Call `Engine_RenderFrame` (`0x000f3138`), including `Renderer_Present` when rendering is enabled.
6. Update the audio manager.
7. Complete platform video and clear transient input flags.
8. Call `Engine_LimitFrameRate` (`0x000f31bc`).

`Engine_Start` (`0x000f2ce0`), `Engine_Suspend` (`0x000f2dac`), and `Engine_Resume` (`0x000f2e20`) coordinate video, audio, and exit state. `StartGameEngineTimer` (`0x000f2c8c`) receives `0x11` during game initialization, but its exact platform unit should not be inferred from the currently unreliable import label.

The frame limiter reads the current platform tick, computes elapsed time against the configured interval, sleeps for the remainder when positive, and records a new tick. SDL's monotonic `SDL_GetTicksNS` plus `SDL_Delay` is the direct replacement for this responsibility.

## SDL replacement boundary

The draft app in `Reversed_Game/src/app/` deliberately mirrors these boundaries without attaching engine code:

- a fixed 320×240 CPU-generated RGBA framebuffer;
- an SDL streaming texture used only after CPU rendering is complete;
- nearest-neighbor, aspect-preserving presentation;
- explicit fallback to SDL's `software` renderer when the requested/default renderer fails;
- SDL events mapped by an isolated controls stub;
- SDL nanosecond timing and frame pacing;
- no engine renderer, sound mixer, or game object hooked in yet.

When reconstruction continues, recover the `Raster + 0x1a7c` presenter and the exact 32-bit channel order before replacing the placeholder frame generator.

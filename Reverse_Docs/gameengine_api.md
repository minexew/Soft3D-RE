# GameEngine and software rendering API

## Engine and game-specific object

RTTI distinguishes `Engine` from its game-specific `GameEngine` subclass. `S3dEngine_ctor` (`0x000f1584`) constructs the reusable engine prefix; `GameEngine_ctor` (`0x0006e4dc`) extends it to a 0x1125c-byte object containing the scene, UI, script, font, menu, and game-state data.

### Verified `Engine` fields

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `s3dBase` | Base and engine link. |
| `+0x08` | `int32_t` | Running flag. |
| `+0x0c`, `+0x10` | `uint32_t` | Frame-limiter timestamps. |
| `+0x1c` | `uint32_t` | Frame interval. |
| `+0x20` | `int16_t` | Pre-update hook enable. |
| `+0x22` | `int16_t` | Post-update hook enable. |
| `+0x24` | `int16_t` | Renderable-submission hook enable. |
| `+0x26` | `int16_t` | Render-preparation hook enable. |
| `+0x28` | `int32_t` | Presentation enable. |
| `+0x2c` | time source | Current-time provider. |
| `+0x38` | `s3dIO` | Embedded IO. |
| `+0x44` | `s3dLog` | Embedded logger. |
| `+0x58` | `s3dTimer` | Embedded timer. |
| `+0x60` | `s3dExit` | Exit-state helper. |
| `+0x68`, `+0x6c` | `int32_t` | Width and height. |
| `+0x74` | `s3dVideo` | Embedded platform-video forwarder. |
| `+0x80` | `RenderQueues *` | 0xb0c-byte, seven-queue allocation. |
| `+0x84` | `s3dResManager *` | Resource cache and FilePak owner. |
| `+0x88` | audio manager pointer | Software audio subsystem. |
| `+0x8c` | input pointer | Input state. |
| `+0x90` | camera/projection pointer | 0x168-byte camera helper. |
| `+0x94` | `Renderer *` | 0x1a388-byte renderer. |
| `+0x98` | `ScreenBlitter *` | 0x134-byte 2D blitter. |
| `+0x9c` | platform timer helpers | Two Symbian helper objects. |

The `Engine` vtable at `0x0015d7b0` and `GameEngine` vtable at `0x00141888` establish the hook order below. Previously undefined vtable-target functions have been created and named in Ghidra.

| Slot | Base purpose | `GameEngine` override |
|---:|---|---|
| `+0x0c` | Init | `GameEngine_Init` `0x0006ebc0` |
| `+0x10` | Shutdown | `GameEngine_Shutdown` `0x000705b4` |
| `+0x14` | Load state | `GameEngine_LoadGameState` `0x0006f39c` |
| `+0x18` | Save state | `GameEngine_SaveGameState` `0x0006f6a4` |
| `+0x1c` | Suspend | `Engine_Suspend` |
| `+0x20` | Resume | `Engine_Resume` |
| `+0x24` | PreUpdate | `0x0007080c` → `Scene_Update` |
| `+0x28` | PostUpdate | `0x000708d8` → `Scene_PostUpdate` |
| `+0x2c` | Update | `0x00070918`, empty in this game build |
| `+0x30` | PrepareRender | `0x00070958` → `Scene_PrepareRender` |
| `+0x34` | SubmitRenderables | `0x000709e4` → `Scene_SubmitRenderables`, UI submission |
| `+0x38` | PostRender | `0x00070a70`, empty in this game build |

### Game-specific ownership points

| Offset | Object |
|---:|---|
| `+0xa4` | 0x6b68-byte `Scene`, constructed by `0x0002a0f4`. |
| `+0xca28` | 0xcc-byte UI/renderable manager. |
| `+0xca2c` | 0x2ac-byte `GameMenu`. |
| `+0xca30` | In-game flag. |
| `+0xca34` | Menu-active flag. |
| `+0xca38`, `+0xca3c` | Two 0x30-byte font/string managers. |
| `+0xca40` | Embedded script engine. |
| `+0x106f0` | Language/resource-variant selector. |
| `+0x106f4`–`+0x106fc` | Three serialized game-state words. |

The remaining large game-state and embedded structures have not been assigned invented field names.

## Frame lifecycle

`Engine_RunFrame` performs:

1. Acquire `VID_impl`’s back buffer through `s3dVideo` and bind it to the renderer.
2. Run `PreUpdate` when enabled.
3. Clear all seven queues and run `Update`.
4. Run `SubmitRenderables` when enabled.
5. Run `PrepareRender` when enabled.
6. Sort the six scene queues, render them in ascending queue order, then render the overlay queue and `PostRender`.
7. Present through `Renderer_Present` when enabled.
8. Run `PostUpdate` when enabled.
9. Update audio, complete `VID_impl::Update`, update input, and limit the frame rate.

The original `Engine_Resume` tests the running flag as nonzero, unlike `Engine_Start`; this unusual condition is retained in the portable reconstruction.

## Render queues

The allocation at `Engine + 0x80` contains seven records of 0x194 bytes:

```text
u32 count
Renderable *entries[100]
```

Queues 0–5 are scene layers. Queue 6 is the unsorted overlay/UI queue. `RenderQueues_Submit` (`0x0001e044`) drops entries beyond 100 and rejects no other input in the original. Scene queues sort by signed `Renderable + 0x04`, either ascending or descending. The frame path uses ascending order.

Recovered functions are `RenderQueues_Clear` `0x00031874`, `RenderQueues_SortQueue` `0x000f78d8`, `RenderQueues_SortSceneQueues` `0x000f37c4`, `RenderQueues_RenderSceneQueues` `0x000f7994`, and `RenderQueues_RenderOverlayQueue` `0x000f7a7c`.

## Object-to-pixel path

```text
GameEngine::SubmitRenderables
  Scene_SubmitRenderables / RenderQueues_Submit
  RenderQueues::Render*
    Object_Render
      Object_RenderIndexed or Object_RenderSequential(Textured)
        Renderer_DrawIndexed(Textured) / Renderer_DrawSequential
          Renderer_TransformVertex(Streams)
          Renderer_ClipAndRasterizeTriangle
            Raster_PrepareTriangle
            Raster_RasterizeTriangle
              current RGB565/depth texture
  Renderer_Present
    Raster_Present
      active BasicShader::Render
        platform XRGB back buffer
  VID_impl::Update
```

`Renderer` owns a `Raster` at `+0x1a374`, a function-selection helper at `+0x1a378`, and its current `Texture` at `+0x1a37c`. `Object_RenderIndexed` supplies material state, the matrix palette, a vertex buffer, an index buffer, primitive type, first index, and primitive count.

## Presenter identification

`Raster + 0x1a7c` is the active `BasicShader *`, not a separate unknown presenter class:

- `+0x1a80`: default 0x14-byte `BasicShader`;
- `+0x1a84`: 0x38-byte `FogShader`;
- `+0x1a7c`: active shader selected by `Raster_SelectShader` (`0x00031dc4`).

`BasicShader_Render` (`0x000aec98`, previously `ConvertRgb565To32Bit`) is the vtable `+0x08` routine called by `Raster_Present`. `FogShader_Render` is `0x000af3e4`. `BasicShader_SetEnabled` (`0x000aec50`) writes the shader field and changes zero to one.

Internal frame pixels use RGB565 in the low 16 bits and depth in the high 16 bits of each 32-bit word. Presentation expands RGB565 to `0x00RRGGBB` without bit replication:

```c
xrgb = ((pixel & 0xf800) << 8) |
       ((pixel & 0x07e0) << 5) |
       ((pixel & 0x001f) << 3);
```

The default enabled traversal walks source pixels linearly while writing destination columns from bottom to top. This unusual orientation is original behavior and is covered by the portable pixel tests. SDL uses `SDL_PIXELFORMAT_XRGB8888` for the resulting words.

`FogShader` adds depth attenuation and a 16×16×16×16 RGB565 interpolation table. `FogShader_BuildAttenuationTable` and `FogShader_BuildColorTable` recover that table-driven path.

## Portable reconstruction and tests

`Reversed_Game/src/soft3d/` now includes `Engine`, `GameEngine`, `Renderable`, `Renderer`, `Raster`, `BasicShader`, `Object`, and `Fixed`. `GameEngine` retains scene/UI ownership and hook ordering. Scene/gameplay, script, font, and audio internals are explicit stubs.

`tests/test_engine.cpp` checks queue capacity and ordering, exact RGB565/XRGB mapping, a complete `GameEngine` → dummy `VID_impl` frame, indexed SOJ rasterization, packed material names, animation interpolation truncation, and animation wrapping. The SDL app renders its placeholder through the same engine presentation path rather than writing the backend framebuffer directly.

The portable rasterizer is functional but not yet bit-identical for every original clipping, lighting, perspective-correct texturing, blending, and specialized scanline case. See `soft3d_models_animations.md` for format details and `symbian3rd_video.md` for platform video.

# GameEngine and software rendering API

## Engine and game-specific object

RTTI distinguishes `Engine` from its game-specific `GameEngine` subclass. `S3dEngine_ctor` (`0x000f1584`) constructs the reusable engine prefix; `GameEngine_ctor` (`0x0006e4dc`) extends it to a 0x1125c-byte object containing the scene, UI, script, font, menu, and game-state data.

### Verified `Engine` fields

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `s3dBase` | Base and engine link. |
| `+0x08` | `int32_t` | Running flag. |
| `+0x0c`, `+0x10` | `uint32_t` | Frame-limiter timestamps. |
| `+0x14` | `uint32_t` | Target FPS, set to 17 by `GameEngine_Init`. |
| `+0x18` | `int32_t` | Legacy real-FPS display field; not initialized or updated in this build. |
| `+0x1c` | `uint32_t` | Frame interval in microseconds. |
| `+0x20` | `int16_t` | Pre-update hook enable. |
| `+0x22` | `int16_t` | Post-update hook enable. |
| `+0x24` | `int16_t` | Renderable-submission hook enable. |
| `+0x26` | `int16_t` | Render-preparation hook enable. |
| `+0x28` | `int32_t` | Presentation enable. |
| `+0x2c` | `s3dTimer` | Current-time provider, constructed at `0x000e47e8`. |
| `+0x38` | `s3dIO` | Embedded IO. |
| `+0x44` | `s3dLog` | Embedded logger. |
| `+0x58` | `s3dMemInfo` | Empty memory-info class shell; see `symbian3rd_debug.md`. |
| `+0x60` | `s3dExit` | Empty exit-state class shell with no exit operation. |
| `+0x68`, `+0x6c` | `int32_t` | Width and height. |
| `+0x74` | `s3dVideo` | Embedded platform-video forwarder. |
| `+0x80` | `RenderQueues *` | 0xb0c-byte, seven-queue allocation. |
| `+0x84` | `s3dResManager *` | Resource cache and FilePak owner. |
| `+0x88` | audio manager pointer | Software audio subsystem. |
| `+0x8c` | input pointer | Input state. |
| `+0x90` | `Camera *` | 0x168-byte camera/projection helper; constructor `Camera_Ctor` `0x000fc8c0`. |
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
| `+0x44` | DrawFPS | `0x00070ab0`, empty in this game build |

The reusable `Engine` declares the initialization, state, update, submission, and rendering hooks as pure virtual. `Suspend` and `Resume` are concrete virtual methods. `UpdateFrame` and `RenderFrame` are nonvirtual helpers called directly by `RunFrame`, not vtable slots. The final `DrawFPS` implementation at `Engine_DrawFPS` (`0x000f2c30`) formats the real-FPS field with `"%d"` and draws it through `s3dVideo`; `GameEngine` overrides it with an empty method. No `DrawFPS` caller exists in the recovered frame path, and the field is not initialized or updated in this build. See `symbian3rd_debug.md`.

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

### Early initialization order

`AppUi_ConstructL` creates `GameView`; `GameView_ConstructL` creates the 320×240 descriptor and `GameEngine`, invokes virtual `Init`, and then starts it. `GameEngine_Init` (`0x0006ebc0`) performs the significant early work in this order:

1. Select the language/resource variant and open `res.spk`.
2. Construct/initialize the script engine and load `str.sdt` into the dialog manager.
3. Bind the renderer and screen blitter to the UI and initialize resource-name arrays.
4. Start the `0x11` frame interval.
5. Allocate both 0x30-byte `StringManager` objects and load the language-specific `basefont.sbn` into each.
6. Load `system.sbn` into the secondary manager, adding local glyphs and packed `VisString` records.
7. Bind both managers to the renderer texture, then construct and initialize `GameMenu` and its dialogs.
8. Load `state.sdt`, read configuration data, bind scene subsystems, and enter the non-game initial state.

The initial `GameMenu` update selects state index 7 with time/selection zero. This is the handedness screen documented in `sdt_format.md`; it needs neither a BSP map nor the 3D scene. Its labels and phone are palettized STX images, although SBN font initialization precedes them.

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

The original `Engine_Resume` tests the running flag as nonzero, unlike `Engine_Start`; this unusual condition is retained in the portable reconstruction. Start and resume also rebind `ScreenBlitter` to the renderer's current texture. Start clears all key state, and `Input_Update` clears only `FIRSTDOWN` and `CLICK` at the end of each frame. `EngineClock` provides the microsecond limiter; the original `Engine::DrawFPS` helper is not called by this frame path.

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

`Reversed_Game/src/soft3d/` now includes `Engine`, `GameEngine`, `Input`, `EngineClock`, `Renderable`, `Renderer`, `Raster`, `BasicShader`, `Bsp`, `BezierCurve3`, `CameraPathCollection`, `Camera`, `CameraSpot`, `DummySpot`, `MenuBackdrop`, `Object`, `Fixed`, `Font`, `VisString`, `StringManager`, `ScreenBlitter`, `UiManager`, and the initial `GameMenu` state. `GameEngine` retains scene/UI ownership and hook ordering. `GameEngine::LoadMenuBackdrop` activates the real SPL/BSP/camera background; episode scene/gameplay, later menu states, BSP collision, and script internals remain explicit stubs. `AudioDevice`, `AudioManager`, and `SoundInstance` implement the recovered audio update, mixing, control, fade, and positional paths; see `symbian3rd_audio.md`. `Input` and platform clock backends implement recovered key transitions and 17 FPS limiting; `SetFpsDisplayEnabled`/`--show-fps` provide explicit FPS restoration.

`tests/test_engine.cpp` checks queue capacity and ordering, exact RGB565/XRGB mapping including a non-square portrait mapping, a complete `GameEngine` → dummy `VID_impl` frame, indexed SOJ rasterization, packed material names, animation interpolation truncation, and animation wrapping. `test_input.cpp` and `test_sdlinput.cpp` cover key transitions, translation, repeat thresholds, simultaneous keys, deterministic timing, lifecycle, and opt-in FPS text. `test_bsp.cpp` covers real/synthetic map loading, while `test_bsprender.cpp` verifies PVS selection, lightmaps, S3DSTY wrapping, visible model submission, and exact real-menu framebuffer/presented hashes. `test_spl.cpp` covers all real SPL records, exact evaluation and projection arithmetic, malformed inputs, path completion, and 120 moving menu-background frames through dummy video; see `spl_format.md`. `test_font.cpp` exercises the real SBN resources and glyph drawing; `test_ui.cpp` validates the real SDT tables and both handedness frames through engine input. The SDL app loads `res.spk` and renders the initial screen through the normal overlay and presentation path.

The portable rasterizer is functional but not yet bit-identical for every original clipping, lighting, perspective-correct texturing, blending, and specialized scanline case. See `soft3d_models_animations.md` for format details and `symbian3rd_video.md` for platform video.

# Engine completion inventory and staged plan

## Purpose and boundary

This plan covers reusable Soft3D engine functionality and the platform shell needed to run it. It deliberately postpones concrete enemies, player state implementations, episode scripts, game-specific command handlers, and content-specific game-data semantics. Generic base classes and interfaces used by those systems remain in scope.

Every stage should include the corresponding Ghidra names/types/comments, English Markdown notes, real-resource or synthetic tests, and a full CTest run. A stage is complete only when its portable behavior is exercised end to end, not merely when a parser compiles.

## Current baseline

### Functional foundations already present

- SPK/FilePak, four-level resource cache, `Resource`, Linux file IO, logging, and wide formatting;
- SAU/WAR decoding, the 16-voice mixer, music/SFX/fade controls, positional audio, dummy audio, and SDL audio output;
- STX loading, SBN fonts/strings and rectangle/range text, generic SDT resource/state tables, all seven dialog runtimes, key interpolation, secondary textures, and every ScreenBlitter mode;
- SOJ/SAI resource parsing and a functional Object → Renderer → Raster → BasicShader → VID_impl path;
- SBP container/runtime loading, all chunk readers, incremental progress, map-model resources, generic entity-factory ownership, and link cleanup;
- BSP/PVS visibility, fixed-point face rendering, RGB565 lightmaps, S3DSTY loading/advancement, and visible map-model submission;
- SPL parsing/evaluation/derivatives, chord-weighted path timing, camera projection, `CameraSpot`/`DummySpot`, and the complete menu 3D backdrop;
- engine queue ordering and presentation, dummy video, and the SDL application shell;
- Basic/Fog plus Fade, Gray, MotionBlur, Ink, Blind, and Twist presentation shaders;
- 23-key input transitions and translation, SDL event mapping, microsecond target-FPS limiting, lifecycle helpers, and opt-in FPS text;
- complete startup/sound/main-menu flow, all seven concrete dialog classes, orientation-aware controls, and `config.sdt`/save-slot persistence;
- SST loading, typed values/references, recursive command trees, the generic VM/host interface, task lifecycle, and script update integration;
- generic messages, units, visual/physics bases, reusable state templates, eight-bucket `GameUnitManager`, and BSP collision/path queries;
- memory/file streams, big-endian typed IO, generic `GameUnit`/`VisUnit`/`SimpleScript` state serialization, narrow-path IO, and standalone RAM diagnostics.

The game hooks that invoke audio are not automatically supplied by the audio implementation. Stages 7 and 8 resolved renderer-facing SOJ material fields, raster feature parity, and all presentation shaders. Stage 9 completed Object loading/control APIs and all four renderable effects. Stage 10 completed the generic script parser, VM, and interface. Stage 11 completed generic units, state machines, physics, and BSP collision. Stage 12 completed streams, generic state serialization, narrow IO, and standalone diagnostics.

### Missing or incomplete engine facilities

- episode-specific `Scene` gameplay, concrete units, and game-data semantics;
- No secondary-format loader remains to be invented for Soft3D: S3DTEX belongs to the recovered S2D tool path, while the Symbian image class supplies TGA. Remaining format questions are limited to unused/reserved or game-specific fields without consumer evidence.

The old video documentation's statement that audio remains a stub is obsolete; `symbian3rd_audio.md` is the current audio reference.

## New evidence that changes the ordering

The menu background directly depends on three engine facilities:

1. scene rendering setup loads `.\common\default.s3dsty` and `.\common\1000.spl`;
2. `Scene_LoadMenuBackdrop` (`0x0002c268`) loads `.\common\menu.sbp` through the incremental `LoadSbpMapChunks` state machine;
3. map models and rendering use the existing resource/Object/Renderer path.

Therefore S3DSTY and SPL belong with the map/menu milestone, not in an unrelated final-format cleanup. S3DSTY is now known to contain 256 terminated lightmap-frame sequences. SPL is a counted collection of 180-byte Bezier camera-path records. SBP's actual chunk order and loader addresses are documented in `sbp_format.md`.

## Stage 1 — SBP container, `Bsp` ownership, and all chunk readers — complete

**Scope**

- Implement `SbpHeader`, directory validation, and the 0→100 incremental state machine.
- Recover `Bsp` construction/destruction and map resource/directory handling.
- Implement every chunk reader through PVS, including map models and link resolution.
- Keep serialized records separate from runtime records; use portable indices where original native pointers would be unstable.
- Investigate directory block 25 and the `Bsp` render-vtable target before inventing behavior.

**Verification**

- Parse the real `menu.sbp` from the SPK without extraction and complete progress 100.
- Validate counts, offsets, fixed-point conversion, cross-links, cleanup, and error codes with synthetic malformed chunks.
- Compare basic geometry with `Tools/sbp.py`/BspViewer after correcting the tool's chunk names.

**Completion**

`Bsp.*` and `test_bsp.cpp` implement and test every item. The real map completes in 26 calls with all five models and six generic factory entities. Chunk 25 is confirmed as PVS, chunk 17 is unused, entities/links are chunks 14/15, and the apparent render target is invalid mid-function code rather than a recoverable `Bsp::Render` entry. Exact lightmap metadata conversion is included. The complete CTest suite and standalone geometry comparison pass; see `sbp_format.md` for layouts and safety/ownership differences.

## Stage 2 — BSP rendering, PVS, lightmaps, and S3DSTY runtime — complete

**Scope**

- Implement face construction, node/leaf traversal, PVS selection, visible brush/face submission, and map-model submission.
- Use the recovered face lightmap offsets/dimensions and style bytes; implement current-frame modulation.
- Implement S3DSTY loading and its sequence-advance cadence.
- Add only the raster features needed by real map faces; leave unrelated specialized paths for stage 7.

**Verification**

- Render `menu.sbp` from a fixed camera through dummy video and check exact framed output against recovered ARM arithmetic.
- Test PVS bit selection, leaf/node boundaries, style wrapping, and lightmap sampling with small synthetic maps.
- Document the actual rendering path and any newly recovered `Bsp` fields in Ghidra; do not use the already-refuted `0x000f0e20` target.

**Completion**

`BspRender.cpp` and `test_bsprender.cpp` implement and test every item. The fixed real-menu view selects 68 faces in cluster 18 and submits three model entities; its exact raw/presented hashes are `0x9919103f` and `0x566dc515`. Visibility uses direct `-child` leaf indices, parent-marked PVS traversal, nonnegative-side child 0 first, four extracted side planes with two-corner bounds tests and mask narrowing, marked node-owned faces, and separately submitted visible-brush faces. Directed tests cover unmarked-node rejection and offscreen bounds. PVS zero-run decoding, untextured faces, complete-block lightmap generation, full style wrapping, edge-zero coverage, and near/far reciprocal-depth occlusion remain covered. Depth alone is reset for each portable polygon render so stale previous-frame values cannot create trails while uncovered color remains preserved. Valid intensity-derived offsets stay within each 64-byte palette group; malformed-resource accesses remain defensively clamped. Remaining UBSan messages are in the older generic SOJ raster path and belong to Stage 7.

## Stage 3 — SPL evaluation and the complete menu 3D background — complete

**Scope**

- Implement SPL parsing, Bezier evaluation, path timing/interpolation, and camera-path collection ownership.
- Recover the engine camera/projection helper and `CameraSpot` interface needed by the menu.
- Wire the menu backdrop host to `Bsp` and the camera without implementing the episode-specific `Scene` state machine.

**Verification**

- Parse all 11 records in `1000.spl` and verify fixed-point conversion and curve samples.
- Exercise the complete menu-background frame with dummy video, including camera movement over time.
- Confirm that this path needs no concrete enemy, trigger, or episode classes.

**Completion**

`BezierCurve3.*`, `Camera.*`, `CameraSpot.*`, `MenuBackdrop.*`, and `test_spl.cpp` implement and test every item. All 11 real cubic records pass exact fixed-point and derivative checks; collection timing uses the recovered control-polygon lengths. The original `snap == 0` start-position polarity is covered with a nonzero path origin. `GameEngine::LoadMenuBackdrop` drives 484 real-map dummy-video frames with moving camera, 73,555 depth-covered pixels in the first frame, corrected BSP/model visibility and transforms, and exact hashes (`0x9be83101`/`0x3593f585` initially, `0xff34b1b9`/`0x68f2b585` at frame 120). Frames 52 and 86 explicitly cover the cabinet interval, and the wrapped second round verifies two submitted models without episode classes. See `spl_format.md` for layouts, projection arithmetic, update order, and hashes. Stage 13 later added the complete original tangent table and the non-default `SOFT3D_TRIG_USE_MATH` option; see below.

## Stage 4 — Input, target-FPS timer, lifecycle, and restorable FPS display — complete

**Scope**

- Implement the 23-key engine state machine, down/press/up flags, repeat counts, transient update clearing, and key translation.
- Isolate SDL event mapping in `src/platform` and replace `ControlsStub`/`InputStub` with engine input.
- Move timing behind a platform clock/sleep interface and remove the application's duplicate limiter and unused `TimerStub` delta path.
- Preserve the original formula `frameIntervalMicroseconds = 1000000 / targetFps`; the game passes 17.
- Implement `Engine::DrawFPS` text and coordinates, retain `GameEngine`'s empty default override, and provide an explicit opt-in restoration path rather than enabling it unconditionally.
- Recover small lifecycle helpers such as `s3dExit` where they affect start/suspend/resume/quit behavior.

**Verification**

- Test key transitions, repeat thresholds, simultaneous keys, orientation, and SDL-to-engine mapping.
- Verify target interval 58,823 microseconds for 17 FPS with a deterministic clock.
- Test default-off and opt-in FPS frames through dummy video.

**Completion**

`Input.*`, `EngineClock.hpp`, Linux `SystemClock`, SDL `SdlClock`/`SdlInput`, and `test_input.cpp`/`test_sdlinput.cpp` implement and test every item. The app stubs and duplicate limiter are removed, menu selection consumes engine input, and `--show-fps` explicitly restores the base text helper. The recovered frame path has no original `DrawFPS` caller and does not update the real-FPS field; portable zero initialization is a deterministic safety choice, not a recovered FPS calculation. `s3dExit` and `s3dMemInfo` are confirmed empty class shells. Standalone `MemTotal`/`MemRemain` RAM queries were recovered separately and remain uncalled; see `symbian3rd_input.md` and `symbian3rd_debug.md`. All 16 CTest tests and an SDL `--show-fps` smoke run pass.

## Stage 5 — Generic SDT dialog behavior — complete

**Scope**

- Complete `DialogManager`/`Dialog` state selection, key interpolation, visibility outside key ranges, text/behavior word handling, and secondary-texture setup.
- Recover all `ScreenBlitter` modes, clipping/scaling details, and text integration with `StringManager`.
- Identify the `str.sdt` unknown word and `state.sdt` cached-total field from consumers.
- Support all seven generic dialog tables rather than hardcoding dialog 0 or exact key matches.

**Verification**

- Use real `state.sdt` to cover every element/behavior variant and synthetic keys for interpolation boundaries.
- Compare exact dummy pixels for image, text, scaling, clipping, and secondary textures.

**Completion**

`Dialog.*`, `UiManager.*`, `ScreenBlitter.*`, and the expanded `Font.*` implement every item. All seven dialog runtimes and all 30 states are available; every one of the 390 elements and 923 real keys is checked. Behavior zero selects base-rendered elements, while behavior one is left for derived dialogs and is not a text index. Secondary selectors intern `<primary>_alp`, producing 138 resources from the initial 136. The first `state.sdt` word is a stored-but-unused declared visual/element total, and the second `str.sdt` word is reserved/ignored with no consumer; neither supports a more specific semantic claim.

All 26 blitter entries are implemented: six modes with scaled/unscaled and opaque/color-key variants, plus two secondary-alpha loops. Tests cover exact RGB565 arithmetic, fixed-point clipping/scaling, interpolation boundaries, primary/secondary pixels, the scaled secondary color-key bug, the real logo's exact red-channel extraction and darkened outline result, and the clear loop's unchanged pixel zero. The packed secondary-alpha wrap is preserved; the former blue fringe was a portable red-shift bug (`>> 23` instead of the ARM `>> 15`), not original behavior. Text coverage includes wrapping, right/center anchoring, and ranged drawing. The full CTest suite and both SDL smoke tests pass; the new UI and font suites also pass under AddressSanitizer and UndefinedBehaviorSanitizer. See `sdt_format.md` and `symbian3rd_video.md` for layouts, formulas, addresses, and original quirks.

## Stage 6 — Menu flow, remaining dialogs, and configuration SDT — complete

**Scope**

- Recover `MainUIDlg`, `PlayDlg`, `OptionDlg`, `LoadDlg`, `HelpDlg`, `AboutDlg`, and `GameUIDlg` engine-facing behavior. RTTI corrected the earlier `GameMenu` name to `MainUIDlg`.
- Implement handedness acceptance, logo/loading transitions, main-menu navigation, options/load/help/about flow, and their audio/video calls.
- Recover `config.sdt` as the menu/configuration persistence facility.
- Reuse the stage 3 background and stage 4 input rather than adding parallel menu-only input or rendering paths.

**Verification**

- Drive handedness → startup → main menu and each dialog with deterministic input and dummy video.
- Test configuration round trips, menu audio triggers, and returning to the background without resource leaks.

**Completion**

`MenuDialogs.*`, `GameUIDlg.*`, `GameConfig.*`, `GameContent.*`, and the expanded `GameEngine`/`Dialog`/`UiManager` implement the recovered behavior. Construction order matches SDT indices 0–6: Main, Option, About, Help, Play, Load, GameUI. Startup includes the previously missed state-3 sound-enable prompt before the main menu. The exact 1,252-byte `config.sdt` layout and 44-byte save-slot headers are implemented and documented. UI sound indices use the separate `ui_select`/`ui_selectitem`/etc. table, not `str.sdt` sound names.

`tests/test_menu.cpp` drives real startup resources through handedness, logos, sound selection, every concrete dialog family, model/document/photo navigation, GameUI inventory/options/map states, UI voices, and configuration round trips. All 17 CTest tests and both SDL smoke tests pass. UI, menu, and audio also pass under AddressSanitizer and UndefinedBehaviorSanitizer; sanitizer testing exposed and fixed the SAU decoder's final overlapping-word out-of-bounds read without changing decoded samples. GameUI resource acquisition and portable menu state are implemented in the engine; game-completion stage 8 supplies concrete episode map/HUD/model rendering and item effects through `game::GameUiController`. See `sdt_format.md`, `gameengine_api.md`, `symbian3rd_input.md`, and `symbian3rd_audio.md`.

## Stage 7 — Rasterizer and renderer feature parity — complete

**Scope**

- Recover clipping, winding, depth behavior, affine versus perspective-correct attributes, lighting, blend modes, texture offsets/wrapping, detail textures, and specialized scanline loops.
- Implement renderer sequential/indexed/textured entry points and draw-state selection not represented in the portable subset.
- Resolve remaining SOJ material fields when they control renderer state.
- Preserve original fixed-point overflow/truncation and known bugs.

**Verification**

- Build directed triangle/scanline fixtures for every selected path and compare exact RGB565/depth output.
- Re-run real SOJ and SBP scenes to detect regressions in previously working paths.

**Completion**

`Renderer.*`, `Raster.*`, `Object.*`, and `test_raster.cpp` implement and test the recovered path. The renderer now supports sequential, indexed, and secondary-stream textured draws; camera/screen clipping; winding culling; reciprocal-table projection; fixed-step edge walking; affine depth/UV/intensity interpolation; palette lighting; color keys; all blend levels; detail alpha; and independent depth test/write behavior. All 100 table selectors are mapped and all 96 used scanline functions are named in Ghidra. SOJ UVs are affine; perspective-correct lightmap sampling remains in the separate BSP path.

Material culling is distinct from RGB blending. Directional scale, ambient intensity, U/V scroll velocities, runtime intensity bounce/wrap, and opaque-only default depth writes are implemented. Original full-pixel depth comparisons, signed optimized selectors 80/85, fixed-point truncation/wrapping, near-intersection depth constant, and palette-intensity double scaling are retained. The old UV formula was corrected from the ARM listing: the right shift applies to the sum of shifted V and masked U.

All 21 normal CTest tests and all 15 ASan/UBSan CTest tests pass, including the real SOJ test, real BSP/SPL/menu scenes, and SDL smoke tests. All six standalone SOJ converter tests also pass. See `symbian3rd_raster.md` for layouts, formulas, selector mapping, and verification details.

## Stage 8 — All presentation shaders — complete

**Scope**

- Implement Fade, Gray, MotionBlur, Blind, Ink, and Twist alongside the existing Basic/Fog shaders.
- Recover constructors, state changes, history buffers, completion flags, orientation traversal, and color arithmetic.
- Recovered render entries now identified for naming are MotionBlur `0x000e2950`, Gray `0x000e3064`, Fade `0x000e34d0`, Ink `0x000e3ae4`, Blind `0x000e3f00`, and Twist `0x000e43f0`.

**Verification**

- Test exact pixels for initial, intermediate, terminal, enabled, disabled, and orientation states.
- Include depth/color-key special cases and history-buffer reuse.

**Completion**

`PresentationShaders.*` and `test_shaders.cpp` implement and test all six effects. The reconstruction preserves packed Fade interpolation and delayed completion, Gray's two depth bypasses, MotionBlur reset/history and saturating 1/4+7/8 arithmetic, Ink's previous-destination neighborhoods/threshold polarity and engine-hook control, Blind's 16-bit-only traversal and wrapping, and Twist's row/frame bounce state. Both orientations and inactive/intermediate/terminal states are covered. All 22 normal CTest tests and all 16 SDL-independent ASan/UBSan tests pass without changing existing real-resource frames. Ghidra now has typed layouts, named control methods/destructors/helpers, and formula comments for the complete shader family. See `symbian3rd_shaders.md`.

## Stage 9 — Object API completion and engine renderable effects — complete

**Scope**

- Recover `Object_Load`, animation control, visibility flags, material loading, and remaining model/runtime state.
- Implement Lensflare, BillboardGroup, FakeShadow, and GunFire as reusable `Renderable` implementations.
- Complete texture secondary-allocation and remaining STX/SOJ/SAI unknowns when consumers identify them.

**Verification**

- Exercise real map models and animated SOJ/SAI assets through Object and the renderer.
- Test effect placement, visibility/depth checks, resource lifetime, and exact output.

**Completion**

`Object.*`, `Effects.*`, `Fixed.*`, `Camera.*`, `Renderer.*`, `Raster.*`, and `test_effects.cpp` implement and test the recovered behavior. Object now covers resource/model and material loading, bounds and world-corner tracking, transforms and scaling, animation pause/direction/loop/speed, secondary streams, bone overrides, transparency, wireframe rendering, and camera-depth sorting. Animated palettes include the view matrix, and secondary streams preserve palette selection. The matrix helper now performs the original dot product rather than an outer product; real BSP/SPL/menu hashes are unchanged.

Lensflare, BillboardGroup, FakeShadow, and GunFire are reusable renderables with recovered geometry, placement, visibility, depth, resource, and countdown behavior. Fixed-point sine/cosine uses the original 2049-entry table, and arctangent uses the original polynomial. Texture resize preserves the previous type byte and uses its argument only for bytes-per-pixel selection. No additional consumer-backed STX/SOJ/SAI layout change was justified. Span clipping was made memory-safe after Valgrind identified a one-past-end framebuffer access; the new offscreen regression and all original hashes pass.

All 23 normal CTest tests and all 17 SDL-independent ASan/UBSan tests pass, as do all six standalone SOJ converter tests. Ghidra has typed Object/material/effect layouts and 95 recovered or corrected function documentations. See `soft3d_models_animations.md` and `symbian3rd_effects.md`.

## Stage 10 — SST parser, VM, and generic script interface — complete

**Scope**

- Recover `SimpleScript` loading, typed variables/records, instruction stream, execution state, stack/call behavior, and lifecycle.
- Align the C# `SimpleScriptDecompiler` with the ARM loader and VM, including Win32/Mobile variants.
- Implement the generic `ScriptInterface` contract and engine callbacks, but not `GameSI`'s game command set.

**Verification**

- Compare parsed/decompiled output for real SST files with the C# tool.
- Run directed synthetic scripts for each generic opcode, variable type, branch, call, and termination path.
- Verify loading/unloading and suspended execution without episode logic.

**Completion**

`SimpleScript.*` and `test_script.cpp` implement the recovered loader, six value types, three reference scopes, 32-argument command records, recursive bodies/conditions, continuation linking, all internal opcodes, task states, entry conditions, borrowed `runTask` streams, initializer/update cadence, shared words, and generic host callbacks. `GameEngine` now owns and updates the real engine facility in game mode; episode commands remain external. The C# Mobile reader/decompiler matches the ARM layout and all 217 shipped Mobile SST files complete with zero failures, while the distinct Win32 reader remains intact.

Real-resource tests verify exact counts for `day1/ep0`, `day1/ep1010`, `day5/ep2050`, and `day7/ep7009`, including 829 commands and 244 values in the third. Synthetic tests cover every generic and implemented internal opcode, typed arithmetic/comparisons, control flow, suspension, termination, nested calls, reset, unload/reload, and malformed inputs. All 24 normal CTest tests and all 18 SDL-independent ASan/UBSan tests pass. Ghidra has saved typed layouts, function/value names, prototypes, tags, comments, and variable names. See `sst_format.md` for the full format, VM, addresses, and portable safety differences.

## Stage 11 — Generic unit, messaging, state, and physics infrastructure — complete

**Scope**

- Recover `MsgReceiverInterface`/`MsgReceiver`, `GameUnit`, `VisUnit`, `PhysicUnit`, `Things`, `Creature`, and generic `GameUnitManager` behavior.
- Implement reusable `StateMachine`/`StateSet` templates without concrete player state classes.
- Implement generic transforms, visibility, movement, collision, and BSP brush/leaf queries needed by these bases.
- Keep trigger/enemy/player concrete behavior outside this stage.

**Verification**

- Use synthetic derived classes to test virtual dispatch, messages, state transitions, ownership, movement, and map collision.
- Exercise the infrastructure with real BSP geometry without loading an episode.

**Completion**

`Units.*`, `StateMachine.hpp`, `BspCollision.cpp`, and `test_units.cpp` implement and test the recovered infrastructure. Message queues, all three unit queries, eight-bucket ownership/update/submission, child-first state transitions, LOD/visuals, simple and slide movement, ground/shadow traces, `Things` destruction, and `Creature` hearing/navigation are covered. The real `menu.sbp` is exercised without episode logic, and real SOJ resources exercise the visual path.

The shared BSP leaf decoding now matches ARM's direct `-child` leaf index, including collision and point queries; synthetic maps include the unused leaf-0 sentinel. The fixed render camera reports cluster 18, five PVS bytes, 68 visible faces, and three queue-3 models, with raw/presented hashes `0x9919103f`/`0x566dc515`. The update also fixes per-frame light assignment to object intensity rather than depth bias. All 25 normal CTest tests and all 19 SDL-independent ASan/UBSan tests pass, as do all six standalone SOJ converter tests. See `symbian3rd_units.md` for exact behavior, addresses, original quirks, and portable safety differences. Stream-backed state serialization is completed in stage 12.

## Stage 12 — Streams, state serialization, and small engine utilities — complete

**Scope**

- Implement `Stream`, `InputStream`/`OutputStream`, data streams, and file streams, including endian and error behavior.
- Recover engine-facing load/save interfaces without implementing the game's serialized state schema.
- Implement portable `MemTotal`/`MemRemain` diagnostics and remaining unused narrow IO/debug functions; `s3dMemInfo` and `s3dExit` themselves are already confirmed empty class shells.
- Remove remaining app-only duplicates of engine facilities.

**Verification**

- Test stream read/write/reopen/truncation/error paths and standalone state fixtures.
- Exercise suspend/resume/quit, memory diagnostics, and deterministic restart.

**Completion**

`Stream.*`, `Units.*`, `SimpleScript.*`, `s3dIO.*`, `s3dMemory.hpp`, `platform/linux/Memory.cpp`, `test_stream.cpp`, and the expanded `test_script.cpp` implement the recovered behavior. All seven stream classes preserve big-endian typed words, raw little-endian UTF-16 payloads, sentinel-inclusive sizes, strict bounds, growth, whole-file input, explicit buffered-file flushing, and truncating `wb` output. Portable checks reject malformed reads and zero the original indeterminate sentinel byte. `GameUnit` omits collision-enabled and consumes type separately; `VisUnit` restores resources and applies the last object's playback record to all objects. `SimpleScript` archives preserve name/shared/global/local/task/call state with working ordinal resolution. Narrow IO and shared byte-to-UTF-16 conversion are implemented, and the SDL app no longer duplicates widening. Standalone Linux RAM queries use 64-bit totals.

All 26 normal CTest tests, all 20 stdio-logging tests, and all 20 SDL-independent ASan/UBSan tests pass, as do all six standalone SOJ converter tests. Ghidra has saved stream layouts and 82 recovered function documentations, plus the corrected render-mode/external-transform fields and 0x48-byte `ObjectMaterial` layout. See `symbian3rd_streams.md` for exact layouts, addresses, original quirks, portable safety differences, and verification.

## Stage 13 — S3DTEX, secondary formats, and final engine audit

**Scope**

- Recover S2D/S3DTEX loader semantics, `unknown04`, color keys, palette variants, and any real `s3dImageLoader` engine path.
- Bring `Tools/s3dtex.py` into agreement with recovered behavior and test all 173 vendored samples.
- Audit STX/SOJ/SAI/SBN/SBP/SPL/SST/SDT notes against the implementation and update README status only where justified.
- Inventory any remaining unnamed RTTI engine class or stub and either implement it or document why it is platform/game-specific.

**Verification**

- Run the full CTest matrix, relevant decoder/layout/logging variants, Python converter suites, and headless SDL scenarios.
- Perform a final fresh-start scenario covering handedness, menu background, navigation, configuration, audio, timing, and shutdown.

**Completion**

Stage 13 is complete. S3DTEX is now decoded from the recovered S2D header: byte depth, palette count, 16-bit format/flags, dimensions, and signed 32-bit color key. `Tools/s3dtex.py` supports direct/keyed images, luminance, all six palette representations, palette alignment, and malformed-input checks. `Tools/test_s3dtex.py` loads all 173 vendored samples and synthetic variants; the previous `unknown04` was the format word's high byte. No Soft3D S3DTEX loader was invented: the Symbian `s3dImageLoader` instead supplies the previously uncalled TGA loader, reconstructed in `s3dImageLoader.*` with safe RLE/bounds behavior and tests.

The original 2049-entry tangent table is linked as `FixedTanTable.inc` and checked byte-for-byte against ARM and the SDK. Both sine and tangent tables are default; `SOFT3D_TRIG_USE_MATH=ON` explicitly selects host trigonometry. The final unused-code pass implements camera-path degree elevation/editing/bounds/transforms and storage lifecycle, camera/renderer viewports, file-based SBN loading, dialog field/runtime accessors and direct blits, and a replaceable `SceneHost` instead of a named stub. Camera projection now preserves the separate projection aspect ratio and the setter's original ignored near/far arguments.

The RTTI audit accounts for every recovered class. Concrete gameplay classes, episode `Scene` semantics, `GameSI` commands, and Symbian network/application services remain game/platform work under the boundary below. No unnamed RTTI class requires a new portable engine facility. STX/SOJ/SAI reserved fields were left unchanged where no consumer evidence justified new semantics.

Verification passes 27 default CTest tests, 21 math tests, 21 stdio-logging tests, 21 ASan/UBSan tests, and 21 WAR-decoder tests. The English SPK build passes its ten applicable non-Chinese-fixture tests; real-archive tests still require the Chinese fixture as documented. Eight S3DTEX and six SOJ Python tests pass. The final Xvfb/xdotool run uses real SDL input and 16 kHz dummy audio, reaches and captures the moving main menu, enters and captures Exit Game, confirms shutdown, and writes the expected 1252-byte `config.sdt`. See `s3dtex_format.md`, `symbian3rd_image.md`, `spl_format.md`, `sdt_format.md`, `sbn_format.md`, `symbian3rd_raster.md`, and `symbian3rd_rtti.md`.

## Explicitly postponed after engine completion

- `Scene`'s episode state machine and game progression;
- concrete Player/Enemy/Things descendants and trigger behavior;
- `GameSI` command implementations and episode-specific SST scripts;
- license `message.sdt`/`domain.sdt` consumers; `itemdata.sdt`, `gameitem*.sdt`, and `einfo*.sdt` are now implemented under game stage 1 in `game_data_tables.md`;
- Symbian network/IMEI/SMS application services unless a portable engine feature actually depends on them.

This boundary lets the engine become reusable and testable without embedding unfinished game behavior in its base systems.

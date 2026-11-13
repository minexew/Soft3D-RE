# Engine completion inventory and staged plan

## Purpose and boundary

This plan covers reusable Soft3D engine functionality and the platform shell needed to run it. It deliberately postpones concrete enemies, player state implementations, episode scripts, game-specific command handlers, and content-specific game-data semantics. Generic base classes and interfaces used by those systems remain in scope.

Every stage should include the corresponding Ghidra names/types/comments, English Markdown notes, real-resource or synthetic tests, and a full CTest run. A stage is complete only when its portable behavior is exercised end to end, not merely when a parser compiles.

## Current baseline

### Functional foundations already present

- SPK/FilePak, four-level resource cache, `Resource`, Linux file IO, logging, and wide formatting;
- SAU/WAR decoding, the 16-voice mixer, music/SFX/fade controls, positional audio, dummy audio, and SDL audio output;
- STX loading, SBN fonts/strings, the initial SDT resource/state tables, and exact-key image drawing;
- SOJ/SAI resource parsing and a functional Object → Renderer → Raster → BasicShader → VID_impl path;
- SBP container/runtime loading, all chunk readers, incremental progress, map-model resources, generic entity-factory ownership, and link cleanup;
- engine queue ordering and presentation, dummy video, and the SDL application shell.

These foundations still have partial areas: material fields, Object loading/control APIs, raster feature parity, generic dialog behavior, and the game hooks that invoke audio are not automatically supplied by the audio implementation.

### Missing or incomplete engine facilities

- `Bsp` rendering, PVS evaluation, lightstyle playback, collision, and integration of the loaded map models/entities into later unit systems;
- SPL curve/path loading, camera evaluation, and menu-background orchestration;
- real engine input state, Symbian-to-SDL key mapping, target-FPS timing, and restorable FPS drawing;
- generic `Dialog` interpolation/text/secondary-texture behavior, later `GameMenu` states, and menu dialog classes;
- SST parsing/execution and the generic script interface;
- specialized raster clipping, depth, perspective, blend, lighting, texture-coordinate, detail-texture, and inner-loop paths;
- Fade, Gray, MotionBlur, Blind, Ink, and Twist presentation shaders;
- Lensflare, BillboardGroup, FakeShadow, and GunFire renderables, plus unresolved SOJ material state;
- generic unit/physics/messaging/state-machine infrastructure and `GameUnitManager` support;
- generic streams, state load/save support, `s3dExit`, `s3dMemInfo`, and currently unused IO/debug facilities;
- S3DTEX engine/tool parity and remaining STX/SOJ/SAI unknown fields.

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

## Stage 2 — BSP rendering, PVS, lightmaps, and S3DSTY runtime

**Scope**

- Implement face construction, node/leaf traversal, PVS selection, visible brush/face submission, and map-model submission.
- Use the recovered face lightmap offsets/dimensions and style bytes; implement current-frame modulation.
- Implement S3DSTY loading and its sequence-advance cadence.
- Add only the raster features needed by real map faces; leave unrelated specialized paths for stage 7.

**Verification**

- Render `menu.sbp` from a fixed camera through dummy video and check exact framed output against recovered ARM arithmetic.
- Test PVS bit selection, leaf/node boundaries, style wrapping, and lightmap sampling with small synthetic maps.
- Document the actual rendering path and any newly recovered `Bsp` fields in Ghidra; do not use the already-refuted `0x000f0e20` target.

## Stage 3 — SPL evaluation and the complete menu 3D background

**Scope**

- Implement SPL parsing, Bezier evaluation, path timing/interpolation, and camera-path collection ownership.
- Recover the engine camera/projection helper and `CameraSpot` interface needed by the menu.
- Wire the menu backdrop host to `Bsp` and the camera without implementing the episode-specific `Scene` state machine.

**Verification**

- Parse all 11 records in `1000.spl` and verify fixed-point conversion and curve samples.
- Exercise the complete menu-background frame with dummy video, including camera movement over time.
- Confirm that this path needs no concrete enemy, trigger, or episode classes.

## Stage 4 — Input, target-FPS timer, lifecycle, and restorable FPS display

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

## Stage 5 — Generic SDT dialog behavior

**Scope**

- Complete `DialogManager`/`Dialog` state selection, key interpolation, visibility outside key ranges, text/behavior word handling, and secondary-texture setup.
- Recover all `ScreenBlitter` modes, clipping/scaling details, and text integration with `StringManager`.
- Identify the `str.sdt` unknown word and `state.sdt` cached-total field from consumers.
- Support all seven generic dialog tables rather than hardcoding dialog 0 or exact key matches.

**Verification**

- Use real `state.sdt` to cover every element/behavior variant and synthetic keys for interpolation boundaries.
- Compare exact dummy pixels for image, text, scaling, clipping, and secondary textures.

## Stage 6 — Menu flow, remaining dialogs, and configuration SDT

**Scope**

- Recover `GameMenu`, `MainUIDlg`, `PlayDlg`, `OptionDlg`, `LoadDlg`, `HelpDlg`, `AboutDlg`, and `GameUIDlg` engine-facing behavior.
- Implement handedness acceptance, logo/loading transitions, main-menu navigation, options/load/help/about flow, and their audio/video calls.
- Recover `config.sdt` as the menu/configuration persistence facility.
- Reuse the stage 3 background and stage 4 input rather than adding parallel menu-only input or rendering paths.

**Verification**

- Drive handedness → startup → main menu and each dialog with deterministic input and dummy video.
- Test configuration round trips, menu audio triggers, and returning to the background without resource leaks.

## Stage 7 — Rasterizer and renderer feature parity

**Scope**

- Recover clipping, winding, depth behavior, affine versus perspective-correct attributes, lighting, blend modes, texture offsets/wrapping, detail textures, and specialized scanline loops.
- Implement renderer sequential/indexed/textured entry points and draw-state selection not represented in the portable subset.
- Resolve remaining SOJ material fields when they control renderer state.
- Preserve original fixed-point overflow/truncation and known bugs.

**Verification**

- Build directed triangle/scanline fixtures for every selected path and compare exact RGB565/depth output.
- Re-run real SOJ and SBP scenes to detect regressions in previously working paths.

## Stage 8 — All presentation shaders

**Scope**

- Implement Fade, Gray, MotionBlur, Blind, Ink, and Twist alongside the existing Basic/Fog shaders.
- Recover constructors, state changes, history buffers, completion flags, orientation traversal, and color arithmetic.
- Recovered render entries now identified for naming are MotionBlur `0x000e2950`, Gray `0x000e3064`, Fade `0x000e34d0`, Ink `0x000e3ae4`, Blind `0x000e3f00`, and Twist `0x000e43f0`.

**Verification**

- Test exact pixels for initial, intermediate, terminal, enabled, disabled, and orientation states.
- Include depth/color-key special cases and history-buffer reuse.

## Stage 9 — Object API completion and engine renderable effects

**Scope**

- Recover `Object_Load`, animation control, visibility flags, material loading, and remaining model/runtime state.
- Implement Lensflare, BillboardGroup, FakeShadow, and GunFire as reusable `Renderable` implementations.
- Complete texture secondary-allocation and remaining STX/SOJ/SAI unknowns when consumers identify them.

**Verification**

- Exercise real map models and animated SOJ/SAI assets through Object and the renderer.
- Test effect placement, visibility/depth checks, resource lifetime, and exact output.

## Stage 10 — SST parser, VM, and generic script interface

**Scope**

- Recover `SimpleScript` loading, typed variables/records, instruction stream, execution state, stack/call behavior, and lifecycle.
- Align the C# `SimpleScriptDecompiler` with the ARM loader and VM, including Win32/Mobile variants.
- Implement the generic `ScriptInterface` contract and engine callbacks, but not `GameSI`'s game command set.

**Verification**

- Compare parsed/decompiled output for real SST files with the C# tool.
- Run directed synthetic scripts for each generic opcode, variable type, branch, call, and termination path.
- Verify loading/unloading and suspended execution without episode logic.

## Stage 11 — Generic unit, messaging, state, and physics infrastructure

**Scope**

- Recover `MsgReceiverInterface`/`MsgReceiver`, `GameUnit`, `VisUnit`, `PhysicUnit`, `Things`, `Creature`, and generic `GameUnitManager` behavior.
- Implement reusable `StateMachine`/`StateSet` templates without concrete player state classes.
- Implement generic transforms, visibility, movement, collision, and BSP brush/leaf queries needed by these bases.
- Keep trigger/enemy/player concrete behavior outside this stage.

**Verification**

- Use synthetic derived classes to test virtual dispatch, messages, state transitions, ownership, movement, and map collision.
- Exercise the infrastructure with real BSP geometry without loading an episode.

## Stage 12 — Streams, state serialization, and small engine utilities

**Scope**

- Implement `Stream`, `InputStream`/`OutputStream`, data streams, and file streams, including endian and error behavior.
- Recover engine-facing load/save interfaces without implementing the game's serialized state schema.
- Complete `s3dMemInfo`, remaining `s3dExit` behavior, unused narrow IO/debug functions, and platform timer/exit wrappers.
- Remove remaining app-only duplicates of engine facilities.

**Verification**

- Test stream read/write/reopen/truncation/error paths and standalone state fixtures.
- Exercise suspend/resume/quit, memory diagnostics, and deterministic restart.

## Stage 13 — S3DTEX, secondary formats, and final engine audit

**Scope**

- Recover S2D/S3DTEX loader semantics, `unknown04`, color keys, palette variants, and any real `s3dImageLoader` engine path.
- Bring `Tools/s3dtex.py` into agreement with recovered behavior and test all 173 vendored samples.
- Audit STX/SOJ/SAI/SBN/SBP/SPL/SST/SDT notes against the implementation and update README status only where justified.
- Inventory any remaining unnamed RTTI engine class or stub and either implement it or document why it is platform/game-specific.

**Verification**

- Run the full CTest matrix, relevant decoder/layout/logging variants, Python converter suites, and headless SDL scenarios.
- Perform a final fresh-start scenario covering handedness, menu background, navigation, configuration, audio, timing, and shutdown.

## Explicitly postponed after engine completion

- `Scene`'s episode state machine and game progression;
- concrete Player/Enemy/Things descendants and trigger behavior;
- `GameSI` command implementations and episode-specific SST scripts;
- semantic implementations of `itemdata.sdt`, `gameitem*.sdt`, `einfo*.sdt`, and `message.sdt` as game content;
- Symbian network/IMEI/SMS application services unless a portable engine feature actually depends on them.

This boundary lets the engine become reusable and testable without embedding unfinished game behavior in its base systems.

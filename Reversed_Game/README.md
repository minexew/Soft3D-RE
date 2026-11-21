# 7 Days: Salvation — Soft3D reconstruction

This directory is a self-contained C++17/CMake reconstruction of the Soft3D engine and an SDL3 application shell for **7 Days: Salvation**. It is based on reverse engineering of the original binaries and preserves recovered behavior, including fixed-point arithmetic, lookup tables, rendering quirks, and known original bugs where they affect observable behavior.

The reusable engine reconstruction is substantially complete. The application supports startup and handedness selection, logos and sound setup, the animated 3D main-menu background, menu/dialog navigation, UI audio, and configuration persistence. Stage 2 also installs the concrete `Scene` and `GameSI`: a new game loads day 1, selects `day1/ep0`, and incrementally loads its initial map through the real engine loop. **This is not yet a complete playable recreation.** Camera/player/gameplay commands, full scene persistence, and license services remain staged work; unimplemented script commands fail explicitly rather than advancing silently.

No parent repository, analysis tools, Ghidra installation, or external format documentation is required to build or use this directory.

## Contents

- `CMakeLists.txt` — library, application, build options, and CTest definitions.
- `src/soft3d/` — the reconstructed engine:
  - SPK archives, resource caching, and file/stream IO;
  - STX textures, TGA image loading, SBN fonts and strings, and SDT UI data;
  - SOJ/SAI models and animation;
  - SBP maps, BSP rendering/collision/path queries, S3DSTY light styles, and SPL camera paths;
  - software rasterization, render queues, objects, visual effects, and presentation shaders;
  - input, timing, dialogs, menu flow, configuration, and save-slot support;
  - SAU/WAR decoding, a 16-voice software mixer, fades, and positional sound;
  - the generic SST script loader, VM, task lifecycle, and host interface.
- `src/game/` — the `seven_days_game` library: checked `itemdata`/`gameitem`/`einfo` parsing, fixed animation/sound/door resource tables, `MyGameUnitMgr`, concrete `Scene` loading/lifecycle, and the 218-slot `GameSI` dispatch/context. Concrete camera, player, unit, UI, and save behavior remains staged work.
- `src/platform/linux/` — Linux file, clock, logging, and memory support.
- `src/platform/sdl/` — SDL3 video, audio, input, and clock backends.
- `src/platform/dummy/` — deterministic video/audio backends used by tests and other headless hosts.
- `src/app/` — the `seven_days` SDL application.
- `src/resources/res.spk` — the reference resource archive used by the application and real-resource tests.
- `tests/` — synthetic and real-resource tests, including exact rendering/audio checks.

The original game resources are proprietary. Preserve any applicable rights and restrictions when copying this directory or distributing the archive.

## Requirements

- Linux; the project intentionally rejects other CMake target systems until a platform backend is added.
- CMake 3.16 or later.
- A C++17 compiler.
- SDL3 with its CMake package configuration when building the application.

The `soft3d` library, dummy backends, and SDL-independent tests do **not** require SDL when the application is disabled.

## Build

From this directory:

```sh
cmake -S . -B build
cmake --build build --parallel
```

This builds the `soft3d` and `seven_days_game` static libraries, the `seven_days` executable when SDL3 is available, and the test executables.

To build only the engine and SDL-independent tests:

```sh
cmake -S . -B build-core -DSOFT3D_BUILD_APP=OFF
cmake --build build-core --parallel
```

## Run

Supply the included archive explicitly when running from this directory:

```sh
./build/seven_days --resources src/resources/res.spk
```

The application opens a resizable window and starts at the handedness-selection screen. It uses 320×240 internal rendering, a software rasterizer, and 16 kHz mono audio. SDL's software renderer is available as a fallback.

Useful options:

```text
--renderer auto|software|NAME   SDL renderer selection; default: auto
--scale 1..4                   Integer window scale; default: 3
--orientation auto|landscape|portrait
                               Display orientation; default: auto
--frames N                     Exit after N frames; zero means unlimited
--resources PATH               Resource archive path
--no-audio                     Disable audio initialization/output
--show-fps                     Restore the dormant FPS text helper
--new-game                     Start day 1 immediately; print scene state on exit
--help                         Print command-line help
```

Automatic orientation presents handedness selection in portrait with direct phone-style directions, then switches to landscape and compensates for the selected handedness so controls remain screen-relative. Explicit orientation modes force one presentation throughout.

`--show-fps` enables the recovered text helper, but the original game does not update the underlying real-FPS field in this frame path; it should not be interpreted as a new FPS counter.

### Controls

- Arrow keys or WASD: navigate/change selection.
- Space or Enter: accept/activate.
- Escape: quit the application shell.

Choosing **Exit Game** and confirming writes `config.sdt` in the process working directory. The configuration contains sound, vibration, control, and unlock-related state. Save-slot APIs are also present, but episode-specific save content belongs to the gameplay host.

For a nonvisual smoke run:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  ./build/seven_days --resources src/resources/res.spk \
  --renderer software --frames 3
```

## Build options

Pass options with `-DNAME=VALUE` during CMake configuration. Use separate build directories for incompatible variants.

| Option | Default | Purpose |
|---|---|---|
| `SOFT3D_BUILD_APP` | `ON` | Build the SDL3 application. Disable it to remove the SDL dependency. |
| `BUILD_TESTING` | `ON` through CTest | Build/register the test suite. |
| `SOFT3D_LOG_BACKEND` | `NULL` | `NULL` preserves release-like disabled logging; `STDIO` writes UTF-16LE `log.log` output. |
| `SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER` | `OFF` | Decode high-nibble-first WAR instead of the default low-nibble-first SAU stream order. Select this only for matching resources. |
| `SOFT3D_SPK_ENGLISH_LAYOUT` | `OFF` | Use English archives with 32-byte path entries instead of the bundled Chinese archive's 64-byte entries. |
| `SOFT3D_TRIG_USE_MATH` | `OFF` | Use host trigonometry instead of the original 2049-entry sine and tangent lookup tables. This is an explicit compatibility/accuracy tradeoff, not the faithful default. |

Examples:

```sh
cmake -S . -B build-log -DSOFT3D_LOG_BACKEND=STDIO
cmake --build build-log --parallel

cmake -S . -B build-math -DSOFT3D_TRIG_USE_MATH=ON
cmake --build build-math --parallel
```

The bundled archive uses the Chinese SPK layout and SAU audio. Do not enable the English-layout or WAR-decoder options for it unless converting or replacing the corresponding resources.

## Test

After a default build:

```sh
ctest --test-dir build --output-on-failure
```

The default suite has 30 tests, including SDL application smoke tests and a real new-game/map-load run. A `SOFT3D_BUILD_APP=OFF` build has 23 tests. Coverage includes:

- resource/archive parsing and cache lifecycle;
- all shipped item/day data tables, fixed resource names, malformed data, missing files, and engine integration;
- all 218 `GameSI` records, recovered scene commands, explicit unsupported-command errors, real `ep0` loading, incremental SBP progress/finalization/fallback, null entities, and repeated scene/menu teardown;
- stream and file behavior;
- real font, UI, map, model, animation, camera-path, and audio resources;
- exact software-rendering and presentation pixels;
- input translation, deterministic 17 FPS timing, and lifecycle;
- menu navigation and configuration round trips;
- generic script opcodes/tasks/state and unit/physics/collision infrastructure;
- image decoders, error paths, and malformed inputs.

Variant builds should be tested independently:

```sh
ctest --test-dir build-core --output-on-failure
ctest --test-dir build-log --output-on-failure
ctest --test-dir build-math --output-on-failure
```

The English-layout option requires a matching English resource archive for real-archive tests; those tests cannot use the bundled Chinese fixture.

## Use as a library

The reusable engine target is `soft3d`; game data and content ownership are in `seven_days_game`, which links it publicly. Public headers are under `src/`:

```cmake
target_link_libraries(your_target PRIVATE seven_days_game)
target_include_directories(your_target PRIVATE path/to/this/directory/src)
```

A host normally supplies platform backends through `EngineConfig`, sets the resource archive on `GameEngine`, initializes it, and drives `RunFrame`. The dummy backends are suitable for deterministic/headless use; the SDL backends show the interactive equivalents.

`GameplayHooks`, `SceneHost`, and `ScriptInterface` are intentional boundaries for game-specific behavior. They allow a host to supply episode startup/loading, scene processing, rendering, item actions, and script commands without embedding unfinished episode semantics in the generic engine.

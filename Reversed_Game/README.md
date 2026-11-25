# 7 Days: Salvation — Soft3D reconstruction

This directory is a self-contained C++17/CMake reconstruction of the Soft3D engine and an SDL3 application shell for **7 Days: Salvation**. It is based on reverse engineering of the original binaries and preserves recovered behavior, including fixed-point arithmetic, lookup tables, rendering quirks, and known original bugs where they affect observable behavior.

The reusable engine reconstruction is substantially complete. The application supports startup and handedness selection, logos and sound setup, the animated 3D main-menu background, menu/dialog navigation, UI audio, and configuration persistence. Stages 1–16 install concrete game data, `Scene`, the complete 218-slot `GameSI`, the eight-mode game camera, map camera/light triggers, door transitions, environment audio, scene presentation, the concrete player with all 20 states, Things/map-unit/trap/StayBox implementations, all nine interaction trigger classes, concrete GameUI/HUD/map/inventory/dialog/QTE/extras consumers, common enemy AI/pooling, all 18 concrete enemy classes with their script commands, deterministic full scene/script-sidecar save/load, and recovered license/device services with menu/scene authorization and vibration. A new game loads day 1, selects `day1/ep0`, incrementally loads its initial map through the real engine loop, spawns real map items, and continues through the running script. All 217 shipped episodes and all 152 shipped SBP files have integration coverage, with a real save/load continuation on every day. Stage 16 completes the native function/vtable inventory and source-layering audit. Phase-D Session 30 adds the recovered unused scalar conversion/reciprocal, Color4, Vec2/Vec3/Vec4, interpolation, and quaternion APIs with native-derived directed tests. Session 31 adds the missing matrix determinant/inversion/rotation/factory and projection variants, project/unproject and homogeneous transforms, public Plane/Ray/frustum/slide operations, and owning PointCloud construction/editing/clipping/validation APIs; Bezier, camera-path, and SPL storage were confirmed already complete. Automated fixed-frame coverage is not a human full playthrough or a claim of exhaustive audiovisual parity; exact limits are recorded in `../Reverse_Docs/stage16_audit.md`.

No parent repository, analysis tools, Ghidra installation, or external format documentation is required to build or use this directory.

## Contents

- `CMakeLists.txt` — library, application, build options, and CTest definitions.
- `src/soft3d/` — the reconstructed engine:
  - SPK archives, resource caching, and file/stream IO;
  - STX textures, TGA image loading, SBN fonts and strings, and SDT UI data;
  - SOJ/SAI models and animation;
  - SBP maps, BSP rendering/collision/path queries, enemy blood-splat lightmap blending, S3DSTY light styles, and SPL camera paths;
  - software rasterization, render queues, objects, visual effects, and presentation shaders;
  - input, timing, dialogs, menu flow, configuration, and save-slot support;
  - SAU/WAR decoding, a 16-voice software mixer, fades, and positional sound;
  - the generic SST script loader, VM, task lifecycle, and host interface;
  - shared fixed-point scalar/vector/matrix operations, recovered Color4/interpolation/quaternion helpers, matrix inversion/rotation/projection and homogeneous project/unproject APIs, plus `Geometry.*` bounds, Plane/Ray/frustum/slide, and owning PointCloud construction/editing/clipping/validation; `Color.hpp` provides RGB565 and `Endian.hpp` raw-byte helpers.
- `src/game/` — the `seven_days_game` library: checked `itemdata`/`gameitem`/`einfo` parsing, fixed animation/sound/door resource tables, `MyGameUnitMgr`, the eight-mode `CameraController`, map camera/light and all seven script-created interaction triggers, concrete `Scene` loading/lifecycle/transitions/presentation, `Player`/`Sword` and all 20 player states, `MapUnits` (`Breakable`/`Pushable`/`Pickable`/`BspUnit`/`ElectricSaw`/`Trap`/`StayBox`), common `Enemy` AI/pooling/waypoints/combat/state serialization, concrete `MaidBully` classes, `SpecialEnemies` (Worm/Ghost/Lamper/Weed), `NamedEnemies` (Victor/Sam/Prisoner/Kate/DarkKen), the complete `Lord` encounter, and the separate `Boss` sequence controller. `SaveGame.cpp` implements full deterministic scene/slot persistence, typed unit reconstruction, and script sidecars. `License.*` implements recovered DES/MD5 verification, device state, SMS retry/timeout behavior, and default-cracked authorization gates. The directory also contains the complete 218-slot `GameSI` dispatch/context and `GameUiController` with concrete HUD/dialog/QTE/inventory/map/extras behavior. `DebugConsole.*` reconstructs the native dead debug tooling, disabled by default and available only through an explicit scene-host opt-in. Stage 16 deduplicates concrete enemy configuration/resource loading in `Enemy.*` and keeps episode behavior out of the reusable engine.
- `src/platform/` — device identity, network, SMS, and vibration interfaces; Linux file/clock/logging/memory/test-identity/SMS support, SDL3 video/audio/input/clock/haptic backends, and deterministic dummy video/audio/license/vibration backends.
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

Choosing **Exit Game** and confirming writes `config.sdt` in the process working directory. The configuration contains sound, vibration, control, and unlock-related state and remains separate from slot progression. In-game saves write `slot%d.sav` plus `spt\\slot%d.sav`; loading reconstructs the full scene, concrete units, player, content, and script task/local state rather than only reading the display header.

License checks default to the recovered cracked-binary behavior. The application uses the explicit test IMEI `001001000000007` and deterministic Linux SMS completion, while network access is unsupported unless a host injects a transport. Menu authorization can be skipped through its native back path. `OfflineAuthorized` is a separate test/host policy, not the default. SDL haptic rumble is used when available; unsupported vibration returns false.

The recovered debug console has no caller in the original build and is disabled by default here as well. Library hosts can opt in with `Scene::SetDebugConsoleEnabled`; the normal SDL application exposes no cheat shortcut.

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

Run tests serially within each build directory; some tests use fixed temporary filenames and can overwrite one another's files when run in parallel.

The default suite has 41 tests, including SDL application smoke tests and a real new-game/map-load run. A `SOFT3D_BUILD_APP=OFF` build has 34 tests. Coverage includes:

- native-derived fixed-point scalar conversions, reciprocal ranges, Color4 operations, Vec2/Vec3/Vec4 transforms/interpolation/normalization, quaternion conversions, matrix determinants/inversion/factories, all projection/sign variants, and project/unproject through `test_math.cpp`;
- Plane/Ray distances and thresholds, five-plane frusta, slide arithmetic, PointCloud ownership/construction/editing/clipping, degenerate inputs, and every polygon-validation result through `test_geometry.cpp`;
- resource/archive parsing and cache lifecycle;
- all shipped item/day data tables, fixed resource names, malformed data, missing files, and engine integration;
- all 218 `GameSI` records with explicit handlers, synthetic coverage of the 53 shipped-unused IDs and final slots, real `ep0` execution through input ID 47 and beyond, incremental SBP progress/finalization/fallback, null entities, and repeated scene/menu teardown;
- archive-derived execution of all 217 episodes through real engine frames and all 151 day maps plus the menu through the production BSP/entity factory, all 19,251 shipped host-call records, one real save/load continuation on each of days 1–7, and a final new-game/exit cycle through `test_game_integration.cpp`;
- the recovered debug console's real key/input, reset/seven-character behavior, health/weapon/item/healing cheats, map/day transitions, event preservation, rendering, and default-disabled policy through `test_game_scene.cpp`;
- every game-camera mode, binding/freeze/interpolation, timed/projected SPL playback, shake, nested trigger restoration, real map camera/light triggers, and type-102 values;
- every player state and virtual slot, attack chains/damage/hit windows, hurt/death, QTE input/results, hide/FPS, pushing, inventory/save fields, all player script commands, real input, and exact representative player frames;
- every AddThing variant and thing command, break/push/pickup/trap/StayBox behavior, derived persistence records, real `BspUnit` models, and a real lensflare entity;
- every interaction-trigger command, finite/infinite events, pickup accept/cancel/full-inventory flows, locked/tracked/special doors, act choices and camera warmup restoration, hide confirmation/masks, eight-item use zones, multi-line Talk timing and both completion modes, second-save bytes, and real map-2008 item spawning/use through the engine frame loop;
- stage-8 GameSI commands, dialog/result state, item use/combination and unlock progression, map calibration/markers, and exact HUD/dialog/QTE/inventory/map/extras frames through `test_game_ui.cpp`;
- all 17 enemy constructor configurations field-by-field, common states/perception/waypoints/combat/death, four-word state round trips, pool exhaustion/reuse, every enemy/collision command, real `einfo2` spawning, StayBox return behavior, all seven Maid/Bully variants and their extra resources, Bully hit windows, FatMaid projectile behavior, Day3 charge/QTE/death, Day5 saw states/segment hits/reward, all four stage-11 special enemies, Worm colour/jump/reward behavior, Ghost Twist/proximity effects, Lamper one-hit death, Weed custom damage, all seven stage-12 classes, Victor/Sam/Prisoner behavior and rewards, Kate/Bully linkage, DarkKen region/dual-gun combat, Lord's complete equipment/corpse encounter, Boss sequence/death behavior, real `einfo`/SST encounters, and real map combat through `test_game_enemy.cpp`;
- exact blood-splat palette/blend pixels, transparency, U/V flips, clipping, axis-plane behavior, and native normalized-reciprocal crossing through `test_bsprender.cpp`;
- full scene/header/environment/player/manager/unit/script-sidecar round trips, every reconstruction family, concrete enemy and StayBox persistence, invalid links/types, truncation, deterministic second-save bytes, and resumed real engine frames through `test_game_save.cpp`;
- native DES/MD5 and real message/domain fixtures, protocol success/malformed/failure/pending/cancel cases, license policy differences, missing capabilities, SMS retry/timeout, vibration, later-day gates, and exact authorization frames through `test_game_license.cpp`;
- door transition frames, randomized environment channels, listener selection, ending-specific day titles, and game-over/Ink/restart behavior;
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

Stage-16 final validation passes 39/39 default, STDIO-logging, host-math, and WAR tests; 32/32 SDL-independent core and ASan/UBSan tests; and 18/18 applicable English-layout tests (17 engine tests plus `game_camera`). Fourteen Chinese-only font/UI/menu/audio/game entrypoints are excluded from that English applicable set, not reported as successful English-archive validation. The SOJ and S3DTEX Python suites also pass 6/6 and 8/8 tests. See `../Reverse_Docs/stage16_audit.md` for the exact inventory, commands, sanitizer fix, and unverified visual/live-platform limits.

Session-31 validation passes 41/41 default, STDIO, host-math, and WAR tests; 34/34 core and ASan/UBSan tests; 20/20 applicable English-layout tests; and 6/6 SOJ plus 8/8 S3DTEX Python tests. The same 14 Chinese-only English exclusions apply. See `../Reverse_Docs/soft3d_geometry.md` and `soft3d_sdk_inventory.md` for the recovered API ledger and safety differences.

## Use as a library

The reusable engine target is `soft3d`; game data and content ownership are in `seven_days_game`, which links it publicly. Public headers are under `src/`:

```cmake
target_link_libraries(your_target PRIVATE seven_days_game)
target_include_directories(your_target PRIVATE path/to/this/directory/src)
```

A host normally supplies platform backends through `EngineConfig`, sets the resource archive on `GameEngine`, initializes it, and drives `RunFrame`. The dummy backends are suitable for deterministic/headless use; the SDL backends show the interactive equivalents.

`GameplayHooks`, `SceneHost`, and `ScriptInterface` are intentional boundaries for game-specific behavior. They allow a host to supply episode startup/loading, scene processing, rendering, item actions, and script commands without embedding unfinished episode semantics in the generic engine.

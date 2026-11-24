# Stage 16 final feature-parity and validation audit

## Scope and conclusion

Stage 16 closes the remaining native-inventory and portable-layering gaps for the Symbian 3rd build. ARM disassembly and decompilation remain the behavioral authority. Shipped resources and automated engine-frame tests provide coverage evidence, not a substitute for native semantics or a claim that every conditional gameplay branch has been played by a human.

The native audit leaves no `FUN_*` or `vfunc_*` function names. Concrete episode logic remains in `Reversed_Game/src/game`; reusable utilities remain in `src/soft3d`; device/network/SMS/vibration interfaces remain in `src/platform`. Original empty methods and duplicate/dead compiled bodies are identified rather than filled with invented behavior.

## Function and vtable inventory

- The final database contains **3,940 functions**. Searches for `^FUN_` and `^vfunc_` both return zero. **1,099** recovered functions carry the `stage16` audit tag.
- The resumable decompilation inventory examined 775 remaining bodies in the game/engine code range `0x00090ddc`–`0x00139c2c`, including uncalled methods, destructors, duplicate constructors, vendor runtime, and Symbian support. Other recovered entry/early-runtime bodies were classified interactively. Symbian application/document/UI boilerplate is named and documented but is not reproduced as portable game logic.
- **153 vtables and 1,734 slots** were rechecked, including secondary-vtable thunks. No slot retains a `FUN_*` target. Missing secondary thunks for Bsp, InkShader, Sword, and Player were created and named.
- Seventy `vfunc_*` placeholders were resolved. Most were destructor slots; the remainder include the original empty `Sword_Render_Empty`, `CameraTrigger_SubmitRenderables_Empty`, and shared `State_Player_Register` behavior.
- False function splits at `0x00034808`, `0x00014ce8`, and `0x00014ddc` were deleted and their owning Player/Prisoner functions repaired. Duplicate compiled implementations remain separate with explicit `Duplicate` names instead of being mislabeled as unknown logic.
- The final 890 apparent high-address functions, from `0x00166866` through `0x001ba69a`, were data rather than ARM code. Their false function records were removed. `Runtime_RunConstructors` ends at `0x00139c67`; bookmarks at `0x00139c68`, `0x00166866`, `0x00198ec0` (constant `0x47ff` words), and `0x001ba1e0` (import cells) record the evidence and code/data boundary.

Ghidra also contains the recovered 0x68-byte `DebugConsoleLayout`, typed debug-console prototypes and behavioral comments, and stage-16 bookmarks/tags. After the complete validation matrix passed, the `Stage16Status` completion property was written at `0x00095730` with the function/vtable and test counts, and the program was saved. Readback confirms `unsaved_changes=false`.

## Portable parity and layering changes

### Dormant debug console

`src/game/DebugConsole.*` reconstructs the uncalled native console at `0x00095730`–`0x000966c7`: seven entered characters, space/digit selection, health/weapon/item/healing cheats, day transitions, same-day preservation of event flags 1000–1002, and SBP-header spawn loading. Reset clears the current choice, the one-space command retains the native `AddItem(0)` call, and rendering uses engine-height centering and the native two-pixel cursor.

The native constructors/update/renderer have no callers. Portable code therefore keeps the feature **disabled by default**; `Scene::SetDebugConsoleEnabled` is an explicit host option. The generic `SceneHost::PostRender` hook lets `Scene` draw the enabled console after the UI, without putting game rules in the engine. Native unchecked map-file failures become a checked false result in portable code. A debug map jump first calls `ResetTransitionForDebugJump`, clearing an in-progress door transition and its animation reference; this host-only safety path prevents the old door model from being submitted after cache teardown and does not alter normal script/door transitions. Full input grammar, layout, and native addresses are in `symbian3rd_debug.md`.

### Shared utilities and deduplication

- `src/soft3d/Geometry.hpp` owns `ObjectBounds`, translation, inclusive containment/overlap, the native fixed-point three-face/three-cross-axis segment test, reset/expand, ordered bounds, and center operations. Consumers no longer duplicate these generic algorithms in game files.
- `Fixed.*` owns shared `LookAt`, `RotateXMatrix`, and `TranslateMatrix`; `Color.hpp` owns RGB565 pack/expand; `Endian.hpp` owns raw endian readers/append. Their arithmetic and byte order are unchanged.
- Concrete enemy configuration/resource loading is deduplicated in `src/game/Enemy.*`, with Maid/Bully, special, named, and Lord variants using the shared game-layer implementation. Episode rules were not moved into `src/soft3d` or `src/platform`.
- `src/soft3d` has no `game/` include dependency. The application still reaches gameplay through `SceneHost`, `GameplayHooks`, and `ScriptInterface`; no concrete game class was added to the reusable library.

## Validation coverage

The integration test counts all 19,251 shipped host-call records, executes all 218 GameSI slots (including synthetic use of the 53 shipped-unused IDs and IDs 258–260), runs all 217 episodes through real engine frames, and loads all 151 day maps plus the menu through production loading/entity construction. It also performs a real `SaveSaveSlot`/`LoadSaveSlot` continuation on each of days 1–7 in separate temporary persistence directories, verifies restored day/map/script/event state, runs 40 subsequent engine frames, and finishes with a new-game/menu/new-game/shutdown cycle.

Existing focused suites cover return-to-menu and repeated teardown (`test_game_scene.cpp`), game-over/Ink/restart (`test_game_scene.cpp`), configuration and unlock round trips (`test_menu.cpp`, `test_game_ui.cpp`), deterministic full saves and malformed records (`test_game_save.cpp`), SDL key translation (`test_sdl_input.cpp`), and dummy SDL video/audio application startup/shutdown (the `seven_days_*_smoke` tests). The debug-console test drives real engine key events and `GameEngine::RunFrame`, including reopen/reset, the seven-character limit, every cheat family, transitions, rendering, and default-disabled behavior. ASan initially exposed a stale door-animation reference in the mid-transition debug map jump; the host-only transition reset fixed it, and the complete sanitizer suite then passed without suppressing the finding.

### Test matrix

| Configuration | Result | Notes |
|---|---:|---|
| Default | 39/39 passed | Includes SDL application smoke tests and real new-game startup. |
| SDL-independent core | 32/32 passed | `SOFT3D_BUILD_APP=OFF`. |
| STDIO logging | 39/39 passed | `SOFT3D_LOG_BACKEND=STDIO`. |
| Host trigonometry | 39/39 passed | `SOFT3D_TRIG_USE_MATH=ON`; compatibility variant, not the faithful default. |
| WAR nibble order | 39/39 passed | `SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER=ON`. |
| ASan/UBSan | 32/32 passed | SDL-independent debug build with address/undefined-behavior sanitizers; includes all episodes, maps, and daily continuations. |
| English SPK layout | 18/18 applicable tests passed | `SOFT3D_SPK_ENGLISH_LAYOUT=ON`, SDL-independent; Chinese-only entrypoints are excluded from this applicable set. |
| SOJ converter | 6/6 passed | `Tools/test_sojconv.py -v`. |
| S3DTEX converter | 8/8 passed | `Tools/test_s3dtex.py -v`, including all 173 vendored samples. |

Variant suites are built separately and run serially because several tests use fixed temporary filenames. The application smoke commands pass an absolute archive path so nested variant build directories do not accidentally depend on the caller's working directory.

## Explicit validation limits

- The episode sweep uses fixed frame budgets. It proves loading, VM/concrete-scene execution, and the exercised paths; it is not a human full-game playthrough and does not prove every script conditional or every possible combat/UI outcome. Static host-call counts are not dynamic branch-coverage counts.
- Exact pixels/audio/state are asserted where native behavior has been recovered and directed fixtures exist. No specific visual discrepancy remains open in those directed checks, but arbitrary uncaptured frames are unverified. No blanket claim of exhaustive pixel parity is made, and no original Symbian executable was run side by side for every frame.
- The English applicable set contains 17 engine tests plus `game_camera`. Real Chinese-dependent sections inside those tests report their existing skips. The full 32-test English invocation was also tried diagnostically: the same 18 passed and 14 entrypoints requiring the Chinese archive failed at initialization (font/UI/menu/audio and ten game fixtures). Those 14 are excluded rather than misreported as English validation; no matching English archive was supplied.
- The bundled audio is SAU. The WAR build validates the alternate nibble order with its applicable fixtures, not the bundled SAU data decoded under the wrong order.
- Live Symbian device identity, network, SMS, and haptic hardware are not exercised. Portable tests use recovered protocol fixtures and explicit dummy/Linux/SDL policies; `OfflineAuthorized` remains test/host-only rather than the application default.
- The recovered debug console is dead code in this build and stays opt-in. No new cheat shortcut is exposed in normal application controls.

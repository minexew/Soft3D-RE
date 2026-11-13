# AGENTS.md — notes for AI coding agents

## Project overview

Soft3D-RE is a reverse-engineering and preservation project for the **Soft3D** game engine (C++, some builds include RTTI), best known from the mobile game **7 Days: Salvation**. The same engine was also used by 天地道 (Hellstriker II) and 斗地主 on the Gemei A330. Target platforms of the original game: Dingoo A320, Gemei A330, Symbian S60 (v1/v2/v3), and Win32.

The repository is *not* a single buildable application. It is a research collection:

- archived original game releases (binaries),
- reverse-engineering notes and format documentation,
- small standalone analysis/conversion tools written in Python 3 and C#,
- leaked original material (script sources, docs, tools).

Upstream: `git@github.com:minexew/Soft3D-RE.git`. The README's file-format table (S3DTEX, SBN, SBP, SAU, WAR, STX, SOJ, SST, SPL, SDT, SAI, S3DSTY) is the canonical index of what is understood; keep it updated when a format's status changes (legend: ✔️ well-understood, 🔧 mostly understood, ❌ unknown).

## Directory layout

| Path | Contents |
|---|---|
| `AGENTS.md` | Documentaion for AI agents. Upon changing project layout, adding new files, directories or doing major changes, be sure to re-read and fix inconsistencies and mention new additions. |
| `README.md` | Main documentation: file-format status table, platforms, links. |
| `Releases/` | Archived original game builds per platform (`.7z`, `.sis`, `.sisx`, `.zip`). Reference material — do not modify or "clean up". |
| `Tools/` | The main working code (see below). |
| `Tools_Other/` | Third-party/legacy Windows executables (`salvador_*.exe`, `stxgraph.exe`). |
| `Extractor/` | Qt Widgets GUI tool for extracting `.spk` archives (qmake project). Also contains committed build artifacts (`*.o`, `moc_*`, `ui_*.h`, `Makefile*`) and extracted output (`extracted_spkres/`, `a/`, `res.spk`). |
| `Leaked_Source/` | Leaked original engine script sources: `ep0.cpp`, `ep2050.cpp`, `commandlist.txt` (script API reference). **Text is Chinese in GBK/GB2312 encoding with CRLF** — do not re-encode. |
| `Leaked_Docs/` | `音效列表.xls` — sound-effects list (Excel, codepage 936). |
| `Leaked_Tools/` | Original Windows tools for WAR audio playback. |
| `Other/S2DSDK/` | Vendored S2D SDK (predecessor engine) plus a sample game. Marked `linguist-vendored` in `.gitattributes`; original URLs are in `Other/S2DSDK/readme.txt`. Treat as read-only reference. |
| `Reverse_Docs/` | Knowledge base and main library of format and binary-analysis notes. Highly useful, since it both provides a birds-eye on reversing progress and gives important technical details, such as class and structure layouts. In addition to FilePak/RTTI documentation, `symbian3rd_audio.md` and `symbian3rd_video.md` record the recovered software mixer, positional audio, renderer, texture, and presentation APIs. `gameengine_api.md` covers `Engine`/`GameEngine`, queues, object rendering, and shader presentation; `soft3d_models_animations.md` covers corrected SOJ/SAI layouts and consumers. `resmanager_api.md` covers the four-level resource cache; `resource_parsing_api.md` covers `Resource` and SAU/SOJ/SAI/STX loaders and parsers; `symbian3rd_logging.md` covers `s3dLog`, release `NullLog`, and configurable Linux logging; `symbian3rd_io.md` covers file IO, wide formatting, and diagnostic-string references. `s2dsdk_correlation.md` separates confirmed Soft2D lineage from SDK details that Soft3D changed. `sdt_format.md` documents the UI string/resource and dialog-state SDT tables and the initial handedness screen. `sbp_format.md`, `s3dsty_format.md`, `spl_format.md`, and `s3dtex_format.md` document map chunks, lightstyles, camera paths, and legacy S2D images. `engine_completion_plan.md` inventories missing engine facilities and orders their reconstruction into tested stages. `symbian3rd_relocations.md` documents E32 code-relocation encoding, address mapping, and the relocation-backed recovery of functions and RTTI. Be sure to read them, but be wary that it can contain inconsistencies or be incomplete. Upon discovering new or misunderstood facts during research, correct any inconsistencies there. If none of existing .md files suits well (such as documenting new class that doesn't yet have documentation or covering a new research topic/domain), create a new .md file for it. |
| `Reversed_Game/` | CMake-based C++17 reconstruction. Contains shared constants and result codes in `s3dCommon.hpp`, `s3dBase`, `s3dIO`, `s3dLog`, `s3dFilePak`, `s3dResManager`, `Resource`, `AudioData`, `AudioDevice`, `AudioManager`, `SoundInstance`, `ObjectRes`, `Animation`, `TextureRes`, `Texture`, and `VID_impl`. `Engine`, `GameEngine`, `Renderable`, `Renderer`, `Raster`, `BasicShader`, `Object`, `Fixed`, `Font`, `VisString`, `StringManager`, `ScreenBlitter`, and `UiManager` implement the recovered lifecycle, font/string handling, SDT parsing, and software rendering path; scene/gameplay and later menu states remain explicit stubs. Audio includes SAU decoding, the 16-voice mixer, music/SFX/fade controls, and positional sound. Platform backends live in `src/platform/`: Linux file/stdio support, SDL3 video/audio backends in `sdl/`, and SDL-independent recording dummy video/audio backends in `dummy/`. Tests and a reference archive are in `tests/` and `src/resources/res.spk`. `src/app/` is the SDL3 entrypoint running the reconstructed `GameEngine` frame loop, portrait RGB565-to-XRGB presentation, and 16 kHz mono mixer output, with control/timer stubs. Priority preserving original observable behavior, confirmed in disassembly, including known bugs. |

### Tools/ in detail

- **Python 3 scripts** (single-file, run directly, no packaging): `decodesau.py` (SAU/WAR AMI ADPCM → WAV, with WAR nibble-order conversion), `nibbleswap.py` (IMA ADPCM → SAU nibble swap), `extractspk.py` (SPK archive extractor), `s3dtex.py` / `stx.py` (texture converters, require **Pillow**), `sbn.py` (font), `sbp.py` / `parse-sbp.py` (Quake-2-BSP-derived map format), `sojconv.py` (SOJ↔OBJ/MTL and SAI-sampled OBJ frame series), `sis_unpacker.py` (Symbian SIS), plus small helpers (`app_get_date.py`, `convert-inplace.py`, `hex_splitter.py`). `test_sojconv.py` is its standalone unittest suite, including real archive resources.
- `BspViewer/` — C# .NET Framework 4.5 / OpenTK 3.0.0 3D map viewer (Visual Studio `.csproj` + `packages.config`). Usage in `Tools/BspViewer/README.md`.
- `SimpleScriptDecompiler/` — C# .NET Framework 4.5 console decompiler for SST "simple script" files; supports `Win32` and `Mobile` script variants (`Script/Win32`, `Script/Mobile`, `Decompiler/*`).

## Build and test commands

There is no repository-wide build or CI. Build/run each research tool independently; `Reversed_Game` has its own CMake build and CTest suite:

- Reconstructed game: `cmake -S Reversed_Game -B Reversed_Game/build`, `cmake --build Reversed_Game/build --parallel`, and `ctest --test-dir Reversed_Game/build --output-on-failure`. The tests exercise synthetic SPK archives and resource files, cache lifecycle and directory normalization, real SAU/SOJ/SAI/STX/SBN/SDT resources from `Reversed_Game/src/resources/res.spk`, FilePak streaming and memory-resident modes, standalone IO writes/reopen/wide formatting, both decoder nibble orders, both logging backends, `VID_impl` lifecycle/error handling and exact dummy-presented pixels, engine queue ordering/capacity, portrait RGB565-to-XRGB presentation, complete engine-to-dummy-video frames, SOJ rasterization, packed material names, SAI interpolation and wrapping, real font drawing, both initial handedness selections, exact 16-voice mixing and positional/fade behavior, threaded audio lifecycle, real SAU playback, and SDL dummy-video/audio app smoke tests. The decoder defaults to SAU; use `-DSOFT3D_AUDIO_REVERSE_NIBBLE_ORDER=ON` for WAR's high-nibble-first variant. SPK defaults to the Chinese 64-byte-path layout; configure a separate directory with `-DSOFT3D_SPK_ENGLISH_LAYOUT=ON` for English 32-byte-path archives, and test that variant independently because real-archive tests require the Chinese fixture. Logging defaults to release-like `NULL`; use `-DSOFT3D_LOG_BACKEND=STDIO` to enable UTF-16LE `log.log` output, and test that variant in a separate build directory.
- SDL3 app: run `Reversed_Game/build/seven_days` after building. It requires SDL3 (`find_package(SDL3 CONFIG)`), runs `GameEngine::RunFrame` with a 320×240 CPU framebuffer, a 240×320 portrait XRGB8888 SDL streaming texture, and the 16 kHz mono software-mixer output through `SDL_AudioStream`, and falls back to SDL's software renderer. Options: `--renderer auto|software|NAME`, `--scale 1..4`, `--frames N`, `--resources PATH`, and `--no-audio`. Controls are arrows/WASD, Space/Enter, and Escape. Disable the app target with `-DSOFT3D_BUILD_APP=OFF`; the core library and dummy video/audio backend tests do not require SDL.
- Headless app validation: Xvfb, xdotool, xwd, and ImageMagick are available. Run the app with `DISPLAY` pointing to Xvfb, use `xdotool windowfocus` (there is no window manager), capture with `xwd`, and convert with `magick`. SDL's `dummy` video/audio drivers are useful for nonvisual smoke tests.
- Python tools: `python3 Tools/<script>.py ...` (Python 3; only `s3dtex.py` and `stx.py` need Pillow — there is no `requirements.txt`). Run the SOJ/OBJ/SAI converter suite with `python3 Tools/test_sojconv.py -v` from the repository root.
- C# tools: open the `.csproj` in Visual Studio (targets .NET Framework v4.5, NuGet restore via `packages.config`, OpenTK 3.0.0). `msbuild`/`dotnet build` may work on Linux with .NET Framework reference assemblies, but Windows/VS is the intended toolchain.
- Extractor: `qmake` + Qt Widgets, built with Qt Creator (`Extractor.pro`); generated Makefiles are checked in.

For research tools without automated tests, verification means running the tool on sample assets and comparing output against known-good results or original game behavior.

## Code style and conventions

- Documentation and code comments are in **English**; leaked sources/docs are Chinese (GBK). Keep new docs in English.
- Python style used across `Tools/`: `argparse` CLI (`description=`, positional `type=Path` args), `struct` with little-endian format strings (`"<..."`) for all binary I/O, `pathlib.Path`, plain functions + `print` output, no tests. Match the existing script you are editing rather than introducing frameworks.
- Binary formats are little-endian; SBP maps derive from the Quake 2 BSP format. 3D coordinates: -Z forward, Y up (see comment in `sojconv.py`).
- The Python scripts deliberately target the raw binary formats; when a format variant differs per platform (e.g. SBN 32-bit on GA330 vs 7-bit on Win32, SPK `--variant eng|chi`), handle the variant explicitly like the existing code does.
- Keep the README format table in sync when improving a format parser.

## Security and handling considerations

- `Releases/` and `Leaked_*` contain proprietary third-party binaries and documents preserved for research. Do not execute the Windows executables on sensitive systems, do not redistribute modified copies, and do not delete or "deduplicate" them.
- Do not commit large extracted game assets; `.gitignore` excludes `__pycache__/`, `Thumbs.db`, and `Reversed_Game/build/` — check `git status` before committing and leave generated outputs (extracted resources, decoded WAVs, build directories) out.

## Notes for Ghidra MCP Usage

- Ghidra will likely be run with inline scripting disabled, so for bulk changes and batch operations prefer running Python scripts that are directly calling into MCP server via REST API over using inline script tool.

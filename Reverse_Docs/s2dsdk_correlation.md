# Soft2D SDK and its relationship to Soft3D

## Scope and evidence rules

This analysis compares `Other/S2DSDK` with the Symbian 3rd `out.bin` in Ghidra (image base zero) and the existing notes in `Reverse_Docs`. The SDK is an earlier Dingoo A320 implementation. It is evidence for API concepts, layouts, and algorithms only when the ARM binary independently supports the match.

Three confidence levels are used below:

- **Exact/strong match**: identical constants, state transitions, record layouts, error codes, or algorithms are present in both implementations.
- **Adapted match**: the same subsystem and responsibilities remain, but Soft3D changes layout, channel counts, pixel format, platform backend, or serialized representation.
- **Conceptual match only**: names or general responsibilities survive without enough evidence to import SDK fields or algorithms.

No MIPS instruction is compared as ARM machine code. Correlation is based on public headers, demangled symbols, MIPS control flow, and independent ARM evidence.

## SDK inventory and binary formats

The SDK contains generated API documentation, public headers, a sample game, libraries, and three IDA databases:

- `sdk/lib/s2d.a` is a normal Unix `!<arch>` archive containing little-endian 32-bit MIPS ELF relocatable objects (`elf32-tradlittlemips`). `mipsel-linux-gnu-objdump` and `mipsel-linux-gnu-nm` read it directly. It has full symbols and relocations.
- `sdk/lib/s2d.lib` also begins with `!<arch>`. Its `./Debug/*.obj` members are not recognized as ELF by the installed MIPS binutils. The archive container is therefore standard, but the debug object format is different and should not be assumed to be ELF.
- `entry.a` is another MIPS archive.
- The `.obj.idb` files begin with `IDA1`, identifying older IDA database artifacts, not native Dingoo executables.
- `s2dcore_texture.o`, `s2dengine_engine.o`, `s2dengine_input.o`, `s2dengine_screenshader.o`, `s2dengine_stringres.o`, `s2dbase_audio.o`, and `s2dbase_video.o` were disassembled for this comparison.

Useful demangled SDK symbols include `s2d::Engine`, `s2d::Input`, `s2d::Texture`, `s2d::Draw2D`, `s2d::VID_impl`, `s2d::AudioPlayer`, `s2d::AudioData`, `s2d::AudioDevice`, `s2d::FadeShader`, `s2d::StringManager`, `s2d::Font`, and `s2d::VisString`. These names substantially overlap Soft3D RTTI, but overlap alone was not treated as proof of compatible behavior.

## Overall architectural relationship

The evidence supports Soft3D as a direct extension of Soft2D rather than an unrelated successor:

- engine startup/shutdown, frame presentation, input state handling, timing, and resource error codes retain the same architecture;
- texture storage and power-of-two helpers preserve concrete implementation details;
- ADPCM decoding shares tables and predictor/index logic;
- string and font classes preserve layouts and glyph-record semantics;
- `VID_impl` exists in both, with different platform backends;
- Soft3D adds `s3dBase`/`s3dIO` prefixes, `Resource`/`ResManager`, 3D model and animation resources, fixed-point 3D math, `Raster`/`Renderer`, and a 32-bit software framebuffer.

Soft3D is not merely a header-compatible newer S2D build. Several core details were deliberately changed, including key count, audio channel count, texture serialization, pixel width, fade implementation, and resource management.

## Shared result codes

`s2dbase_common.h` defines `s2d::result` as:

| Value | SDK name | Soft3D evidence |
|---:|---|---|
| 0 | `S2D_OK` | Successful loaders. |
| 1 | `S2D_MATH` | No conflicting use identified. |
| 2 | `S2D_PARAM` | Invalid parameter category. |
| 3 | `S2D_FILE` | SOJ/STX selection failures and other missing-file paths. |
| 4 | `S2D_MEMORY` | Allocation failures. |
| 5 | `S2D_ACCESS` | FilePak uses this for selection/read/seek failures. |
| 6 | `S2D_FORMAT` | SOJ object/subrecord parse errors. |
| 7 | `S2D_STATE` | Invalid render state category. |
| 8 | `S2D_UNKNOWN` | Size/tell failures and miscellaneous loader errors. |

This is an exact enum-level match across independently recovered Soft3D loaders. It also explains why SAU/SAI propagate FilePak status 5 while SOJ/STX translate selection failure to 3.

## Logging: exact compile-time design, adapted output type

S2D's `LOG_OUT` macro selects `Log::On`/`Log::Output` or an empty variadic `NullLog`. The Symbian binary contains both sides of the same design: functional `s3dLog::On`/`Output` methods and a 24-byte empty `NullLog` with 43 callers. This is an exact architectural match, not an inference based only on surviving strings.

The implementation changed from static narrow-character functions writing `log.s2dlog` to an `s3dBase`-derived object with embedded `s3dIO`, wide formatting, and UTF-16LE `log.log` output. The general resource/debug macro calls are disabled in the examined release, while one direct `s3dLog::Output` caller remains. See `symbian3rd_logging.md` for addresses, signatures, the formatting chain, and the configurable Linux reconstruction.

## Input: exact state machine, changed key layout

S2D uses these flags:

```text
KS_UP=0, KS_DOWN=1, KS_FIRSTDOWN=2, KS_PRESS=4, KS_CLICK=8
```

Both implementations clear first-down and click state with `state &= 0xf5` during update. Both set `DOWN|FIRSTDOWN` on a new down transition, set `PRESS` on repeat, and replace a released down state with `CLICK`.

Soft3D changes the concrete layout:

| Object | Layout |
|---|---|
| S2D `Input` | 20 key-state bytes, 20 repeat-counter bytes, any-key byte, three mouse-state bytes, mouse X/Y; total 52 (`0x34`) bytes. |
| Soft3D input | Any-key byte at `+0x00`, 24 key-state bytes at `+0x01`, 24 repeat-counter bytes at `+0x19`; no mouse fields; total 49 (`0x31`) bytes. |

Soft3D also adds event filtering: key zero is ignored, hardware events are gated by current input state, and keys `0x11..0x13` bypass that gate. Its repeat threshold is greater than five, unlike S2D's threshold of two. These differences must be preserved.

Confirmed ARM functions now named in Ghidra:

| Address | Function |
|---:|---|
| `0x000eec68` | `Input_Ctor` |
| `0x000eecec` | `Input_ClearKey` |
| `0x000eed30` | `Input_Update` |
| `0x000eeda4` | `Input_KeyDown` |
| `0x000eef64` | `Input_KeyPress` |
| `0x000ef004` | `Input_KeyUp` |
| `0x000ad658` | `TranslateKey` |

`TranslateKey` maps Symbian scan codes to the same abstract key positions used by the input arrays; it should not be replaced with Dingoo hardware mappings.

## Math and texture: some of the strongest evidence

### Exact power-of-two helpers

The SDK exports `GetPow2` and `ConvPow2` from `s2dmath_util.o`. Their algorithms match ARM functions exactly:

| Address | Function | Behavior |
|---:|---|---|
| `0x0012a0e4` | `GetPow2` | Returns the exact base-2 exponent, `0` for 1, and `-1` for a non-power-of-two input. |
| `0x0012a17c` | `ConvPow2` | Returns an existing power of two unchanged, otherwise returns the next higher power of two. |

`GetPow2` is used by texture creation, STX parsing, and fade-speed calculations in S2D. Soft3D uses it for texture width shifts and other size calculations.

### Texture layout lineage

S2D `Texture` stores 16-bit width/height, signed 16-bit power shift, format, palette count, byte depth, color-key flag/value, raw allocation, four-byte-aligned data, and palette. Its create/free routines:

- allocate `width * height * depth + 4` bytes;
- align the usable pointer with `(allocation + 3) & ~3`;
- release palette and data independently;
- return 4 on allocation failure;
- calculate width shift with `GetPow2`.

Soft3D repeats those behaviors. Its fields are widened/reorganized and prefixed by `s3dBase`, `s3dIO`, and `s3dLog`:

| Soft3D offset | Meaning |
|---:|---|
| `+0x28`, `+0x2c` | 32-bit width and height. |
| `+0x30`, `+0x34` | Added 16.16 fixed-point dimensions. |
| `+0x38` | Width shift (`GetPow2` result). |
| `+0x40` | Pixel byte count. |
| `+0x44` | Byte depth. |
| `+0x45`, `+0x46` | Color-key flag and byte-sized color key. |
| `+0x48`, `+0x4c` | Allocation base and four-byte-aligned pixels. |
| `+0x50`, `+0x54` | Palette count and palette allocation. |

The serialized formats differ. S2D's file loader uses narrower header fields and a 32-bit color key; Soft3D STX packs palette count/type/transparency into the first word and stores 32-bit width/height. Palette alignment and pixel-copy logic remain closely related. STX therefore must not be replaced with the older SDK header parser.

`Texture_Resize` is the Soft3D counterpart of S2D `Create`; `Texture_Reset` corresponds to `Free`. Existing Soft3D names accurately describe the observed behavior and are retained.

## Audio: shared decoder and lifecycle, changed format and scale

`AudioData::DecoderVer` in S2D and `DecodeSauAdpcm` in Soft3D use the same:

- eight-entry index-adjustment table `{-1,-1,-1,-1,2,4,6,8}`;
- 89-entry step table from 7 through 32767;
- predictor and step-index initialization to zero;
- `magnitude * step / 4 + step / 8` difference calculation;
- index clamp to `0..88` and signed-16-bit output saturation.

The tables were checked directly in the MIPS `.rodata` and ARM image and are identical.

The MIPS decoder consumes the **high nibble first**, then the low nibble, while the examined Soft3D SAU build consumes the **low nibble first**. WAR is not a separate audio encoding: it is the same ADPCM stream with high-nibble-first order. Early Dingoo A320, Symbian 9.1/S60v1 English, and Windows English builds use that ordering. Since the runtimes contain a fixed order and no runtime nibble swap, the engine most likely selected it with a compile-time switch, potentially for architecture-specific optimization. This should not be characterized as a Soft2D-to-Soft3D format change. The reconstruction exposes `SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER` for this purpose and defaults to SAU.

S2D `AudioPlayer` exposes `Play`, `Stop`, `StopAll`, `Pause`, `Resume`, channel repeat/volume, and master volume, with `MaxChannels = 8`. Soft3D expands its mixer to 16 voices, reserves voice 0 for music, and uses voices 1–15 for effects. It retains equivalent operations but adds fades, resource-manager integration, and distance attenuation.

Confirmed lifecycle names applied in Ghidra:

| Address | Function |
|---:|---|
| `0x000f3728` | `AudioManager_Pause` |
| `0x000f3750` | `AudioManager_Resume` |
| `0x000a6200` | `AudioDevice_Pause` |
| `0x000a6248` | `AudioDevice_Resume` |

The pause/resume functions conditionally change the platform stream state and update a flag, matching the S2D lifecycle rather than stopping every voice.

## Video and frame lifecycle

The exact RTTI name `VID_impl` occurs in both engines. S2D provides `vidinit`, `vidstart`, `vidstop`, `vidupdate`, `videxit`, and `vidptr`; its implementation owns an offscreen backbuffer and presents it to the Dingoo LCD. Soft3D replaces the backend with Symbian direct-screen access but retains the same start/stop/update responsibilities.

Soft3D `VidImpl_Update` (`0x000a6cf4`, formerly `VidImpl_CompleteFrame`) and `S3dVideo_Update` (`0x000a705c`, formerly `S3dVideo_CompleteFrame`) complete the direct-screen frame after software rendering. The new names align them with S2D `VID_impl::Update`/`vidupdate`; they do not imply identical platform code.

S2D `Engine::Start`, `Stop`, and `Resume` closely match Soft3D `Engine_Start`, `Engine_Suspend`, and `Engine_Resume`: start video, update running state, clear input on initial start, and pause/resume audio. Soft3D adds renderer/screen-descriptor rebinding.

S2D `Engine::Sync` sleeps for the unexpired frame interval and records timing. Soft3D `Engine_LimitFrameRate` performs the same core operation at the end of its expanded frame. S2D also calculates real FPS and synchronizes at frame start; Soft3D moves additional virtual handlers and audio/video work around that limiter, so `Engine_LimitFrameRate` remains the more precise name.

`GetCurTime` (`0x000a72ac`) and `Sleep` (`0x000a73a8`) are now named after their matching S2D API roles. The SDK describes timer values as microseconds, but this should not override platform-specific evidence about Symbian tick units.

## StringManager, Font, and VisString: exact structural match

### StringManager

S2D `StringManager` contains string count/list, font pointer, VRAM pointer, screen width/height, and language. Soft3D adds the `s3dBase` and `s3dIO` prefix (`0x14` bytes), after which the same sequence appears:

| Offset | Meaning |
|---:|---|
| `+0x14` | String count. |
| `+0x18` | String-pointer list. |
| `+0x1c` | `Font *`. |
| `+0x20` | Render-target pointer. |
| `+0x24`, `+0x28` | Render-target width and height. |
| `+0x2c` | Language. |

The constructor at `0x000de9ec` was previously called `BaseFont_ctor`. Its vtable is the `StringManager` RTTI vtable, and game initialization allocates `0x30` bytes, loads a font into it, and sets render properties. It has been corrected to **`StringManager_Ctor`**. Its destructors at `0x000dec2c` and `0x000dee04` were defined and named; `StringManager_Clear` is `0x000df1b0`.

### Font and glyph records

MIPS accessors establish S2D's layout and semantics. Soft3D has the same fields behind the `0x14`-byte prefix:

| S2D offset | Soft3D offset | Meaning |
|---:|---:|---|
| `+0x00` | `+0x14` | Maximum glyph height. |
| `+0x04` | `+0x18` | Base character count (`u16`). |
| `+0x08` | `+0x1c` | Base glyph-record array. |
| `+0x0c` | `+0x20` | Local character count (`u16`). |
| `+0x10` | `+0x24` | Local glyph-record array. |
| `+0x14` | `+0x28` | Language. |

Both use 12-byte glyph records with width at `+0x00`, height at `+0x04`, and glyph data at `+0x08`. Character code 3 returns the base array's first glyph width through a special path. Base/local lookup is selected by subtracting 4 from the character code and comparing against the base count.

The following ARM functions now have their SDK-validated names:

| Address | Function |
|---:|---|
| `0x000dc7f8` | `Font_Ctor` |
| `0x000dcbb4`, `0x000dcedc` | `Font_Dtor`, `Font_DeletingDtor` |
| `0x000dc57c` | `Font_GetMaxHeight` |
| `0x000dc450` | `Font_GetBaseCharCnt` |
| `0x000dc478` | `Font_GetLocalCharCnt` |
| `0x000dc4a0` | `Font_GetCharHeight` |
| `0x000dc5a4` | `Font_GetCharWidth` |
| `0x000dc688` | `Font_GetFontRes` |

`VisString_AppendText` (`0x000de3f8`) and `VisString_AppendNumber` (`0x000de72c`) preserve S2D's coded-character representation: lowercase starts at 4, uppercase at 30, digits at 56, and punctuation uses the remaining codes. This independently agrees with `basefont.sbn_info.txt`.

## FadeShader: conceptual lineage, incompatible implementation

S2D `FadeShader` has a 16-bit RGB565 backbuffer, power-of-two fade-speed shift, and two signed-three-channel interpolation buffers. Its render loop accumulates channel deltas and clamps/repacks RGB565 pixels.

Soft3D keeps the `BasicShader`/`FadeShader` names and software-fade responsibility, but its renderer works with 32-bit destinations and different state fields. It does not allocate or use the S2D interpolation buffers. Its public behavior is recovered directly from ARM:

| Address | Function |
|---:|---|
| `0x000e32f0` | `FadeShader_Ctor` |
| `0x000e3370` | `FadeShader_StartFade` |
| `0x000e3448` | `FadeShader_IsActive` |
| `0x000e3414` | `FadeShader_IsCompleted` |
| `0x000e347c` | `FadeShader_Complete` |
| `0x000e34d0` | `FadeShader_Render` |
| `0x000e4a88`, `0x000e4ae0` | `FadeShader_Dtor`, `FadeShader_DeletingDtor` |

The S2D fade fields and `GetPow2(fadeSpeed)` algorithm must **not** be imported into this Soft3D class.

## What should not be inferred from the SDK

- **Resource management**: S2D only forward-declares `TexManager` in the reviewed engine header and uses Dingoo DL resource routines. It does not establish Soft3D's four-level `ResManager`, SPK FilePak, SAU/SOJ/SAI/STX class hierarchy, or cache-node layout. Those remain ARM-derived.
- **3D rendering**: S2D `Draw2D` explains the software-blitting heritage but not Soft3D's `Raster`, `Renderer`, fixed-point triangle pipeline, SOJ/SAI data, or 32-bit framebuffer.
- **Blitter identities**: hundreds of SDK `DrawOL_*` symbols are not evidence for matching ARM function names without independent control-flow comparison. No wholesale renaming was performed.
- **Audio spatialization**: the reviewed S2D player does not establish Soft3D's 16-voice behavior or `285 - 3.5 * distance` attenuation.
- **Fade layouts**, key repeat threshold, STX header layout, and Symbian timer units differ or remain platform-dependent. SAU/WAR nibble order is a compile-time format variant, not evidence of a changed decoder generation.
- **Binary formats**: MIPS objects help with symbols and logic but cannot establish ARM addresses or instruction identity.

## Ghidra changes from this pass

Confirmed function names and parameter names were applied for input, `TranslateKey`, `GetPow2`, `ConvPow2`, timer functions, video update, audio pause/resume, FadeShader, StringManager, Font, and VisString. Previously undefined ARM destructors for FadeShader, StringManager, and Font were created. Layout bookmarks record the input, texture, string/font, and FadeShader findings; the report deliberately avoids assigning S2D-only types to incompatible Soft3D classes.

These annotations support the existing resource, audio, video, RTTI, and SBN documentation rather than replacing their ARM-derived conclusions.

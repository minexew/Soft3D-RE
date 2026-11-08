# Soft3D IO and wide formatting on Symbian 3rd

## Scope

`s3dIO` is the engine-level file wrapper around a separately allocated platform file object. This document supplements `filepak_api.md` and `symbian3rd_logging.md` with the recovered write, formatted-output, reopen, narrow-path, and wide-string formatting routines.

All addresses use the Ghidra `out.bin` image base of zero.

## Object and basic file API

The class is 12 bytes: an 8-byte `s3dBase` followed by a platform-file pointer at `+0x08`. The pointer is allocated by the open routines and released by close. The constructor does not initialize that pointer; normal use opens before closing.

| Address | Symbol | Behavior |
|---:|---|---|
| `0x0000f21c` | `S3dIO_Ctor` | Constructs `s3dBase` and installs the IO vtable. |
| `0x0000f004`, `0x0000f37c` | `S3dIO_Dtor`, `S3dIO_DeletingDtor` | Base destruction; no automatic platform-file close in the observed body. |
| `0x000ac628` | `OpenIoFile` | Parses a wide stdio-like mode and calls `OpenIoFileWithFlags`. |
| `0x000aad58` | `OpenIoFileWithFlags` | Allocates/opens the Symbian platform file and returns `1` on success, `0` on failure. |
| `0x000ab4a8` | `CloseIoFile` | Closes/deletes the platform object and clears the pointer. |
| `0x000ab600` | `SeekIoFile` | Seeks with the engine's origin convention. |
| `0x000ab69c` | `TellIoFile` | Returns the current position. |
| `0x000ab864` | `ReadIoFile` | Reads a byte count and returns the resulting descriptor length, or `-1` on read error. |
| `0x000ab7d0` | `ReadIoFileElements` | Reads `size * count` bytes with the same result convention. |
| `0x000ab8e4` | `S3dIO_Write` | Writes `size * count` bytes; returns `0` on success and `-1` on failure, not a byte count. |
| `0x000ac788` | `ReopenIoFile` | Calls close followed by wide open. No callers were found in this binary. |

The mode parser accepts `r`, `w`, `a`, `b`, `t`, and `+`; an unrecognized character makes open fail. It does not require the order used by libc.

### Narrow-path variants

Three additional routines form a narrow-character counterpart to wide open/reopen:

| Address | Symbol | Behavior |
|---:|---|---|
| `0x000ab308` | `OpenIoFileNarrow` | Parses narrow mode characters and forwards to the flags-based narrow open. |
| `0x000aacdc` | `OpenIoFileNarrowWithFlags` | Converts the narrow path to wide, calls `OpenIoFileWithFlags`, and frees the conversion. |
| `0x000ab464` | `ReopenIoFileNarrow` | Close followed by `OpenIoFileNarrow`. No callers were found. |

The narrow conversion allocates `(length + 1) * 4` bytes even though the wide output uses two-byte characters. This over-allocation is present in the binary and is not evidence of 32-bit wide characters.

The Linux reconstruction currently exposes the wide operations used by the engine and `s3dIO::Reopen`; the unused narrow public overloads remain documented but are not needed by reconstructed callers.

## Formatted output

### `S3dIO_Printf` and `S3dIO_VPrintf`

| Address | Symbol | Behavior |
|---:|---|---|
| `0x000aca80` | `S3dIO_Printf` | Starts variadic processing and forwards to `S3dIO_VPrintf`. |
| `0x000acee8` | `S3dIO_VPrintf` | Formats into a 258-element wide buffer; returns `-1` if formatted length is zero or negative, otherwise writes `length * 2` bytes through `S3dIO_Write`. |

An empty message therefore returns `-1` and writes nothing. These are the routines used by `s3dLog::On` and `s3dLog::Output`.

### General wide formatter

`FormatWideString` at `0x000acf60` performs formatting independently of file IO. `WideSprintf` at `0x000acca8` is its variadic wrapper and has 19 callers, including resource-path construction and other non-logging code. It should not be treated as a logger-only helper.

Recovered portable API:

```cpp
int FormatWideString(char16_t* output, const char16_t* format, va_list arguments);
int WideSprintf(char16_t* output, const char16_t* format, ...);
```

The output length excludes the null terminator. The original has fixed 258-character output/temporary buffers and no capacity argument; reconstructed callers must provide enough space, as original callers did.

Supported conversions:

| Conversion | Input | Raw output |
|---|---|---|
| `%s` | `wchar16*` / reconstructed `char16_t*` | Wide string. |
| `%S` | Narrow string | Bytewise widening. |
| `%c` | Integer-promoted character | One wide character. |
| `%d`, `%i` | Signed 32-bit integer | Decimal, including `-` for negative values. |
| `%u` | Unsigned 32-bit integer | Decimal. |
| `%o` | Unsigned 32-bit integer | Octal. |
| `%x`, `%X` | Unsigned 32-bit integer | Hexadecimal. |

The conversion helpers now named in Ghidra are `ConvertAsciiToWide` (`0x000975a4`), `SignedToWideString` (`0x00099d0c`), `UnsignedToWideString` (`0x00099dcc`), `HexToWideString` (`0x000aa1dc`), and `OctalToWideString` (`0x000aa2dc`).

### Format parser

`ParseWideFormatSpec` at `0x000aa3e4` accepts:

1. at most one flag from `#`, `+`, `-`, `0`, or space;
2. an optional decimal width;
3. an optional dot followed by decimal precision digits;
4. one conversion character.

Missing width/precision are represented by `-1`; a missing flag is `0xffff`. A bare dot has precision `-1`. There is no `*`, length modifier, or floating-point handling in this routine.

### Nonstandard field behavior that must be preserved

`ApplyWideFieldFormat` at `0x000aa6a4` is not equivalent to libc `printf`:

- Width padding uses spaces for `X`, `c`, `o`, `s`, and `x`, but zeroes for `d`.
- `i`, `u`, and uppercase `S` do not receive width/precision padding.
- The `-` and space flags have no observable role in the field-formatting body; there is no left-alignment implementation.
- `#` adds `0` for octal and `0x`/`0X` for hexadecimal.
- `+` adds a plus sign only for nonnegative `%d` output.
- Width and precision can truncate the returned output rather than merely setting a minimum field size.
- Precision adds padding when the raw input is shorter than requested. For example, `%.3s` with `x` produces two spaces followed by `x`.
- Hexadecimal output uses uppercase digits for both `%x` and `%X`.
- Hexadecimal and octal zero produce an empty raw string, unlike libc's `0`.
- `%d` zero-pads without a separate `0`-flag check; `%05d` is sufficient.

Examples confirmed by the reconstruction tests:

```text
%05d, 12       -> 00012
%+d, 12        -> +12
%#x, 42        -> 0x2A
%#X, 42        -> 0X2A
%#o, 8         -> 010
%x, 0          -> <empty>
%4s, x         -> "   x"
%2s, wide      -> wi
%.3s, x        -> "  x"
%-4S, yz       -> yz
%4u, 7         -> 7
```

The original `%c` path does not visibly append a terminator before field-length calculation, and the literal-percent loop has poorly behaved edge cases. The portable implementation terminates these paths safely; valid supported conversions retain the recovered formatting behavior.

## Linux implementation

`Reversed_Game/src/soft3d/s3dIO.cpp` now contains:

- `s3dIO::Write`, `Printf`, `VPrintf`, and `Reopen`;
- `FormatWideString` and `WideSprintf`;
- internal equivalents of the format parser, field formatter, and numeric/string converters.

`Reversed_Game/src/platform/linux/File.cpp` adds stdio-backed `File::Write`, returning `0` only when the complete requested byte count was written and `-1` otherwise.

`StdioLog` no longer duplicates a formatter or writes directly with `fopen`. It opens `log.log` through the supplied `s3dIO`, invokes `Printf`/`VPrintf`, and closes it. This makes the optional stdio logger exercise the same reconstructed IO path as the original.

## UTF-16 diagnostic references

The logging strings near the end of the image are referenced indirectly through literal-pool pointers. Following those references identifies the calling functions even when the release target is only `NullLog`.

Ghidra now has a **`Logging`** function tag on all 43 known `NullLog` callers plus the real log methods and direct output caller (46 functions total). **`LoggingString`** bookmarks identify the major string clusters and their callers:

| String/cluster address | Calling function(s) |
|---:|---|
| `0x0013ad70` | `FUN_0000e3d8` |
| `0x0013cc24..0x0013cccc` | `FUN_0002bd6c` |
| `0x0013cd5c..0x0013cec4` | `FUN_0002c3e0` |
| `0x0014156c` and adjacent initialization strings | `GameEngine_Init` |
| `0x001416e4`, `0x00141718` | `FUN_0006f9b8` |
| `0x00141774..0x001417c0` | `FUN_000702a0` |
| `0x001417f0`, `0x00141824` | `FUN_00070464` |
| `0x001427c0` | `LoadSplCameraResource` |
| `0x001464d8..0x00146588` | TGA loader `FUN_0009ebac` |
| `0x0014cd28` | `FUN_000a3438` |
| `0x0014dc50` | `VidImpl_GetBackBufferPtr` |
| `0x0015b4b0` cluster | Renderer functions `FUN_000b106c`, `FUN_000b1404`, `FUN_000b1c38`, and destructors |
| `0x0015bb10` | `Texture_Resize`, `ParseStxTexture` |
| `0x0015bb6c` cluster | `Texture_LoadStx`, `FUN_000d85d4` |
| `0x0015cac0`, `0x0015cae8` | `LoadSojResource`, `ParseSojModel` |
| `0x0015d6f0`, `0x0015d724`, `0x0015d750` | `S3dEngine_ctor`, `FUN_000f222c` |
| `0x0015d9c0` cluster | `LoadStrSdtResource`, `LoadStateSdtResource` |
| `0x0015e528..0x0015e690` | `LoadS3dStyleResource` |
| `0x0015e710..0x0015f4e4` | `LoadSbpMapChunks` |
| `0x0015f8f8`, `0x0015f924` | `LoadSauResource` |
| `0x0015fbf0..0x0015fc7c` | `LoadSaiResource` |
| `0x001a41b4..0x001a4270` | `LoadSstScript` |
| `0x001a42d4` | `FUN_00135268` |

This annotation is intended to prevent mistaking the retained strings for evidence of active release output while keeping them available for identifying logging call sites in later passes.

## Verification

`Reversed_Game/tests/test_s3dio.cpp` is separate from the FilePak and logger tests. It covers:

- raw writes and reads;
- element reads, seek, and tell;
- reopening;
- `Printf` and `VPrintf`, including empty-output status;
- every recovered conversion family;
- prefixes, padding, truncation, precision, uppercase hexadecimal, and zero-value behavior.

The complete suite passes in both the default null-logging and `SOFT3D_LOG_BACKEND=STDIO` configurations.

# Soft3D logging on Symbian 3rd

## Conclusion

The logging implementation is present in the Symbian 3rd binary. It is not accurate to describe the whole subsystem as optimized away:

- `s3dLog` has real, nonvirtual `On` and variadic `Output` methods which write `log.log` through the embedded `s3dIO` object.
- The usual debug-message macro path instead calls an empty variadic `NullLog` in this release. Its 43 call sites retain many useful format strings but produce no output.
- `s3dLog::On` has no callers in this binary. `s3dLog::Output` has one direct caller, at `0x00065660`, which is independent of the disabled `NullLog` path.

This closely matches the Soft2D SDK's compile-time `LOG_OUT` design, adapted from static narrow-character functions to an `s3dBase`-derived object with wide-character output.

## Soft2D SDK evidence

`Other/S2DSDK/sdk/inc/s2d/base/inc/s2dbase_log.h` declares:

```cpp
struct Log {
    static void On();
    static void Output(const Char* msg, ...);
};
```

With `LOG_OUT` enabled, `LOG_ON` calls `Log::On` and `LOG` calls `Log::Output`. With it disabled under GCC, `LOG_ON` expands to nothing and `LOG` maps to an empty inline variadic `NullLog`.

The SDK's MIPS `s2dbase_log.o` contains the enabled implementation:

- `Log::On` opens `log.s2dlog` with `w`, writes `===== Log On =====\n`, and closes it.
- `Log::Output` opens the same file with `a`, calls `vfprintf`, and closes it.
- Both use `CompleteFilenameWithAppPath`.

The SDK archive also contains weak out-of-line copies of `NullLog` in several other objects. That independently shows that disabled logging can remain visible as real empty calls rather than disappearing completely.

## Soft3D class and methods

`s3dLog` remains 20 bytes: an 8-byte `s3dBase` followed by a 12-byte `s3dIO`. Its RTTI vtable contains only the destructors and `SetEngine`; `On` and `Output` are nonvirtual and do not change the recovered layout.

| Address | Symbol | Recovered behavior |
|---:|---|---|
| `0x0000f1b4` | `S3dLog_Ctor` | Constructs the base and embedded IO member. |
| `0x0000f05c`, `0x0009b3e8` | `S3dLog_Dtor`, `S3dLog_DeletingDtor` | Destroy the IO member and base. |
| `0x0009b3ac` | `S3dLog_SetEngine` | Propagates the engine/context pointer to IO and base. |
| `0x0009b2a0` | `S3dLog_On` | Opens resolved `log.log` as `w`, writes `===== Log On =====\n`, and closes it if open succeeded. |
| `0x0009b31c` | `S3dLog_Output` | Opens resolved `log.log` as `a`, formats and writes one message, and closes it if open succeeded. |
| `0x0000f298` | `NullLog` | Empty variadic function; no output and no side effects. |

The Soft3D filename and mode strings are UTF-16 data at `0x00146468`, `0x00146478`, and `0x001464a4`; the header is at `0x0014647c`. The file data written by the original is UTF-16LE, without a BOM.

### Formatting and IO chain

The following functions were identified and annotated in Ghidra:

| Address | Symbol | Signature/behavior |
|---:|---|---|
| `0x000aca80` | `S3dIO_Printf` | Variadic forwarder to `S3dIO_VPrintf`. |
| `0x000acee8` | `S3dIO_VPrintf` | Formats into a 258-element wide buffer; returns `-1` for a nonpositive formatted length, otherwise returns the write result. |
| `0x000acf60` | `FormatWideString` | Wide formatter returning the number of output characters, excluding the terminator. |
| `0x000ab8e4` | `S3dIO_Write` | Writes `size * count` bytes and returns `0` on success or `-1` on failure, not a byte count. |

The formatter has explicit cases for:

- `%s`: UTF-16 string;
- `%S`: narrow string converted to wide characters;
- `%c`: character;
- `%d`, `%i`: signed decimal integer;
- `%u`: unsigned decimal integer;
- `%o`: octal integer;
- `%x`, `%X`: hexadecimal integer.

It delegates field formatting to `ApplyWideFieldFormat` (`0x000aa6a4`) and format parsing to `ParseWideFormatSpec` (`0x000aa3e4`). The original uses fixed 258-character buffers and does not show bounds checks. The general `WideSprintf` wrapper at `0x000acca8` also serves 19 non-logging callers. See `symbian3rd_io.md` for the complete formatting semantics and reconstruction.

`S3dLog_Output` does not return the helper result. Open failure silently drops the message. Each output call opens and closes the file rather than keeping a persistent stream.

## Disabled debug-call path

`NullLog` at `0x0000f298` is only 24 bytes. It saves the incoming argument registers and immediately returns, with no callee or data references. Its prototype is:

```cpp
void NullLog(wchar16* format, ...);
```

There are 43 callers, including engine startup, renderer/video code, resource loaders, and parsers. This explains why strings such as `s3d Engine launch start.\n`, allocation errors, and missing-file messages survive even though those calls do not write anything in this release.

This is stronger evidence than the debug strings alone: the common empty variadic callee and the exact Soft2D macro counterpart establish the disabled logging path.

## Messages restored in the reconstruction

The following observed calls are now represented by `S3D_LOG` in the corresponding C++ classes:

| Class/path | Message |
|---|---|
| `AudioData::Load` selection failure | ` open file error %s \n` |
| `AudioData::Load` either allocation failure | ` memory out %s \n` |
| `ObjectRes::Load` selection failure | `file %s not found!\n` |
| `ObjectRes::ParseSoj` object parse failure | `Object::LoadFromMemory: failed 1!\n` |
| `Animation::Load` selection/tell/allocation failure | `Load Animation %s: ...\n` variants |
| `Texture::LoadStx` selection failure | `Texture::CreateFromPak %s: can't find!\n` |
| `Texture::LoadStx` tell/allocation failure | Original `Texture::CreateFromFile %s: ...\n` wording retained |
| `Texture::ParseStx` pixel allocation failure | `Texture::Create: failed to alloc %d memory!\n`, with `pixelSize + 4` |

No new log messages were invented for FilePak or ResManager paths where the binary has no logging call.

## Linux reconstruction

Files:

- `Reversed_Game/src/soft3d/s3dLog.hpp/.cpp`: original object, `On`, `Output`, `NullLog`, `S3D_LOG_ON`, and `S3D_LOG`.
- `Reversed_Game/src/platform/linux/StdioLog.hpp/.cpp`: stdio backend adapter using the log object's `s3dIO`.
- `Reversed_Game/src/soft3d/s3dIO.cpp`: recovered write and wide-formatting implementation.
- `Reversed_Game/tests/test_s3dlog.cpp`: backend and integration tests; `test_s3dio.cpp` separately covers IO and formatting.

CMake selects the backend at compile time:

```sh
# Release-like default: macro logging uses NullLog and creates no file.
cmake -S Reversed_Game -B Reversed_Game/build

# Functional file logging.
cmake -S Reversed_Game -B Reversed_Game/build/stdio \
  -DSOFT3D_LOG_BACKEND=STDIO -DSOFT3D_BUILD_APP=OFF
```

`STDIO` creates `log.log` in the process working directory, writes the header with `w`, appends messages with `a`, and emits UTF-16LE. It supports the observed conversion set and field widths/precision. Symbian application-path resolution is replaced by the relative Linux path. The backend adds no field to `s3dLog`, so the recovered object layout remains unchanged.

The tests cover null behavior, initialization, append behavior, signed/unsigned/hex formatting, wide and narrow strings, character output, padding, and a real recovered `Texture::LoadStx` missing-entry path. Both default and `STDIO` suites pass.

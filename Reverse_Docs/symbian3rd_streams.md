# Symbian 3rd streams and generic state serialization

## Scope

The binary contains a small polymorphic stream hierarchy used by save files and by the verification/network code. Engine stage 12 reconstructs its portable, non-platform-specific behavior, generic unit and script state records, narrow-path IO, and standalone RAM diagnostics. Game-completion stage 13 builds the complete episode scene/slot schema on these primitives; see `symbian3rd_savegame.md`.

The RTTI hierarchy and native sizes are:

| Class | Base | Size | Additional fields |
|---|---|---:|---|
| `Stream` | — | `0x10` | vptr, owned buffer `+0x04`, stored size `+0x08`, cursor `+0x0c` |
| `InputStream` | `Stream` | `0x10` | none |
| `InputDataStream` | `InputStream` | `0x10` | none |
| `InputFileStream` | `InputDataStream` | `0x10` | none; the whole file is buffered during construction |
| `OutputStream` | `Stream` | `0x14` | capacity `+0x10` |
| `OutputDataStream` | `OutputStream` | `0x14` | none |
| `OutputFileStream` | `OutputDataStream` | `0x1c` | `s3dIO *+0x14`, open flag `+0x18` |

All vtables contain only ordinary and deleting destructors. Read/write/seek operations are nonvirtual.

## Stored-size and cursor rules

The implementation uses a trailing sentinel byte in its size accounting:

- `Stream_GetSize` (`0x0011773c`) returns `size - 1`, not `size`.
- `Stream_IsEnd` (`0x001177b8`) tests `position >= size - 1`.
- `Stream_IsValid` (`0x00117768`) returns true for a non-null buffer **or** `size == 0`. Empty and missing-file streams therefore report valid.
- `Stream_Seek` (`0x001177f8`) accepts only destinations `0 <= destination < size`. Invalid origins return `S3D_PARAM` (2); out-of-range seeks return `S3D_ACCESS` (5).
- Raw reads require `position + bytes < size`, otherwise they return `S3D_ACCESS` without advancing.
- Output writes require `position + bytes < capacity`, growing until it holds. After writing, stored size becomes at least `position + 1`.
- `OutputStream_Clear` resets size and cursor but retains allocation. Assignment/append helpers copy the other stream's **stored** size, including its sentinel.

The raw `Stream` constructor at `0x001174b8` stores the supplied byte count directly, making the last supplied byte a sentinel. `InputStream_SetData` (`0x00117a94`) instead stores `bytes + 1`, so all supplied bytes are readable. `InputFileStream` stores the file length directly, likewise making the last file byte a sentinel.

Typed integers are **big-endian**. Raw blocks and UTF-16 payloads retain native little-endian byte order. Fixed-point values use the same big-endian word representation as signed 32-bit integers. Vector helpers stop at the first error.

## Input methods

| Address | Function | Behavior |
|---:|---|---|
| `0x001179fc` | `InputStream_Read` | Raw block read with the sentinel bounds rule. |
| `0x00117c44` | `InputStream_ReadBool` | One byte, converted to zero/nonzero. |
| `0x00117cd8` | `InputStream_ReadByte` | One raw byte. |
| `0x00117d60` | `InputStream_ReadInt16BE` | Big-endian signed 16-bit word. |
| `0x00117e18` | `InputStream_ReadInt32BE` | Big-endian signed 32-bit word. |
| `0x00117f18` | `InputStream_ReadWideString` | Big-endian signed byte count followed by raw UTF-16 code units. |
| `0x0011800c` | `InputStream_ReadFixedBE` | Big-endian 16.16 fixed-point word. |
| `0x0011810c` | `InputStream_ReadVec2BE` | Two fixed-point words. |
| `0x00118190` | `InputStream_ReadVec3BE` | Three fixed-point words. |
| `0x00118244` | `InputStream_ReadVec4BE` | Four fixed-point words. |
| `0x00118328` | `InputStream_ReadBoundsBE` | Minimum and maximum vectors. |

The wide-string reader compares its byte count directly with the destination's **character** capacity, rejects negative lengths, and zeroes the entire destination before copying. It does not append a terminator as part of the serialized representation. The original fails to bounds-check the payload copy; the portable implementation returns `S3D_ACCESS` instead of reading beyond the buffer.

`InputFileStream_Ctor` (`0x0011851c`, duplicate `0x001183ac`) opens `rb`, obtains the length, allocates that many bytes, reads the file, and closes it immediately. A tell failure closes the file and leaves an empty stream. Open failure also leaves an empty stream, which `IsValid` reports as valid; `GetSize` is then `-1` and reads fail.

## Output methods

| Address | Function | Behavior |
|---:|---|---|
| `0x00118a2c` | `OutputStream_Write` | Raw block write with growth and sentinel-size maintenance. |
| `0x00118b08` | `OutputStream_Reserve` | New capacity is old capacity plus twice the requested count; if unchanged, add one. |
| `0x00118be4` | `OutputStream_Clear` | Reset size and cursor. |
| `0x00118c18` | `OutputStream_Assign` | Clear and write the other stored size. |
| `0x00118ccc` | `OutputStream_Append` | Set cursor to `size - 1` and write the other stored size. |
| `0x00118d18` | `OutputStream_WriteBool` | One boolean byte. |
| `0x00118de8` | `OutputStream_WriteByte` | One raw byte. |
| `0x00118eb8` | `OutputStream_WriteInt16BE` | Big-endian signed 16-bit word. |
| `0x00118fcc` | `OutputStream_WriteInt32BE` | Big-endian signed 32-bit word. |
| `0x00119150` | `OutputStream_WriteWideString` | Big-endian int16 UTF-16 byte count, then raw code units without a terminator. |
| `0x001191e8` | `OutputStream_WriteFixedBE` | Big-endian 16.16 fixed-point word. |
| `0x0011936c` | `OutputStream_WriteVec2BE` | Two fixed-point words. |
| `0x00119400` | `OutputStream_WriteVec3BE` | Three fixed-point words. |
| `0x001194cc` | `OutputStream_WriteVec4BE` | Four fixed-point words. |
| `0x001195d0` | `OutputStream_WriteBoundsBE` | Minimum and maximum vectors. |

Null wide-string input returns `S3D_PARAM`. The byte count is narrowed to signed 16 bits by the original; portable code rejects overlong strings instead of turning that into a negative block copy. Allocation growth is checked before replacing the old buffer, avoiding the original null-copy/leak-on-failure behavior.

`OutputFileStream_Ctor` (`0x001197c8`, duplicate `0x00119654`) opens `wb`, truncating an existing file. `OutputFileStream_Flush` (`0x00119cc4`) writes the buffer at its **stored size**, including the trailing sentinel. It returns `S3D_FILE` when no file is open and otherwise returns zero without checking the underlying write result. Destruction closes the owner but does not implicitly flush. The original sentinel contents are uninitialized; portable buffers are zero-initialized for deterministic output.

## `GameUnit` state

`GameUnit_SaveState` (`0x000267a0`) writes 18 big-endian words, 72 bytes:

1. unit type;
2. unit id;
3. visibility;
4. position vector;
5. local minimum and maximum;
6. collision minimum and maximum.

`GameUnit_LoadState` (`0x00026334`) starts at the id, not the type. The unit manager reads the type first to instantiate the appropriate derived class. Collision-enabled is neither saved nor loaded. Standalone callers must reproduce that type-first contract rather than calling the loader at offset zero.

`PhysicUnit`, `Things`, and `Creature` reuse `VisUnit` slots `+0x28/+0x2c`; they do not add independent base-state records.

## `VisUnit` state

`VisUnit_SaveState` (`0x000263b8`) first writes the 72-byte `GameUnit` record, then:

- cache level and object count;
- for a nonempty group, model filename/directory from object zero and an 88-byte record per object:
  - Z, X, Y axes;
  - depth bias and object intensity;
  - render mode;
  - frustum-culled, lighting-enabled, ambient-only, and external-transform bytes;
  - pause, time, direction, loop, and speed words;
  - material-zero minimum, maximum, current intensity, and intensity speed;
- shadow-enabled and shadow-transform-dirty bytes;
- shadow-object presence and, when present, its model filename;
- animation presence and, when present, animation filename/directory;
- animation-finished and positional-audio words.

An empty group therefore produces a 92-byte record including `GameUnit`, or 88 bytes after the separately consumed type word.

`VisUnit_LoadState` (`0x00025eac`) reloads models/materials and restores each record. After applying a shared animation, it restores the **last** object's five playback words to every object. This behavior is intentional in the reconstruction. The ARM routine passes the saved text pointers directly to the `SetAnimation` vtable slot, which expects an `Animation *`; portable code resolves the saved name/directory through `ResourceAnimation` instead of treating text as a pointer.

The native `Object` field at `+0x64` is render mode. The bytes at `+0x68`–`+0x6b` are frustum-culled, lighting-enabled, ambient-only, and external-transform-enabled; the external matrix pointer follows at `+0x6c`. The external-transform byte also disables material culling in the render path, so there is no separate native culling byte. The portable record preserves this exact order.

The corrected 0x48-byte `ObjectMaterial` layout places U/V at `+0x2c/+0x30`, minimum/maximum/current/speed at `+0x34/+0x38/+0x3c/+0x40`, and resource manager at `+0x44`. Earlier Ghidra layouts shifted the intensity names one word and omitted the manager.

## `SimpleScript` archive state

Script archives are a separate little-endian format, not the big-endian stream format. `SimpleScript_GetArchiveSize` (`0x00135804`) starts at `0x440`:

| Region | Serialized form |
|---|---|
| Script name | 32 UTF-16 code units, 64 bytes |
| Shared variables | 256 little-endian words, 1024 bytes |
| Scope-0 values | Aligned value records |
| Globals | Aligned value records |
| Each task | Four words followed by aligned local values |

A value record is a little-endian payload-size word followed by raw payload and zero padding, with total size `(payloadSize + 7) & ~3`. Numeric payloads are four bytes. String payloads contain UTF-16 code units including a terminator. The four task words are task state, current-command ordinal, called-task index, and called-command ordinal. Zero ordinals encode null pointers.

Scope 0 is the separate array at native `SimpleScript +0x34b4`; it is not the current function's locals. The portable reference resolver now reflects that distinction. Scope 1 is globals and scope 2 is dynamic current locals.

`SimpleScript_SaveArchive` (`0x00135560`) writes this layout directly through `s3dIO`. `SimpleScript_LoadArchive` (`0x00135268`) reloads the named SST from the resource archive before restoring values and ordinals, then enters state 4 (`Running`). The original ordinal lookup has a recursion bug; portable restoration performs a real tree search and rejects truncated, inconsistent, or trailing records instead of overrunning them.

`ScriptValue_WriteState`, `ReadState`, and `GetSerializedSize` are at `0x001371fc`, `0x001372a0`, and `0x001373a8`.

## Small utilities and diagnostics

- `OpenIoFileNarrow` (`0x000ab308`), `OpenIoFileNarrowWithFlags` (`0x000aacdc`), and `ReopenIoFileNarrow` (`0x000ab464`) now have public portable narrow-path overloads. Conversion widens each unsigned byte to one UTF-16 code unit through `ConvertAsciiToWide` (`0x000975a4`). The SDL application uses this shared helper instead of its own widening loop.
- `WriteIoFile` (`0x000ab974`) writes a raw byte count and returns 0 on success and -1 on failure, not a byte count.
- `NarrowStringLength` and `WideStringLength` are at `0x00097680` and `0x00098b7c`; `ResolveGamePath` is at `0x000ab068`; `FillMemory` is at `0x000a76dc`.
- `MemTotal` (`0x000a77ec`) and `MemRemain` (`0x000a7840`) remain standalone diagnostics, not methods of the empty `s3dMemInfo` shell. Linux uses `sysinfo` and returns total/free bytes or -1. The portable signature uses 64-bit values to avoid overflow on modern hosts; the original returns signed 32-bit values.

## Reconstruction and verification

`Reversed_Game/src/soft3d/Stream.*` implements all seven classes and vector/fixed helpers. `Units.*` implements the generic state slots, `SimpleScript.*` implements archive sizing and file round trips, `s3dIO.*` adds narrow overloads and shared widening, and `platform/linux/Memory.cpp` plus `s3dMemory.hpp` implement RAM queries. The SDL app no longer duplicates that widening logic. Game stage 13 adds a sticky portable read-error query for checked scene reconstruction without changing native size/cursor semantics.

`tests/test_stream.cpp` checks exact typed bytes, sentinel and EOF behavior, seeking, growth/copying, file flush/reopen/truncation/error paths, narrow IO, RAM diagnostics, `GameUnit` and empty/real `VisUnit` state, lifecycle persistence, quit, and fresh-engine restoration. The real fixture uses `cg_bully_tuomao_a` and `cg_bully_cc_aa`. `tests/test_script.cpp` checks real SST archive round trips, exact second serialization, typed globals/locals, shared words and task state, and truncated/missing archives.

At engine stage 12, all 26 normal CTest tests, 20 stdio-logging tests, and 20 SDL-independent ASan/UBSan tests passed. After game stage 14, the current totals are 38 default tests and 31 tests in each SDL-independent STDIO/host-math/WAR/ASan-UBSan build, all passing serially. Ghidra contains saved stream structures, 82 engine-stage-12 function annotations, corrected state comments, and the 0x48-byte `ObjectMaterial` layout; game-stage-13 save functions and stage-14 license functions are annotated separately.

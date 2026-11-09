# Symbian `s3dFilePak` and SPK archive API

This document describes the FilePak implementation in `SymbianBin/out.bin` and the 64-byte-path SPK variant used by `SymbianBin/res.spk`. Addresses refer to the existing Ghidra program with image base 0; they have not been rebased using the E32Image header suggestion.

The class RTTI name is `s3dFilePak` (`0x0014662c`). Its primary vtable address point is at `0x00146620`. The class layout, method names, prototypes, and helper identities described below have been applied in Ghidra.

## SPK file layout

All integer fields are little-endian. The archive begins with a 16-bit entry count, followed immediately by fixed-size entry records:

```c
struct PakEntry {             // 0x44 bytes
    char     name[64];        // NUL-terminated/padded ASCII path
    uint32_t dataOffset;      // absolute offset from the beginning of the archive
};

struct PakArchive {
    int16_t  entryCount;
    PakEntry entries[entryCount];
    uint8_t  data[];
};
```

There is no stored entry size. The implementation derives it from the next entry's `dataOffset`, or from the archive size for the final entry:

```text
entrySize[i] = entries[i + 1].dataOffset - entries[i].dataOffset
entrySize[last] = archiveSize - entries[last].dataOffset
```

Consequently, lookup requires entries to be sorted by name, while size calculation expects payload offsets to be nondecreasing. Name comparison is a case-sensitive, bytewise comparison; it is not case-insensitive.

Validation of `SymbianBin/res.spk` confirms:

- archive size: 51,984,563 bytes;
- entry count: 3,322;
- record size: `0x44` bytes;
- entry table ends at offset 225,898, which is also the first payload offset;
- names are sorted by their raw byte values;
- payload offsets are nondecreasing and within the archive.

The entry paths in this archive look like `.\audio\7DaysPiano03.sau`.

## English and Chinese entry layouts

`Tools/extractspk.py` documents a second, smaller record layout used for English archives:

| Variant | Path area | Offset | Record size | C++ constants |
|---|---:|---:|---:|---|
| Chinese (default) | 64 bytes | 4 bytes | `0x44` (68) | `pakPathBytes = 64`, `pakEntryBytes = 68` |
| English | 32 bytes | 4 bytes | `0x24` (36) | `pakPathBytes = 32`, `pakEntryBytes = 36` |

The entry count, little-endian absolute payload offsets, binary-search lookup, and payload-size derivation are shared. There is no runtime format marker or auto-detection in the original parser.

The Python code's extra `fo.read(1)` for Chinese archives is not evidence of a separate binary prefix field. In the real archive the first path byte is `'.'`; skipping it removes the root marker before the C-string read. The subsequent padding calculations (`31 - length` for English and `62 - length` for Chinese) produce the 32- and 64-byte path areas respectively.

No English archive is currently available for direct comparison. The layout difference may reflect packaging-tool versions or a need for longer paths, but a language/encoding-specific reason is **not established** by the code. The examined Symbian 3rd binary only implements the 68-byte Chinese layout.

The reconstruction retains Chinese layout by default. English support is selected at configure time:

```sh
cmake -S Reversed_Game -B Reversed_Game/build/eng-spk \
    -DSOFT3D_SPK_ENGLISH_LAYOUT=ON -DSOFT3D_BUILD_APP=OFF
cmake --build Reversed_Game/build/eng-spk --parallel
ctest --test-dir Reversed_Game/build/eng-spk --output-on-failure
```

`SOFT3D_SPK_ENGLISH_LAYOUT` is a public compile definition for the `soft3d` target and its tests. The reader uses the shared constants for table allocation, record traversal, offset access, and name conversion instead of hardcoded `0x44`/`64` values. Tests generate the active layout and verify both streaming and memory-resident reads. Real Chinese archive tests are explicitly skipped in the English configuration.

## Class layout

`s3dFilePak` inherits directly from `s3dBase`. It also owns an `s3dIO` file member at `+0x08` and an `s3dLog` member at `+0x34`; these are members, not additional RTTI bases. The unknown word formerly recorded at `+0x10` is the `s3dIO` platform-file pointer.

| Offset | Size | Type/name | Meaning |
|---:|---:|---|---|
| `+0x00` | 8 | `s3dBase base` | Base vptr at `+0x00`; engine/context pointer at `+0x04`, initialized to null. |
| `+0x08` | 12 | `s3dIO file` | Base vptr/engine at `+0x08`/`+0x0c`; platform-file pointer at `+0x10`. |
| `+0x14` | 4 | `archiveSize` | Obtained by seeking to archive end and telling the position. |
| `+0x18` | 4 | `allocationBase` | Allocated buffer base; also the base used for entry-address arithmetic. |
| `+0x1c` | 2 | `entryCount` | Read as a signed 16-bit value. |
| `+0x1e` | 2 | `selectedEntryIndex` | Selected table index; `-1` means no selection. |
| `+0x20` | 4 | `entryTable` | Points to the first `PakEntry`, not to the two-byte count. |
| `+0x24` | 4 | `selectedEntryStart` | Start of the selected payload relative to `allocationBase`. |
| `+0x28` | 4 | `selectedEntryEnd` | End of the selected payload relative to `allocationBase`. |
| `+0x2c` | 4 | `currentPosition` | Current selected-entry position relative to `allocationBase`. |
| `+0x30` | 4 | `memoryResident` | Zero selects streaming mode; nonzero selects whole-file memory mode. |
| `+0x34` | 20 | `s3dLog log` | Base vptr/engine at `+0x34`/`+0x38`; embedded 12-byte `s3dIO` at `+0x3c`. |

The total class size is **`0x48` bytes**. This is confirmed independently by the resource-manager allocation: its constructor allocates `0x70` bytes and places the FilePak object at manager offset `+0x28`, leaving exactly `0x48` bytes. `s3dBase` is therefore 8 bytes, `s3dIO` 12 bytes, and `s3dLog` 20 bytes.

The `s3dIO` member is a Soft3D wrapper, not a raw Symbian `RFile`. Its platform pointer refers to a separately allocated 12-byte Symbian file object whose file operations act on the handle at object offset `+0x04`. The Linux reconstruction uses a stdio-backed counterpart in `Reversed_Game/src/platform/linux/File.*`.

### Meaning of `allocationBase` in each mode

The pointer-like fields have deliberately mode-dependent behavior:

- **Streaming mode (`memoryResident == 0`)**: `allocationBase` points only to the allocated entry table. `selectedEntryStart`, `selectedEntryEnd`, and `currentPosition` are base-biased file offsets, not directly dereferenceable payload pointers. A disk read seeks to `currentPosition - allocationBase`.
- **Memory-resident mode (`memoryResident != 0`)**: `allocationBase` points to a buffer containing the entire archive. The selected-entry fields are real pointers into that buffer, and reads use memory copying.

## API functions

| Address | Ghidra function | Behavior |
|---:|---|---|
| `0x000317c8` | `GetFilePakFromManager` | Returns `manager + 0x28`. |
| `0x00071b5c` | `OpenGameResourceArchive` | Calls `s3dFilePak::Open(manager + 0x28, fileName, 0)`, selecting streaming mode. |
| `0x0009f8bc` | `S3dFilePak_Ctor` | Constructs the base and members; sets `allocationBase = 0`, `entryCount = 0`, and `selectedEntryIndex = -1`. |
| `0x0009fa38` | `S3dFilePak_Dtor` | Calls `Close`, then destroys the log, IO member, and base. |
| `0x0009fb08` | `S3dFilePak_DeletingDtor` | Destroys the object and conditionally releases its memory. Now defined as a function in Ghidra. |
| `0x0009fbd8` | `S3dFilePak_Open` | Opens an archive and initializes its entry table and mode-specific state. |
| `0x0009fdf8` | `S3dFilePak_Close` | Frees the allocation, closes the file, and resets selection/count state. |
| `0x0009fe60` | `S3dFilePak_Select` | Finds an entry by name using binary search and initializes its bounds and position. |
| `0x000a00e8` | `S3dFilePak_Seek` | Moves the current position within the selected entry. |
| `0x000a01f0` | `S3dFilePak_Tell` | Returns the byte position relative to selected-entry start. |
| `0x000a0244` | `S3dFilePak_Read` | Reads from the selected entry and advances its position. |
| `0x000a0348` | `S3dFilePak_GetEntryCount` | Returns the sign-extended `entryCount`. |
| `0x000a0370` | `S3dFilePak_SetEngine` | Propagates the engine/context pointer to `file`, this object, and `log` (including its embedded IO member). |

## Opening an archive

Approximate signature:

```c
int OpenFilePakArchive(s3dFilePak *filePak,
                       uint16_t *fileName,
                       uint32_t loadIntoMemory);
```

The routine performs the following steps:

1. Frees an existing `allocationBase`, if present, and clears it.
2. Copies the UTF-16 filename and resolves/normalizes the path through the platform file helpers.
3. Opens the file using mode string `rb`.
4. Seeks to the end, stores the resulting position in `archiveSize`, and seeks back to the beginning.
5. Stores `loadIntoMemory` in `memoryResident`.
6. Initializes the archive using one of two paths.

Streaming path:

```c
read(&filePak->entryCount, 2, 1);
tableBytes = filePak->entryCount * 0x44;
filePak->allocationBase = allocate(tableBytes);
read(filePak->allocationBase, tableBytes);
filePak->entryTable = filePak->allocationBase;
// The file remains open.
```

Memory-resident path:

```c
filePak->allocationBase = allocate(filePak->archiveSize);
if (filePak->allocationBase == NULL) {
    close(file);
    return 4;
}
read(filePak->allocationBase, filePak->archiveSize);
close(file);
filePak->entryCount = *(int16_t *)filePak->allocationBase;
filePak->entryTable = filePak->allocationBase + 2;
```

On either successful path, `selectedEntryIndex` is set to `-1` and the function returns zero.

Open returns:

| Status | Meaning |
|---:|---|
| `0` | Success. |
| `3` | File open failed. |
| `4` | Whole-file allocation failed in memory-resident mode. |
| `8` | Obtaining the archive size returned `-1`. |

The streaming allocation and most read results are not checked by this routine.

## Closing an archive

If `allocationBase` is non-null, `CloseFilePakArchive` frees it, sets it to null, and closes the file subobject. It then sets:

```c
selectedEntryIndex = -1;
entryCount = 0;
```

Other fields, including `archiveSize`, `entryTable`, and `memoryResident`, are not cleared. The file close is conditional on `allocationBase` being non-null.

## Selecting an entry

Approximate signature:

```c
int SelectFilePakEntry(s3dFilePak *filePak, uint16_t *entryName);
```

Selection fails with status `5` if `allocationBase` is null. Otherwise it:

1. Resets `selectedEntryIndex` to `-1`.
2. Converts the UTF-16 name to ASCII by taking the low byte of each 16-bit character.
3. Performs a binary search over `entryTable` using case-sensitive bytewise string comparison.
4. On a match, stores the table index and computes:

```c
selectedEntryStart = allocationBase + entry->dataOffset;
selectedEntryEnd = allocationBase + nextEntry->dataOffset;
// For the final entry:
selectedEntryEnd = allocationBase + archiveSize;
currentPosition = selectedEntryStart;
```

Success returns `0`; no match returns `5`.

## Seeking and telling

Approximate signature:

```c
int SeekFilePakEntry(s3dFilePak *filePak, int offset, int origin);
```

`origin` follows the usual C file conventions:

| Origin | New position before clamping |
|---:|---|
| `0` | `selectedEntryStart + offset` |
| `1` | `currentPosition + offset` |
| `2` | `selectedEntryEnd + offset` |

The result is clamped to the inclusive boundary range from `selectedEntryStart` to `selectedEntryEnd`. Thus the end position itself is permitted. Seek returns `5` when no entry is selected and `0` otherwise.

`TellFilePakEntry` returns `currentPosition - selectedEntryStart`, or `-1` when no entry is selected.

## Reading an entry

Approximate signature:

```c
int ReadFilePakEntry(s3dFilePak *filePak, void *destination, int requestedBytes);
```

Read returns `5` without a selected entry. Otherwise it chooses a byte count, reads the data, advances `currentPosition`, and returns `0`; it does not return the number of bytes read.

- Streaming mode seeks the open file to `currentPosition - allocationBase` and reads from disk.
- Memory-resident mode copies from `currentPosition` directly.

The boundary clamp in the machine code is exactly:

```c
bytesToRead = requestedBytes;
if (currentPosition + requestedBytes > selectedEntryEnd) {
    bytesToRead = selectedEntryEnd - currentPosition + 1;
}
```

The `+1` is confirmed in the disassembly. Since the end pointer otherwise behaves as an exclusive boundary and seeking permits `currentPosition == selectedEntryEnd`, this appears to be an original off-by-one bug. It should be treated as observed behavior, not corrected in documentation or reimplementations without further runtime evidence.

Underlying seek/read results are ignored, and `currentPosition` advances by the computed `bytesToRead`, not by an actual count returned from the file helper.

## Typical use

```text
GetFilePakFromManager
S3dFilePak_Open(..., loadIntoMemory = 0)
    S3dFilePak_Select(name)
        S3dFilePak_Tell / S3dFilePak_Seek / S3dFilePak_Read
S3dFilePak_Close
```

The normal game resource path uses streaming mode. Memory-resident support is present in the class, but no caller passing a nonzero `loadIntoMemory` value has been identified in this binary.

## C++ reconstruction and verification

The reconstructed classes live in `Reversed_Game/src/soft3d/`: `s3dBase.*`, `s3dIO.*`, `s3dLog.*`, and `s3dFilePak.*`. The Linux file counterpart is in `Reversed_Game/src/platform/linux/File.*`; it accepts the original UTF-16-style path/mode arguments, converts them bytewise, and maps backslashes to POSIX separators.

`Reversed_Game/tests/test_s3dfilepak.cpp` covers construction, open failures, selection, case-sensitive lookup, seeking/clamping, telling, reading, closing/reopening, both storage modes, selected layout constants/synthetic archive sizes, and the intentional boundary overread. The default build also reads the real 3,322-entry `Reversed_Game/src/resources/res.spk` in both modes. Build and run with:

```sh
cmake -S Reversed_Game -B Reversed_Game/build
cmake --build Reversed_Game/build --parallel
ctest --test-dir Reversed_Game/build --output-on-failure
```

The reconstructed archive is also the SAU resource source in the audio path documented by `symbian3rd_audio.md`; `symbian3rd_video.md` covers the corresponding software-rendering and presentation paths.

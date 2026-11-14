# Symbian 3rd E32 code relocations

## Encoding and address validation

The source is the code-relocation directory dumped in `Raw_Binaries/7days_Symbian3rd.log`. The E32 header fields are defined by `E32ImageHeader` in the SDK's `epoc32/include/f32image.h`; `iCodeRelocOffset` is a file offset in the uncompressed E32 image, while relocation target offsets are relative to the start of the code section.

For a dumped relocation with `address = A`:

```text
offset in Raw_Binaries/7days_Symbian3rd.bin = A
Ghidra address                               = A + 0x8000
```

`0x8000` is the E32 `iCodeBase` and the start of Ghidra's `ram` block. For example, relocation `A = 0x2a90` reads raw file offset `0x2a90`, describes Ghidra word `0x0000aa90`, and that word contains the linked pointer `0x0013a4f0`. Subtracting `0x8000` from `A` before indexing the raw file would be incorrect.

Each 16-bit entry uses its low 12 bits as the page offset and its high four bits as the relocation kind. This image has 5785 kind-1 (`text`) entries. Every target is word-aligned, unique, and contains a linked pointer value. Of those values, 5784 land in the linked image and one is the exact image-end sentinel `0x001c0b10`. None of the 5785 cells overlaps the 600 import cells in `symbian3rd_imports.tsv`.

## Blocks and zero entries

The directory contains 216 page-based blocks. Each dumped block size satisfies:

```text
block_size = 8 + 2 * entry_count
```

The sum of block sizes is 13532 bytes, matching the header's relocation-section size; the separate eight-byte `E32RelocSection` header is not included in that sum. Its real-relocation count is 5785.

A zero word is padding used when a block otherwise has an odd number of real relocations. There are 117 such words, all at the end of a block, and every block has an even total entry count. They are neither target relocations nor required terminators. The log's 5902 parsed entries comprise 5785 real relocations plus 117 padding words.

## Ghidra import and recovered information

All 5785 real relocation words are represented by typed data and references in Ghidra:

- 4212 undefined or untyped words were converted to pointers;
- 1573 existing pointer, RTTI record, and RTTI array units were preserved;
- three pre-existing AppUI pointer cells needed explicit references;
- verification found the expected reference for every one of the 5785 relocation sites.

The pointer graph exposes literal pools, switch/jump tables, callback tables, RTTI records, and vtables. It also provided function-entry evidence independent of direct call analysis:

- 218 functions were recovered from RTTI vtable slots and named `<Class>_vfunc_<slot>`, using the base-most owner when implementations are shared;
- 22 additional functions were recovered from other relocation-backed function pointers and retain neutral `FUN_*` names pending behavioral analysis;
- all 240 entries begin with valid ARM function prologues and were not contained in an existing function.

Twenty previously missing single-inheritance RTTI classes and vtables were recovered for `PlayerStateSet` states. All derive from `State<Player>`, have five virtual slots, and are listed in `symbian3rd_rtti.md`. Their addresses range from `0x0013d328` through `0x0013d588` in `0x20`-byte vtable strides. Their 80 state-specific implementations were named `<State>_vfunc_<slot>`, and the shared fourth-slot implementation was named `State_Player_vfunc_04`. This also proves a vptr for the otherwise layout-unknown `State<Player>` base.

Two superficially vtable-shaped runs at `0x00139c68` and `0x00140cb4` were rejected because their supposed type-info records did not contain valid type-name pointers. Relocation membership alone does not prove a vtable; the type-info and function-entry evidence must agree.

The inventory audit also found three encoded RTTI name strings whose bytes had been misdisassembled as instructions. Fixed-length character data now covers `0x001a4a88` (`CSymbian3rdApplication`), `0x001a5dac` (`CSymbian3rdAppUi`), and `0x001a5268` (`CSymbian3rdAppView`), preserving their existing `rtti_name_*` labels.

## Portable-source validation

The expanded vtable map was checked against `Reversed_Game/src/soft3d`. The destructor/`SetEngine` slots of `s3dBase` and its resource subclasses, the pure-virtual `Renderable::Render` and `Resource::Load` slots, and the shader/object interfaces agree with the portable hierarchy. Two interfaces required corrections:

- `Engine`: `Suspend` and `Resume` are virtual; `PrepareRender` precedes `SubmitRenderables` in the vtable; `UpdateFrame` and `RenderFrame` are nonvirtual helpers; and the final slot is `DrawFPS`, overridden as empty by `GameEngine`.
- `DialogManager` (portable `UiManager`): `Renderable` and `s3dBase` are separate primary/secondary bases, and the primary vtable contains `Render`, `Update(context)`, and `SetEngine`. The portable implementation now follows `Render -> Update(0)` and includes the secondary-base interface.

The normal CMake build and all 16 CTest tests pass after these corrections, including the real-archive UI tests and SDL smoke tests. Stage 5 subsequently added a portable `Dialog` runtime for every parsed dialog while retaining this manager interface.

# 7 Days: Salvation — Symbian entry-point analysis

## Binary layout

`out.bin` is the decompressed code section of the Symbian 9.3 E32Image. The import settings reported by `out.log` are:

```text
Language:   ARM:LE:32:v5t
Image base: 0x8000
Entry:      0x139BBC (file offset 0x131BBC)
```

Addresses map to file offsets as follows:

```text
virtual address = file offset + 0x8000
file offset     = virtual address - 0x8000
```

The E32 header entry point is therefore **0x139BBC**, at **file offset 0x131BBC**, executing in **ARM mode**.

## Startup call chain

| Address | File offset | Mode | Purpose |
|---|---:|---|---|
| `0x139BBC` | `0x131BBC` | ARM | E32 header entry/trampoline |
| `0x80010` | `0x10` | Thumb | Normal runtime startup |
| `0x138DF8` | `0x130DF8` | ARM | Runtime initialization and application dispatch |
| `0x1374B0` | `0x12F4B0` | ARM | `E32Main`-style application startup |
| `0x137444` | `0x12F444` | ARM | Application factory |
| `0x138304` | `0x130304` | ARM | `CSymbian3rdAppUi` constructor |

### Header entry and Thumb transition

At `0x139BBC`, the normal path is `r4 == 0`. The trampoline at `0x139BAC` calculates `0x80011` and executes `bx r12`:

```asm
139BAC: ldr r12, [pc, #4]
139BB0: add r12, r12, pc
139BB4: bx  r12
139BB8: .word 0xFFECE459
```

The low address bit switches execution to **Thumb**, so the actual startup code begins at `0x80010`.

The Thumb stub at `0x80010`:

1. Calls import stub `0x13A1D4` for runtime initialization.
2. If initialization succeeds, calls `0x139BA4`, a Thumb-to-ARM veneer leading to `0x138DF8`.
3. Calls import stub `0x139E9C` after the application returns.

The veneer uses `bx pc` to return to ARM mode, followed by an ARM branch at `0x139BA8` to `0x138DF8`.

### Runtime dispatch

`0x138DF8` performs additional runtime/static initialization. On the normal path it calls the `E32Main`-style application body at `0x1374B0`.

`0x1374B0` sets up runtime parameters and registers `0x137444` as the application factory.

## Application objects

### Application

The factory at `0x137444` allocates **560 bytes** and constructs an object at `0x137514`.

```text
Virtual table: 0x1A4A40
Class name:    CSymbian3rdApplication
```

Useful methods reached through this object:

- `0x1375D4` returns **0xE534CAFF**, matching the application UID in the original E32 header.
- `0x1375B0` branches to `0x138A64`, which creates the document object.

### Document

```text
Creation function: 0x138A64
Virtual table:     0x1ABE60
Class name:        CSymbian3rdDocument
```

The document method at `0x138D10` allocates **124 bytes** and calls `0x138304` to construct the application UI.

### Application UI

```text
Constructor:  0x138304
Virtual table: 0x1A5C60
Class name:   CSymbian3rdAppUi
```

This is the current boundary between the identified Symbian startup boilerplate and application-specific initialization.

## Recommended next steps in Ghidra

1. Import `out.bin` as a raw binary using `ARM:LE:32:v5t` and image base `0x8000`.
2. Create the entry point at `0x139BBC` and disassemble it in ARM mode.
3. Explicitly disassemble `0x80010` in Thumb mode; do not treat the entire image as ARM.
4. Label the functions in the startup chain above.
5. Examine `CSymbian3rdAppUi` at `0x138304` and the internal function pointers in its virtual table at `0x1A5C60`. Its construction and initialization methods are promising routes into game initialization.
6. Use `symbian3rd_imports.tsv` for the resolved import inventory. It is derived from the original E32Image import directory and the SDK's ARMv5 DSO exports. For each import cell, the low 16 bits are the real DLL ordinal and the high 16 bits are the address adjustment; an import-list index is not an ordinal. Actual `LDR PC, [PC, #-4]` veneers are four bytes before their cells.

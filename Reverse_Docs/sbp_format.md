# SBP maps and the `Bsp` engine object

## Scope and reference data

SBP is the Soft3D map format. It derives from Quake 2 BSP ideas but has its own chunk directory, serialized records, lightstyles, entity links, and runtime objects. The Symbian 3rd implementation is the RTTI class `Bsp`, with `Renderable` at object offset zero and `s3dBase` at `+0x18`.

Use `Reversed_Game/src/resources/res.spk` or the intact extraction under `Tools/outdir/` as reference data. `Tools/outdir/common/menu.sbp` is 75,548 bytes and begins with the expected signature. The copy at `Extractor/extracted_spkres/common/menu.sbp` has a different first byte and must not be treated as canonical.

`Tools/sbp.py` recognizes the signature and exports basic geometry, but its chunk names beyond the first five entries are incomplete or incorrect. The ARM loader call graph is the authority for the table below.

## Header and directory

```c
struct SbpChunkDescriptor {
    uint32_t offset;
    uint32_t length;
};

struct SbpHeader {
    char magic[4];                  // "S3D\0"
    float version;                  // 0x40f33333 = 7.6f
    float values[6];
    SbpChunkDescriptor chunks[27];
};                                  // 0xf8 bytes
```

The six header floats are converted to 16.16 fixed point. Their precise world/fog meaning remains to be established; they should not be assigned invented names. `LoadSbpMapHeader` (`0x0009639c`) reads only the `0xf8`-byte header for one map-preview path. The full loader is `LoadSbpMapChunks` (`0x0010c174`).

The loader selects the FilePak entry, reads the complete map into memory, validates `version` with a floating-point equality test, converts the six floats, and retains the allocation as the source for subsequent chunk loads. Errors use the established result codes: selection failure, 4 for allocation, 6 for format, and 8 for size/tell failure.

## Actual chunk order

| Index | ARM loader | Serialized record | Runtime behavior |
|---:|---|---:|---|
| 0 | `Bsp_LoadVertices` `0x0010e1e8` | 12 bytes | Three floats per vertex, converted to three fixed-point coordinates. |
| 1 | `Bsp_LoadEdges` `0x0010e3e8` | 4 bytes | Two 16-bit vertex indices. |
| 2 | `Bsp_LoadFaceEdges` `0x0010e514` | 4 bytes | Signed edge index; sign reverses edge direction in the geometry tool. |
| 3 | `Bsp_LoadPlanes` `0x0010e630` | 20 bytes | Four floats plus a word; expands to 24 bytes with plane flags. |
| 4 | `Bsp_LoadTexInfo` `0x0010e948` | 48 bytes | Eight floats and a packed 5-bit texture name; links a type-3 `TextureRes`. |
| 5 | `Bsp_LoadFaces` `0x0010ee30` | 24 bytes | Links plane, texinfo, face edges, lightmap data, and four style bytes; runtime record is 72 bytes. |
| 6 | `Bsp_LoadBrushes` `0x0010f160` | 4 bytes | Two 16-bit indices; runtime record is 8 bytes. |
| 7 | `Bsp_LoadBrushSides` `0x0010f298` | 4 bytes | Copied 32-bit entries. |
| 8 | `Bsp_LoadVisBrushes` `0x0010f378` | 4 bytes | First/count pair, expanded to 12 bytes and range-checked against faces. |
| 9 | `Bsp_LoadVisFaces` `0x0010f4f4` | 4 bytes | Face index, converted to a runtime-face pointer. |
| 10 | `Bsp_LoadVertexLights` `0x0010f664` | 1 byte | Expands each byte to a fixed-point light value. |
| 11 | `Bsp_LoadModels` `0x0010f838` | 88 bytes | Creates an `Object`, loads SOJ/SAI resources, transform fields, and optional vertex lights. |
| 12 | `Bsp_LoadEntityPairs` `0x0011008c` | 32 bytes | Copied into 32-bit runtime entries. |
| 13 | `Bsp_LoadEntityBrushes` `0x001101a4` | 12 bytes | Six 16-bit entries. |
| 14 | `Bsp_LoadEntityLinks` `0x00110760` | 4 bytes | Copied 32-bit entries. |
| 15 | `Bsp_LoadEntities` `0x00110344` | 16 bytes | Resolves model, pair, and brush links, creates renderable entities, then releases temporary records. |
| 16 | `Bsp_LoadVisibleEntityIndices` `0x00110884` | 4 bytes | Resolves indices through the entity-pointer array. |
| 17 | `Bsp_LoadEntityDictionaryIndices` `0x001109ec` | 4 bytes | Resolves dictionary indices through the entity-pointer array. |
| 18 | `Bsp_LoadLeafBrushes` `0x00110b54` | 4 bytes | Resolves entries to brush pointers. |
| 19 | `Bsp_LoadLeafVisibleBrushes` `0x00110cb8` | 4 bytes | Resolves entries to visible-brush pointers. |
| 20 | `Bsp_LoadPathPairs` `0x00110e28` | 4 bytes | Two 16-bit entries. |
| 21 | `Bsp_LoadPathNodes` `0x00110f54` | 16 bytes | Three floats plus links; expands to 20 bytes. |
| 22 | `Bsp_LoadLeaves` `0x00111190` | 32 bytes | Expands to 56 bytes and resolves face/entity/brush references. |
| 23 | `Bsp_LoadNodes` `0x001115ac` | 24 bytes | Expands to 40 bytes and links child nodes or leaves. |
| 24 | `Bsp_LoadPVS` `0x0011192c` | bytes | Copies the potentially visible-set bitstream. |
| 25 | none in the current state machine | unknown | `menu.sbp` has a 378-byte block; its consumer remains to be found. |
| 26 | `Bsp_LoadLightmapData` `0x0010ed7c` | bytes | Copies raw lightmap data used by face loading. |

This corrects the old tool's identification of index 5 as `LIGHTMAP` and index 6 as `FACE`. Index 26 is lightmap data; index 5 is faces and index 6 is brushes.

## Incremental loading

`LoadSbpMapChunks(bsp, directory, fileName, &progress)` supports incremental loading. A null progress pointer runs the whole sequence. Most stages add four; nodes and PVS add two each; successful completion sets progress to 100. It then releases the complete-file allocation and temporary loading arrays.

The menu path `Scene_LoadMenuBackdrop` (`0x0002c268`) repeatedly calls:

```c
LoadSbpMapChunks(scene->bsp, L".\\common\\", L"menu.sbp", &progress);
```

until progress is 100. The in-game scene uses the same routine for numbered day maps, with a day1 fallback. No episode-specific scene behavior is required to implement or test the `Bsp` object itself.

`Bsp_FreeLastLevel` (`0x0010d6a8`) releases chunk-owned data and resets styles, `Bsp_FreeAll` (`0x0010ba60`) also releases persistent allocations, and `Bsp_ResetLightStyles` (`0x0010d568`) clears 256 eight-byte style records.

## Rendering and remaining research

The format is not merely a geometry container. A complete engine implementation needs:

- face construction and lightstyle modulation using `default.s3dsty`;
- BSP node/leaf traversal and PVS visibility;
- brush collision and entity/leaf visibility links;
- map models through the existing SOJ/SAI/Object path;
- material/texture-coordinate modes and the rasterizer's lightmap/blending paths;
- resolution of directory block 25 and the unusual `Bsp` render-vtable target at `0x000f0e20`, which currently lies inside a larger recovered function and must be validated before being treated as a normal function entry.

The portable implementation should keep serialized records separate from runtime records and use indices at the format boundary. The original converts many of them to native pointers; portable vectors plus stable indices are safer and easier to test, provided observable rendering/collision behavior is retained.

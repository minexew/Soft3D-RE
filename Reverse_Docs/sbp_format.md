# SBP maps and the `Bsp` engine object

## Scope and reference data

SBP is the Soft3D map format. It derives from Quake 2 BSP ideas but has its own chunk directory, serialized records, lightstyles, entity links, and runtime objects. The Symbian 3rd implementation is the RTTI class `Bsp`, with `Renderable` at object offset zero and `s3dBase` at `+0x18`.

Use `Reversed_Game/src/resources/res.spk` or the intact extraction under `Tools/outdir/` as reference data. `Tools/outdir/common/menu.sbp` is 75,548 bytes and begins with the expected signature. The copy at `Extractor/extracted_spkres/common/menu.sbp` has a different first byte and must not be treated as canonical.

Stage 1 of `engine_completion_plan.md` is implemented in `Reversed_Game/src/soft3d/Bsp.*`: the container, all active chunk readers, incremental loading, map models, entity-factory ownership, link resolution, and cleanup. Rendering, PVS evaluation, collision, and lightstyle playback remain later-stage work. `Tools/sbp.py` and BspViewer use the corrected chunk names below.

## Header and directory

```c
struct SbpChunkDescriptor {
    uint32_t offset;
    uint32_t length;
};

struct SbpHeader {
    char magic[4];                  // normally "S3D\0"
    float version;                  // 0x40f33333 = 7.6f
    float values[6];
    SbpChunkDescriptor chunks[27];
};                                  // 0xf8 bytes
```

The six header floats are converted to 16.16 fixed point using float multiplication by 65536 followed by truncation. Their precise world/fog meaning remains unestablished; they should not be assigned invented names. `LoadSbpMapHeader` (`0x0009639c`) reads only the `0xf8`-byte header for one map-preview path. The full loader is `LoadSbpMapChunks` (`0x0010c174`).

The loader selects the FilePak entry, reads the complete map into memory, checks `version` with a floating-point equality test, converts the six floats, and retains the allocation as the source for subsequent chunk loads. The full ARM loader does not validate the four-byte magic. Errors use the established result codes: selection failure, 4 for allocation, 6 for format, and 8 for size/tell failure. The portable loader additionally rejects short files and directory ranges outside the source allocation instead of permitting out-of-bounds reads.

## Actual chunk order

| Index | ARM loader | Serialized record | Runtime behavior |
|---:|---|---:|---|
| 0 | `Bsp_LoadVertices` `0x0010e1e8` | 12 bytes | Three floats per vertex, converted to three fixed-point coordinates. |
| 1 | `Bsp_LoadEdges` `0x0010e3e8` | 4 bytes | Two 16-bit vertex indices. |
| 2 | `Bsp_LoadFaceEdges` `0x0010e514` | 4 bytes | Signed edge index. A negative value selects the edge's second vertex; a nonnegative value selects its first. |
| 3 | `Bsp_LoadPlanes` `0x0010e630` | 20 bytes | Four floats plus a word; expands to 24 bytes with bits 0–2 marking negative normal components. |
| 4 | `Bsp_LoadTexInfo` `0x0010e948` | 48 bytes | Eight floats and a packed 5-bit texture name; links a type-3 `TextureRes`. |
| 5 | `Bsp_LoadFaces` `0x0010ee30` | 24 bytes | Links plane, texinfo, face edges, lightmap data, and four style bytes; runtime record is 72 bytes. |
| 6 | `Bsp_LoadBrushes` `0x0010f160` | 4 bytes | Two 16-bit first-side/count values; runtime record is 8 bytes. |
| 7 | `Bsp_LoadBrushSides` `0x0010f298` | 4 bytes | Copied 32-bit entries. Values can contain flags such as `0x00400000`; the whole word is not a plane index. |
| 8 | `Bsp_LoadVisBrushes` `0x0010f378` | 4 bytes | First-face/count pair, expanded to 12 bytes and range-checked against faces. |
| 9 | `Bsp_LoadVisFaces` `0x0010f4f4` | 4 bytes | Face index, converted to a runtime-face pointer in ARM and retained as an index in the portable code. |
| 10 | `Bsp_LoadVertexLights` `0x0010f664` | 1 byte | Expands each byte to a fixed-point light value. |
| 11 | `Bsp_LoadModels` `0x0010f838` | 88 bytes | Creates an `Object`, loads SOJ/SAI resources, transform fields, and optional vertex lights. |
| 12 | `Bsp_LoadEntityPairs` `0x0011008c` | 32 bytes | Two 16-byte character fields, retained as key/value pairs. |
| 13 | `Bsp_LoadEntityBrushes` `0x001101a4` | 12 bytes | Six 16-bit entries. |
| 14 | `Bsp_LoadEntities` `0x00110344` | 16 bytes | Resolves model, pair, link, and brush fields, creates host entities, then releases temporary records. |
| 15 | `Bsp_LoadEntityLinks` `0x00110760` | 4 bytes | Copied 32-bit entries. This reader runs immediately before the entity reader despite its directory index being one higher. |
| 16 | `Bsp_LoadVisibleEntityIndices` `0x00110884` | 4 bytes | Resolves indices through the entity-pointer array. |
| 17 | none | unused | Empty in `menu.sbp`; no reader exists in the current state machine. |
| 18 | `Bsp_LoadEntityDictionaryIndices` `0x001109ec` | 4 bytes | Resolves dictionary indices through the entity-pointer array. |
| 19 | `Bsp_LoadLeafBrushes` `0x00110b54` | 4 bytes | Resolves entries to brushes. |
| 20 | `Bsp_LoadLeafVisibleBrushes` `0x00110cb8` | 4 bytes | Resolves entries to visible brushes. |
| 21 | `Bsp_LoadPathPairs` `0x00110e28` | 4 bytes | Two 16-bit entries. |
| 22 | `Bsp_LoadPathNodes` `0x00110f54` | 16 bytes | Three floats plus links; expands to 20 bytes. |
| 23 | `Bsp_LoadLeaves` `0x00111190` | 32 bytes | Expands to 56 bytes and resolves face/entity/brush references. |
| 24 | `Bsp_LoadNodes` `0x001115ac` | 24 bytes | Expands to 40 bytes and links child nodes or leaves. |
| 25 | `Bsp_LoadPVS` `0x0011192c` | bytes | Copies the potentially visible-set data. The 378-byte block in `menu.sbp` belongs here. |
| 26 | `Bsp_LoadLightmapData` `0x0010ed7c` | bytes | Copies raw lightmap data used by face loading. |

The earlier documentation incorrectly placed entity links at 14, entities at 15, dictionary indices at 17, and PVS at 24, and treated 25 as unknown. The corrected assignments are established by the descriptor addresses at the ARM call sites, confirmed by BspViewer's existing `LumpTypes` and by successfully loading the real map.

## Important serialized records

### Face (24 bytes)

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `uint16_t` | Plane index. |
| `+0x02` | `uint16_t` | First face-edge index. |
| `+0x04` | `uint16_t` | Unused by the ARM face reader. |
| `+0x06` | `uint16_t` | Face-edge count. |
| `+0x08` | `int16_t` | Texinfo index; `-1` means no texinfo. |
| `+0x0a` | `int16_t` | Copied to the low 16 bits at runtime offset `+0x08`; its precise semantic name is not yet established. |
| `+0x0c` | `int16_t[2]` | Two values shifted left by four into runtime offsets `+0x2c` and `+0x30`. |
| `+0x10` | `int32_t` | Byte offset into lightmap chunk 26; `-1` means no lightmap. |
| `+0x14` | `uint8_t[4]` | Lightstyle indices. |

The old BspViewer reader incorrectly consumed the style bytes as the lightmap offset. Its geometry path also used the second endpoint for positive face edges; ARM and `Tools/sbp.py` use the first.

`Bsp_BuildFaceLightmapInfo` (`0x00111a4c`, previously unnamed) runs for faces with texinfo. For every selected face vertex it evaluates:

```text
u = fixed_dot(vertex, texinfo[0..2]) + texinfo[3]
v = fixed_dot(vertex, texinfo[4..6]) + texinfo[7]
```

The three products are summed in 64 bits before the single 16-bit shift. For each axis, minimum and maximum are divided by fixed-point 16 (`0x100000`), then rounded down and up respectively. Runtime face fields are:

- `+0x18/+0x1a`: minimum coordinates, multiplied by 16 and narrowed to 16 bits;
- `+0x1c/+0x1e`: `(maximum - minimum) * 16`, clamped to at least 16;
- `+0x20`: width rounded up to a power of two;
- `+0x22`: base-2 shift of that rounded width;
- `+0x24`: `(width / 16 + 1) * (height / 16 + 1)` lightmap sample count.

For example, real face 0 in `menu.sbp` produces minima `(-448, 112)`, size `(16, 64)`, rounded width 16, shift 4, and 10 samples.

### Vertex lights

For serialized byte `b`, the ARM conversion is exactly:

```text
light = (256 - b) * 0x1f0000
```

Thus 0 becomes `0x1f000000`, 128 becomes `0x0f800000`, and 255 becomes `0x001f0000`. This is not a conventional `b << 16` conversion. The global converted array is temporary and is released after successful loading; models that need it receive their own copied vertex buffer.

### Map model (88 bytes)

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | packed 5-bit name[16] | SOJ resource basename. |
| `+0x10` | packed 5-bit name[16] | SAI resource basename; a first word of `0x1f` means no animation. |
| `+0x20` | `float[12]` | Four 3D vectors converted to 16.16 fixed point and copied into `Object + 0x18` through `+0x44`. |
| `+0x50` | `uint32_t` | First global vertex-light index. |
| `+0x54` | `uint16_t` | Vertex-light count. |
| `+0x56` | `uint8_t` | Mode controlling optional vertex-light creation and object flags. |
| `+0x57` | `uint8_t` | Copied to `Object + 0x6b`. |

When mode is 1, count is nonzero, and global lights exist, the model receives a format-8, four-byte-stride vertex buffer containing the selected fixed-point values. Missing animation resources are nonfatal in the ARM reader. Model and texture resources use cache level 3.

### Entity (16 bytes)

```text
s16 type
s16 modelIndex                 // -1 means none
u16 firstLink
s16 linkCount
u16 firstPair
s16 pairCount
s32 entityBrushIndex           // -1 means none
```

If a target array is absent, the ARM reader clears the corresponding runtime link instead of resolving it. Otherwise it forms native pointers into link, pair, brush, and model arrays. It then invokes the callback at `Bsp + 0x2aae0` with the runtime record, callback context at `+0x2aae4`, and the record-array base. The host owns concrete entity behavior; the portable `BspEntityFactory` preserves this boundary without implementing game-specific entities.

### Leaves and nodes

A serialized leaf is 32 bytes: three signed 16-bit minima, three maxima, contents, signed cluster, then four 16-bit first/count pairs for visible faces, visible entities, brushes, and visible brushes. The runtime form resolves those arrays and writes the owning leaf into every referenced visible brush.

A serialized node is 24 bytes: a 32-bit plane index, two signed 16-bit child indices, three minima, three maxima, and a 16-bit first-face/count pair. A negative child encodes leaf `-child - 1`; a nonnegative child is another node. `Bsp_LinkNodeParents` (`0x001119e0`) recursively initializes node-parent links from node zero.

## Incremental loading and ownership

`LoadSbpMapChunks(bsp, directory, fileName, &progress)` supports incremental loading. A null progress pointer runs the whole sequence. The externally observed sequence is:

```text
0, 4, 8, ..., 92, 96, 98, 100
```

The header and vertices are loaded at the initial zero stage. Lightmap chunk 26 is read at progress 20 before faces at 24. Entity links are read at 60 and entities at 64. Nodes and PVS add two rather than four. Successful completion takes 26 calls with a nonnull progress pointer.

Completion releases the complete-file allocation, global vertex lights, entity links, and the ARM model-record array. It also sets the enabled byte to 1 for lightstyles 1 through 254. `Bsp_FreeLastLevel` (`0x0010d6a8`) releases chunk-owned data and sets each style to frame count 8, current frame 0, enabled 1. `Bsp_FreeAll` (`0x0010ba60`) additionally calls `Bsp_ResetLightStyles` (`0x0010d568`) and releases persistent allocations.

The ARM completion path frees the model-record array without deleting every `Object`; host entities normally retain model objects. The portable code instead keeps explicit `BspModel`/`Object` owners until `FreeLastLevel`, which also covers maps with no entities and avoids leaks. Serialized/runtime links use checked indices rather than unstable native pointers. Missing host factories and invalid references produce explicit errors instead of null calls or out-of-bounds accesses.

The menu path `Scene_LoadMenuBackdrop` (`0x0002c268`) repeatedly calls:

```c
LoadSbpMapChunks(scene->bsp, L".\\common\\", L"menu.sbp", &progress);
```

until progress is 100. No episode-specific scene behavior is required to test the `Bsp` object itself.

## Texture-directory behavior

`Bsp_LoadTexInfo` optionally tries the resource directory at `Bsp + 0x2acec` when the flag at `+0x2ace8` is set, then tries the default common directory. In ARM, the returned `TextureRes *` has `0x12c` added before the null comparison. A null resource therefore becomes nonnull address `0x12c`, so the fallback and error check are defective. The portable reader uses a real null check and safe fallback rather than fabricating that invalid pointer.

## PVS and render-vtable findings

Chunk 25 is definitively PVS, not an unknown block. In `menu.sbp` it begins with a 32-bit value 39 followed by increasing 32-bit offsets; evaluating that structure belongs with leaf/cluster visibility in Stage 2.

The primary Bsp vtable contains `0x000f0e20` in the apparent render slot. Direct disassembly shows this is the middle of `FUN_000f06e8`, an unrelated SOJ fallback-vertex builder: there is no function prologue and the instructions use the enclosing function's frame. It must not be created or named as `Bsp::Render`. The actual map-rendering path must be recovered from the Bsp rendering functions and their callers in Stage 2 rather than from this invalid target. The portable `Bsp::Render` remains an explicit non-rendering stub until then.

## Portable verification

`Reversed_Game/tests/test_bsp.cpp` covers:

- the real `menu.sbp` directly from `res.spk`, reaching 100 in exactly 26 calls;
- all real chunk counts, 421 vertices, 718 edges, 316 faces, 281 textures, five animated models, six factory-created entities, 178 leaves, 176 nodes, and the 378-byte PVS;
- exact fixed-point, vertex-light, model-transform, and first-face lightmap calculations;
- entity-factory lifetime, cleanup, malformed record sizes, out-of-file chunks, bad versions, invalid indices, and missing factories;
- the original acceptance of arbitrary magic with a valid version.

The full CTest suite passes. Running `Tools/sbp.py` on the real map exports the same 421 vertices and 316 polygons used by the portable checks.

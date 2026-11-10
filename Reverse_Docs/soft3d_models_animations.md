# Soft3D SOJ models and SAI animations

## Scope

This document describes the Symbian 3rd `out.bin` implementation and the matching portable code in `Reversed_Game/src/soft3d/`. It records corrections established by following `Object`, `Renderer`, and `Animation_SampleTransform`, not just the resource loaders. Integer coordinates and transforms are signed 16.16 fixed point unless stated otherwise.

## SOJ top-level layout

All serialized words are little-endian.

```text
u32 materialCount
SojMaterial materials[materialCount]       // 52 bytes each
u32 surfaceCount
u32 unknown138
u32 unknown13c
SojSurface surfaces[surfaceCount]
s32 boundsMin[3]
s32 boundsMax[3]
```

The six trailer values are the model bounding box. `Object_Load` copies them, computes its center and extents, and uses them for visibility and depth work. They are not six unrelated values.

`ObjectRes` remains 0x15c bytes. Its material array is at `+0x130`, surface array at `+0x140`, and bounds at `+0x144`. If surface count is zero, the parser frees both arrays but returns success. It checks material parse errors and ignores surface parse return values. Serialized blocks advance by `byteSize & ~3`; this is not conventional round-up alignment.

## Materials (formerly “objects”)

A material record is 52 bytes serialized and 0x34 bytes internally. It describes render state, not a scene node.

| Serialized offset | Internal offset | Meaning |
|---:|---:|---|
| `+0x00` | `+0x00` | Likely fixed-point material intensity/brightness. Usually approximates the average RGB intensity, but authored exceptions exist. Not a UV scale. |
| `+0x04` | `+0x04` | Second unknown fixed-point material parameter. |
| `+0x08` | `+0x08` | RGB bytes packed to RGB565. |
| `+0x0b` | `+0x0c` | Blend mode byte. |
| `+0x0c` | `+0x10` | Texture selector; low 16 bits are retained. Selector `0x00ff` requests a detail texture. |
| `+0x10` | `+0x14` | Texture-present flag. |
| — | `+0x16` | Detail-texture flag derived from selector `0x00ff`. |
| `+0x14` | `+0x18` | Four packed 32-bit words containing the texture name. |
| `+0x24` | `+0x28` | Unknown fixed-point material parameter. |
| `+0x28` | `+0x2c` | Unknown fixed-point material parameter. |
| `+0x2c`, `+0x30` | `+0x30` | Texture-coordinate generation mode: first nonzero → 3; otherwise second nonzero → 4; both zero → 0. |

The last field was incorrectly called `primitiveType` in the earlier reconstruction. Topology belongs to the surface, not the material.

An archive scan supports the intensity interpretation of the first field: 850 of 883 parsed materials satisfy `unknown00 = gray * 257`, and many non-gray materials approximately satisfy `unknown00 = average(rgb) * 257`. Examples such as 38550 for RGB(150,150,150) and 34781 for RGB(155,134,117) fit this relationship. Other values, including 20303 for RGB(45,16,16) and 38535 for RGB(149,149,149), show that it is authored independently or transformed by an additional rule. It should not be used for UV normalization.

### Packed texture names

`DecodePacked5BitString` (`0x000dc31c`) reads at most 24 five-bit symbols from the four words. The alphabet is:

```text
abcdefghijklmnopqrstuvwxyz_1234
```

Code `0x1f` terminates the string. `ObjectMaterial_LoadTexture` resolves that name through `ResManager`. When the detail flag is set, it also resolves the name with `_alp` appended.

## Surfaces and primitive topology

Each surface starts with three 32-bit words:

| Field | Meaning |
|---|---|
| `materialIndex` | Signed index into the material array; `-1` skips material setup. |
| `primitiveType` | 4 = triangle list, 5 = triangle strip, 6 = triangle fan. |
| `primitiveCount` | Number of triangles. |

The derived index count is `primitiveCount * 3` for lists and `primitiveCount + 2` for strips/fans. Strip rendering alternates winding on odd triangles. Fan rendering keeps the first index and advances the other two.

### Vertex header correction

The four vertex-header words are:

```text
u32 format
u32 strideBytes
u32 vertexCount
u32 dataByteSize
u8 vertexData[dataByteSize]
```

The original parser internally changes `strideBytes` to `strideBytes / 4`, then multiplies it by four when addressing a vertex. This does not change the serialized meaning. Earlier notes and C++ incorrectly called `vertexCount` the byte size and ignored `dataByteSize`.

A real `cg_bully_tuomao_a.soj` surface confirms this directly: stride 36, vertex count 379, and data size 13644 = 36 × 379.

The index header is:

```text
u32 format
u32 dataByteSize
u8 indexData[dataByteSize]
```

The rendering accessor reads 16-bit indices. The index format word has not been shown to select another width in this binary.

### Vertex format bits

| Bit | Serialized stream, in order |
|---:|---|
| `0x01` | Position: three fixed-point words. |
| `0x02` | Normal: three fixed-point words. |
| `0x04` | UV: two fixed-point words. |
| `0x08` | Color: one 32-bit value; the raster path uses its RGB565 value. |
| `0x10` | Leading matrix-palette index, before all other streams. |

Bit `0x10` selects a 0x40-byte matrix for skinned transformation. Without it, vertices use the object/world transform. Position is transformed as:

```text
x = m[0]*x + m[4]*y + m[8]*z + m[12]
y = m[1]*x + m[5]*y + m[9]*z + m[13]
z = m[2]*x + m[6]*y + m[10]*z + m[14]
```

### Bone-local geometry is not a default pose

A SOJ with matrix-indexed vertices is not necessarily in a meaningful global pose when viewed without SAI. `Object_UpdateMatrixPalette` fills the palette from sampled SAI matrices, and `Renderer_TransformVertex` directly transforms the raw vertex by `palette[matrixIndex]`. There is no additional inverse-bind matrix, T-pose correction, or hidden default-pose transform in this path.

For example, `r_ken_a.soj` has 365 matrix-indexed vertices and `r_ken_run_a.sai` has 21 tracks. The raw per-bone vertex centers are clustered near the origin, so a raw `soj-to-obj` export looks collapsed or twisted. Sampling the SAI matrices distributes those vertices into a recognizable body. This is expected bone-local source geometry, not evidence of an exporter defect. Static models without matrix indices do not undergo this change.

Use `sai-to-obj` to obtain a meaningful posed mesh. Raw `soj-to-obj` output for a skinned model should be treated as a local-space/debug view, not as an intended character posture.

## SAI top-level layout

```text
s32 startTime
s32 endTime
u32 flags
Bounds bounds[flags & 0xff]                // 24 bytes each
s16 trackCount
s16 padding
SaiTrack tracks[trackCount]
optional morph section                    // flags bit 8
```

Each bounds record is two fixed-point 3D vectors. If exactly one is present, the parser duplicates it into the second internal bounds slot. These records are bounding volumes, not transform matrices. `Object_SetAnimation` uses them to replace the model bounds.

`startTime` and `endTime` are established by `Object_AdvanceAnimation` and both samplers. Forward playback completes at `endTime`; reverse playback completes at `startTime`. Loop mode wraps the excess to the opposite endpoint. Non-loop mode clamps. `Object_SetAnimationTime(-1)` selects `endTime`; a time above `endTime` selects `startTime`.

## Transform tracks and keyframes

Each track begins with a 32-bit keyframe count. A serialized keyframe is 36 bytes:

```text
s32 time
u16 signFlags
u16 rotationMagnitude[9]
s32 translation[3]
```

The nine magnitudes fill the 3×3 part of a 4×4 fixed-point matrix in this order:

```text
0, 1, 2, 4, 5, 6, 8, 9, 10
```

Sign bit N negates magnitude N. Matrix elements 3, 7, and 11 are zero; translation occupies 12, 13, and 14; element 15 is `0x10000`. The internal keyframe is therefore a 32-bit time plus a 0x40-byte matrix (0x44 bytes total).

`Animation_SampleTransform` (`0x00115f30`) returns an exact key on a time match. Otherwise it linearly interpolates rotation, translation, and scale between surrounding keys. The constant matrix elements 3, 7, and 11 come from the previous key. After the final key it interpolates from the final key toward the first using `endTime` as the interval endpoint.

The original reciprocal table contains `0x40000000 / interval`, shifted right by 14 before fixed-point interpolation. Its truncation is observable: a 100-unit interval uses reciprocal `0x28f`, and time 50 produces factor `0x7fee`, not an ideal `0x8000`.

## Morph section

When flags bit 8 is set:

```text
s16 targetCount                    // only 2 is accepted
s16 morphCount                     // present only when targetCount == 2
SaiMorph morphs[morphCount]
```

Each morph record contains a 32-bit time followed by `targetCount` fixed-point 3D vectors. The accepted runtime form has two targets. Any other target count is changed to zero and no records are consumed.

`Animation_SampleMorph` (`0x001166b4`) selects one target and interpolates its vector between time-keyed morph records using the same reciprocal-table rule and final-to-first interval convention.

## Portable reconstruction status

`ObjectRes.*` and `Animation.*` use the corrected names and layouts. `Object.*` implements material loading, animation timing, matrix-palette construction, and indexed surface submission. `Fixed.hpp` documents the matrix convention in code.

`Renderer.*` and `Raster.*` provide a functional fixed-point software path to `BasicShader` and `VID_impl`. They implement the verified vertex layouts and primitive modes, but do not yet reproduce every original clipping, lighting, perspective-correct texturing, blend, or specialized inner-loop case bit-for-bit. Those remain reconstruction work; the portable implementation must not be cited as proof for details that are only established in the disassembly.

## Converter and animation-frame export

`Tools/sojconv.py` supports all documented materials, surfaces, vertex streams, primitive modes, bounds, and packed texture names:

```sh
python3 Tools/sojconv.py soj-to-obj model.soj model.obj
python3 Tools/sojconv.py obj-to-soj model.obj model.soj
python3 Tools/sojconv.py sai-to-obj animation.sai model.soj frame \
    --start 0 --end 66000 --step 1000
```

SOJ export writes a sibling MTL by default; `--mtl` selects another path. Normalized OBJ UVs are now the default: the converter reads each texture's PNG dimensions from the model directory, or from `--texture-dir DIR`. Serialized UVs are fixed-point texel coordinates, so the normalized value is `raw_uv / (texture_size * 0x10000)` in floating-point OBJ terms. Without normalization, Blender interprets large texel values as repeated textures. `--raw-uv` explicitly selects the data-only raw representation when no PNG is available. The MTL records texture dimensions, and `# s3d_uv_raw` metadata preserves exact source coordinates for byte-exact reimport. `sai-to-obj` supports the same `--texture-dir` and `--raw-uv` options.

The engine samples STX rows directly from top to bottom, while Blender uses a bottom-left UV origin. Therefore normalized export also applies `v_obj = 1 - v_game` by default. `--no-flip-v` disables this conversion. `# s3d_v_origin bottom|top` records the choice; reimport inverts it when needed, then recovers fixed-point texel coordinates by multiplying normalized fixed-point values by texture dimensions. The real archive test uses actual STX dimensions and verifies byte-exact reimport when raw UV metadata is present.

OBJ and MTL custom comments/directives preserve unknown material fields, raw RGB8 colors, packed names, detail-texture selection, texture-coordinate modes, vertex colors, matrix-palette indices, per-vertex extra bytes, primitive types, index formats/tails, top-level unknown words, and bounds. Per-surface source-vertex maps preserve original ordering and unused vertices. Other OBJ readers may ignore these comments. New OBJ files without the metadata default to triangle lists, computed bounds, and a vertex format inferred from their streams.

SAI export samples the matrix palette at each requested time, transforms vertices with a leading matrix index, and writes numbered `prefix_00000000.obj` files. Sampling preserves exact-key handling and reciprocal-table truncation. The inclusive end time is emitted even when it is not on a step boundary. Morph-target vectors are written as `# s3d_morph` comments because their mapping to arbitrary OBJ geometry is not established; they are not blindly applied to every vertex.

Run `python3 Tools/test_sojconv.py -v` from the repository root. The six-test suite covers synthetic binary/OBJ round trips, all primitive modes and vertex fields, packed names, normalized UVs with negative V coordinates, SAI interpolation, and real SOJ/SAI resources from `Reversed_Game/src/resources/res.spk`. Real SOJ→OBJ/MTL→SOJ and direct binary serialization are verified byte-for-byte.

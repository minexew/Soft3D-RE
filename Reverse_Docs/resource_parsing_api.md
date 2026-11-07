# Soft3D resource objects and SAU/SOJ/SAI/STX parsing

## Shared `Resource` base

RTTI identifies `Resource` as a direct `s3dBase` base class. Constructor analysis establishes a size of **0x12c bytes**:

| Offset | Size | Meaning |
|---:|---:|---|
| `+0x00` | 8 | `s3dBase`. |
| `+0x08` | 12 | Embedded `s3dIO`. |
| `+0x14` | 20 | Embedded `s3dLog`. |
| `+0x28` | 128 | Full path/cache key, 64 UTF-16 units. |
| `+0xa8` | 64 | Filename, 32 UTF-16 units. |
| `+0xe8` | 64 | Directory, 32 UTF-16 units. |
| `+0x128` | 4 | Unknown zero-initialized word. |

The vtable contains destructors, `Resource_SetEngine`, and a pure virtual loader slot. `Resource_SetEngine` (`0x0000f3d4`) propagates to the IO and log members before the base.

`Resource_Initialize` (`0x000e4db8`) copies directory and filename, builds the full path, removes the filename's four-character extension, and calls the virtual loader with the full path and FilePak:

```c
fileName[StringLength(fileName) - 4] = 0;
return Load(fullPath, filePak);
```

The store is addressed as `resource + 0xa0 + 2 * length`, which is exactly `fileName[length - 4]`. `Resource_GetPath` (`0x000e555c`) returns the full-path member at `+0x28` and is used by manager release.

The original uses unbounded wide-string copies and no serialized-data bounds checks. The reconstruction retains the format logic; valid inputs are assumed, as in the original.

## Common loader pattern and status codes

SOJ, SAI, and STX select the entry, seek to its end to obtain its size, allocate a temporary buffer, seek back, read the complete entry, invoke a parser, and free the temporary buffer. Common statuses are:

| Status | Meaning |
|---:|---|
| 0 | Success. |
| 3 | Entry selection/opening the resource failed in SOJ and STX. |
| 4 | Allocation failed. |
| 6 | Object/subrecord parse failure. |
| 8 | Size/tell failure. |

SAU and SAI propagate FilePak's selection status directly (normally 5). SOJ and STX translate it to 3. These values match the S2D SDK's `s2d::result` enum: 3 is `S2D_FILE`, 4 is `S2D_MEMORY`, 5 is `S2D_ACCESS`, 6 is `S2D_FORMAT`, and 8 is `S2D_UNKNOWN`. The original debug calls target an empty `NullLog` in this release; the reconstruction preserves those calls through `S3D_LOG`, with an optional stdio backend. See `symbian3rd_logging.md` and `s2dsdk_correlation.md`.

## SAU/WAR: `AudioData`

`AudioData` adds two words to `Resource`, giving a total size of 0x134:

| Offset | Type | Correct meaning |
|---:|---|---|
| `+0x12c` | `uint32_t` | Decoded PCM **sample count**, `compressedBytes * 2`. |
| `+0x130` | `int16_t *` | Mono signed-16-bit PCM allocation. |

`LoadSauResource` (`0x00114b30`) allocates `pcmSampleCount * 2` bytes. That is exactly two bytes for every sample produced by `DecodeSauAdpcm`; there is no additional decoded-data tail. Earlier notes that called `+0x12c` a byte count and inferred a zero-filled tail were incorrect.

`GetAudioDataPcmSampleCount` (`0x001145c8`) and `GetAudioDataPcmBuffer` (`0x001145f0`) expose these fields. The mixer compares its sample position against the sample count, so its indexing is consistent.

`DecodeSauAdpcm` (`0x00114ce0`) is headerless mono ADPCM. SAU consumes low nibbles before high nibbles in the little-endian input stream; WAR is the same format with the opposite nibble order. The decoder starts predictor and step index at zero, clamps the index to `0..88`, saturates output to signed 16-bit range, and produces two samples per compressed byte. Since each original runtime has a fixed order without runtime conversion, this was probably a compile-time engine option. The reconstruction defaults to SAU and provides `SOFT3D_AUDIO_REVERSE_NIBBLE_ORDER=ON` for WAR.

## STX: `TextureRes` and `Texture`

`TextureRes` embeds a 0x58-byte `Texture` at `+0x12c`, giving 0x184 bytes total. Its loader at `0x0000f500` forwards to the Texture loader at `0x000d84a0`. `TextureRes_SetEngine` (`0x0000f538`) updates both the texture and resource base.

Recovered Texture fields:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x28`, `+0x2c` | `uint32_t` | Width and height. |
| `+0x30`, `+0x34` | fixed `int32_t` | Width and height shifted left by 16. |
| `+0x38` | `int32_t` | Exact base-2 logarithm of width, or `-1` if width is not a power of two. |
| `+0x3c` | `uint16_t` | STX type byte. |
| `+0x40` | `uint32_t` | Pixel byte count: width × height × bytes per pixel. |
| `+0x44` | `uint8_t` | Bytes per pixel: type bits 4–5, with zero mapped to 4. |
| `+0x45` | `uint8_t` | Transparency-present flag. |
| `+0x46` | `uint8_t` | Transparent palette index/color. |
| `+0x48` | pointer | Pixel allocation base. |
| `+0x4c` | pointer | Four-byte-aligned pixel pointer within that allocation. |
| `+0x50` | `uint32_t` | Palette entry count; zero becomes 256 when palette data is present. |
| `+0x54` | pointer | Palette allocation. |

`ParseStxTexture` (`0x000d818c`) reads three little-endian 32-bit header words. The first packs palette size, type, transparent color, and an unused high byte. Transparency is disabled when the signed 16-bit combined transparency/unused field is negative; otherwise the low byte is the transparent color.

If type bits 6–7 are nonzero, palette data precedes pixels. Bits equal to `0x40` select 64 bytes per palette entry; other nonzero values select 2 bytes per entry. The palette input pointer advances to a four-byte boundary. Pixel storage is copied directly and remains palette/index data, not converted RGBA.

`Texture_Reset` (`0x000d8744`) frees pixel and palette allocations and resets selected dimension fields. It does not clear every field.

## SOJ: `ObjectRes`

`ObjectRes` is 0x15c bytes. Its important fields are:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x12c` | `uint32_t` | Object count. |
| `+0x130` | pointer | Array of 0x34-byte internal object records. |
| `+0x134` | `uint32_t` | Surface count. |
| `+0x138`, `+0x13c` | `uint32_t` | Unknown serialized words. |
| `+0x140` | pointer | Array of 0x30-byte internal surfaces. |
| `+0x144`–`+0x158` | six `int32_t` | Serialized trailer values. |

`LoadSojResource` is `0x000e8198`; `ParseSojModel` is `0x000e829c`.

Each serialized object record is 52 bytes. It contains two raw 32-bit values, three RGB bytes packed into an internal RGB565 value, a byte field, a 16-bit texture selector, and additional raw fields. Texture selector `0x00ff` maps to internal index zero and sets the flag at internal `+0x16`; other values retain the low 16 bits. The final two serialized words determine the internal primitive type: first nonzero gives 3; otherwise second nonzero gives 4; both zero give 0.

Each surface starts with three 32-bit words, followed by:

1. Four vertex-header words and `vertexByteSize` bytes of vertex data. The vertex count is the second header word divided by four.
2. Two index-header words and `indexByteSize` bytes of index data.

Both variable blocks advance by `byteSize & ~3`, not by rounding up. Surface index count becomes `primitiveCount * 3` when surface primitive type is 4, otherwise `primitiveCount + 2`.

The top-level parser checks object-record status and returns 6 on error, but **ignores surface parser return values**. If surface count is zero, it releases both arrays before returning success.

## SAI: `Animation`

`Animation` is 0x178 bytes. `LoadSaiResource` is `0x00115184`; `ParseSaiAnimation` is `0x001152b0`.

The serialized layout is:

```text
u32 header0
u32 header1
u32 flagsAndTransformCount
six s32 first transform values       if count >= 1
six s32 second transform values      if count >= 2
s16 trackCount
s16 padding                          // track count occupies a 32-bit slot
tracks[trackCount]
optional morph section               if flags bit 8 is set
```

Transform count is flags byte zero. If exactly one transform is serialized, all six values are copied to the second internal transform. The two internal transform blocks begin at `+0x12c` and `+0x144`.

Each track has a 32-bit keyframe count followed by 36-byte serialized keyframes:

```text
s32 time
u16 signFlags
u16 magnitude[9]
s32 translation[3]
```

A set sign bit negates the corresponding magnitude. Internally, keyframes are 0x44 bytes: the nine values occupy sparse offsets `+0x04`, `+0x08`, `+0x0c`, `+0x14`, `+0x18`, `+0x1c`, `+0x24`, `+0x28`, and `+0x2c`; fields `+0x10`, `+0x20`, and `+0x30` are zeroed; translation is at `+0x34`; scale at `+0x40` is set to `0x10000`.

When morph data is enabled, a 16-bit morph frame count follows the tracks. Only the value **2** is accepted. Any other value is replaced by zero and no morph records are consumed. For the accepted form, a signed 16-bit morph count follows; each morph has one 32-bit word and two serialized three-`int32_t` frames.

## Reconstruction and verification

Portable sources are under `Reversed_Game/src/soft3d/`: `Resource.*`, `AudioData.*`, `Texture.*`, `TextureRes.*`, `ObjectRes.*`, and `Animation.*`. Tests in `tests/test_s3dresmanager.cpp` use synthetic data to verify field conversions and real archive entries:

- `.\\audio\\7DaysPiano03.sau`
- `.\\common\\aj_zhiwu_a.stx`
- `.\\common\\cg_bully_tuomao_a.soj`
- `.\\common\\cg_bully_cc_aa.sai`

The real SAI asset was essential in confirming the 32-bit track-count slot. All four real resources load successfully in the reconstruction.

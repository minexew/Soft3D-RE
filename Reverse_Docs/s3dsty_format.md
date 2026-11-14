# S3DSTY lightmap styles

## Identification

S3DSTY is the BSP lightmap-style animation table, not a general engine style sheet. `LoadS3dStyleResource` (`0x0010bdfc`) is called with `.\common\default.s3dsty` during scene rendering-resource setup. Its log strings call the operation `Bsp::LoadLightmapStyles` and `Bsp::change LM style`.

The loader resets 256 records and then reads exactly 256 serialized records:

```text
record:
    u8 allocationSize
    u8 frameIndices[]       # values 0x00..0x10
    u8 0xff                 # terminator
```

Each value is a lightmap frame index, not a literal byte count or RGB value. Values above `0x10` produce format error 6. `allocationSize` controls the frame-array allocation. The file has no global header or count; the fixed 256-record count is implicit. Selection failure returns 3, size/tell failure returns 8, allocation failure returns 4, and malformed input returns 6.

The runtime record at `Bsp + 0x2a2b4 + style * 8` is:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `uint8_t` | Current light value, initialized to 8. |
| `+0x01` | `uint8_t` | Current sequence position, initialized to 0. |
| `+0x02` | `uint8_t` | Allocation size. |
| `+0x03` | `uint8_t` | Enabled/valid flag; loader sets 1. |
| `+0x04` | `uint8_t *` | Allocated frame-index sequence. |

The ARM loader does not validate that the number of bytes before `0xff` equals `allocationSize`. The portable parser rejects truncation and sequences exceeding the allocation instead of overflowing the frame array.

## Update cadence

`Bsp_AdvanceLightStyles` (`0x000fe9e8`) is called from `Bsp_Update` (`0x00106f88`) once per scene update, after dictionary-entity updates. It does not run during map loading or `Scene_PrepareRender`.

For every style 1 through 254:

- disabled styles have their light value cleared to 0;
- enabled styles load `frames[position]` into the light value;
- position increments and wraps to 0 at `allocationSize`.

Styles 0 and 255 are not advanced. Loading resets light to 8 and position to 0, so a render performed before the first update still uses that initial light rather than automatically selecting frame zero. `SetLightStyleEnabled` changes the flag and immediately advances all styles, matching the ARM helper at `0x0006dd80`.

## Lightmap consumption

A face's first lightmap block is always scaled by `0x100`. For each additional nonzero style, the next block is shifted left by the style's current light and added. Values then become palette intensities:

```text
intensity = value < 0xff00 ? max((0xff00 - value) >> 3, 0x20) : 0x20
```

Only complete 16×16 blocks are rendered into the cached RGB444 lightmap. Width padding is zero. `sbp_format.md` documents interpolation, palette indexing, scrolling, and the safe handling of out-of-range palette reads.

## Reference asset

`Tools/outdir/common/default.s3dsty` is 2500 bytes and parses exactly to end of file. All 256 records have sequences. Examples:

- record 0: allocation size 1, sequence `[8]`;
- record 1: allocation size 203, with 203 frame indices;
- record 6: allocation size 20, sequence `[0,1,2,1,0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1]`.

Observed frame values include 0 through 8. The loader accepts up to 16.

## Reconstruction status

The container, runtime ownership, update cadence, and lightmap consumer are implemented. `Reversed_Game/tests/test_bsprender.cpp` validates the real records, disabled styles, a complete 20-frame cycle, lightmap output, and a real menu frame through dummy video. The full CTest suite and sanitizer verification pass.

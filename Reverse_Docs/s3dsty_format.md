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

Each value is a lightmap frame index, not a literal byte count or RGB value. Values above `0x10` produce format error 6. `allocationSize` controls the frame-array allocation. The file has no global header or count; the fixed 256-record count is implicit.

The runtime record at `Bsp + 0x2a2b4 + style * 8` is:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `uint8_t` | Current/reset light value, initialized to 8. |
| `+0x01` | `uint8_t` | Current sequence position, initialized to 0. |
| `+0x02` | `uint8_t` | Allocation size. |
| `+0x03` | `uint8_t` | Enabled/valid flag; loader sets 1. |
| `+0x04` | `uint8_t *` | Allocated frame-index sequence. |

The loader does not validate that the number of bytes before `0xff` equals `allocationSize`; a faithful parser should preserve the input behavior while the portable implementation can report truncation safely.

## Reference asset

`Tools/outdir/common/default.s3dsty` is 2500 bytes and parses exactly to end of file. All 256 records have sequences. Examples:

- record 0: allocation size 1, sequence `[8]`;
- record 1: allocation size 203, with 203 frame indices;
- record 6: allocation size 20, sequence `[0,1,2,1,0,1,2,3,4,5,6,7,8,7,6,5,4,3,2,1]`.

Observed frame values include 0 through 8. The loader accepts up to 16. These sequences drive time-varying lightmap selection in `Bsp`; the precise update cadence belongs with the BSP lightstyle runtime recovery.

## Reconstruction status

The serialized layout and loader are understood. The remaining work is the consumer: style timing, current-frame advancement, and the exact lightmap/raster modulation path. The README status can therefore move from unknown to mostly understood, with that runtime caveat.

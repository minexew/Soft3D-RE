# S3DTEX images and their relationship to STX

## Status and evidence

S3DTEX is the older S2D image container used by the vendored sample game. The format and loader behavior are now recovered from both the MIPS and Win32 `s2dcore_texture` objects in `Other/S2DSDK/sdk/lib/`. `Tools/s3dtex.py` decodes direct and paletted variants, and `Tools/test_s3dtex.py` successfully loads all 173 vendored samples as well as synthetic images for every palette representation.

The current Symbian 3rd Soft3D binary does **not** contain an S3DTEX resource name or an S3DTEX `TextureRes` loader. Its resources use STX instead. The RTTI class `s3dImageLoader` is unrelated to this container: its only substantive method is an uncalled TGA loader at `0x0009ebac`, documented in `symbian3rd_image.md`. No S3DTEX entry point has been invented for the reconstructed Soft3D `Texture`.

## Serialized header

```c
struct S3dTexHeader {
    uint8_t  bytesPerPixel;     // 1, 2, or 4 accepted by S2D allocation helpers
    uint8_t  paletteEntryCount; // zero means 256 when a palette is present
    uint16_t format;            // flags plus TF_* pixel format
    uint32_t width;             // loader truncates to 16 bits
    uint32_t height;            // loader truncates to 16 bits
    int32_t  colorKey;          // negative disables keying
};                              // 16 bytes, little-endian
```

The previous `<HBBIII` interpretation happened to produce the right dimensions, but misnamed the fields. In particular, the old `unknown04` byte is the high byte of `format`, not a palette count or an independent unknown. The sample values `03 40` and `00 80` are the little-endian words `0x4003` and `0x8000`.

The format constants in `s2dcore_common.h` are:

| Bits | Meaning |
|---|---|
| `0x8000` | `TF_ALPHA`; the observed 8-bit sample is luminance/alpha data. |
| `0x4000` | `TF_TRUECOLOR`; no serialized palette follows the header. |
| `0x2000` | `TF_G_I8`; global-palette indexed variant. |
| `0x1fff` | Low pixel-format selector, RGB332 through RGB888. |

| Low selector | Format | Bytes per palette entry |
|---:|---|---:|
| 0 | RGB332 | 1 |
| 1 | RGB444 | 2 |
| 2 | RGB555 | 2 |
| 3 | RGB565 | 2 |
| 4 | RGB666 | 3 |
| 5 | RGB888 | 3 |

## Loader behavior

`Texture::CreateFromFile` releases the previous palette and pixel allocations, reads the complete resource, and applies these rules:

1. Copy `bytesPerPixel`, `format`, width, height, and color key into the runtime texture.
2. A negative signed 32-bit color key clears `hasColorKey`. Any nonnegative value sets it and is retained verbatim in the 32-bit runtime field.
3. If either `TF_ALPHA` or `TF_TRUECOLOR` is set, skip palette loading. Otherwise require a low selector in `0..5`, replace palette count zero with 256, allocate `count * entrySize` bytes, and copy that many bytes.
4. Advance past the palette to a four-byte boundary: `input += (paletteBytes & ~3) + (paletteBytes & 3 ? 4 : 0)`.
5. Palette loading forces the pixel depth to one byte, so the following payload consists of palette indices.
6. Allocate `width * height * bytesPerPixel` bytes, align the usable pointer to four bytes, copy the pixels, and compute the width power with `GetPow2`.
7. Return `S2D_FILE` (3) for a missing resource, `S2D_UNKNOWN` (8) for a size query failure, `S2D_MEMORY` (4) for allocation failure, and `S2D_FORMAT` (6) for an unsupported palette selector.

The original performs unchecked reads and truncates dimensions/palette arithmetic to 16 bits in several places. The Python converter instead rejects truncated, trailing, overflowing, unsupported, and out-of-range data.

All 173 vendored files have exactly `16 + width * height * bytesPerPixel` bytes. There are 172 direct RGB565 images (`0x4003`) and one 8-bit luminance image (`0x8000`); 157 have a nonnegative color key. None contains a serialized palette.

## Pixel conversion and color keys

`Tools/s3dtex.py` follows S2D's `Pixel::r/g/b` extraction and deliberately retains the original left-shift-only expansion rather than bit replication:

- RGB332: `r = value & 0xe0`, `g = (value & 0x1c) << 3`, `b = (value & 3) << 6`.
- RGB444: extract three nibbles and shift each left four.
- RGB555/RGB565: extract little-endian packed channels and shift left three/three/three or three/two/three respectively.
- RGB666: three bytes in R,G,B order, each shifted left two.
- RGB888: three bytes in R,G,B order.
- `TF_ALPHA` plus depth one is emitted as equal R,G,B luminance rather than RGB332.

For keyed images the converter emits RGBA and gives matching pixels alpha zero. The S2D drawing routines compare the **packed palette color**, not the palette index: the paletted keyed loop looks up `palette[index]` and compares that value with the low runtime pixel bits. Direct-color loops likewise compare the packed source pixel. The comparison uses the width of the platform `Pixel`; the converter applies the corresponding source-format mask.

## Difference from STX

STX is a different container, not an S3DTEX alias:

- STX has a 12-byte header with palette size, type, transparency, an unused byte, width, and height;
- its type controls palette representation, implicit grayscale palettes, and pixel depth;
- palette data can precede pixels and is followed by four-byte alignment;
- Soft3D `Texture::ParseStx` preserves indexed pixels and palette data for its software rasterizer.

Do not route S3DTEX through `Texture::ParseStx`. The shared allocation/alignment logic reflects lineage, not binary compatibility. See `s2dsdk_correlation.md` for the engine-level comparison.

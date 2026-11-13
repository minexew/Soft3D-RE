# S3DTEX images and their relationship to STX

## Current evidence

S3DTEX is an older S2D image container used extensively by the vendored sample game. `Tools/s3dtex.py` decodes its two observed pixel formats. The S2D texture header describes bit depth, format, width, height, and color key. The current Symbian 3rd binary has an RTTI `s3dImageLoader` class but no `.s3dtex` resource string or demonstrated `TextureRes` loader for this extension; Symbian 7 Days resources use STX instead.

The README's former “well-understood” status was too strong: the basic decoder works, but header fields and unsupported variants remain undocumented and there is no reconstructed engine loader.

## Header and observed payloads

```c
struct S3dTexHeader {
    uint16_t bitDepth;          // 1 or 2 bytes per pixel in observed files
    uint8_t format;             // 3 in RGB565 samples, 0 in the grayscale sample
    uint8_t unknown04;          // 128 for grayscale, 64 for RGB565 samples
    uint32_t width;
    uint32_t height;
    uint32_t colorKey;          // 0 or 0xffffffff occur; other values occur too
};                              // 16 bytes
```

The names `format` and `colorKey` follow the S2D header documentation and sample distributions. `unknown04` remains unresolved and must not be renamed as a palette count without code evidence.

Observed image types:

| `bitDepth` | Payload | Tool behavior |
|---:|---|---|
| 1 | `width * height` unsigned bytes | Expanded to grayscale RGB by `Tools/s3dtex.py`. |
| 2 | `width * height` little-endian RGB565 words | Expanded to RGB with left shifts and no bit replication. |

`ImageType::PALETTE = 0x91` is declared in the tool but has no implementation. All 173 vendored sample assets have valid observed payload lengths: 172 are RGB565 and one is 8-bit grayscale. No palette sample was found in that set.

## Difference from STX

STX is a different container, not an S3DTEX alias:

- STX has a 12-byte header with palette size, type, transparency, an unused byte, width, and height;
- its type controls palette representation, implicit grayscale palettes, and pixel depth;
- palette data can precede pixels and is followed by four-byte alignment;
- the Soft3D `Texture::ParseStx` loader preserves indexed pixels and palette data for the software rasterizer.

S3DTEX instead has the 16-byte header above and observed direct 8-bit/RGB565 payloads. Do not route S3DTEX through `Texture::ParseStx`.

## Next work

Recover the S2D `Texture::CreateFromFile`/MIPS loader behavior, identify `unknown04` and color-key semantics, test conversion against every sample, and determine whether `s3dImageLoader` ever supplied a Soft3D platform path. This is a secondary compatibility/tooling stage, not a prerequisite for the Symbian menu's STX/SBP path.

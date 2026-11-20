# Symbian 3rd image loader

## `s3dImageLoader` RTTI and lifecycle

`s3dImageLoader` is a real portable engine class, not an S3DTEX handler. Its RTTI record is at `0x00146600`, its encoded name at `0x001465ec`, and its vtable starts at `0x001465d8`. It derives from `s3dBase` and embeds `s3dIO` at `+0x08` and `s3dLog` at `+0x14`.

The recovered virtual methods are:

- `S3dImageLoader_SetEngine` (`0x0009f69c`): propagates the engine to the embedded IO and log objects, then to the base;
- `S3dImageLoader_DeletingDtor` (`0x0009f798`): destroys the log, IO, and base subobjects and conditionally frees the instance.

The class has no S3DTEX method or resource-extension string. Soft3D `TextureRes` continues to load STX through `Texture_LoadStx`/`TextureRes_LoadStx`.

## TGA entry point

`ImageLoader_LoadTGA` (`0x0009ebac`, formerly `FUN_0009ebac`) is present but has no caller in the examined binary. It accepts a wide path and a 20-byte output record:

```c
struct ImageInfo {
    uint8_t *palette;   // +0x00, initialized to null; TGA does not populate it
    uint8_t *data;      // +0x04, allocated RGB(A) pixels
    int32_t width;      // +0x08
    int32_t height;     // +0x0c
    int32_t bitDepth;   // +0x10, actually bytes per pixel: 3 or 4
};
```

The loader supports only non-paletted true-color TGA:

- image type 2, uncompressed;
- image type 10, run-length encoded;
- 24-bit BGR and 32-bit BGRA input;
- color-map type zero;
- an optional ID field, skipped using header byte zero.

It reads the whole file, allocates `width * height * (bits / 8)`, converts BGR(A) to RGB(A), and writes scanlines in reverse order. The descriptor's origin bits are not consulted. RLE packets may cross scanline boundaries; packet length is `(control & 0x7f) + 1`, with bit `0x80` selecting a repeated pixel rather than a raw run.

Return codes are `S3D_OK` (0), `S3D_FILE` (3), `S3D_UNKNOWN` (8) for a failed size query, `S3D_MEMORY` (4), and `S3D_FORMAT` (6) for unsupported input. Diagnostic strings occupy `0x001464d8..0x00146588`; release builds route them to `NullLog`.

## Portable reconstruction and verification

`Reversed_Game/src/soft3d/s3dImageLoader.*` implements the class, engine propagation, file entry point, parser, and `ImageInfo::Free`. `ParseTga` exposes the memory parser for directed tests while `LoadTGA` exercises the real `s3dIO` file path.

Unlike ARM, the reconstruction validates headers, dimensions, multiplication, packet lengths, and source bounds, rejects zero-sized images and packets that exceed the declared pixel count, and releases temporary/output storage on errors. These are memory-safety differences for malformed inputs, not changes to valid decoding. Palette/color-mapped, grayscale, 15/16-bit, and origin-aware variants remain unsupported because the original function rejects or does not implement them.

`tests/test_image.cpp` covers 24-bit vertical flipping and channel conversion, ID skipping, 32-bit RLE across rows, mixed raw/repeated packet rules, actual file loading, missing files, unsupported formats, truncation, and oversized packets. The full CTest and sanitizer suites exercise the ownership and bounds behavior.

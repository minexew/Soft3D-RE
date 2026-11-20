from pathlib import Path
import struct

from PIL import Image


TF_ALPHA = 0x8000
TF_TRUECOLOR = 0x4000
TF_PALETTE_MASK = 0xc000
TF_FORMAT_MASK = 0x1fff

RGB332 = 0
RGB444 = 1
RGB555 = 2
RGB565 = 3
RGB666 = 4
RGB888 = 5

PALETTE_ENTRY_SIZES = (1, 2, 2, 2, 3, 3)


def decode_pixel(data, offset, pixel_format):
    if pixel_format == RGB332:
        value = data[offset]
        return value & 0xe0, (value & 0x1c) << 3, (value & 3) << 6
    if pixel_format in (RGB444, RGB555, RGB565):
        value, = struct.unpack_from("<H", data, offset)
        if pixel_format == RGB444:
            return ((value >> 8) & 0xf) << 4, ((value >> 4) & 0xf) << 4, (value & 0xf) << 4
        if pixel_format == RGB555:
            return ((value >> 10) & 0x1f) << 3, ((value >> 5) & 0x1f) << 3, (value & 0x1f) << 3
        return ((value >> 11) & 0x1f) << 3, ((value >> 5) & 0x3f) << 2, (value & 0x1f) << 3
    if pixel_format in (RGB666, RGB888):
        red, green, blue = data[offset:offset + 3]
        shift = 2 if pixel_format == RGB666 else 0
        return red << shift, green << shift, blue << shift
    raise ValueError(f"Unsupported S3DTEX pixel format: {pixel_format}")


def packed_pixel(data, offset, pixel_format):
    size = PALETTE_ENTRY_SIZES[pixel_format]
    return int.from_bytes(data[offset:offset + size], "little")


def load(f, verbose=False):
    header = f.read(16)
    if len(header) != 16:
        raise ValueError("Truncated S3DTEX header")
    depth, palette_count, image_format, width, height, color_key = struct.unpack(
        "<BBHIII", header)
    if verbose:
        print(f"depth={depth} palette_count={palette_count} format={image_format:04x}h "
              f"{width=} {height=} color_key={color_key:08x}h")
    if depth not in (1, 2, 4):
        raise ValueError(f"Unsupported S3DTEX byte depth: {depth}")
    pixel_format = image_format & TF_FORMAT_MASK
    if pixel_format >= len(PALETTE_ENTRY_SIZES):
        raise ValueError(f"Unsupported S3DTEX format: {image_format:04x}h")

    offset = 16
    data = f.read()
    palette = None
    if image_format & TF_PALETTE_MASK == 0:
        if palette_count == 0:
            palette_count = 256
        palette_bytes = palette_count * PALETTE_ENTRY_SIZES[pixel_format]
        aligned_bytes = (palette_bytes + 3) & ~3
        if len(data) < aligned_bytes:
            raise ValueError("Truncated S3DTEX palette")
        palette = data[:aligned_bytes]
        offset += aligned_bytes
        depth = 1

    pixel_bytes = width * height * depth
    if len(data) != offset - 16 + pixel_bytes:
        raise ValueError("S3DTEX payload length does not match its header")
    pixels = data[offset - 16:]

    keyed = color_key < 0x80000000
    image = Image.new("RGBA" if keyed else "RGB", (width, height))
    for index in range(width * height):
        source_offset = index * depth
        if palette is not None:
            palette_index = pixels[source_offset]
            palette_offset = palette_index * PALETTE_ENTRY_SIZES[pixel_format]
            if palette_offset + PALETTE_ENTRY_SIZES[pixel_format] > palette_bytes:
                raise ValueError("S3DTEX pixel references an out-of-range palette entry")
            rgb = decode_pixel(palette, palette_offset, pixel_format)
            packed = packed_pixel(palette, palette_offset, pixel_format)
        else:
            if image_format & TF_ALPHA:
                luminance = pixels[source_offset]
                rgb = (luminance, luminance, luminance)
            else:
                rgb = decode_pixel(pixels, source_offset, pixel_format)
            packed = int.from_bytes(pixels[source_offset:source_offset + depth], "little")
        if keyed:
            key_size = depth if palette is None else PALETTE_ENTRY_SIZES[pixel_format]
            key_mask = (1 << (key_size * 8)) - 1
            image.putpixel((index % width, index // width),
                           (*rgb, 0 if packed == (color_key & key_mask) else 255))
        else:
            image.putpixel((index % width, index // width), rgb)
    return image


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert S3DTEX files")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        image = load(f, args.verbose)
        image.save(args.output)

from enum import Enum
from pathlib import Path
import os
import struct
import sys

from PIL import Image

class ImageType(Enum):
    PALETTE_RGB444 = 0x00
    IMPLICIT_PALETTE = 0x12
    IMPLICIT_PALETTE_2 = 0x80
    PALETTE_32x = 0x52
    PALETTE_32x_2 = 0x02
    PALETTE = 0x91

def load(f, verbose=False):
    palsiz, type, transcol, unk4, width, height = struct.unpack("<BBBBII", f.read(12))

    if verbose:
        print(f"{palsiz=} {type=:02x}h {transcol=:02x}h {unk4=:02x}h {width=} {height=}")

    if palsiz == 0:
        palsiz = 0x100

    type = ImageType(type)

    # read palette
    if type == ImageType.PALETTE:
        # 2 bytes per palette entry
        palette = []

        for i in range(palsiz):
            rgb565, = struct.unpack("<H", f.read(2))
            b = (rgb565 & 0x1f) << 3
            g = ((rgb565 >> 5) & 0x3f) << 2
            r = ((rgb565 >> 11) & 0x1f) << 3
            palette.append((r, g, b))
    elif type == ImageType.PALETTE_32x or type == ImageType.PALETTE_32x_2:
        # 32 entries (64 bytes) per palette entry
        palette = []

        for i in range(palsiz):
            rgb565, = struct.unpack("<H", f.read(2))
            b = (rgb565 & 0x1f) << 3
            g = ((rgb565 >> 5) & 0x3f) << 2
            r = ((rgb565 >> 11) & 0x1f) << 3
            palette.append((r, g, b))

            # skip alternative palettes
            f.read(31 * 2)
    elif type == ImageType.IMPLICIT_PALETTE or type == ImageType.IMPLICIT_PALETTE_2:
        # unclear if mapping correct (this type is used for alpha maps)
        palette = [(i, i, i) for i in range(palsiz)]
    else:
        assert type != type

    # image data is aligned to 4 bytes
    f.seek((f.tell() + 3) & ~3)

    # read pixel map
    image_data = f.read(width * height)

    assert len(image_data) == width * height

    img = Image.new("RGB", (width, height))

    for y in range(height):
        for x in range(width):
            pixel_data = image_data[y * width + x]
            img.putpixel((x, y), palette[pixel_data])

    # verify that we have reached end of the file
    assert len(f.read(1)) == 0

    return img

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert STX textures")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        image = load(f)
        image.save(args.output)

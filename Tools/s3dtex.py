from enum import Enum
from pathlib import Path
import struct
import sys

from PIL import Image

class ImageType(Enum):
    T_8BIT = 1
    T_16BIT = 2
    PALETTE = 0x91

def load(f, verbose=False):
    type, unk3, unk4, width, height, unk5 = struct.unpack("<HBBIII", f.read(16))

    if verbose:
        print(f"{type=:04x}h {unk3=:02x}h {unk4=:02x}h {width=} {height=} {unk5=:08x}h")

    type = ImageType(type)

    img = Image.new("RGB", (width, height))

    if type == ImageType.T_8BIT:
        for y in range(height):
            for x in range(width):
                l8, = f.read(1)

                img.putpixel((x, y), (l8, l8, l8))
    elif type == ImageType.T_16BIT:
        for y in range(height):
            for x in range(width):
                rgb565, = struct.unpack("<H", f.read(2))
                b = (rgb565 & 0x1f) << 3
                g = ((rgb565 >> 5) & 0x3f) << 2
                r = ((rgb565 >> 11) & 0x1f) << 3

                img.putpixel((x, y), (r, g, b))
    else:
        assert type != type

    # verify that we have reached end of the file
    assert len(f.read(1)) == 0

    return img

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert S3DTEX files")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        image = load(f)
        image.save(args.output)

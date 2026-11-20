#!/usr/bin/env python3

import io
import struct
import unittest
from pathlib import Path

import s3dtex


ROOT = Path(__file__).resolve().parent.parent
SAMPLES = ROOT / "Other/S2DSDK"


def make_image(depth=2, palette_count=0, image_format=0x4003, width=2, height=1,
               color_key=0xffffffff, payload=b"\x00\x00\x00\x00"):
    return struct.pack("<BBHIII", depth, palette_count, image_format, width, height,
                       color_key) + payload


class S3dTexTests(unittest.TestCase):
    def test_all_vendored_samples(self):
        paths = list(SAMPLES.glob("**/*.s3dtex"))
        self.assertEqual(len(paths), 173)
        formats = set()
        keyed = 0
        for path in paths:
            data = path.read_bytes()
            depth, palette_count, image_format, width, height, color_key = struct.unpack_from(
                "<BBHIII", data)
            self.assertEqual(len(data), 16 + width * height * depth, path)
            self.assertEqual(palette_count, 0, path)
            formats.add(image_format)
            keyed += color_key < 0x80000000
            with path.open("rb") as stream:
                image = s3dtex.load(stream)
            self.assertEqual(image.size, (width, height), path)
            self.assertEqual(image.mode, "RGBA" if color_key < 0x80000000 else "RGB", path)
        self.assertEqual(formats, {0x4003, 0x8000})
        self.assertEqual(keyed, 157)

    def test_direct_rgb565(self):
        image = s3dtex.load(io.BytesIO(make_image(payload=struct.pack("<HH", 0xf800, 0x07e0))))
        self.assertEqual(list(image.getdata()), [(248, 0, 0), (0, 252, 0)])

    def test_direct_color_key(self):
        image = s3dtex.load(io.BytesIO(make_image(
            color_key=0xf800, payload=struct.pack("<HH", 0xf800, 0x07e0))))
        self.assertEqual(list(image.getdata()), [(248, 0, 0, 0), (0, 252, 0, 255)])

    def test_alpha_grayscale(self):
        image = s3dtex.load(io.BytesIO(make_image(
            depth=1, image_format=0x8000, width=2, payload=b"\x00\xff")))
        self.assertEqual(list(image.getdata()), [(0, 0, 0), (255, 255, 255)])

    def test_palette_formats(self):
        cases = {
            s3dtex.RGB332: (b"\xe0\x03", [(224, 0, 0), (0, 0, 192)]),
            s3dtex.RGB444: (struct.pack("<HH", 0x0f00, 0x000f), [(240, 0, 0), (0, 0, 240)]),
            s3dtex.RGB555: (struct.pack("<HH", 0x7c00, 0x001f), [(248, 0, 0), (0, 0, 248)]),
            s3dtex.RGB565: (struct.pack("<HH", 0xf800, 0x001f), [(248, 0, 0), (0, 0, 248)]),
            s3dtex.RGB666: (b"\x3f\x00\x00\x00\x00\x3f", [(252, 0, 0), (0, 0, 252)]),
            s3dtex.RGB888: (b"\xff\x00\x00\x00\x00\xff", [(255, 0, 0), (0, 0, 255)]),
        }
        for pixel_format, (palette, expected) in cases.items():
            with self.subTest(pixel_format=pixel_format):
                padding = b"\x00" * (-len(palette) % 4)
                image = s3dtex.load(io.BytesIO(make_image(
                    image_format=pixel_format, palette_count=2,
                    payload=palette + padding + b"\x00\x01")))
                self.assertEqual(list(image.getdata()), expected)

    def test_palette_alignment_and_default_count(self):
        data = make_image(image_format=s3dtex.RGB565, palette_count=1, width=1,
                          payload=struct.pack("<H", 0xf800) + b"xx\x00")
        self.assertEqual(s3dtex.load(io.BytesIO(data)).getpixel((0, 0)), (248, 0, 0))
        palette = struct.pack("<H", 0x07e0) * 256
        data = make_image(image_format=s3dtex.RGB565, width=1, payload=palette + b"\xff")
        self.assertEqual(s3dtex.load(io.BytesIO(data)).getpixel((0, 0)), (0, 252, 0))

    def test_palette_key_compares_color_not_index(self):
        data = make_image(image_format=s3dtex.RGB565, palette_count=2, color_key=0xf800,
                          payload=struct.pack("<HH", 0xf800, 0x07e0) + b"\x00\x01")
        self.assertEqual(list(s3dtex.load(io.BytesIO(data)).getdata()),
                         [(248, 0, 0, 0), (0, 252, 0, 255)])

    def test_malformed_inputs(self):
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(b"short"))
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(make_image(depth=3)))
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(make_image(image_format=6)))
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(make_image(image_format=0, palette_count=2, payload=b"\x00")))
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(make_image(payload=b"\x00")))
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(make_image() + b"extra"))
        data = make_image(image_format=s3dtex.RGB565, palette_count=1, width=1,
                          payload=struct.pack("<H", 0xf800) + b"xx\x01")
        with self.assertRaises(ValueError):
            s3dtex.load(io.BytesIO(data))


if __name__ == "__main__":
    unittest.main()

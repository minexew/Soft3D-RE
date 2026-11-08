#!/usr/bin/env python3

import struct
import tempfile
import unittest
from pathlib import Path

import sojconv


ROOT = Path(__file__).resolve().parent.parent
ARCHIVE = ROOT / "Reversed_Game/src/resources/res.spk"


def archive_resource(name):
    with ARCHIVE.open("rb") as stream:
        count = struct.unpack("<H", stream.read(2))[0]
        entries = []
        for _ in range(count):
            path = stream.read(64).split(b"\0", 1)[0].decode()
            offset = struct.unpack("<I", stream.read(4))[0]
            entries.append((path, offset))
        for index, (path, offset) in enumerate(entries):
            if path.lower() == name.lower():
                stream.seek(offset)
                size = entries[index + 1][1] - offset if index + 1 < count else None
                return stream.read(size)
    raise KeyError(name)


def make_model():
    materials = [
        sojconv.Material(
            name="test_1", unknown00=0x10000, unknown04=-20,
            red=12, green=34, blue=56, blend_mode=2, texture_selector=0xff,
            has_texture=1, packed_name=sojconv.pack_name("test_1"),
            unknown28=0x30000, unknown2c=-400, texcoord_first=1,
            texcoord_second=0),
        sojconv.Material(name="plain", red=255, green=128, blue=64),
    ]
    list_surface = sojconv.Surface(
        name="list", material_index=0, primitive_type=4,
        vertex_format=0x19, stride_bytes=20, index_format=3,
        vertices=[
            sojconv.Vertex((0, 0, 0), color=0xf800, matrix_index=0),
            sojconv.Vertex((65536, 0, 0), color=0x07e0, matrix_index=1),
            sojconv.Vertex((0, 65536, 0), color=0x001f, matrix_index=0),
        ], indices=[0, 1, 2])
    strip_surface = sojconv.Surface(
        name="strip", material_index=1, primitive_type=5,
        vertex_format=7, stride_bytes=32,
        vertices=[
            sojconv.Vertex((index * 65536, 0, 0), (0, 65536, 0),
                           (index * 65536, 65536))
            for index in range(5)
        ], indices=[0, 1, 2, 3, 4])
    fan_surface = sojconv.Surface(
        name="fan", material_index=-1, primitive_type=6,
        vertex_format=0x0d, stride_bytes=24, index_tail=b"\x34\x12\x00\x00",
        vertices=[
            sojconv.Vertex((0, 0, 0), uv=(0, 0), color=0xffff),
            sojconv.Vertex((65536, 0, 0), uv=(65536, 0), color=0xffff),
            sojconv.Vertex((65536, 65536, 0), uv=(65536, 65536), color=0xffff),
            sojconv.Vertex((0, 65536, 0), uv=(0, 65536), color=0xffff),
        ], indices=[0, 1, 2, 3])
    return sojconv.Model(
        materials, [list_surface, strip_surface, fan_surface],
        unknown138=22, unknown13c=23,
        bounds_min=(-1, -2, -3), bounds_max=(100000, 200000, 300000))


def make_animation():
    result = bytearray(struct.pack("<iiIHH", 0, 100, 0, 1, 0))
    result += struct.pack("<I", 2)
    for key in range(2):
        result += struct.pack("<iH", key * 100, 0)
        magnitudes = [0] * 9
        magnitudes[0] = magnitudes[3] = magnitudes[6] = 0xffff
        result += struct.pack("<9H", *magnitudes)
        result += struct.pack("<3i", key * 0x20000, 0, 0)
    return bytes(result)


class SojConvTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(
            dir=ROOT / "Reversed_Game/build", prefix="sojconv-tests-")
        self.directory = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_packed_names(self):
        for name in ("", "abc_1234", "a" * 24):
            self.assertEqual(sojconv.unpack_name(sojconv.pack_name(name)), name)
        with self.assertRaises(ValueError):
            sojconv.pack_name("not-valid!")
        with self.assertRaises(ValueError):
            sojconv.pack_name("a" * 25)

    def test_binary_round_trip(self):
        model = make_model()
        encoded = model.to_bytes()
        decoded = sojconv.Model.from_bytes(encoded)
        self.assertEqual(decoded.to_bytes(), encoded)
        self.assertEqual(decoded.surfaces[0].triangle_count, 1)
        self.assertEqual(decoded.surfaces[1].triangle_count, 3)
        self.assertEqual(decoded.surfaces[2].triangle_count, 2)

    def test_obj_round_trip(self):
        model = make_model()
        original = model.to_bytes()
        mtl = self.directory / "model.mtl"
        sojconv.write_mtl(model, mtl)
        obj = self.directory / "model.obj"
        with obj.open("w", encoding="utf-8") as output:
            sojconv.write_obj(model, output, mtl)
        converted = sojconv.read_obj(obj)
        self.assertEqual(converted.to_bytes(), original)
        self.assertEqual(converted.surfaces[1].indices, [0, 1, 2, 3, 4])
        self.assertEqual(converted.surfaces[2].indices, [0, 1, 2, 3])
        self.assertEqual(converted.surfaces[2].index_tail, b"\x34\x12\x00\x00")

    def test_sai_frame_interpolation(self):
        animation = sojconv.Animation.from_bytes(make_animation())
        model = sojconv.Model(surfaces=[sojconv.Surface(
            name="animated", vertex_format=0x11, stride_bytes=16,
            vertices=[sojconv.Vertex((65536, 0, 0), matrix_index=0)])])
        model_path = self.directory / "animated.soj"
        model_path.write_bytes(model.to_bytes())
        animation_path = self.directory / "animated.sai"
        animation_path.write_bytes(make_animation())
        frames = sojconv.convert_sai_to_obj(
            animation_path, model_path, self.directory / "frame", step=50,
            normalize_uv=False)
        self.assertEqual(len(frames), 3)

        def first_position(path):
            for line in path.read_text(encoding="utf-8").splitlines():
                if line.startswith("v "):
                    return round(float(line.split()[1]) * 65536)
            self.fail("Frame has no vertex")

        self.assertEqual(first_position(frames[0]), 65535)
        self.assertEqual(first_position(frames[1]), 131035)
        self.assertEqual(first_position(frames[2]), 196607)
        self.assertIn("# s3d_frame_time 50", frames[1].read_text(encoding="utf-8"))

    def test_normalized_uv_regression(self):
        (self.directory / "uvtest.png").write_bytes(
            b"\x89PNG\r\n\x1a\n" + b"\x00\x00\x00\rIHDR" +
            struct.pack(">II", 64, 64) + b"\x00\x00\x00\x00")
        cases = [
            ((4156334, -4155068), (0.9909472465515137, -0.9906454086303711)),
            ((3296363, -1063398), (0.7859141826629639, -0.25353384017944336)),
        ]
        for index, (raw_uv, expected_uv) in enumerate(cases):
            with self.subTest(case=index):
                material = sojconv.Material(
                    name="uvtest", has_texture=1,
                    packed_name=sojconv.pack_name("uvtest"))
                surface = sojconv.Surface(
                    name="textured", material_index=0, vertex_format=7,
                    stride_bytes=32,
                    vertices=[sojconv.Vertex((vertex * 65536, 0, 0),
                                             (0, 65536, 0), raw_uv)
                              for vertex in range(3)],
                    indices=[0, 1, 2])
                source = self.directory / f"case{index}.soj"
                output = self.directory / f"case{index}.obj"
                original = sojconv.Model([material], [surface]).to_bytes()
                source.write_bytes(original)
                sojconv.convert_soj_to_obj(source, output)
                text = output.read_text(encoding="utf-8")
                uv_line = next(line for line in text.splitlines()
                               if line.startswith("vt "))
                actual_uv = tuple(map(float, uv_line.split()[1:3]))
                self.assertAlmostEqual(actual_uv[0], expected_uv[0], places=4)
                self.assertAlmostEqual(actual_uv[1], expected_uv[1], places=4)
                self.assertIn("# s3d_uv normalized", text)
                self.assertIn("# s3d_uv_raw", text)
                self.assertEqual(sojconv.read_obj(output).to_bytes(), original)

    @unittest.skipUnless(ARCHIVE.exists(), "reference SPK archive is unavailable")
    def test_real_resources(self):
        raw_model = archive_resource(r".\common\cg_bully_tuomao_a.soj")
        model = sojconv.Model.from_bytes(raw_model)
        self.assertEqual(model.to_bytes(), raw_model)
        self.assertGreater(len(model.surfaces[0].vertices), 300)

        mtl = self.directory / "real.mtl"
        sojconv.write_mtl(model, mtl)
        obj = self.directory / "real.obj"
        with obj.open("w", encoding="utf-8") as output:
            sojconv.write_obj(model, output, mtl)
        self.assertEqual(sojconv.read_obj(obj).to_bytes(), raw_model)

        raw_animation = archive_resource(r".\common\cg_bully_cc_aa.sai")
        animation = sojconv.Animation.from_bytes(raw_animation)
        self.assertEqual(len(animation.tracks), 19)
        self.assertEqual((animation.start_time, animation.end_time), (0, 66000))
        model_path = self.directory / "real.soj"
        model_path.write_bytes(raw_model)
        animation_path = self.directory / "real.sai"
        animation_path.write_bytes(raw_animation)
        frames = sojconv.convert_sai_to_obj(
            animation_path, model_path, self.directory / "real_frame", step=66000,
            normalize_uv=False)
        self.assertEqual(len(frames), 2)
        for frame in frames:
            text = frame.read_text(encoding="utf-8")
            self.assertIn("f ", text)
            self.assertIn("# s3d_frame_time", text)


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3

import argparse
from dataclasses import dataclass, field
from pathlib import Path
import struct

# -Z forward, Y up. All engine geometry values are signed 16.16 fixed point.

FIXED_ONE = 0x10000
NAME_ALPHABET = "abcdefghijklmnopqrstuvwxyz_1234"
PRIMITIVE_NAMES = {4: "triangles", 5: "triangle_strip", 6: "triangle_fan"}
MATRIX_INDICES = (0, 1, 2, 4, 5, 6, 8, 9, 10)


def i32(value):
    value &= 0xffffffff
    return value - 0x100000000 if value & 0x80000000 else value


def fixed_mul(left, right):
    return i32((left * right) >> 16)


def fixed_from_float(value):
    return round(value * FIXED_ONE)


def float_text(value):
    return format(value / FIXED_ONE, ".17g")


def pack_name(name):
    if len(name) > 24:
        raise ValueError("SOJ texture names are limited to 24 characters")
    try:
        codes = [NAME_ALPHABET.index(character) for character in name.lower()]
    except ValueError as error:
        raise ValueError(f"Unsupported character in SOJ texture name: {name}") from error
    if len(codes) < 24:
        codes.append(31)
    words = [0] * 4
    for index, code in enumerate(codes):
        words[index // 6] |= code << ((index % 6) * 5)
    return words


def unpack_name(data):
    result = []
    for word in data:
        for shift in range(0, 30, 5):
            code = (word >> shift) & 31
            if code == 31:
                return "".join(result)
            result.append(NAME_ALPHABET[code])
    return "".join(result)


class Reader:
    def __init__(self, data):
        self.data = memoryview(data)
        self.offset = 0

    def take(self, size):
        if size < 0 or self.offset + size > len(self.data):
            raise ValueError(f"Unexpected end of data at 0x{self.offset:x}, need {size} bytes")
        result = self.data[self.offset:self.offset + size]
        self.offset += size
        return result

    def u16(self):
        return struct.unpack("<H", self.take(2))[0]

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def i32(self):
        return struct.unpack("<i", self.take(4))[0]

    def aligned_block(self, size):
        if size & 3:
            raise ValueError(f"SOJ data block size is not 32-bit aligned: {size}")
        return bytes(self.take(size))


@dataclass
class Material:
    name: str = "material"
    unknown00: int = 0
    unknown04: int = 0
    red: int = 255
    green: int = 255
    blue: int = 255
    blend_mode: int = 0
    texture_selector: int = 0
    has_texture: int = 0
    packed_name: list = field(default_factory=lambda: pack_name(""))
    unknown28: int = 0
    unknown2c: int = 0
    texcoord_first: int = 0
    texcoord_second: int = 0
    texture_width: int = 0
    texture_height: int = 0

    @property
    def texture_name(self):
        return unpack_name(self.packed_name)

    @property
    def has_detail_texture(self):
        return (self.texture_selector & 0xffff) == 0xff

    def write_mtl(self, output):
        print(f"newmtl {self.name}", file=output)
        print(f"Kd {self.red / 255:.9g} {self.green / 255:.9g} {self.blue / 255:.9g}", file=output)
        if self.has_texture and self.texture_name:
            print(f"map_Kd {self.texture_name}.png", file=output)
        if self.has_detail_texture and self.texture_name:
            print(f"map_Kd_detail {self.texture_name}_alp.png", file=output)
        print(f"s3d_rgb8 {self.red} {self.green} {self.blue}", file=output)
        print(f"s3d_unknown00 {self.unknown00}", file=output)
        print(f"s3d_unknown04 {self.unknown04}", file=output)
        print(f"s3d_blend {self.blend_mode}", file=output)
        print(f"s3d_texture_selector {self.texture_selector}", file=output)
        print(f"s3d_has_texture {self.has_texture}", file=output)
        print("s3d_packed_name " + " ".join(f"{word:#x}" for word in self.packed_name), file=output)
        print(f"s3d_unknown28 {self.unknown28}", file=output)
        print(f"s3d_unknown2c {self.unknown2c}", file=output)
        print(f"s3d_texcoord {self.texcoord_first} {self.texcoord_second}", file=output)
        if self.texture_width > 0 and self.texture_height > 0:
            print(f"s3d_texture_size {self.texture_width} {self.texture_height}", file=output)

    def serialize(self):
        return struct.pack(
            "<iiBBBBiI4iiiII", self.unknown00, self.unknown04,
            self.red, self.green, self.blue, self.blend_mode,
            self.texture_selector, self.has_texture, *self.packed_name,
            self.unknown28, self.unknown2c, self.texcoord_first, self.texcoord_second)


@dataclass
class Vertex:
    position: tuple = (0, 0, 0)
    normal: tuple = None
    uv: tuple = None
    color: int = None
    matrix_index: int = None
    extra: bytes = b""


@dataclass
class Surface:
    material_index: int = -1
    primitive_type: int = 4
    vertices: list = field(default_factory=list)
    indices: list = field(default_factory=list)
    vertex_format: int = 1
    stride_bytes: int = 12
    index_format: int = 0
    index_tail: bytes = b""
    name: str = "surface"

    @property
    def primitive_count(self):
        return self.triangle_count

    @property
    def triangle_count(self):
        if self.primitive_type == 4:
            return len(self.indices) // 3
        if self.primitive_type in (5, 6):
            return max(0, len(self.indices) - 2)
        raise ValueError(f"Unsupported SOJ primitive type {self.primitive_type}")

    def triangles(self):
        if self.primitive_type == 4:
            for index in range(self.triangle_count):
                yield tuple(self.indices[index * 3:index * 3 + 3])
        elif self.primitive_type == 5:
            for index in range(self.triangle_count):
                a, b, c = self.indices[index:index + 3]
                yield (a, c, b) if index & 1 else (a, b, c)
        else:
            for index in range(self.triangle_count):
                yield (self.indices[0], self.indices[index + 1], self.indices[index + 2])

    def serialize(self):
        vertex_data = bytearray()
        for vertex in self.vertices:
            if self.vertex_format & 0x10:
                vertex_data += struct.pack("<I", vertex.matrix_index or 0)
            if self.vertex_format & 1:
                vertex_data += struct.pack("<iii", *vertex.position)
            if self.vertex_format & 2:
                vertex_data += struct.pack("<iii", *(vertex.normal or (0, 0, 0)))
            if self.vertex_format & 4:
                vertex_data += struct.pack("<ii", *(vertex.uv or (0, 0)))
            if self.vertex_format & 8:
                vertex_data += struct.pack("<I", vertex.color or 0)
            vertex_data += vertex.extra
        if len(vertex_data) != len(self.vertices) * self.stride_bytes:
            raise ValueError(f"Vertex data does not match stride in {self.name}")
        index_data = struct.pack("<" + "H" * len(self.indices), *self.indices) + self.index_tail
        index_data += bytes(-len(index_data) & 3)
        return (
            struct.pack("<iIIIIII", self.material_index, self.primitive_type,
                        self.triangle_count, self.vertex_format, self.stride_bytes,
                        len(self.vertices), len(vertex_data)) + vertex_data +
            struct.pack("<II", self.index_format, len(index_data)) + index_data)


@dataclass
class Model:
    materials: list = field(default_factory=list)
    surfaces: list = field(default_factory=list)
    unknown138: int = 0
    unknown13c: int = 0
    bounds_min: tuple = (0, 0, 0)
    bounds_max: tuple = (0, 0, 0)

    @classmethod
    def from_bytes(cls, data):
        reader = Reader(data)
        materials = []
        for index in range(reader.u32()):
            values = struct.unpack("<iiBBBBiI4iiiII", reader.take(52))
            material = Material(
                name=f"material_{index}", unknown00=values[0], unknown04=values[1],
                red=values[2], green=values[3], blue=values[4], blend_mode=values[5],
                texture_selector=values[6], has_texture=values[7],
                packed_name=list(values[8:12]), unknown28=values[12], unknown2c=values[13],
                texcoord_first=values[14], texcoord_second=values[15])
            if material.texture_name:
                material.name = material.texture_name
            materials.append(material)
        surface_count = reader.u32()
        unknown138 = reader.u32()
        unknown13c = reader.u32()
        surfaces = []
        for index in range(surface_count):
            material_index = reader.i32()
            primitive_type = reader.u32()
            primitive_count = reader.u32()
            vertex_format = reader.u32()
            stride_bytes = reader.u32()
            vertex_count = reader.u32()
            vertex_data = reader.aligned_block(reader.u32())
            if vertex_format & ~0x1f:
                raise ValueError(f"Unknown vertex format bits: {vertex_format:#x}")
            if stride_bytes == 0 or len(vertex_data) != stride_bytes * vertex_count:
                raise ValueError(f"Invalid vertex stride/count in surface {index}")
            vertices = []
            for vertex_index in range(vertex_count):
                vertex_reader = Reader(vertex_data[vertex_index * stride_bytes:
                                                    (vertex_index + 1) * stride_bytes])
                matrix_index = vertex_reader.u32() if vertex_format & 0x10 else None
                position = tuple(vertex_reader.i32() for _ in range(3)) if vertex_format & 1 else (0, 0, 0)
                normal = tuple(vertex_reader.i32() for _ in range(3)) if vertex_format & 2 else None
                uv = (vertex_reader.i32(), vertex_reader.i32()) if vertex_format & 4 else None
                color = vertex_reader.u32() if vertex_format & 8 else None
                vertices.append(Vertex(position, normal, uv, color, matrix_index,
                                       bytes(vertex_reader.take(stride_bytes - vertex_reader.offset))))
            index_format = reader.u32()
            index_data = reader.aligned_block(reader.u32())
            if len(index_data) & 1:
                raise ValueError(f"Odd index data size in surface {index}")
            indices = list(struct.unpack("<" + "H" * (len(index_data) // 2), index_data))
            index_count = primitive_count * 3 if primitive_type == 4 else primitive_count + 2
            if len(indices) < index_count:
                raise ValueError(f"Insufficient indices in surface {index}")
            surfaces.append(Surface(material_index, primitive_type, vertices,
                                    indices[:index_count], vertex_format, stride_bytes,
                                    index_format, index_data[index_count * 2:],
                                    f"surface_{index}"))
        bounds_min = tuple(reader.i32() for _ in range(3))
        bounds_max = tuple(reader.i32() for _ in range(3))
        if reader.offset != len(data):
            raise ValueError(f"Trailing SOJ data at 0x{reader.offset:x} ({len(data) - reader.offset} bytes)")
        return cls(materials, surfaces, unknown138, unknown13c, bounds_min, bounds_max)

    def to_bytes(self):
        result = bytearray(struct.pack("<I", len(self.materials)))
        for material in self.materials:
            result += material.serialize()
        result += struct.pack("<III", len(self.surfaces), self.unknown138, self.unknown13c)
        for surface in self.surfaces:
            result += surface.serialize()
        result += struct.pack("<6i", *self.bounds_min, *self.bounds_max)
        return bytes(result)


@dataclass
class ObjSurface:
    name: str
    faces: list = field(default_factory=list)
    material_name: str = None
    material_index: int = None
    primitive_type: int = 4
    vertex_format: int = None
    stride_bytes: int = None
    index_format: int = 0
    index_tail: bytes = b""
    source_vertices: list = None


def png_size(path):
    with path.open("rb") as stream:
        if stream.read(8) != b"\x89PNG\r\n\x1a\n":
            raise ValueError(f"Not a PNG file: {path}")
        if stream.read(4) != b"\x00\x00\x00\r" or stream.read(4) != b"IHDR":
            raise ValueError(f"PNG has no leading IHDR: {path}")
        width, height = struct.unpack(">II", stream.read(8))
    if width <= 0 or height <= 0:
        raise ValueError(f"Invalid PNG dimensions: {path}")
    return width, height


def face_triangles(surface):
    return list(surface.triangles())


def write_mtl(model, path):
    used = set()
    for index, material in enumerate(model.materials):
        base = material.name or f"material_{index}"
        material.name = base
        suffix = 1
        while material.name in used:
            material.name = f"{base}_{suffix}"
            suffix += 1
        used.add(material.name)
    with path.open("w", encoding="utf-8") as output:
        for material in model.materials:
            material.write_mtl(output)
            print(file=output)


def write_obj(model, output, mtl_path=None, positions=None, frame_time=None, morphs=None,
              uv_normalized=False, raw_uv=None):
    vertices = [vertex for surface in model.surfaces for vertex in surface.vertices]
    if positions is not None and len(positions) != len(vertices):
        raise ValueError("Animated position count does not match model")
    if raw_uv is not None and len(raw_uv) != len(vertices):
        raise ValueError("Raw UV count does not match model")
    print("# Soft3D SOJ conversion", file=output)
    print(f"# s3d_unknown138 {model.unknown138}", file=output)
    print(f"# s3d_unknown13c {model.unknown13c}", file=output)
    print("# s3d_bounds_min " + " ".join(str(value) for value in model.bounds_min), file=output)
    print("# s3d_bounds_max " + " ".join(str(value) for value in model.bounds_max), file=output)
    print("# s3d_uv normalized" if uv_normalized else "# s3d_uv texels", file=output)
    if frame_time is not None:
        print(f"# s3d_frame_time {frame_time}", file=output)
    if morphs:
        for target, vector in morphs:
            print(f"# s3d_morph {target} {vector[0]} {vector[1]} {vector[2]}", file=output)
    if mtl_path is not None:
        print(f"mtllib {mtl_path.name}", file=output)

    vertex_offset = 0
    for vertex_index, vertex in enumerate(vertices):
        position = vertex.position if positions is None else positions[vertex_index]
        print("v " + " ".join(float_text(value) for value in position), file=output)
        if vertex.color is not None:
            print(f"# s3d_color {vertex_index + 1} {vertex.color}", file=output)
        if vertex.matrix_index is not None:
            print(f"# s3d_matrix {vertex_index + 1} {vertex.matrix_index}", file=output)
        if vertex.extra:
            print(f"# s3d_extra {vertex_index + 1} {vertex.extra.hex()}", file=output)
        if raw_uv is not None and raw_uv[vertex_index] is not None:
            print(f"# s3d_uv_raw {vertex_index + 1} {raw_uv[vertex_index][0]} "
                  f"{raw_uv[vertex_index][1]}", file=output)
    for vertex in vertices:
        if vertex.uv is not None:
            print(f"vt {float_text(vertex.uv[0])} {float_text(vertex.uv[1])}", file=output)
    for vertex in vertices:
        if vertex.normal is not None:
            print("vn " + " ".join(float_text(value) for value in vertex.normal), file=output)

    uv_offset = 0
    normal_offset = 0
    for surface in model.surfaces:
        print(f"o {surface.name}", file=output)
        print(f"# s3d_material_index {surface.material_index}", file=output)
        print(f"# s3d_primitive {surface.primitive_type}", file=output)
        print(f"# s3d_vertex_format {surface.vertex_format:#x}", file=output)
        print(f"# s3d_stride {surface.stride_bytes}", file=output)
        print(f"# s3d_index_format {surface.index_format}", file=output)
        if surface.index_tail:
            print(f"# s3d_index_tail {surface.index_tail.hex()}", file=output)
        source_tokens = []
        for index, vertex in enumerate(surface.vertices):
            vertex_id = vertex_offset + index + 1
            uv_id = uv_offset + index + 1 if vertex.uv is not None else None
            normal_id = normal_offset + index + 1 if vertex.normal is not None else None
            source_tokens.append(str(vertex_id) + "/" + (str(uv_id) if uv_id else "") + "/" +
                                 (str(normal_id) if normal_id else ""))
        print(f"# s3d_vertices {len(source_tokens)} " + " ".join(source_tokens), file=output)
        if 0 <= surface.material_index < len(model.materials):
            print(f"usemtl {model.materials[surface.material_index].name}", file=output)
        for triangle in face_triangles(surface):
            tokens = []
            for index in triangle:
                vertex_id = vertex_offset + index + 1
                uv_id = uv_offset + index + 1 if surface.vertices[index].uv is not None else None
                normal_id = normal_offset + index + 1 if surface.vertices[index].normal is not None else None
                tokens.append(str(vertex_id) + "/" + (str(uv_id) if uv_id else "") + "/" +
                              (str(normal_id) if normal_id else ""))
            print("f " + " ".join(tokens), file=output)
        vertex_offset += len(surface.vertices)
        uv_offset += sum(vertex.uv is not None for vertex in surface.vertices)
        normal_offset += sum(vertex.normal is not None for vertex in surface.vertices)


def normalize_model_uv(model, texture_dir):
    raw_uv = [vertex.uv for surface in model.surfaces for vertex in surface.vertices]
    for material in model.materials:
        if not material.texture_name:
            continue
        texture_path = texture_dir / (material.texture_name + ".png")
        if not texture_path.is_file():
            raise ValueError(f"Missing texture {texture_path}; use --raw-uv for a data-only conversion")
        material.texture_width, material.texture_height = png_size(texture_path)
    for surface in model.surfaces:
        if not (0 <= surface.material_index < len(model.materials)):
            continue
        material = model.materials[surface.material_index]
        if material.texture_width <= 0 or material.texture_height <= 0:
            raise ValueError(f"No texture dimensions for {surface.name}")
        for vertex in surface.vertices:
            if vertex.uv is not None:
                vertex.uv = (round(vertex.uv[0] / material.texture_width),
                             round(vertex.uv[1] / material.texture_height))
    return raw_uv


def convert_soj_to_obj(input_path, output_path, mtl_path=None, texture_dir=None, normalize_uv=True):
    model = Model.from_bytes(input_path.read_bytes())
    raw_uv = normalize_model_uv(model, input_path.parent if texture_dir is None else texture_dir) \
        if normalize_uv else None
    if mtl_path is None:
        mtl_path = output_path.with_suffix(".mtl")
    write_mtl(model, mtl_path)
    with output_path.open("w", encoding="utf-8") as output:
        write_obj(model, output, mtl_path, uv_normalized=normalize_uv, raw_uv=raw_uv)
    return model


def parse_int(value):
    return int(value, 0)


def parse_mtl(path):
    materials = []
    current = None
    if path is None or not path.exists():
        return materials
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        parts = raw_line.split()
        if not parts or parts[0].startswith("#"):
            continue
        command, values = parts[0], parts[1:]
        if command == "newmtl":
            current = Material(name=values[0])
            materials.append(current)
        elif current is None:
            continue
        elif command == "Kd":
            current.red = round(float(values[0]) * 255)
            current.green = round(float(values[1]) * 255)
            current.blue = round(float(values[2]) * 255)
        elif command == "map_Kd":
            current.packed_name = pack_name(Path(values[0]).stem)
            current.has_texture = 1
        elif command == "map_Kd_detail":
            current.texture_selector = 0xff
        elif command == "s3d_rgb8":
            current.red, current.green, current.blue = map(parse_int, values)
        elif command == "s3d_unknown00":
            current.unknown00 = parse_int(values[0])
        elif command == "s3d_unknown04":
            current.unknown04 = parse_int(values[0])
        elif command == "s3d_blend":
            current.blend_mode = parse_int(values[0])
        elif command == "s3d_texture_selector":
            current.texture_selector = parse_int(values[0])
        elif command == "s3d_has_texture":
            current.has_texture = parse_int(values[0])
        elif command == "s3d_packed_name":
            current.packed_name = [parse_int(value) for value in values]
        elif command == "s3d_unknown28":
            current.unknown28 = parse_int(values[0])
        elif command == "s3d_unknown2c":
            current.unknown2c = parse_int(values[0])
        elif command == "s3d_texcoord":
            current.texcoord_first, current.texcoord_second = map(parse_int, values)
        elif command == "s3d_texture_size":
            current.texture_width, current.texture_height = map(parse_int, values)
    return materials


def obj_index(value, count):
    index = int(value)
    return index - 1 if index > 0 else count + index


def read_obj(path):
    positions, uvs, normals = [], [], []
    colors, matrices, extras = {}, {}, {}
    raw_uvs = {}
    surfaces = []
    current = None
    unknown138 = unknown13c = 0
    bounds_min = bounds_max = None
    uv_normalized = False
    mtl_path = None

    def start_surface(name):
        nonlocal current
        current = ObjSurface(name)
        surfaces.append(current)

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        parts = raw_line.split()
        if not parts:
            continue
        command, values = parts[0], parts[1:]
        if command == "#":
            if not values:
                continue
            key, args = values[0], values[1:]
            if key == "s3d_unknown138":
                unknown138 = parse_int(args[0])
            elif key == "s3d_unknown13c":
                unknown13c = parse_int(args[0])
            elif key == "s3d_bounds_min":
                bounds_min = tuple(map(parse_int, args))
            elif key == "s3d_bounds_max":
                bounds_max = tuple(map(parse_int, args))
            elif key == "s3d_uv":
                uv_normalized = args[0] == "normalized"
            elif key == "s3d_uv_raw":
                raw_uvs[parse_int(args[0])] = (parse_int(args[1]), parse_int(args[2]))
            elif key == "s3d_color":
                colors[parse_int(args[0])] = parse_int(args[1])
            elif key == "s3d_matrix":
                matrices[parse_int(args[0])] = parse_int(args[1])
            elif key == "s3d_extra":
                extras[parse_int(args[0])] = bytes.fromhex(args[1])
            elif current is not None:
                if key == "s3d_material_index":
                    current.material_index = parse_int(args[0])
                elif key == "s3d_primitive":
                    current.primitive_type = parse_int(args[0])
                elif key == "s3d_vertex_format":
                    current.vertex_format = parse_int(args[0])
                elif key == "s3d_stride":
                    current.stride_bytes = parse_int(args[0])
                elif key == "s3d_index_format":
                    current.index_format = parse_int(args[0])
                elif key == "s3d_index_tail":
                    current.index_tail = bytes.fromhex(args[0]) if args[0] else b""
                elif key == "s3d_vertices":
                    current.source_vertices = args[1:]
        elif command == "v":
            positions.append(tuple(fixed_from_float(float(value)) for value in values[:3]))
        elif command == "vt":
            uvs.append(tuple(fixed_from_float(float(value)) for value in values[:2]))
        elif command == "vn":
            normals.append(tuple(fixed_from_float(float(value)) for value in values[:3]))
        elif command == "mtllib":
            mtl_path = path.parent / " ".join(values)
        elif command in ("o", "g"):
            start_surface(" ".join(values) or f"surface_{len(surfaces)}")
        elif command == "usemtl":
            if current is None:
                start_surface(f"surface_{len(surfaces)}")
            current.material_name = values[0]
        elif command == "f":
            if current is None:
                start_surface(f"surface_{len(surfaces)}")
            keys = []
            for token in values:
                fields = token.split("/")
                vi = obj_index(fields[0], len(positions))
                ti = obj_index(fields[1], len(uvs)) if len(fields) > 1 and fields[1] else None
                ni = obj_index(fields[2], len(normals)) if len(fields) > 2 and fields[2] else None
                keys.append((vi, ti, ni))
            for index in range(1, len(keys) - 1):
                current.faces.append((keys[0], keys[index], keys[index + 1]))

    materials = parse_mtl(mtl_path)
    material_names = {material.name: index for index, material in enumerate(materials)}
    result_surfaces = []
    for obj_surface in surfaces:
        if not obj_surface.faces:
            continue
        vertices = []
        vertex_map = {}
        triangles = []
        if obj_surface.source_vertices is not None:
            for token in obj_surface.source_vertices:
                fields = token.split("/")
                vi = obj_index(fields[0], len(positions))
                ti = obj_index(fields[1], len(uvs)) if len(fields) > 1 and fields[1] else None
                ni = obj_index(fields[2], len(normals)) if len(fields) > 2 and fields[2] else None
                key = (vi, ti, ni)
                vertex_map[key] = len(vertices)
                vertices.append(Vertex(
                    positions[vi], normals[ni] if ni is not None else None,
                    raw_uvs.get(vi + 1, uvs[ti] if ti is not None else None),
                    colors.get(vi + 1), matrices.get(vi + 1), extras.get(vi + 1, b"")))
        for face in obj_surface.faces:
            triangle = []
            for key in face:
                if key not in vertex_map:
                    vi, ti, ni = key
                    vertex_map[key] = len(vertices)
                    vertices.append(Vertex(
                        positions[vi], normals[ni] if ni is not None else None,
                        raw_uvs.get(vi + 1, uvs[ti] if ti is not None else None),
                        colors.get(vi + 1), matrices.get(vi + 1), extras.get(vi + 1, b"")))
                triangle.append(vertex_map[key])
            triangles.append(tuple(triangle))
        primitive_type = obj_surface.primitive_type
        indices = []
        if primitive_type == 4:
            indices = [index for face in triangles for index in face]
        elif primitive_type == 5:
            indices = list(triangles[0])
            for index, face in enumerate(triangles[1:], 1):
                indices.append(face[1] if index & 1 else face[2])
        elif primitive_type == 6:
            indices = [triangles[0][0], triangles[0][1]]
            indices.extend(face[2] for face in triangles)
        else:
            raise ValueError(f"Unsupported primitive type {primitive_type}")
        vertex_format = obj_surface.vertex_format
        if vertex_format is None:
            vertex_format = 1
            if any(vertex.normal is not None for vertex in vertices):
                vertex_format |= 2
            if any(vertex.uv is not None for vertex in vertices):
                vertex_format |= 4
            if any(vertex.color is not None for vertex in vertices):
                vertex_format |= 8
            if any(vertex.matrix_index is not None for vertex in vertices):
                vertex_format |= 0x10
        stride_bytes = obj_surface.stride_bytes
        if stride_bytes is None:
            stride_bytes = (4 if vertex_format & 0x10 else 0) + 12 + \
                (12 if vertex_format & 2 else 0) + (8 if vertex_format & 4 else 0) + \
                (4 if vertex_format & 8 else 0)
        material_index = obj_surface.material_index
        if material_index is None:
            material_index = material_names.get(obj_surface.material_name, -1)
        if uv_normalized and not raw_uvs and material_index >= 0:
            material = materials[material_index]
            if material.texture_width <= 0 or material.texture_height <= 0:
                raise ValueError("Normalized OBJ needs s3d_texture_size in its MTL")
            for vertex in vertices:
                if vertex.uv is not None:
                    vertex.uv = (round(vertex.uv[0] * material.texture_width / FIXED_ONE),
                                 round(vertex.uv[1] * material.texture_height / FIXED_ONE))
        result_surfaces.append(Surface(
            material_index, primitive_type, vertices, indices, vertex_format,
            stride_bytes, obj_surface.index_format, obj_surface.index_tail, obj_surface.name))
    if bounds_min is None or bounds_max is None:
        bounds_min = tuple(min((position[axis] for position in positions), default=0)
                           for axis in range(3))
        bounds_max = tuple(max((position[axis] for position in positions), default=0)
                           for axis in range(3))
    return Model(materials, result_surfaces, unknown138, unknown13c, bounds_min, bounds_max)


def convert_obj_to_soj(input_path, output_path):
    model = read_obj(input_path)
    output_path.write_bytes(model.to_bytes())
    return model


@dataclass
class SaiTrack:
    keyframes: list


@dataclass
class Animation:
    start_time: int
    end_time: int
    bounds: list
    tracks: list
    morphs: list

    @classmethod
    def from_bytes(cls, data):
        reader = Reader(data)
        start_time = reader.i32()
        end_time = reader.i32()
        flags = reader.u32()
        bounds = [tuple(reader.i32() for _ in range(6)) for _ in range(flags & 0xff)]
        track_count = reader.u16()
        reader.u16()
        tracks = []
        for _ in range(track_count):
            keyframes = []
            for _ in range(reader.u32()):
                time = reader.i32()
                signs = reader.u16()
                matrix = [0] * 16
                for index, matrix_index in enumerate(MATRIX_INDICES):
                    value = reader.u16()
                    matrix[matrix_index] = -value if signs & (1 << index) else value
                matrix[12] = reader.i32()
                matrix[13] = reader.i32()
                matrix[14] = reader.i32()
                matrix[15] = FIXED_ONE
                keyframes.append((time, matrix))
            tracks.append(SaiTrack(keyframes))
        morphs = []
        if flags & 0x100:
            target_count = reader.u16()
            if target_count == 2:
                for _ in range(reader.u16()):
                    time = reader.i32()
                    targets = [tuple(reader.i32() for _ in range(3)) for _ in range(target_count)]
                    morphs.append((time, targets))
        if reader.offset != len(data):
            raise ValueError(f"Trailing SAI data at 0x{reader.offset:x}")
        return cls(start_time, end_time, bounds, tracks, morphs)

    def interpolation_factor(self, time, previous, interval_end):
        interval = (interval_end - previous) & 0xffff
        reciprocal = 0x7fffffff if interval == 0 else (0x40000000 // interval) >> 14
        return fixed_mul(i32((time - previous) << 16), reciprocal)

    def sample_track(self, track_index, time):
        keyframes = self.tracks[track_index].keyframes
        if len(keyframes) == 1:
            return list(keyframes[0][1])
        previous = None
        next_key = None
        interval_end = self.end_time
        for index, keyframe in enumerate(keyframes):
            if keyframe[0] == time:
                return list(keyframe[1])
            if time < keyframe[0]:
                previous = keyframes[index - 1]
                next_key = keyframe
                interval_end = keyframe[0]
                break
        if previous is None:
            previous = keyframes[-1]
            next_key = keyframes[0]
        factor = self.interpolation_factor(time, previous[0], interval_end)
        result = list(previous[1])
        for index in range(16):
            if index not in (3, 7, 11):
                result[index] = i32(previous[1][index] +
                    fixed_mul(next_key[1][index] - previous[1][index], factor))
        return result

    def sample_morph(self, target, time):
        if not self.morphs:
            return (0, 0, 0)
        previous = None
        next_key = None
        interval_end = self.end_time
        for index, morph in enumerate(self.morphs):
            if morph[0] == time:
                return morph[1][target]
            if time < morph[0]:
                previous = self.morphs[index - 1]
                next_key = morph
                interval_end = morph[0]
                break
        if previous is None:
            previous = self.morphs[-1]
            next_key = self.morphs[0]
        factor = self.interpolation_factor(time, previous[0], interval_end)
        return tuple(i32(previous[1][target][axis] + fixed_mul(
            next_key[1][target][axis] - previous[1][target][axis], factor))
            for axis in range(3))


def transform_position(matrix, position):
    return tuple(i32(sum(fixed_mul(matrix[row + column * 4], position[column])
                         for column in range(3)) + matrix[row + 12])
                 for row in range(3))


def convert_sai_to_obj(input_path, model_path, output_prefix, start=None, end=None,
                       step=1, mtl_path=None, texture_dir=None, normalize_uv=True):
    animation = Animation.from_bytes(input_path.read_bytes())
    model = Model.from_bytes(model_path.read_bytes())
    raw_uv = normalize_model_uv(model, model_path.parent if texture_dir is None else texture_dir) \
        if normalize_uv else None
    start = animation.start_time if start is None else start
    end = animation.end_time if end is None else end
    if step <= 0:
        raise ValueError("SAI frame step must be positive")
    times = list(range(start, end + 1, step))
    if not times or times[-1] != end:
        times.append(end)
    output_prefix = Path(output_prefix)
    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    if mtl_path is None:
        mtl_path = output_prefix.with_suffix(".mtl")
    write_mtl(model, mtl_path)
    written = []
    for frame_index, time in enumerate(times):
        matrices = [animation.sample_track(track, time) for track in range(len(animation.tracks))]
        positions = []
        for surface in model.surfaces:
            for vertex in surface.vertices:
                if vertex.matrix_index is not None and vertex.matrix_index < len(matrices):
                    positions.append(transform_position(matrices[vertex.matrix_index], vertex.position))
                else:
                    positions.append(vertex.position)
        morphs = []
        if animation.morphs:
            morphs = [(target, animation.sample_morph(target, time)) for target in range(2)]
        frame_path = Path(f"{output_prefix}_{frame_index:08d}.obj")
        with frame_path.open("w", encoding="utf-8") as output:
            write_obj(model, output, mtl_path, positions, time, morphs,
                      uv_normalized=normalize_uv, raw_uv=raw_uv)
        written.append(frame_path)
    return written


def main():
    parser = argparse.ArgumentParser(
        description="Convert Soft3D SOJ models, OBJ files, and SAI animation frames")
    subparsers = parser.add_subparsers(dest="command", required=True)

    soj_parser = subparsers.add_parser("soj-to-obj", help="Convert every SOJ surface/material to OBJ/MTL")
    soj_parser.add_argument("input", type=Path)
    soj_parser.add_argument("output", type=Path)
    soj_parser.add_argument("--mtl", type=Path)
    soj_parser.add_argument("--texture-dir", type=Path)
    soj_parser.add_argument("--normalize-uv", action="store_true",
                            help="Normalize UVs (default; retained for compatibility)")
    soj_parser.add_argument("--raw-uv", action="store_true",
                            help="Write fixed-point texel UVs instead of normalized UVs")

    obj_parser = subparsers.add_parser("obj-to-soj", help="Convert OBJ/MTL to SOJ")
    obj_parser.add_argument("input", type=Path)
    obj_parser.add_argument("output", type=Path)

    sai_parser = subparsers.add_parser("sai-to-obj", help="Sample SAI skinning into an OBJ frame series")
    sai_parser.add_argument("input", type=Path)
    sai_parser.add_argument("model", type=Path)
    sai_parser.add_argument("output_prefix", type=Path)
    sai_parser.add_argument("--start", type=int)
    sai_parser.add_argument("--end", type=int)
    sai_parser.add_argument("--step", type=int, default=1)
    sai_parser.add_argument("--mtl", type=Path)
    sai_parser.add_argument("--texture-dir", type=Path)
    sai_parser.add_argument("--raw-uv", action="store_true",
                            help="Write fixed-point texel UVs instead of normalized UVs")

    args = parser.parse_args()
    if args.command == "soj-to-obj":
        convert_soj_to_obj(args.input, args.output, args.mtl,
                           args.texture_dir, not args.raw_uv)
    elif args.command == "obj-to-soj":
        convert_obj_to_soj(args.input, args.output)
    else:
        frames = convert_sai_to_obj(args.input, args.model, args.output_prefix,
                                    args.start, args.end, args.step, args.mtl,
                                    args.texture_dir, not args.raw_uv)
        print(f"Wrote {len(frames)} OBJ frames")


if __name__ == "__main__":
    main()

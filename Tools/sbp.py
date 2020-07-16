from enum import Enum
from pathlib import Path
import struct

def u(f, format):
    bytes = f.read(struct.calcsize(format))
    return struct.unpack(format, bytes)

class ChunkType(Enum):
    VERTEX = 0
    EDGE = 1
    FACE_EDGE = 2
    PLANE = 3
    TEXINFO = 4
    LIGHTMAP = 5
    FACE = 6
    OBJECT = 12

    LIGHTMAP_DATA = 26
    NUM = 27

def load(f, dump_geometry_to=None, verbose=False):
    magic = f.read(8)
    assert magic == b"S3D\0\x33\x33\xF3\x40"

    unk_0 = u(f, "<ffffff")
    print("HEADER", unk_0)

    chunks = []
    for i in range(ChunkType.NUM.value):
        start, length = u(f, "<II")
        try:
            t = ChunkType(i)
            print(t, f"{start:06X}h {length:06X}h")

            if t == ChunkType.VERTEX: print(f"\t{length // 12} vertices")
            if t == ChunkType.EDGE: print(f"\t{length // 4} edges")
            if t == ChunkType.FACE_EDGE: print(f"\t{length // 4} face edge indices")
            if t == ChunkType.PLANE: print(f"\t{length // 20} planes")
            if t == ChunkType.TEXINFO: print(f"\t{length // 48} texinfo entries")
            if t == ChunkType.LIGHTMAP: print(f"\t{length // 24} lightmap entries")
        except ValueError:
            print(f"BLOCK_{i:02X}h {start:06X}h {length:06X}h")

        chunks.append((start, length))

    assert f.tell() == chunks[0][0]

    if dump_geometry_to is not None:
        f.seek(chunks[ChunkType.VERTEX.value][0])

        for i in range(chunks[ChunkType.VERTEX.value][1] // 12):
            vtx = u(f, "<fff")
            dump_geometry_to.write(f"v {vtx[0]} {vtx[1]} {vtx[2]}\n")

        f.seek(chunks[ChunkType.EDGE.value][0])

        linesegs = []
        for i in range(chunks[ChunkType.EDGE.value][1] // 4):
            a, b = u(f, "<HH")
            linesegs.append((a, b))

        f.seek(chunks[ChunkType.FACE_EDGE.value][0])

        while f.tell() < chunks[ChunkType.FACE_EDGE.value][0] + chunks[ChunkType.FACE_EDGE.value][1]:
            start = None
            indexes = []

            while True:
                idx, = u(f, "<i")

                a, b = linesegs[abs(idx)]

                if idx < 0:
                    a, b = b, a

                if start is None:
                    start = a
                else:
                    pass

                indexes.append(a)

                if b == start:
                    break

            verts = " ".join([str(1 + x) for x in indexes])
            dump_geometry_to.write(f"f {verts}\n")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Parse SBP format")
    parser.add_argument("input", type=Path)
    args = parser.parse_args()

    with open(args.input, "rb") as f:
        # For now, dump scene geometry
        dump_path = Path(args.input.name).with_suffix(".obj")
        with open(dump_path, "wt") as fobj:
            load(f, dump_geometry_to=fobj, verbose=True)
        print("Written", dump_path)

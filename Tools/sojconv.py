from enum import Enum
import os
import struct
import sys

# -Z forward, Y up

def soj_to_obj(f, output):
    # unknown header
    for i in range(19):
        c, = struct.unpack("<i", f.read(4))
        print(f"{c:8d}", end=" ")
    print()

    num_faces, unk1, vertex_size, num_vertices, size_vertices = struct.unpack("<IIIII", f.read(20))
    print(f"{num_faces=} {unk1=} {vertex_size=} {num_vertices=} {size_vertices=}")
    assert vertex_size in (32, 36)
    assert size_vertices == num_vertices * vertex_size

    for i in range(num_vertices):
        if vertex_size == 32:
            stuff = struct.unpack("<iiiiiiii", f.read(32))
            x, y, z = stuff[0:3]
            nx, ny, nz = stuff[3:6]
            unk = stuff[6:]
        else:
            stuff = struct.unpack("<iiiiiiiii", f.read(36))
            # stuff[0] might be bone ID for animation
            x, y, z = stuff[1:4]
            nx, ny, nz = stuff[4:7]
            unk = stuff[0:1] + stuff[7:]

        x = x / 0x10000
        y = y / 0x10000
        z = z / 0x10000
        nx = nx / 0x10000
        ny = ny / 0x10000
        nz = nz / 0x10000

        print(f"v {x} {y} {z}", file=output)
        print(f"n {nx} {ny} {nz}", file=output)

        for i in range(len(unk)):
            print(f"{unk[i]:8x}", end=" ")

        #assert abs(nx*nx + ny*ny + nz*nz - 1) < 0.01

        print()

    num_indices, size_indices = struct.unpack("<II", f.read(8))
    print(f"{num_indices=}")
    assert size_indices == num_indices * 2

    for i in range(num_faces):
        a, b, c = struct.unpack("<HHH", f.read(6))
        assert a < num_vertices and b < num_vertices and c < num_vertices
        print(f"f {1+a} {1+b} {1+c}", file=output)

    # unknown trailer
    for i in range(6):
        c, = struct.unpack("<i", f.read(4))
        print(f"{c:8d}", end=" ")

    print()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert SOJ models to OBJ")
    parser.add_argument("input")
    parser.add_argument("output")
    args = parser.parse_args()

    with open(args.input, "rb") as f, open(args.output, "wt") as f2:
        soj_to_obj(f, f2)

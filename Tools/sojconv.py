from enum import Enum
import os
import struct
import sys

# -Z forward, Y up

def unpack_name(data):
    alph = 'abcdefghijklmnopqrstuvwxyz_1234'
    result = []
    for i in range(4):
        v4 = 0
        for j in range(6):
            v3 = ((31 << v4) & data[i]) >> v4;
            if v3 == 31:
                return ''.join(result);
            
            result.append(alph[v3]);
            v4 += 5;
    
    return ''.join(result);

def create_mtl_file(name, tex_path):
    f = open(f"{name}.mtl", "wt")
    
    print(f"newmtl {name}", file=f)
    print(f"map_Kd {tex_path}", file=f)
    f.close()
    
def get_png_size(tex_path):

    f = open(tex_path, "rb")
    
    ident = struct.unpack("<i", f.read(4))[0]
    #Check png ident
    assert ident == 0x474E5089
    f.seek(0x12)
    w = struct.unpack(">H", f.read(2))[0] 
    f.seek(0x16)
    h = struct.unpack(">H", f.read(2))[0]
    f.close()
    
    assert w > 0 and h > 0
    return w, h

def soj_to_obj(f, output):
    # unknown header
    #for i in range(19):
    #    c, = struct.unpack("<i", f.read(4))
    #    print(f"{c:8d}", end=" ")
    #print()
    
    f.seek(0x14)
    stuff = struct.unpack("<hh", f.read(4))
    tex_exist = stuff[0] == 1
    apha_tex_exist = stuff[1] == 1 #?
    
    assert tex_exist
    
    stuff = struct.unpack("<iiiiii", f.read(6 * 4))
    tex_name = unpack_name(stuff)
    print(f"{tex_name=}")
    tex_path = tex_name + ".png"
    assert os.path.isfile(tex_path) 
    w, h = get_png_size(tex_path)
    f.seek(0x4C)
    #mtl_name = os.path.basename(output.name)
    mtl_name = "material"
    
    num_faces, unk1, vertex_size, num_vertices, size_vertices = struct.unpack("<IIIII", f.read(20))
    print(f"{num_faces=} {unk1=} {vertex_size=} {num_vertices=} {size_vertices=}")
    assert vertex_size in (24, 32, 36)
    assert size_vertices == num_vertices * vertex_size
    
    print(f"mtllib {mtl_name}.mtl", file=output)

    for i in range(num_vertices):
        if vertex_size == 24:
            stuff = struct.unpack("<iiiiii", f.read(24))
            x, y, z = stuff[0:3]
            u, v = stuff[3:5]
            unk = stuff[5]
        elif vertex_size == 32:
            stuff = struct.unpack("<iiiiiiii", f.read(32))
            x, y, z = stuff[0:3]
            nx, ny, nz = stuff[3:6]
            u, v = stuff[6:8]
        else:
            stuff = struct.unpack("<iiiiiiiii", f.read(36))
            # stuff[0] might be bone ID for animation
            x, y, z = stuff[1:4]
            nx, ny, nz = stuff[4:7]
            u, v = stuff[7:9]

        x = x / 0x10000
        y = y / 0x10000
        z = z / 0x10000
        
        
        u = u / 0x10000
        v = v / 0x10000
        
        u = u / w
        v = v / h
        
        if vertex_size != 24:
            nx = nx / 0x10000
            ny = ny / 0x10000
            nz = nz / 0x10000
            print(f"n {nx} {ny} {nz}", file=output)
        
        print(f"v {x} {y} {z}", file=output)
        print(f"vt {u} {v}", file=output)


    num_indices, size_indices = struct.unpack("<II", f.read(8))
    print(f"{num_indices=}")
    assert size_indices == num_indices * 2
    
    print(f"usemtl {mtl_name}", file=output)

    for i in range(num_faces):
        a, b, c = struct.unpack("<HHH", f.read(6))
        assert a < num_vertices and b < num_vertices and c < num_vertices
        if vertex_size != 24:
            print(f"f {1+a}/{1+a}/{1+a} {1+b}/{1+b}/{1+b} {1+c}/{1+c}/{1+c}", file=output)
        else:
            print(f"f {1+a}/{1+a} {1+b}/{1+b} {1+c}/{1+c}", file=output)

    # unknown trailer
    for i in range(6):
        c, = struct.unpack("<i", f.read(4))
        print(f"{c:8d}", end=" ")
    
    create_mtl_file(mtl_name, tex_path)

    print()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Convert SOJ models to OBJ")
    parser.add_argument("input")
    parser.add_argument("output")
    args = parser.parse_args()

    with open(args.input, "rb") as f, open(args.output, "wt") as f2:
        soj_to_obj(f, f2)
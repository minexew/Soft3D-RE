import io
import os
import struct
import sys

with open(sys.argv[1], "rb") as f:
    f.seek(0, os.SEEK_END)
    filesize = f.tell()
    f.seek(0, os.SEEK_SET)

    magic1, magic2, unk3, unk4 = struct.unpack("<4sIII", f.read(16))
    print(f"{filesize=:06x}")
    print(f"{magic1=} {magic2=:08x} {unk3=:08x} {unk4=:08x}")
    assert magic1 == b"S3D\0" and magic2 == 0x40F33333

    # TODO

import argparse
import struct
from pathlib import Path

PATH = 0
OFFSET = 1

parser = argparse.ArgumentParser(description="Extract spk archive")
parser.add_argument("archive", type=Path)
parser.add_argument("dir", type=Path)
parser.add_argument("--variant", choices=["eng", "chi"], default="eng")
args = parser.parse_args()

def read_c_string(fo):
    readbytes = []
    while True:
        try:
            inchar = fo.read(1)
        except IOError:
            break
        if inchar == b'\x00':
            break
        readbytes.append(inchar)
    return b''.join(readbytes).decode('utf-8')


def path_redelimit(basepath, path):
    split_path = path.replace('\\', '/').split('/')
    tpath = basepath
    while split_path:
        tpath = tpath / split_path.pop(0)
    return tpath


def read_file_headers(fo, english=True):
    if not english:
        fo.read(1)
    path = read_c_string(fo)
    fo.read((31 if english else 62)-len(path))
    offset = struct.unpack("<i", arc.read(4))[0]
    print(f"path: {path}, offset: {offset}")
    return (path, offset)
    

# mkdir if not exists
args.dir.mkdir(parents=True, exist_ok=True)
# open the archive
arc = open(args.archive, 'rb')
file_count = struct.unpack("<h", arc.read(2))[0]
print(f'{file_count} files in archive')
assert file_count > 0, "File count should be more that zero!"

headers = []
for _ in range(file_count):
    headers.append(read_file_headers(arc, english=(args.variant == "eng")))
for i in range(len(headers)):
    tpath = path_redelimit(args.dir, headers[i][PATH])
    tpath.parent.mkdir(parents=True, exist_ok=True)
    dest = open(tpath, "wb")
    arc.seek(headers[i][1]+1)
    if i == len(headers) - 1:
        dest.write(arc.read())
    else:
        dest.write(arc.read(headers[i+1][OFFSET]-headers[i][OFFSET]))
    dest.close()
    print(f"Extracted {tpath}")

import argparse
import glob
from pathlib import Path
import struct
import sys

import stxconv

parser = argparse.ArgumentParser(description="Convert resources in-place")
parser.add_argument("dir", type=Path)
args = parser.parse_args()

for filename in glob.iglob(str(args.dir) + "/**/*", recursive=True):
     path = Path(filename)

     if path.suffix == ".stx":
          new_path = path.with_suffix(".png")
          print(filename, "-->", new_path)

          with open(path, "rb") as f:
               stxconv.stx_to_image(f, new_path)

import argparse
import glob
from pathlib import Path
import struct
import sys

import s3dtex
import stx

parser = argparse.ArgumentParser(description="Convert resources in-place")
parser.add_argument("dir", type=Path)
args = parser.parse_args()

for filename in glob.iglob(str(args.dir) + "/**/*", recursive=True):
     path = Path(filename)

     if path.suffix == ".s3dtex":
          new_path = path.with_suffix(".png")
          print(filename, "-->", new_path)

          with open(path, "rb") as f:
               s3dtex.load(f).save(new_path)
     elif path.suffix == ".stx":
          new_path = path.with_suffix(".png")
          print(filename, "-->", new_path)

          with open(path, "rb") as f:
               stx.load(f).save(new_path)

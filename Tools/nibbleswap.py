import argparse
import struct
import wave
from pathlib import Path

parser = argparse.ArgumentParser(description="IMA ADPCM -> SAU nibbleswapper")
parser.add_argument("ima", type=Path)
parser.add_argument("sau", type=Path)
args = parser.parse_args()


def nibbleswap_data(byte):
    return ((byte & 15)<<4 | (byte & 240)>>4)


def byteswap_data(in_adpcm):
    # In-place nibble order swap
    for i in range(len(in_adpcm)):
        in_adpcm[i] = nibbleswap_data(in_adpcm[i])


print("Reading input file...")
ima = open(args.ima, 'rb')
in_data = bytearray(ima.read())
ima.close()
print("Byteswapping the input (IMA -> SAU)...")
byteswap_data(in_data)
print("Writing the output...")
sau = open(args.sau, 'wb')
sau.write(in_data)  # Write nibbleswapped data
sau.close()
print("Finished!")

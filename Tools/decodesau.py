import argparse
import struct
import wave
from pathlib import Path

TABLE2 = (-1, -1, -1, -1, 0x00000002, 0x00000004, 0x00000006, 0x00000008)
TABLE1 = (0x00000007, 0x00000008, 0x00000009, 0x0000000A, 0x0000000B,
          0x0000000C, 0x0000000D, 0x0000000E, 0x00000010, 0x00000011,
          0x00000013, 0x00000015, 0x00000017, 0x00000019, 0x0000001C,
          0x0000001F, 0x00000022, 0x00000025, 0x00000029, 0x0000002D,
          0x00000032, 0x00000037, 0x0000003C, 0x00000042, 0x00000049,
          0x00000050, 0x00000058, 0x00000061, 0x0000006B, 0x00000076,
          0x00000082, 0x0000008F, 0x0000009D, 0x000000AD, 0x000000BE,
          0x000000D1, 0x000000E6, 0x000000FD, 0x00000117, 0x00000133,
          0x00000151, 0x00000173, 0x00000198, 0x000001C1, 0x000001EE,
          0x00000220, 0x00000256, 0x00000292, 0x000002D4, 0x0000031C,
          0x0000036C, 0x000003C3, 0x00000424, 0x0000048E, 0x00000502,
          0x00000583, 0x00000610, 0x000006AB, 0x00000756, 0x00000812,
          0x000008E0, 0x000009C3, 0x00000ABD, 0x00000BD0, 0x00000CFF,
          0x00000E4C, 0x00000FBA, 0x0000114C, 0x00001307, 0x000014EE,
          0x00001706, 0x00001954, 0x00001BDC, 0x00001EA5, 0x000021B6,
          0x00002515, 0x000028CA, 0x00002CDF, 0x0000315B, 0x0000364B,
          0x00003BB9, 0x000041B2, 0x00004844, 0x00004F7E, 0x00005771,
          0x0000602F, 0x000069CE, 0x00007462, 0x00007FFF);


parser = argparse.ArgumentParser(description="Decode sau audio")
parser.add_argument("sau", type=Path)
parser.add_argument("wav", type=Path)
parser.add_argument("-s", "--byteswap", action="store_true", default=False)
parser.add_argument("-n", "--no-byteswap", action="store_true", default=False)
args = parser.parse_args()


def decode_sau(in_adpcm):
    bitoffset = 4
    z = 0
    a = 0
    b = 0
    output = []
    pointer = 0
    while True:
        bitoffset = (bitoffset + 4) & 7
        if bitoffset == 0:
            if pointer >= len(in_adpcm):
                break
            b = in_adpcm[pointer]
            pointer += 1
        x = (b >> bitoffset) & 15
        y = x & 7
        w = (TABLE1[z]*y // 4) + (TABLE1[z] // 8)
        z += TABLE2[y]
        if z < 0:
            z = 0
        elif z > 88:
            z = 88
        if (x & 8) > 0:
            w = -w
        a += w
        if a > 32767:
            a = 32767
        elif a < -32768:
            a = -32768
        output.append(a)
    return output


def nibbleswap_data(byte):
    return ((byte & 15)<<4 | (byte & 240)>>4)


def byteswap_data(in_adpcm):
    # In-place nibble order swap
    for i in range(len(in_adpcm)):
        in_adpcm[i] = nibbleswap_data(in_adpcm[i])


byteswap = None
# Args preprocess
assert not (args.byteswap and args.no_byteswap),\
    "Only one of --byteswap or --no-byteswap is allowed!"
s = str(args.sau).lower()
if s.endswith('sau'):
    byteswap = False
    print("Guessed nibble order (12) from extension: no byteswap needed")
elif s.endswith('war'):
    byteswap = True
    print("Guessed nibble order (21) from extension: byteswap required")
else:
    print("Unknown extension, nibble order cannot be inferred automatically")
    if args.byteswap != args.no_byteswap:  # Logical XOR
        if args.byteswap:
            byteswap = True
        else:
            byteswap = False
        print(f"Byteswap is {'enabled' if byteswap else 'disabled'} by flag")
    else:
        print("Please, specify nibble order with either"
              " --no-byteswap (direct order) or --byteswap (reverse order)")
        print("Exiting, cannot continue")
        exit(1)


print("Reading input file...")
sau = open(args.sau, 'rb')
in_data = bytearray(sau.read())
sau.close()
if byteswap:
    print("Byteswapping the input (WAR -> SAU)...")
    byteswap_data(in_data)
print("Decoding SAU input...")
out_data = decode_sau(in_data)
# str() because wave.open doesn't seem to accept Path
print(f"Writing the output ({len(out_data)} PCM frames)...")
wav = wave.open(str(args.wav), 'wb')
wav.setnchannels(1)
wav.setsampwidth(2)
wav.setframerate(16000)
packed_frames = []
for i in out_data:
    packed_frames.append(struct.pack("<h", i))
wav.writeframes(b"".join(packed_frames))
wav.close()
print("Finished!")

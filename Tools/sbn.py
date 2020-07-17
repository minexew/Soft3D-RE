from pathlib import Path
import struct

# Load 32-bit SBN (used by GA330 build)
def load_sbn32(f, is_basefont, verbose=False):
    # basefont.sbn: starts with 12d, 64d
    if is_basefont:
        unk, num_glyphs = struct.unpack("<II", f.read(8))
    else:
        # system.sbn: starts with 18
        num_glyphs, = struct.unpack("<I", f.read(4))

    if verbose:
        print("BEGIN GLYPHS", num_glyphs)

    # table of glyphs

    for i in range(num_glyphs):
        pos = f.tell()
        w, h, num_bytes = struct.unpack("<III", f.read(12))

        num_bits = w * h
        num_words = (w * h + 31) // 32
        assert num_words * 4 == num_bytes

        if verbose:
            print("GLYPH", i, "=>", w, "x", h, f" ({w*h} bits, {num_bytes} bytes)")

        bitstr = ""
        for j in range(num_words):
            word = struct.unpack("<I", f.read(4))[0]
            bitstr = bitstr + f"{word:032b}"[::-1]

        if verbose:
            # draw image!
            for i in range(h):
                print(bitstr[i*w:(i+1)*w].replace("1", "██").replace("0", "░░"))

    # end of table of glyphs, follows table of strings

    if not is_basefont:
        num_strings, = struct.unpack("<I", f.read(4))

        for i in range(num_strings):
            str_len, = struct.unpack("<I", f.read(4))
            print(f"STRING {i} `", end="")

            # decode string
            for j in range(str_len):
                # 1 = next word is RRGGBB color
                # 2 = reset color to default
                # 3 = space
                # 4~?? = glyphs
                val, = struct.unpack("<I", f.read(4))

                if val == 3:
                    c = " "
                elif val >= 4 and val < 30:
                    c = chr(ord("a") + (val - 4))
                elif val >= 30 and val < 56:
                    c = chr(ord("A") + (val - 30))
                elif val >= 56 and val < 66:
                    c = chr(ord("0") + (val - 56))
                elif val == 67: c = "…"
                elif val == 69: c = "?"
                elif val == 70: c = "'"
                elif val == 71: c = "!"
                else:
                    c = ""

                if c:
                    print(c, end="")
                else:
                    print(f"<{val}>", end="")

            print("`")

    # check that we are at end of file
    assert not f.read(1)

    # TODO: return something


# Load 7-bit SBN (used by Win32 build)
def load_sbn7(f, verbose=False):
    num_strings, = struct.unpack("<H", f.read(2))
    str_lens = []

    for i in range(num_strings):
        str_len, = struct.unpack("<H", f.read(2))
        str_lens.append(str_len)

    bitstr = ""
    num_bytes = (sum(str_lens) * 7 + 7) // 8

    if num_bytes == 0:
        num_bytes = 1 # ??

    for j in range(num_bytes):
        # TODO: inefficient and ugly!
        word = ord(f.read(1))
        bitstr = bitstr + f"{word:08b}"[::-1]

    for i in range(num_strings):
        print(f"STRING {i} LEN {str_lens[i]} `", end="")

        CHARMAP = " -abcdefghijklmnopqrstuvwxyz0123456789-.CYNSLDUxH?I'AOVERWTGM!,B[/]KP()"

        for j in range(str_lens[i]):
            # TODO: inefficient and ugly!
            val = int(bitstr[0:7][::-1], 2)
            bitstr = bitstr[7:]

            if val < len(CHARMAP):
                c = CHARMAP[val]
            else:
                c = ""

            if c:
                print(c, end="")
            else:
                print(f"<{val}>", end="")

        print("`")

    num_glyphs, = struct.unpack("<H", f.read(2))

    for i in range(num_glyphs):
        pos = f.tell()

        w, h = 6, 12
        num_bits = w * h
        num_bytes = (w * h + 7) // 8

        if verbose:
            print("GLYPH", i, "=>", w, "x", h, f" ({w*h} bits, {num_bytes} bytes)")

        bitstr = ""
        for j in range(num_bytes):
            word = struct.unpack("<B", f.read(1))[0]
            bitstr = bitstr + f"{word:08b}"[::-1]

        if verbose:
            # draw image!
            for i in range(h):
                print(bitstr[i*w:(i+1)*w].replace("1", "██").replace("0", "░░"))

    # TODO: What is the meaning of the final bytes??
    print(f"TRAILER {f.read().hex()}h")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Parse SBN format")
    parser.add_argument("input", type=Path)
    parser.add_argument("-m", dest="format", choices=["ga330", "win32"], required=True)
    args = parser.parse_args()

    if args.format == "ga330":
        is_basefont = ("basefont.sbn" in str(args.input))

        with open(args.input, "rb") as f:
            load_sbn32(f, is_basefont=is_basefont, verbose=True)
    elif args.format == "win32":
        with open(args.input, "rb") as f:
            load_sbn7(f, verbose=True)

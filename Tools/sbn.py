from pathlib import Path
import struct

def load(f, is_basefont, verbose=False):
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

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Parse SBN format")
    parser.add_argument("input", type=Path)
    args = parser.parse_args()

    is_basefont = ("basefont.sbn" in str(args.input))

    with open(args.input, "rb") as f:
        load(f, is_basefont=is_basefont, verbose=True)

from pathlib import Path
import struct

def decode_typedvalue_as_str(f):
    type, len = struct.unpack("<HH", f.read(4))
    data = f.read(len)
    if type == 2 and len == 4:
        return f"int {struct.unpack('<i', data)[0]}"
    elif type == 3:
        return f"string {data}"
    elif type == 4 and len == 4:
        return f"bool {struct.unpack('<I', data)[0]}"
    elif type == 5 and len == 4:
        return f"float {struct.unpack('<i', data)[0] / 65536}"
    elif type == 6 and len == 4:
        return f"var {struct.unpack('<HH', data)}"
    else:
        return f"unknown {type:04X}/{len:04X}/{data}"

def dumpval(f):
    t = f.tell()
    print(f"{t:5x}:\t{decode_typedvalue_as_str(f)}")

def dumph_until(f, end_addr):
    while f.tell() < end_addr:
        t = f.tell()
        x, = struct.unpack("<H", f.read(2))
        print(f"{t:5x}:\tHWORD {x:04X}h ({x})")

def dasm_until(f, end_addr):
    while f.tell() < end_addr:
        dasm_block(f, length=1, indent=0)

def dasm_block(f, length, indent):
    func_names = {
        16: "operator==",
        59: "IsEventHappen",
        102: "SetKeyMask",
        155: "AddDoorTrigger",
        158: "EnableDoorTrigger",
        159: "AddActTrigger",
        160: "SetActTrigSelCnt",
        161: "SetActTrigString",
        163: "ClearActTrigValue",
        165: "EnableActTrigger",
        221: "AddThing",
        224: "SetThingColi",
        228: "SetThingAnim",
        251: "EnableThingLight",
    }

    insn = 0

    while insn < length * 2:    # huh!
        print("\t" * indent, end="")
        print(f"{f.tell():5x}:\t(insn {insn})", end="\t")
        insn += 1

        opcode, = struct.unpack("<H", f.read(2))

        if opcode == 6:
            unk_0, var, unk_4, unk_6, unk_8 = struct.unpack("<HHHHH", f.read(10))

            print(f"SETVAR unk_0={unk_0} var#={var} unk_4={unk_4} unk_6={unk_6} unk_4={unk_8}")
            continue
        elif opcode == 7:    # do-if
            unks = struct.unpack("<HHHHHHH", f.read(14))
            len1, = struct.unpack("<H", f.read(2))
            print("DO", unks, f" (len {len1})")

            dasm_block(f, length=len1, indent=indent + 1)

            len_expr, = struct.unpack("<H", f.read(2))
            print("IF-EXPR", len_expr, f" (len {len_expr})")
            dasm_block(f, length=len_expr, indent=indent + 1)
            print("END IF")

            insn += 1

            continue
        elif opcode == 11:    # else
            unks = struct.unpack("<HHH", f.read(6))
            len_else, = struct.unpack("<H", f.read(2))
            print("ELSE", unks, f" (len {len_else})")
            dasm_block(f, length=len_else, indent=indent + 1)
            print("END ELSE", f.read(2))

            insn += 1

            continue
        elif opcode == 25:
            unk_0, unk_2 = struct.unpack("<HH", f.read(4))
            value = decode_typedvalue_as_str(f)

            print(f"CONST unk_0={unk_0} unk_2={unk_2} value={value}")
            continue
        elif opcode == 36:
            unk_0, unk_2 = struct.unpack("<HH", f.read(4))

            print(f"OP_36 unk_0={unk_0} unk_2={unk_2}")
            continue
        else:
            f.seek(f.tell() - 2)

        func_id, keep_val, num_args = struct.unpack("<HHH", f.read(6))

        assert num_args < 20

        try:
            func_name = func_names[func_id]
        except KeyError:
            func_name = f"f.{func_id}"
        print(f"CALL {func_name} WITH {num_args} ARGS (", end="")

        for i in range(num_args):
            s = decode_typedvalue_as_str(f)
            print(s, end="")

            if i + 1 < num_args:
                print(", ", end="")

        print(f") keep_val={keep_val};")

def load(f, verbose=False):
    dumph_until(f, 0x32)

    # init():()
    dasm_until(f, 0x220a)
    dumph_until(f, 0x226e)

    # ():(GetActTrigValue( 1 ) == 1) condition
    dasm_until(f, 0x22be)
    dumph_until(f, 0x22c0)

    # ():(GetActTrigValue( 1 ) == 1) body
    dasm_until(f, 0x230a)
    # rest is TODO

    dumph_until(f, f.tell() + 64)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Parse SST format")
    parser.add_argument("input", type=Path)
    args = parser.parse_args()

    # only this has been tested
    assert "ep2050.sst" in str(args.input)

    with open(args.input, "rb") as f:
        load(f, verbose=True)

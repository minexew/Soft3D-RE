#!/usr/bin/env python3
'''
    hex_splitter.py v0.1 by Yoti

    Dingoo A320 (old): 0:0x150000 0x150000:0x30a4591 0x31f4591:#
    Dingoo A320 (new): 0:0x150000 0x150000:0x323c762 0x338c762:#
    Gemei A330 (old): 0:0xe4470 0xe4470:0x37025db 0x37e6a4b:#
    Gemei A330 (new): 0:0xe4900 0xe4900:0x37025db 0x37e6edb:#
'''

import os
import sys

def main():
    if len(sys.argv) < 3:
        print('usage: ' + sys.argv[0] + ' <offset:count, ...> <in_file>')
        quit()
    else:
        fin_name = sys.argv[len(sys.argv) - 1]
        with open(fin_name, 'rb') as fin:
            fin_size = len(fin.read())
            print('in: ' + fin_name + ' (' + str(fin_size) + ')')
            for arg in range(1, len(sys.argv) - 1):
                offset, count = sys.argv[arg].split(':')
                if offset.startswith('0x'):
                    offset = int(offset, 16)
                else:
                    offset = int(offset)
                if count == '#':
                    count = fin_size - offset
                elif count.startswith('0x'):
                    count = int(count, 16)
                else:
                    count = int(count)
                fin.seek(offset)
                data = fin.read(count)
                fout_name = sys.argv[len(sys.argv) - 1] + '_' + str(arg).zfill(len(str(len(sys.argv) - 2)))
                print('out: ' + fout_name + ' (' + str(len(data)) + ')')
                with open(fout_name, 'wb') as fout:
                    fout.write(data)

if __name__ == "__main__":
    main()

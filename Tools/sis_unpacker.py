#!/usr/bin/env python3
'''
    sis_unpacker.py v0.1 by Yoti
'''

import os
import sys
import zlib
import struct

def hexalign(value, align = 0):
    return '0x' + hex(value)[2:].zfill(align)

def main():
    if len(sys.argv) < 2:
        print('usage: ' + sys.argv[0] + ' <in_file> [out_dir]')
        quit()
    else:
        if len(sys.argv) == 2:
            out_dir = os.path.splitext(sys.argv[1])[0]
        else:
            out_dir = sys.argv[2]

        fin_name = sys.argv[1]
        with open(fin_name, 'rb') as fin:
            fin_size = len(fin.read()); fin.seek(0)
            print('in: ' + fin_name + ' (' + str(fin_size) + ')', end='')
            uids = struct.unpack('<IIII', fin.read(16))
            if uids[1] != 0x10003a12 or uids[2] != 0x10000419:
                print()
                print('Unsupported file type!')
                quit()

            fin.seek(4, os.SEEK_CUR)
            files_count = struct.unpack('<H', fin.read(2))[0]
            print(' [' + str(files_count) + ']')
            fin.seek(30, os.SEEK_CUR)
            files_offset = struct.unpack('<I', fin.read(4))[0]
            fin.seek(files_offset, os.SEEK_SET)

            for i in range(0, files_count):
                file_record = struct.unpack('<IIIIIIIII', fin.read(36))
                if file_record[0] == 0 or file_record[0] == 1:
                    fin.seek(12, os.SEEK_CUR)
                    tmp = fin.tell()
                    fin.seek(file_record[6], os.SEEK_SET)
                    fout_name = out_dir + os.sep + fin.read(file_record[5]).decode('utf-16')[3:]
                    print('out: ' + fout_name + ' (' + str(file_record[7]) + ' -> ', end='')

                    fin.seek(file_record[8], os.SEEK_SET)
                    fout_data_cmp = fin.read(file_record[7])
                    try:
                        fout_data = zlib.decompress(fout_data_cmp)
                        print(str(len(fout_data)) + ')')
                        if not os.path.exists(os.path.dirname(fout_name)):
                            os.makedirs(os.path.dirname(fout_name))
                        with open(fout_name, 'wb') as fout:
                            fout.write(fout_data)
                    except:
                        print(str(len(fout_data_cmp)) + ')')
                        if not os.path.exists(os.path.dirname(fout_name)):
                            os.makedirs(os.path.dirname(fout_name))
                        with open(fout_name, 'wb') as fout:
                            fout.write(fout_data_cmp)
                    fin.seek(tmp, os.SEEK_SET)
                else:
                    fin.seek(12, os.SEEK_CUR)
                    pass

if __name__ == "__main__":
    main()
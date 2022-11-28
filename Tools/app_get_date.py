#!/usr/bin/env python3
'''
    app_get_date.py v0.1 by Yoti
'''

import sys
import struct
from datetime import datetime

def Int64toTime(time):  # thx to Fire_Head
    return int((time / 1000000 - 3600) - 730497*24*3600 + (30*365*24*60*60+7*24*60*60))

def main():
    if len(sys.argv) < 2:
        print('usage: ' + sys.argv[0] + ' <in_file>')
        quit()

    fin_name = sys.argv[1]
    with open(fin_name, 'rb') as fin:
        fin_data = fin.read()
        print('in: ' + fin_name + ' (' + str(len(fin_data)) + ')')

        uids = struct.unpack('<IIIII', fin_data[:20])
        if uids[0] != 0x10000079 or uids[1] != 0x100039ce or uids[4] != 0x434f5045:
            print('Unsupported file type!')
            quit()

        time = Int64toTime(struct.unpack('<Q', fin_data[36:36+8])[0])
        print('info: Application build timestamp is ', end='')
        print(datetime.fromtimestamp(time).strftime("%Y/%m/%d %H:%M:%S"))

if __name__ == "__main__":
    main()

Description of the file "Basefont.sbn".

Font file. has the following structure:
The beginning of the file
0F 00 00 00 - ?unknown_value
40 00 00 00 - number of characters in file (64 in DEC)

Character data:
07 00 00 00 - character width
0F 00 00 00 - character height
10 00 00 00 - how long does a symbol take (16 byte’s in DEC)
__DATA__    - symbol
---------------------------------------
---------------------------------------
When modifying a font file, you should rely on data from the parser.sbn files: 
...
if val == 3:
    c = " "
elif val >= 4 and val < 30:
    c = chr(ord("a") + (val - 4))
elif val >= 30 and val < 56:
    c = chr(ord("A") + (val - 30))
elif val >= 56 and val < 66:
    c = chr(ord("0") + (val - 56))
elif val == 67: c = "..."
elif val == 69: c = "?"
elif val == 70: c = "'"
elif val == 71: c = "!"
...

The English alphabet takes up 26 letters, which is clearly visible in the parser code.
* 4 to 30 lowercase letters;
* from 30 to 56 - uppercase;
* digits 56 to 66.
Next are special characters.
!all values ​​from the code are written in DEC!

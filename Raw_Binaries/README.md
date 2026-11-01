# Raw binaries

**This directory contains raw Soft3D binaries, ready for analysis and disassembly/decompilation.**

First problem you'll likely encounter if you try to reverse engineer a Soft3D binary is that your RE platform won't be able to load executables directly (e.g. because of compression). Hence, this directory contains raw text section binaries that are ready for disassembling and analysis.

|Binary|Original file|Original format|Version|Architecture|Image base|Entry point|Entry point (File)|
|------|-------------|---------------|-------|------------|----------|-----------|------------------|
|7days_Symbian3rd.bin|Symbian3rd.exe|E32Image|Symbian S60v3 (9.3) 7 Days Salvation (Full Game)|ARM:LE:32:v5t|0x00008000|0x00139BBC|0x00131BBC|

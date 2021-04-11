PATH c:\cygwin\bin;c:\cygwin\mipseltools\bin
rem 
make -f makefile clean
make -f makefile 2>&1 |tee err.txt
call gen_dl.bat
pause
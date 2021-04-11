rem 
make -f makefile clean
make -f makefile 2>&1 |tee err.txt
call gen_dl.bat
copy teris.dl i:\teris.dl
pause
del *.dl
del *.tab
del *.str
del *.dsc
make 2>&1 |tee err.txt
call gen_dl.bat
pause
 
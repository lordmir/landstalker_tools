REM LZ77 Asset test
REM ===============
SETLOCAL 
goto :MAIN

:TEST_LZ77
lz77 -df %IN% orig.bin
lz77 -cf orig.bin orig.lz77
lz77 -df orig.lz77 test.bin
fc /b orig.bin test.bin
EXIT /B %ERRORLEVEL% 

:MAIN
set IN=PASSION
call :TEST_LZ77
rem @echo off
set path=C:\bc.31\bin;C:\bc.31\INCLUDE
rem set INCLUDE=C:\bc.31\INCLUDE
if not exist bin mkdir bin
cd bin
rem del *.* /q
cd ..

if exist bin\noctis.exe del bin\noctis.exe
make -fnoctis.mak -DALL=DEF_ALL -B
if exist bin\noctis.exe goto success
pause
goto done

:success
copy bin\noctis.exe ..\modules\noctis.exe /y
copy supports.nct ..\data /y

rem del noctis.exe
pause
:done

cd ..
modules\noctis.exe
cd ..
cd source
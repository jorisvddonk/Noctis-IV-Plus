rem @echo off
set path=C:\BORLANDC\bin;C:\BORLANDC\INCLUDE
rem set INCLUDE=C:\BORLANDC\INCLUDE
if not exist bin mkdir bin
cd bin
del *.* /q
cd ..

if exist bin\noctis.exe del bin\noctis.exe
make -fnoctis.mak -DALL=DEF_ALL -B
if exist bin\noctis.exe goto success
pause
goto done

:success
copy bin\noctis.exe ..\modules\noctis.exe /y
copy supports.nct ..\data /y
copy bull.ncc ..\data\critter1.ncc /y

rem del noctis.exe
pause
:done

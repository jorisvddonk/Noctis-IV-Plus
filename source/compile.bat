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
rem pause
goto done

:success
copy bin\noctis.exe ..\modules\noctis.exe /y
copy supports.nct ..\data /y

if "%1"=="-run" if "%2"=="-dump" goto run_dump
if "%1"=="-run" goto run
goto done

:run
pushd ..
modules\noctis.exe
popd
goto done

:run_dump
pushd ..
modules\noctis.exe -dump -18928 29680 -67336 3
popd
goto done

rem del noctis.exe
rem pause
:done

rem cd ..
rem modules\noctis.exe
rem cd ..
rem cd source
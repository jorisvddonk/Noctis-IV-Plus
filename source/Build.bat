@echo off

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

set NIVP_DIR=%SCRIPT_DIR%\..
pushd %NIVP_DIR%
set NIVP_DIR=%CD%
popd

set DBX_BIN_INSTALLED=C:\DOSBox-X\dosbox-x.exe
set DBX_BIN_PORTABLE=%NIVP_DIR%\bin\x64\Release\dosbox-x.exe

set CONF_FILE=%NIVP_DIR%\dosbox.conf
set MOUNT_DIR=%NIVP_DIR%

set BCPP31_DIR=%MOUNT_DIR%\bc.31

echo.
echo ^+------------------------^+
echo ^| Noctis IV Plus Builder ^|
echo ^+------------------------^+
echo.

if exist "%DBX_BIN_INSTALLED%" (
    echo DOSBox-X installation found!
    set PORTABLE=0
    set DOSBOX_X_BIN=%DBX_BIN_INSTALLED%
) else (
    echo DOSBox-X is not installed! Looking for portable version...
    if exist "%DBX_BIN_PORTABLE%" (
        echo Portable DOSBox-X found!
        set PORTABLE=1
        set DOSBOX_X_BIN=%DBX_BIN_PORTABLE%
    ) else (
        echo Could not find portable DOSBox-X!
        echo.
        echo ERROR: No DOSBox-X binary found! Unable to launch!
        echo Download DOSBox-X from here: https://github.com/joncampbell123/dosbox-x/releases/latest
        echo.
        <nul set /p "=Press any key to exit . . . "
        pause >nul
        exit /b
    )
)

echo.

if "%PORTABLE%"=="1" (
    echo Using portable binary: "%DOSBOX_X_BIN%"
) else (
    echo Using installed binary: "%DOSBOX_X_BIN%"
)

if exist "%CONF_FILE%" (
    echo Using conf file: "%CONF_FILE%"
) else (
    echo Could not find conf file: "%CONF_FILE%"
    echo.
    echo ERROR: Configuration file not found! Unable to launch!
    echo Download here: https://github.com/jorisvddonk/Noctis-IV-Plus/raw/refs/heads/master/dosbox.conf
    echo.
    <nul set /p "=Press any key to exit . . . "
    pause >nul
    exit /b
)

if not exist "%MOUNT_DIR%\source\compile.bat" (
    echo.
    echo ERROR: Noctis IV / IV Plus not found! Unable to launch!
    echo Download Noctis IV here: https://80.style/packs/zip/hsp/noctis_iv-noctis_iv_download_JmsLdos_onlyK
    echo Download Noctis IV Plus from here: https://github.com/jorisvddonk/Noctis-IV-Plus/releases/latest
    echo.
    <nul set /p "=Press any key to exit . . . "
    pause >nul
    exit /b
)

if not exist "%BCPP31_DIR%\BIN\MAKE.EXE" (
    echo.
    echo ERROR: Borland C++ 3.1 not found! Unable to launch!
    echo Download Borland C++ 3.1 here: https://archive.org/download/bcpp31/BCPP31.ZIP
    echo Extract the contents to: %BCPP31_DIR%
    echo.
    <nul set /p "=Press any key to exit . . . "
    pause >nul
    exit /b
)



echo.
echo Launching Noctis IV...
start "" /b "%DOSBOX_X_BIN%" -c "mount n \"%MOUNT_DIR%\"" -c "n:" -c "cd source" -c "compile.bat" -conf "%CONF_FILE%" -exit

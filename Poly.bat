@echo off
@cls
echo --Listing all NCC files....
echo --------------------------------------------------------
dir *.ncc /b /p
echo --------------------------------------------------------
set /p desc=--Edit what file? (don't add .ncc):
if not exist polyback\ md polyback
if /i "%desc%"=="polyvert" goto polyvert
echo --editing %desc%.ncc
if exist %desc%.ncc echo --%desc%.ncc exist..
if not exist %desc%.ncc echo --%desc%.ncc does not exist..
echo --Backing up file to edit (if it exists)
if exist %desc%.ncc copy %desc%.ncc polyback\%desc%.ncc
echo --Swapping existing Polyvert.NCC file with %desc%.ncc file..
if exist Polyvert.ncc rename Polyvert.ncc Polyback.ncc
if exist %desc%.ncc rename %desc%.ncc Polyvert.ncc
echo --Starting Polyvert...
pause
polyvert.exe
echo --Swapping %desc%.ncc back with Polyvert.ncc again
if exist Polyvert.ncc rename Polyvert.ncc %desc%.ncc
if exist Polyback.ncc rename Polyback.ncc Polyvert.ncc
echo --all done!
pause
exit

:polyvert
echo --Seems like you're editing Polyvert.ncc
echo --Let's just backup Polyvert.ncc.. Nothing else to worry about this time!
copy %desc%.ncc polyback\%desc%.ncc
pause
polyvert.exe
pause
exit

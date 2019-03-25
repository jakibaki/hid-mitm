@ECHO off
cls

:Header
ECHO ////////////////////// hid_mitm (starter script by Krank) //////////////////////
ECHO.
goto start

:start
ECHO.
ECHO Working Directory: %cd%
ECHO.
set /p IP_Adress="#Enter Your Nintendo Switch IP Adress: "
input_pc_win.exe %IP_Adress%
ECHO.
goto start
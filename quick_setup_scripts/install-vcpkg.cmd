@echo off

echo.===========================================================================
echo.                 THIS SCRIPT INSTALLS VCPKG ON YOUR SYSTEM
echo.                     IT REQUIRES ADMINISTRATOR ACCESS
echo.
echo.                   Roughly follows instructions found at
echo.         https://github.com/Microsoft/vcpkg/blob/master/README.md
echo.===========================================================================
echo.
setlocal

net session >nul 2>&1
if not %errorLevel% == 0 goto notadmin

cd /d %~dp0
if exist vcpkg goto :eof
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
call bootstrap-vcpkg.bat
call vcpkg integrate install
pause
goto :eof
:notadmin
echo Fatal Error: Administrative rights required. Please right-click this file
echo and click "Run As Administrator"                      Exiting....
echo.
pause
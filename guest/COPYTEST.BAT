@echo off
mkdir C:\SGLTEST 2>nul
copy /y %~dp0opengl32.dll C:\SGLTEST >nul
copy /y %~dp0sglgears.exe C:\SGLTEST >nul
mkdir C:\SGLDX 2>nul
copy /y %~dp0opengl32.dll C:\SGLDX >nul
copy /y %~dp0DX\*.* C:\SGLDX >nul
echo Copied to C:\SGLTEST and C:\SGLDX

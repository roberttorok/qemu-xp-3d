@echo off
rem Copies all the 3D files next to a game's .exe and sets the DirectX version. Overwrites what's there.
rem Run it from the helper CD, inside XP:  E:\INSTALL.BAT "C:\Game Folder"
setlocal
if "%~1"=="" goto usage
set "G=%~f1"
if "%G:~-1%"=="\" set "G=%G:~0,-1%"
if not exist "%G%\" goto missing
if /i "%G%"=="%SystemRoot%\system32" goto system
if /i "%G%"=="%SystemRoot%" goto system

copy /y "%~dp0opengl32.dll" "%G%\" >nul || goto failed
for %%f in (wined3d d3d8 d3d9 ddraw) do copy /y "%~dp0DX\%%f.dll" "%G%\" >nul || goto failed
regedit /s "%~dp0DXVERSION.REG"
echo Installed in %G%
exit /b 0

:usage
echo Usage: INSTALL.BAT "C:\Game Folder"
exit /b 1

:missing
echo The folder %G% does not exist.
exit /b 1

:system
echo Never copy these files into %G%. Use the game's own folder.
exit /b 1

:failed
echo FAILED: some files could not be copied to %G%. Is the game still running?
exit /b 1

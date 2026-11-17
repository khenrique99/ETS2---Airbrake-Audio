@echo off
rem Builds the Airbrake Audio plugin (x64) and assembles dist\plugins, ready to copy into the game.
rem Requires: Visual Studio Build Tools with "Desktop development with C++" (includes the Windows SDK).
setlocal
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [error] Visual Studio Build Tools not found. Install: winget install Microsoft.VisualStudio.2022.BuildTools
  exit /b 1
)
set "VSDIR="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if not defined VSDIR (
  echo [error] The "C++ x64 build tools" component is missing. Open the Visual Studio Installer and add "Desktop development with C++".
  exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

if not exist build mkdir build
if not exist dist\plugins\airbrake_audio mkdir dist\plugins\airbrake_audio

cl /nologo /std:c++17 /O2 /EHsc /MT /LD /W3 /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
   src\plugin.cpp /Fo:build\ /Fe:dist\plugins\airbrake_audio.dll ^
   /link /DLL xaudio2.lib ole32.lib
if errorlevel 1 (
  echo [error] Build failed.
  exit /b 1
)

copy /y config\airbrake_audio.ini dist\plugins\airbrake_audio.ini >nul
copy /y assets\sounds\brake_loop.wav dist\plugins\airbrake_audio\ >nul
copy /y assets\sounds\air_release.wav dist\plugins\airbrake_audio\ >nul
copy /y CREDITS.md dist\CREDITS.md >nul
copy /y LICENSE dist\LICENSE >nul

echo.
echo [ok] Done: %~dp0dist\plugins
echo Copy the CONTENTS of dist\plugins into "Euro Truck Simulator 2\bin\win_x64\plugins".
endlocal

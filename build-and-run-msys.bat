@echo off
REM Build and run using MSYS2 MinGW64 (install MSYS2 first — see BUILD_WINDOWS.md)
set MSYS=C:\msys64\msys2_shell.cmd
if not exist "%MSYS%" (
  echo MSYS2 not found at C:\msys64
  echo Install from https://www.msys2.org/ then run: pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2 mingw-w64-x86_64-make
  exit /b 1
)
cd /d "%~dp0"
"%MSYS%" -mingw64 -defterm -here -no-start -c "export CUPCAKE_ASSETS=assets && mingw32-make && ./build-pc/cupcake-sdl.exe"

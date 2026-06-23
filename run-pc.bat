@echo off
cd /d "%~dp0"
set CUPCAKE_ASSETS=assets
if not exist build-pc\cupcake-sdl.exe (
  echo Building...
  mingw32-make 2>nul || make
)
build-pc\cupcake-sdl.exe

@echo off
cd /d "%~dp0.."
echo Running sprite export...
python test\export_sprites.py
if errorlevel 1 (
  echo FAILED - is Python installed? Try: py test\export_sprites.py
  pause
  exit /b 1
)
echo.
echo Open test\output\sprites_png\contact_sheet.png
echo Or:  test\output\index.html
start "" "%~dp0output\sprites_png\contact_sheet.png"
pause

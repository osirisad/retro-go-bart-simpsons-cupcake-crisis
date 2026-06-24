# Run on Windows — step by step

Your PC currently has **make** (GnuWin32) but **not gcc or SDL2**. The PC build needs a real C compiler; the easiest path is **MSYS2**.

## 1. Install MSYS2

1. Download the installer: https://www.msys2.org/
2. Run it and install to the default location: `C:\msys64`
3. When the installer finishes, it may open a terminal — let it run the first-update commands if prompted, or close it.

## 2. Install build tools (one time)

1. Open **Start → MSYS2 UCRT64** (or **MSYS2 MinGW 64-bit** / `MINGW64`).
2. In that terminal (not PowerShell), run:

```bash
pacman -Syu
# If it asks to close the window, close it, reopen MINGW64, then:
pacman -Su

pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-make
```

Type `Y` when asked to install packages.

## 3. Build the game

Still in the **MINGW64** (or UCRT64) terminal:

```bash
cd "/c/Users/sfleishman/Desktop/retrofab-acclaim-cupcake-crisis-0.9.10/bart_simpson_cupcake_crisis_port"
mingw32-make
```

MSYS2 installs GNU Make as **`mingw32-make`**, not `make`. If `which make` is empty, that is normal.

Shortcut: `./build.sh` (same as `mingw32-make`).

You should see `build-pc/cupcake-sdl.exe` when it succeeds.

**Do not** use plain PowerShell + `C:\Program Files (x86)\GnuWin32\bin\make` — that make cannot find `gcc`.

## 4. Run the game

In the same MINGW64 terminal:

```bash
export CUPCAKE_ASSETS=assets
./build-pc/cupcake-sdl.exe
```

Default window size is **50%** of the artwork (~512×512). To change:

```bash
./build-pc/cupcake-sdl.exe --scale 0.4    # smaller
./build-pc/cupcake-sdl.exe --scale 0.75   # larger
export CUPCAKE_WINDOW_SCALE=0.35
./build-pc/cupcake-sdl.exe
```

Or from PowerShell **after** a successful build:

```powershell
cd "C:\Users\sfleishman\Desktop\retrofab-acclaim-cupcake-crisis-0.9.10\bart_simpson_cupcake_crisis_port"
$env:CUPCAKE_ASSETS = "assets"
.\build-pc\cupcake-sdl.exe
```

The game needs `assets\screen.jpg` and `assets\sprites-color.png` (already in your port folder).

## 5. Controls

| Key | Action |
|-----|--------|
| Arrow keys / WASD | Move |
| Z | Action / Start |
| X | Select — cycle level in attract (0 → L-1 → L-2 → attract) |
| F6 | Sound on/off (release) |
| 1 / 2 | Quick start level 1 / 2 (PC dev shortcuts) |
| Esc | Quit |
| F3 / F4 | Save / load `cupcake.sav` |

Full mapping table: `docs/INPUT_MAPPING.md`.

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `gcc: command not found` | Use MSYS2 **MINGW64** terminal, not PowerShell |
| `SDL2/SDL.h: No such file` | Run the `pacman -S mingw-w64-x86_64-SDL2` line again |
| `Failed to load assets/...` | Run from the port folder with `CUPCAKE_ASSETS=assets` |
| Black window / no sprites | Confirm `assets\sprites-color.png` exists (~750 KB) |
| `make: mkdir -p` errors in PowerShell | Build only inside MSYS2 MINGW64 |

## Optional: build from PowerShell via MSYS2

After MSYS2 is installed, you can use:

```powershell
C:\msys64\msys2_shell.cmd -mingw64 -defterm -here -no-start -c "cd '$PWD' && make && ./build-pc/cupcake-sdl.exe"
```

Run that from the port directory in PowerShell.

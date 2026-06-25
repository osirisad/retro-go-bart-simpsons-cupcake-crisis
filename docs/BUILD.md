# Build guide

## PC (SDL2) — primary dev target

**Requirements:** gcc, SDL2 dev libs, make.

On Windows, use MSYS2 — see [BUILD_WINDOWS.md](BUILD_WINDOWS.md) for full setup.

From the project root (MINGW64 shell on Windows):

```bash
make
make run
# or: run-pc.bat
```

Output: `build-pc/cupcake-sdl` (or `cupcake-sdl.exe` on Windows).

Set `CUPCAKE_ASSETS=assets` if you run the binary from another working directory.

**Controls:** Arrow keys, **Z** = Action, **X** = Select (cycle level), **F6** = Sound, **Esc** = quit, **F3/F4** = save/load state. Full mapping: [INPUT_MAPPING.md](INPUT_MAPPING.md).

### Unit tests

```bash
make test          # all unit tests (alias: make test-all)
make test-smoke    # headless init + 10 frames
make test-demo-replay
# or individual targets: make test-state, make test-bart-move, etc.
```

Sprite UV export (optional visual check): `python test/export_sprites.py` — see [test/README.md](../test/README.md).

## retro-go / Game & Watch — overlay (active)

Integrate into your retro-go-sd firmware fork using the Celeste overlay model. See [OVERLAY_SPRINT_BOARD.md](../OVERLAY_SPRINT_BOARD.md).

### Linux emu (regression / dev)

Build the linux emu binary from your retro-go firmware tree:

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

Output: `build-cupcake/retro-go-cupcake.elf`

Copy `assets/sprites-color.png`, `assets/screen.jpg` (and fallback `sprites.png`) to `/home/odroid/cupcake/` on the device SD layout your firmware expects (same pattern as other linux emu apps).

Install the produced ELF like Celeste (`retro-go-celeste`).

### Device overlay (hardware)

1. Add Cupcake to your firmware fork (linker `.overlay_cupcake`, `Makefile.common`, `rg_emulators.c` dispatch) — tasks OV-11..OV-15 in the overlay board.
2. Build firmware; extract `cupcake.bin` from the overlay section.
3. Copy `cupcake.bin` to `/roms/homebrew/` on SD.
4. Launch from Homebrew menu.

Assets are embedded at build time (`cupcake_data.h`), not loose files on SD.

## GWHB binary (on hold)

GWHB SD-drop is deferred until GWHB-capable firmware is stable. Scaffold remains for later:

```bash
# Requires arm-none-eabi-gcc (MSYS2: pacman -S mingw-w64-x86_64-arm-none-eabi-{binutils,gcc,newlib})
make -f platform/gwhb/Makefile.gwhb
make -f platform/gwhb/Makefile.gwhb test-header   # after binary exists
```

Output: `build-gwhb/CUPCAKE.bin` — a **single self-contained file** (code + embedded atlas, bezel, audio). Copy only that to `/roms/homebrew/` on SD. See [GWHB_SPRINT_BOARD.md](../GWHB_SPRINT_BOARD.md).

Attribution for bundled itch.io assets: [ATTRIBUTION.md](ATTRIBUTION.md).

Until `platform/gwhb/main_gwhb.c` exists, the makefile only verifies the toolchain.

### GitHub Actions

On push/PR, [.github/workflows/build.yml](../.github/workflows/build.yml) runs:

1. **Unit tests** — `make test-smoke`, `test-state`, `test-rng`, `test-timer` on Ubuntu.
2. **GWHB build** — `arm-none-eabi-gcc` + `Makefile.gwhb`; uploads `CUPCAKE.bin` as an artifact when the GWHB host is implemented.

Tag `v*` releases attach `CUPCAKE.bin` + attribution docs via GitHub Releases (when the GWHB host is implemented). Download from Actions → **cupcake-gwhb-*** — one file to copy to SD.

## Asset pipeline

Assets are not committed to git. After obtaining a HAR capture from itch.io:

```bash
python tools/extract_har.py path/to/capture.har
python tools/extract_audio.py
python tools/gen_sprites.py
python tools/gen_lcd_positions.py
python tools/gen_demo_data.py
```

Extract output goes to `ignore/har_extracted/`; audio copies go to `assets/audio/`. See [HAR.md](HAR.md) and [AUDIO.md](AUDIO.md).

## Host comparison

| | **C + SDL** | **Browser / WebView** |
|---|-------------|------------------------|
| Placement | 2D projection of mesh + masks | Exact Three.js + camera |
| Target | retro-go handheld, low RAM | PC dev, fidelity reference |
| Logic | `cupcake_game.c` | Original JS in RetroFab |

For pixel-perfect reference, run the original in a browser: [HOST_WEB.md](HOST_WEB.md).

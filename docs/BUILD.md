# Build guide

## PC (SDL2) — primary dev target

**Requirements:** gcc, SDL2 dev libs, make.

On Windows, use MSYS2 — see [BUILD_WINDOWS.md](BUILD_WINDOWS.md) for full setup.

From the project root (MINGW64 shell on Windows):

```bash
./build.sh          # MSYS2: make is installed as mingw32-make — see BUILD_WINDOWS.md
./build.sh run
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

Integrate into your retro-go-sd firmware fork using the Celeste overlay model. See [OVERLAY.md](OVERLAY.md) and [OVERLAY_SPRINT_BOARD.md](../OVERLAY_SPRINT_BOARD.md).

After shared-code edits: `./build.sh test-overlay-regression` (PC tests; linux emu when firmware tree is available).

### Linux emu (regression / dev)

Requires retro-go `linux/` tree + SDL2. **Initialize firmware submodules first** (linux emu headers live in `retro-go-stm32`):

```bash
cd ../game-and-watch-retro-go-sd-cupcake
git submodule update --init retro-go-stm32
```

From firmware `linux/` (or via `mingw32-make test-overlay-regression` in the port repo):

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
cd "$RETROGO_FW/linux"   # or ../game-and-watch-retro-go-sd-cupcake/linux
mingw32-make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

Output: `build-cupcake/retro-go-cupcake.elf`

Copy `assets/sprites-color.png`, `assets/screen.jpg` (and fallback `sprites.png`) to `/home/odroid/cupcake/` on the device SD layout your firmware expects (same pattern as other linux emu apps).

Install the produced ELF like Celeste (`retro-go-celeste`).

### Device overlay (hardware)

Celeste model — filename **`cupcake`**, not GWHB magic:

1. Build firmware from `game-and-watch-retro-go-sd-cupcake` (standalone; ships a stub `cupcake.bin`).
2. When the full port overlay is ready, build `cupcake.bin` from this repo and copy to `/roms/homebrew/cupcake.bin` on SD.
3. Launch **Homebrew → cupcake** (stub runs briefly, then returns to menu until replaced).

Game updates that change the overlay binary only require replacing `cupcake.bin` on SD (same linker layout). GWHB SD-drop is a later option.

Assets are embedded at build time (`cupcake_data.h`), not loose files on SD.

## GWHB (deferred)

Not used this round. Scaffold under `platform/gwhb/` remains for a future pass — see [GWHB_SPRINT_BOARD.md](../GWHB_SPRINT_BOARD.md).

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

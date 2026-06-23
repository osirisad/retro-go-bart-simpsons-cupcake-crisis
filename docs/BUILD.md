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

**Controls:** Arrow keys, **Z** = Action, **X** = Select, **Esc** = quit, **F3/F4** = save/load state. Full mapping: [INPUT_MAPPING.md](INPUT_MAPPING.md).

### Unit tests

```bash
make test-all
# or individual targets: make test-state, make test-bart-move, etc.
```

## retro-go / Game & Watch

Build the linux emu binary from your retro-go firmware tree:

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

Output: `build-cupcake/retro-go-cupcake.elf`

Copy `assets/sprites-color.png` (and fallback `sprites.png`) to `/home/odroid/cupcake/` on the device SD layout your firmware expects (same pattern as other linux emu apps).

Install the produced ELF like Celeste (`retro-go-celeste`).

## Asset pipeline

Assets are not committed to git. After obtaining a HAR capture from itch.io:

```bash
python tools/extract_har.py path/to/capture.har
python tools/gen_sprites.py
python tools/gen_lcd_positions.py
python tools/gen_demo_data.py
```

Extract output goes to `ignore/har_extracted/`. See [HAR.md](HAR.md).

## Host comparison

| | **C + SDL** | **Browser / WebView** |
|---|-------------|------------------------|
| Placement | 2D projection of mesh + masks | Exact Three.js + camera |
| Target | retro-go handheld, low RAM | PC dev, fidelity reference |
| Logic | `cupcake_game.c` | Original JS in RetroFab |

For pixel-perfect reference, run the original in a browser: [HOST_WEB.md](HOST_WEB.md).

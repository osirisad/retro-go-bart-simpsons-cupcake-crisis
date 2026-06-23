# Bart Simpson's Cupcake Crisis — Retro-Go port

Port of the [RetroFab Acclaim SuperPlay simulation](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis) to a standalone C game, integrated into [retro-go](https://github.com/ducke1937/retro-go) like [Celeste Classic](https://github.com/lemon-sherbet/ccleste).

## How Celeste was ported (your reference)

Celeste is **not** “Pico-8 ROM loaded at runtime.” The pipeline is:

1. **Original**: Pico-8 cartridge (Lua).
2. **[ccleste](https://github.com/lemon-sherbet/ccleste)**: Game logic hand-translated to portable C (`celeste.c` / `celeste.h`). No SDL, no heap in the game core — only callbacks for drawing, audio, and input.
3. **retro-go** (`linux/celeste/main.c`): Thin platform layer that implements those callbacks (RGB565 framebuffer, SDL audio, odroid input, save states).

The game calls `Celeste_P8_init()` / `update()` / `draw()`; the host implements `p8_spr`, `p8_btn`, etc.

## How Cupcake Crisis is different

This itch release is a **RetroFab 0.9.x web simulator**:

| Piece | Format |
|-------|--------|
| Engine | ~1 MB minified JavaScript (`build.js`) — Three.js + RetroFab |
| Game class | `AcclaimCupcakeCrisis` (+ `Bart`, `Cupcakes`, `Maggie`, …) in JS |
| Assets | `sprites.png` (1024×1024), `screen.jpg`, MP3/WAV in `game/audio/` |
| Layout | `sprites.json` (Three.js mesh + UVs per sprite name) |
| Demo TAS | `demo.model` — frame lists of visible sprite names |

There is **no** small Lua cart or single ROM to drop in. A Celeste-style port means **reimplementing the game rules in C**, using the HAR/assets as the source of truth for art and timing.

Extracted reference:

- `docs/AcclaimCupcakeCrisis.js` — game + entity logic sliced from `build.js`
- `docs/GAME_LOGIC.md` — readable summary
- `ignore/har_extracted/` — full HAR unpack (models, audio, JSON); gitignored
- `assets/` — copies of sprites, screen, audio, `build.js`

## Recommended architecture (mirror ccleste)

```
src/
  cupcake.h          # Public API: init / update / draw / input / savestate
  cupcake_game.c     # Portable game logic (port from JS)
  cupcake_sprites.h  # Generated sprite UV → pixel rects
platform/
  main_sdl.c         # PC test build (optional)
  main_retrogo.c     # Copy/adapt from game-and-watch-retro-go-sd/linux/celeste/main.c
tools/
  extract_har.py
  gen_sprites.py
```

**Phase 1 — Assets & host (you are here)**  
Extract sprites/audio, generate `cupcake_sprites.h`, stub `cupcake_game.c`, SDL smoke test that blits `screen.jpg` + one sprite.

**Phase 2 — Core loop**  
Port state machine: `demo` → `start` → `play` → `over`. Timer tick rates from JS (`onPhaseStart` speed tables for level 1/2). Input: Left/Right/Up/Down/Action/Select.

**Phase 3 — Entities**  
Port in order: `Bart` (positions 0–5, catch/throw), `Cupcakes` grid, `Aircakes`, `Maggie`, `Marge`, `Couch`, `Pacifier`, miss/lives, scoreboard.

**Phase 4 — retro-go**  
Add `linux/cupcake/` + `Makefile.cupcake`, wire odroid audio/display like Celeste (`APP_ID`, save states).

**Phase 5 — Validation**  
Replay `demo.model` frames against your renderer; compare to browser RetroFab.

## Controls (from RetroFab)

| Button | Role |
|--------|------|
| Left / Right | Move Bart |
| Up / Down (at couch) | Sit / bonus |
| Action | Start / throw at Marge |
| Select | Cycle level in demo; continue after game over |

## Legal

`license.txt` in the parent folder applies to RetroFab assets. This port is for personal/educational use on your own hardware; do not redistribute Acclaim/Simpsons assets without rights clearance.

## Dual-target build (PC + retro-go)

The **same** `src/cupcake_game.c` links into two hosts:

| Host | File | Output |
|------|------|--------|
| **PC (SDL2)** | `platform/sdl/main.c` | `build-pc/cupcake-sdl` |
| **retro-go linux emu** | `platform/retrogo/main.c` | `build-cupcake/retro-go-cupcake.elf` |

Shared drawing: `platform/host_draw.c` (sprite blit from atlas).

### PC test (do this first)

**Requirements:** gcc, SDL2 dev libs. Easiest on Windows: [MSYS2](https://www.msys2.org/) then:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 make
```

From the port directory (MINGW64 shell):

```bash
make
make run
# or: run-pc.bat
```

**Controls:** Arrow keys, **Z** = Action, **X** = Select, **Esc** = quit, **F3/F4** = save/load state.

Set `CUPCAKE_ASSETS=assets` if you run the binary from another cwd.

### retro-go / G&W

From `game-and-watch-retro-go-sd/linux/`:

```bash
export CUPCAKE_PORT=/c/path/to/bart_simpson_cupcake_crisis_port
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

Copy `assets/sprites-color.png` (and fallback `sprites.png`) to `/home/odroid/cupcake/` on the device SD layout your firmware expects (same pattern as other linux emu apps).

Install the produced `retro-go-cupcake.elf` like Celeste (`retro-go-celeste`).

## Quick commands

```bash
python tools/extract_har.py "../html-classic.itch.zone_Archive [26-05-29 19-54-45].har"
python tools/gen_sprites.py
make && make run
```

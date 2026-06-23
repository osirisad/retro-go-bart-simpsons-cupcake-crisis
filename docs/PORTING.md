# Porting guide

How this port relates to Celeste on retro-go, what the original game looks like under the hood, and the planned architecture.

## Celeste reference pipeline

Celeste is **not** a Pico-8 ROM loaded at runtime. The pipeline is:

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

## Reference files

| Path | Purpose |
|------|---------|
| `docs/AcclaimCupcakeCrisis.js` | Game + entity logic sliced from `build.js` |
| `docs/GAME_LOGIC.md` | Readable summary of rules and state machine |
| `ignore/har_extracted/` | Full HAR unpack (models, audio, JSON); gitignored |
| `assets/` | Local copies of sprites, screen, audio (gitignored) |

See also [HAR.md](HAR.md), [SPRITES.md](SPRITES.md), [HOST_WEB.md](HOST_WEB.md).

## Architecture

```
src/
  cupcake.h          # Public API: init / update / draw / input / savestate
  cupcake_game.c     # Portable game logic (port from JS)
  cupcake_sprites.h  # Generated sprite UV → pixel rects
platform/
  sdl/main.c         # PC test build
  retrogo/main.c     # retro-go linux emu host
tools/
  extract_har.py
  gen_sprites.py
```

The same `src/cupcake_game.c` links into both PC (SDL2) and retro-go hosts. Shared drawing lives in `platform/host_draw.c`.

## Port phases

**Phase 1 — Assets & host**  
Extract sprites/audio, generate `cupcake_sprites.h`, stub `cupcake_game.c`, SDL smoke test that blits `screen.jpg` + one sprite.

**Phase 2 — Core loop**  
Port state machine: `demo` → `start` → `play` → `over`. Timer tick rates from JS (`onPhaseStart` speed tables for level 1/2). Input: Left/Right/Up/Down/Action/Select.

**Phase 3 — Entities**  
Port in order: `Bart` (positions 0–5, catch/throw), `Cupcakes` grid, `Aircakes`, `Maggie`, `Marge`, `Couch`, `Pacifier`, miss/lives, scoreboard.

**Phase 4 — retro-go**  
Wire odroid audio/display like Celeste (`APP_ID`, save states). See [BUILD.md](BUILD.md).

**Phase 5 — Validation**  
Replay `demo.model` frames against the renderer; compare to browser RetroFab.

Progress tracking: [SPRINT_BOARD.md](../SPRINT_BOARD.md).

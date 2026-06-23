# Sprite / atlas pipeline (RetroFab vs lcdgame.js)

## Two different engines

| | **RetroFab / Cupcake Crisis** | **lcdgame.js** (Highway, etc.) |
|---|-------------------------------|--------------------------------|
| Author | itizso RetroFab (`build.js`) | Bas de Reuver (`lcdgame.js`) |
| Config | `sprites.json` + `game.model` | `highway.json` style frames |
| Atlas | `sprites.png` + `sprites-color.png` (1024×1024) | `*_shapes.png` packed sheet |
| Screen position | 3D mesh on LCD plane (Three.js) | `spriteSourceSize.x/y` in JSON |
| Crop rect | Per-material **mesh UV triangles** | `frame.x/y/w/h` in JSON |
| Runtime | `Spritesheet.show(['bart8','cake04'])` toggles material.visible | `shapeDraw()` blits frame → screen |

Cupcake Crisis **does not** use lcdgame.js JSON. Dropping `lcdgame.js` here is useful as a **reference** for how classic LCD sims work, not as a drop-in atlas for this port.

## What `build.js` does (from `Spritesheet.init`)

1. Load `sprites.json` geometry (same file in `ignore/har_extracted/.../game/sprites.json`).
2. Material `0` = base `"sprites"` texture; every other material shares that map.
3. `game.model` skins: `"original": "sprites.png"`, `"color": "sprites-color.png"`.
4. `setSkin('original'|'color')` swaps the shared texture; **UVs stay the same**.
5. `show(['cake01', …])` sets `material.visible` by **DbgName** (`cake01`, `bart8`, …).

So the authoritative mapping is: **material name → UV triangles in `sprites.json`**, not random atlas guesses.

See **`docs/HAR.md`** for what your itch.io HAR actually contains (JSON yes, atlas PNGs usually no).

**Critical:** `sprites.json` uses Three.js **face bitmasks** (`42` = tri+material+UVs, `43` = quad+…). Tools must parse the full `faces` array — not treat `42` as a literal triangle tag. Wrong parsing was the main cause of “cake4 shows Bart” export bugs (now fixed in `tools/sprite_raster.py`).

## What the port should use

1. **Atlas:** `assets/sprites-color.png` for RGB; **triangle masks** in `src/cupcake_sprite_masks.h` (from `tools/gen_sprites.py`) so blits match `test/export_sprites.py` even when atlas bboxes overlap.
2. **Rects:** `tools/gen_sprites.py` writes `src/cupcake_sprites.h` (largest connected triangle region per material).
3. **Do not** use axis-aligned bbox of UV corners only — many materials share UV vertices.
4. **`cake43` is not a cupcake bitmap** — its only mesh triangle samples the score digits **"80"** on the atlas (`tools/debug_sprites_png_cake43.png`). The browser still uses that material name for gameplay, but our 2D blit of that rect will always look like digits. Use **`cake04`**, **`cake31`**, **`cake8`**, etc. from `demo.model` for cupcake tests.
4. **Positions:** Game code uses LCD coordinates; long-term, align with `demo.model` frame lists and entity positions from `docs/GAME_LOGIC.md` / `AcclaimCupcakeCrisis.js`.

## Regenerate rects

```bash
make gen   # gen_sprites.py, gen_masked_atlas.py, gen_lcd_positions.py, gen_demo_data.py
python tools/audit_sprites.py   # optional: luminance check on color sheet
```

## lcdgame.js (Highway example)

`lcdgame.js/games/highway/js/highway.json`:

```json
{"filename":"car1","frame":{"x":325,"y":0,"w":73,"h":50},
 "spriteSourceSize":{"x":414,"y":503,"w":73,"h":50}}
```

- `frame` = region in the packed **shapes** texture.
- `spriteSourceSize` = where to draw on the **background** photo.

RetroFab instead draws a 3D sheet with per-sprite materials; there is no `spriteSourceSize` in the HAR for Cupcake Crisis.

## Files

| File | Role |
|------|------|
| `ignore/har_extracted/.../sprites.json` | UVs + materials (from HAR) |
| `assets/sprites-color.png` | **Port default** atlas (PC + retro-go) |
| `assets/sprites.png` | Green LCD skin (RetroFab `"original"`; not used by port) |
| `game/demo.model` | Per-frame sprite name list (validation) |
| `docs/AcclaimCupcakeCrisis.js` | Game logic slice from `build.js` |

# Sprite UV test exports

Verify sprite mapping **without** rebuilding `cupcake-sdl.exe`.

## Run (pick one)

**Easiest — double-click:**

`test/run_export.bat`

**Or in a terminal** from `bart_simpson_cupcake_crisis_port`:

```bash
python test/export_sprites.py
```

## What to open

| File | What it is |
|------|------------|
| **`test/output/sprites_png/contact_sheet.png`** | All exported sprites in one grid (best quick check) |
| `test/output/sprites-color_png/contact_sheet.png` | Same on the color atlas |
| `test/output/sprites_png/raster/cake04.png` | Single sprite crop |
| `test/output/index.html` | Browser gallery (needs PNGs beside it) |

Green labels in the contact sheet = names that appear in `demo.model`.

## What each export means

- **raster/** = crop using UV **triangles** from `sprites.json` (same as `tools/gen_sprites.py`).
- All 96 materials should appear after the Three.js face-bitmask fix in `tools/sprite_raster.py` (older runs only showed ~41).
- See `docs/HAR.md` — your HAR has `sprites.json` but usually not the atlas PNGs.

## If the gallery shows no images

The previous run was interrupted — only `index.html` was written, not the PNGs. Re-run `run_export.bat` and confirm you see e.g. **80+ PNG files** in the terminal summary.

See `docs/SPRITES.md` for RetroFab vs lcdgame.js.

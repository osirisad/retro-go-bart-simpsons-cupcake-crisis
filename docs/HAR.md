# What is in the Cupcake Crisis HAR?

File: `html-classic.itch.zone_Archive [26-05-29 19-54-45].har` (repo root)

## Yes — there is a sprite mapping file

| URL in HAR | Size | Role |
|------------|------|------|
| `.../sim-acclaim-cupcakecrisis/game/sprites.json` | ~90 KB | **The mapping file** — Three.js mesh + UVs + materials |
| `.../game/game.model` | ~4 KB | Loads `Spritesheet` + atlas filenames |
| `.../game/demo.model` | ~9 KB | Per-frame **material names** to show (not bitmap rects) |

`sprites.json` was extracted correctly to:

`ignore/har_extracted/sim-acclaim-cupcakecrisis/game/sprites.json`

## No — not lcdgame-style frames

Cupcake Crisis does **not** ship JSON like Highway’s `highway.json` (`frame` + `spriteSourceSize`).
RetroFab uses:

- One **1024×1024** atlas (`sprites.png` / `sprites-color.png`)
- One **3D LCD plane mesh** in `sprites.json` (240 vertices, 341 faces)
- **Material names** (`cake4`, `bart2`, …) = which mesh faces are visible

Runtime: `Spritesheet.show(['cake4','bart2'])` toggles `material.visible` — it does **not** blit a rectangle from a sheet.

## Missing from this HAR (important)

The capture has **no PNG responses** for:

- `sprites.png`
- `sprites-color.png`
- `screen.jpg`

Only `sprites.json` and other JSON/models/audio were recorded. Atlas files must be saved separately (DevTools → Network → save image, or re-export HAR after a full load with cache disabled).

Port copies live in `assets/sprites.png` and `assets/sprites-color.png`.

## Face parsing bug (fixed in tools)

`sprites.json` uses **Three.js JSON Model format 3** face bitmasks (`42`, `43`, `40`, …).
The number `42` is **not** “triangle opcode + material 42”; it means triangle + material + vertex UV indices + normals.

Early port tools mis-parsed this and produced wrong atlas crops (e.g. `cake4` looked like random Bart chunks).

Use the updated `tools/sprite_raster.py` (proper face walk) before trusting `gen_sprites.py` / `test/export_sprites.py`.

## Materials in demo vs geometry

`demo.model` names like `bart2`, `cake21` **do** have faces in `sprites.json` when parsed correctly (e.g. `bart2`: 6 faces, `cake21`: 3). They were missing from the old exporter because only a broken subset of face records was read.

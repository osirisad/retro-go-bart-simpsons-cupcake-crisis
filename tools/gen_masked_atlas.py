#!/usr/bin/env python3
"""
Build assets/sprites-draw.png — same layout as the color atlas but pixels outside
each material's UV triangles are transparent (matches test/export_sprites.py crops).

The C port should load sprites-draw.png for rendering, not the raw color sheet.
"""
from __future__ import annotations

import json
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from sprite_raster import (  # noqa: E402
    ATLAS_H,
    ATLAS_W,
    PAD,
    build_mask,
    collect_triangles,
    crop_masked,
    largest_cc_bbox,
)

ROOT = os.path.join(os.path.dirname(__file__), "..")
SPRITES_JSON = os.path.join(
    ROOT, "ignore", "har_extracted", "sim-acclaim-cupcakecrisis", "game", "sprites.json"
)
SRC = os.path.join(ROOT, "assets", "sprites-color.png")
OUT = os.path.join(ROOT, "assets", "sprites-draw.png")
MAX_SPRITE_AREA = 120000
MIN_VISIBLE_PIXELS = 30


def main() -> None:
    data = json.load(open(SPRITES_JSON, encoding="utf-8"))
    atlas = Image.open(SRC).convert("RGBA")
    tris = collect_triangles(data)
    out = Image.new("RGBA", (ATLAS_W, ATLAS_H), (0, 0, 0, 0))
    n = 0

    for name in sorted(tris.keys()):
        if name == "sprites":
            continue
        mask = build_mask(atlas, tris[name])
        box = largest_cc_bbox(mask)
        if not box:
            continue
        x, y, w, h = box
        if w * h < MIN_VISIBLE_PIXELS or w * h > MAX_SPRITE_AREA:
            continue
        x0, y0 = max(0, x - PAD), max(0, y - PAD)
        im = crop_masked(atlas, box, mask)
        out.paste(im, (x0, y0), im)
        n += 1

    out.save(OUT)
    print(f"Wrote {OUT} ({n} masked regions, {ATLAS_W}x{ATLAS_H})")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Compare generated LCD positions vs mesh-extent formula."""
import json
import re
from pathlib import Path

from PIL import Image

from sprite_raster import (
    build_mask,
    collect_mesh_lcd_extents,
    collect_triangles,
    largest_cc_bbox,
    mask_centroid,
)

ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / "ignore/har_extracted/sim-acclaim-cupcakecrisis/game/sprites.json").read_text())
atlas = Image.open(ROOT / "assets/sprites-color.png").convert("RGBA")
tris = collect_triangles(data)
verts = data["vertices"]
n = len(verts) // 3
xs = [verts[i * 3] for i in range(n)]
ys = [verts[i * 3 + 1] for i in range(n)]
xmin, xmax = min(xs), max(xs)
ymin, ymax = min(ys), max(ys)
ext = collect_mesh_lcd_extents(data, xmin, xmax, ymin, ymax, 1024, 800)

text = (ROOT / "src/cupcake/cupcake_sprite_lcd.h").read_text()
lcd = {m.group(1): (int(m.group(2)), int(m.group(3))) for m in re.finditer(r'"([^"]+)", (\d+), (\d+)', text)}
spr = {
    m.group(1): (int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5)))
    for m in re.finditer(r'"([^"]+)", (\d+), (\d+), (\d+), (\d+)', (ROOT / "src/cupcake/cupcake_sprites.h").read_text())
}

for name in ["bart2", "maggie0", "cake31", "cake32", "cake33", "bart3"]:
    min_lx, min_ly, max_lx, max_ly = ext[name]
    mcx, mcy = (min_lx + max_lx) // 2, (min_ly + max_ly) // 2
    bx, by, bw, bh = spr[name]
    box = largest_cc_bbox(build_mask(atlas, tris[name]))
    mc = mask_centroid(box, build_mask(atlas, tris[name]))
    mask_cx, mask_cy = mc
    calc_x = int(mcx - (mask_cx - bx) + 0.5)
    calc_y = int(mcy - (mask_cy - by) + 0.5)
    lx, ly = lcd[name]
    print(f"{name}: hdr({lx},{ly}) calc({calc_x},{calc_y}) delta({lx-calc_x},{ly-calc_y})")

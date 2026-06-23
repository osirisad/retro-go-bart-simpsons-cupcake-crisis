#!/usr/bin/env python3
"""Audit generated LCD positions vs mesh-centroid formula (TASK-45)."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

from PIL import Image

from sprite_raster import (
    build_mask,
    collect_mesh_lcd_extents,
    collect_triangles,
    mask_centroid,
    sprite_draw_bbox,
)

ROOT = Path(__file__).resolve().parents[1]
SPRITES_JSON = ROOT / "ignore/har_extracted/sim-acclaim-cupcakecrisis/game/sprites.json"
DEMO_MODEL = ROOT / "ignore/har_extracted/sim-acclaim-cupcakecrisis/game/demo.model"
LCD_HDR = ROOT / "src/cupcake_sprite_lcd.h"
SPR_HDR = ROOT / "src/cupcake_sprites.h"
ATLAS = ROOT / "assets/sprites-color.png"
TUNE = ROOT / "assets/lcd_tune.txt"

HOST_MAX_W = 400
HOST_MAX_H = 560
MAX_DELTA = 1

# Hand-tuned in tools/gen_lcd_positions.py (intentional mesh-formula overrides).
MANUAL_LCD = {
    "marge1",
    "maggie0",
    "maggie1",
    "maggie2",
    "maggie3",
}


def load_lcd() -> dict[str, tuple[int, int]]:
    text = LCD_HDR.read_text(encoding="utf-8")
    return {
        m.group(1): (int(m.group(2)), int(m.group(3)))
        for m in re.finditer(r'"([^"]+)", (\d+), (\d+)', text)
    }


def load_spr() -> dict[str, tuple[int, int, int, int]]:
    text = SPR_HDR.read_text(encoding="utf-8")
    return {
        m.group(1): (int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5)))
        for m in re.finditer(r'"([^"]+)", (\d+), (\d+), (\d+), (\d+)', text)
    }


def load_tune() -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    if not TUNE.exists():
        return out
    for line in TUNE.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) >= 3:
            out[parts[0]] = (int(parts[1]), int(parts[2]))
    return out


def demo_sprite_names() -> set[str]:
    if not DEMO_MODEL.exists():
        return set()
    names = set(re.findall(r'"([a-zA-Z][a-zA-Z0-9_]*)"', DEMO_MODEL.read_text(encoding="utf-8")))
    names -= {"gameplay", "rate", "type"}
    return names


def calc_lcd(name: str, ext: dict, tris: dict, atlas: Image.Image, spr: dict) -> tuple[int, int] | None:
    if name not in ext or name not in tris or name not in spr:
        return None
    min_lx, min_ly, max_lx, max_ly = ext[name]
    mcx, mcy = (min_lx + max_lx) // 2, (min_ly + max_ly) // 2
    bx, by, bw, bh = spr[name]
    box = sprite_draw_bbox(build_mask(atlas, tris[name]))
    if not box:
        return None
    mask_cx, mask_cy = mask_centroid(box, build_mask(atlas, tris[name]))
    calc_x = int(mcx - (mask_cx - bx) + 0.5)
    calc_y = int(mcy - (mask_cy - by) + 0.5)
    return calc_x, calc_y


def host_rect_ok(name: str, spr: dict[str, tuple[int, int, int, int]]) -> bool:
    if name not in spr:
        return False
    _, _, w, h = spr[name]
    return 1 <= w <= HOST_MAX_W and 1 <= h <= HOST_MAX_H


def main() -> int:
    if not SPRITES_JSON.exists():
        print(f"missing {SPRITES_JSON}", file=sys.stderr)
        return 1

    data = json.loads(SPRITES_JSON.read_text(encoding="utf-8"))
    atlas = Image.open(ATLAS).convert("RGBA")
    tris = collect_triangles(data)
    verts = data["vertices"]
    n = len(verts) // 3
    xs = [verts[i * 3] for i in range(n)]
    ys = [verts[i * 3 + 1] for i in range(n)]
    ext = collect_mesh_lcd_extents(data, min(xs), max(xs), min(ys), max(ys), 1024, 800)

    lcd = load_lcd()
    spr = load_spr()
    tune = load_tune()
    names = sorted(demo_sprite_names())

    bad_delta: list[str] = []
    bad_rect: list[str] = []
    missing: list[str] = []

    print(f"Demo sprites: {len(names)}")
    for name in names:
        if name not in lcd or name not in spr:
            missing.append(name)
            continue
        if not host_rect_ok(name, spr):
            _, _, w, h = spr[name]
            bad_rect.append(f"{name} {w}x{h}")
            continue
        if name in tune or name in MANUAL_LCD:
            continue
        calc = calc_lcd(name, ext, tris, atlas, spr)
        if not calc:
            bad_delta.append(f"{name}: no calc")
            continue
        lx, ly = lcd[name]
        dx, dy = lx - calc[0], ly - calc[1]
        if abs(dx) > MAX_DELTA or abs(dy) > MAX_DELTA:
            bad_delta.append(f"{name}: hdr({lx},{ly}) calc({calc[0]},{calc[1]}) delta({dx},{dy})")

    if missing:
        print("\nMissing from headers:")
        for line in missing:
            print(f"  {line}")

    if bad_rect:
        print("\nhost_sprite_rect_ok failures:")
        for line in bad_rect:
            print(f"  {line}")

    if bad_delta:
        print(f"\nLCD delta > {MAX_DELTA}px (excluding lcd_tune.txt overrides):")
        for line in bad_delta:
            print(f"  {line}")

    ok = not missing and not bad_rect and not bad_delta
    print("\nPASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())

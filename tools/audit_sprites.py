#!/usr/bin/env python3
"""Score sprite rects against the LCD atlas (default: assets/sprites-color.png)."""
from __future__ import annotations

import re
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "src" / "cupcake" / "cupcake_sprites.h"
SHEET = ROOT / "assets" / (sys.argv[1] if len(sys.argv) > 1 else "sprites-color.png")


def main() -> None:
    im = Image.open(SHEET).convert("RGBA")
    text = HDR.read_text(encoding="utf-8")
    rows: list[tuple] = []
    for m in re.finditer(r'\{ "([^"]+)", (\d+), (\d+), (\d+), (\d+) \}', text):
        name = m.group(1)
        x, y, w, h = (int(m.group(i)) for i in range(2, 6))
        area = w * h
        if area > 120000 or area < 100:
            continue
        c = im.crop((x, y, x + w, y + h))
        op = [p for p in c.getdata() if p[3] > 40]
        if len(op) < 50:
            continue
        avg = tuple(sum(c) // len(op) for c in zip(*op))
        lum = avg[0] + avg[1] + avg[2]
        rows.append((lum, len(op), name, w, h, x, y, avg))

    rows.sort(reverse=True)
    print(f"Top visible on {SHEET.name}:")
    for r in rows[:40]:
        print(f"  {r[2]:12} {r[3]:4}x{r[4]:<4} @({r[5]:4},{r[6]:3}) lum={r[0]:4}")

    print("\ncake* sprites:")
    for r in rows:
        if r[2].startswith("cake"):
            print(f"  {r[2]:12} {r[3]:4}x{r[4]:<4} @({r[5]:4},{r[6]:3}) lum={r[0]:4}")


if __name__ == "__main__":
    main()

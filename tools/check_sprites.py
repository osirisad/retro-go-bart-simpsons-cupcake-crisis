#!/usr/bin/env python3
"""List sprites with visible pixels on an atlas sheet."""
import re
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "src" / "cupcake" / "cupcake_sprites.h"


def main() -> None:
    sheet = ROOT / "assets" / (sys.argv[1] if len(sys.argv) > 1 else "sprites-color.png")
    im = Image.open(sheet).convert("RGBA")
    text = HDR.read_text(encoding="utf-8")
    for m in re.finditer(r'\{ "([^"]+)", (\d+), (\d+), (\d+), (\d+) \}', text):
        name, x, y, w, h = m.group(1), *[int(m.group(i)) for i in range(2, 6)]
        if w * h > 80000:
            continue
        op = [p for p in im.crop((x, y, x + w, y + h)).getdata() if p[3] > 32]
        if len(op) < 80:
            continue
        avg = tuple(sum(c) // len(op) for c in zip(*op))
        if avg[0] + avg[1] + avg[2] > 40:
            print(f"{name:12} {w:4}x{h:<4} rgba={avg}")


if __name__ == "__main__":
    main()

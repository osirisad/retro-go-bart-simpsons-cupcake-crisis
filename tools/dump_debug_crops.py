#!/usr/bin/env python3
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1] / "assets"
OUT = Path(__file__).resolve().parent

CROPS = {
    "cake43": (819, 28, 101, 65),
    "cake04_mid": (692, 424, 93, 51),
    "cake31": (611, 451, 235, 135),
}

for sheet in ["sprites.png", "sprites-color.png"]:
    im = Image.open(ROOT / sheet).convert("RGBA")
    for name, (x, y, w, h) in CROPS.items():
        c = im.crop((x, y, x + w, y + h))
        out = OUT / f"debug_{sheet.replace('.', '_')}_{name}.png"
        c.save(out)
        print(out, c.size)

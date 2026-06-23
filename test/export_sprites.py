#!/usr/bin/env python3
"""
Export RetroFab sprite UV crops for visual review (no game build required).

  python test/export_sprites.py

Outputs under test/output/:
  sprites_png/raster/<name>.png       — triangle-masked crop (significant CC union)
  sprites_png/marked_atlas.png      — green=raster bbox, red=naive UV corner box
  sprites_png/contact_sheet.png     — thumbnail grid
  index.html                        — full gallery (all materials with UV faces)
"""
from __future__ import annotations

import json
import re
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from sprite_raster import (  # noqa: E402
    ATLAS_H,
    ATLAS_W,
    build_mask,
    collect_triangles,
    crop_masked,
    largest_cc_bbox,
    naive_uv_bbox,
    sprite_draw_bbox,
)

SPRITES_JSON = ROOT / "ignore" / "har_extracted" / "sim-acclaim-cupcakecrisis" / "game" / "sprites.json"
DEMO_MODEL = ROOT / "ignore" / "har_extracted" / "sim-acclaim-cupcakecrisis" / "game" / "demo.model"
OUT = Path(__file__).resolve().parent / "output"
THUMB = 96


def load_meta() -> tuple[dict, dict, set[str], list[str]]:
    data = json.loads(SPRITES_JSON.read_text(encoding="utf-8"))
    tris = collect_triangles(data)
    mat = {m["DbgIndex"]: m["DbgName"] for m in data["materials"] if m.get("DbgName")}
    all_names = sorted(n for n in mat.values() if n != "sprites")
    demo: set[str] = set()
    if DEMO_MODEL.exists():
        demo = set(re.findall(r'"([a-zA-Z][a-zA-Z0-9_]*)"', DEMO_MODEL.read_text(encoding="utf-8")))
        demo -= {"gameplay", "rate", "type"}
    return data, tris, demo, all_names


def fit_thumb(im: Image.Image, label: str) -> Image.Image:
    cell = Image.new("RGBA", (THUMB, THUMB + 14), (32, 32, 40, 255))
    if im.width < 1 or im.height < 1:
        return cell
    scale = min((THUMB - 8) / im.width, (THUMB - 8) / im.height, 4.0)
    nw, nh = max(1, int(im.width * scale)), max(1, int(im.height * scale))
    thumb = im.resize((nw, nh), Image.Resampling.NEAREST)
    ox, oy = (THUMB - nw) // 2, (THUMB - nh) // 2
    cell.paste(thumb, (ox, oy), thumb)
    draw = ImageDraw.Draw(cell)
    draw.text((4, THUMB + 1), label[:14], fill=(220, 220, 220))
    return cell


def draw_marked_atlas(atlas: Image.Image, entries: list[dict], out_path: Path) -> None:
    thumb = atlas.copy()
    thumb.thumbnail((512, 512), Image.Resampling.LANCZOS)
    sx = thumb.width / ATLAS_W
    sy = thumb.height / ATLAS_H
    draw = ImageDraw.Draw(thumb)
    for e in entries:
        if e.get("box"):
            x, y, w, h = e["box"]
            draw.rectangle(
                (x * sx, y * sy, (x + w) * sx, (y + h) * sy),
                outline=(80, 220, 80),
                width=1,
            )
        if e.get("naive"):
            x, y, w, h = e["naive"]
            draw.rectangle(
                (x * sx, y * sy, (x + w) * sx, (y + h) * sy),
                outline=(220, 60, 60),
                width=1,
            )
    thumb.save(out_path)


def export_atlas(atlas_key: str, atlas_path: Path, tris: dict, demo: set[str]) -> list[dict]:
    print(f"  loading {atlas_path.name}...")
    atlas = Image.open(atlas_path).convert("RGBA")
    out_base = OUT / atlas_key
    raster_dir = out_base / "raster"
    if raster_dir.exists():
        shutil.rmtree(raster_dir)
    raster_dir.mkdir(parents=True, exist_ok=True)

    entries: list[dict] = []
    thumbs: list[Image.Image] = []

    for n, name in enumerate(sorted(tris.keys())):
        triangles = tris[name]
        mask = build_mask(atlas, triangles)
        box = sprite_draw_bbox(mask)
        naive = naive_uv_bbox(triangles)
        entry: dict = {
            "name": name,
            "demo": name in demo,
            "faces": len(triangles),
            "box": box,
            "naive": naive,
        }
        entries.append(entry)

        if not box:
            continue
        im = crop_masked(atlas, box, mask)
        im.save(raster_dir / f"{name}.png")
        thumbs.append(fit_thumb(im, name))
        if (n + 1) % 10 == 0:
            print(f"    {n + 1}/{len(tris)}...")

    if thumbs:
        cols = 8
        rows = (len(thumbs) + cols - 1) // cols
        cw, ch = THUMB, THUMB + 14
        sheet = Image.new("RGBA", (cols * cw, rows * ch), (24, 24, 30, 255))
        for i, t in enumerate(thumbs):
            c, r = i % cols, i // cols
            sheet.paste(t, (c * cw, r * ch))
        sheet.save(out_base / "contact_sheet.png")
        print(f"  wrote {out_base / 'contact_sheet.png'} ({len(thumbs)} sprites)")

    draw_marked_atlas(atlas, entries, out_base / "marked_atlas.png")
    thumb_atlas = atlas.copy()
    thumb_atlas.thumbnail((512, 512), Image.Resampling.LANCZOS)
    thumb_atlas.save(out_base / "atlas_thumb.png")
    return entries


def write_html(
    atlas_results: dict[str, list[dict]], demo: set[str], all_names: list[str], tris: dict
) -> None:
    n_ok = sum(1 for es in atlas_results.values() for e in es if e.get("box"))
    no_geo = sorted(demo - set(tris.keys()))
    lines = [
        "<!DOCTYPE html><html><head><meta charset='utf-8'>",
        "<title>Sprite UV export</title>",
        "<style>",
        "body{font-family:Segoe UI,system-ui,sans-serif;background:#1e1e28;color:#eee;",
        "margin:16px;max-width:1400px}",
        "h1,h2{color:#ffb347} a{color:#9cf}",
        ".grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:10px}",
        ".card{background:#2d2d3a;border-radius:6px;padding:8px;border:1px solid #555}",
        ".card.demo{border-color:#6c6}",
        ".card img{width:100%;height:100px;object-fit:contain;object-position:center;",
        "background:#111;image-rendering:pixelated}",
        ".meta{font-size:11px;color:#aaa;margin-top:4px}",
        ".warn{color:#f88}.tag{font-size:10px;background:#363;color:#8f8;padding:1px 4px;border-radius:3px}",
        "</style></head><body>",
        "<h1>Sprite UV export</h1>",
        "<p>Generated by <code>test/export_sprites.py</code>. "
        "<span style='color:#6c6'>Green</span> = raster rect (largest connected triangle mask). "
        "<span style='color:#f66'>Red</span> = naive UV corner box (often wrong — digit bleed).</p>",
        f"<p>Materials in sprites.json: <b>{len(all_names)}</b>. "
        f"With mesh faces: <b>{len(tris)}</b>. "
        f"Referenced in demo.model: <b>{len(demo)}</b>. "
        f"Exported crops: <b>{n_ok}</b> per atlas.</p>",
    ]

    for key, entries in atlas_results.items():
        sheet = f"{key}/contact_sheet.png"
        marked = f"{key}/marked_atlas.png"
        thumb = f"{key}/atlas_thumb.png"
        lines.append(f"<h2>{key}</h2>")
        lines.append(
            f"<p><a href='{marked}'>Marked atlas</a> · "
            f"<a href='{sheet}'>contact_sheet.png</a> · "
            f"<a href='{thumb}'>atlas thumb</a></p>"
        )
        lines.append(
            f"<p><img src='{marked}' alt='marked' style='max-width:512px;"
            "image-rendering:pixelated;border:1px solid #555'></p>"
        )
        lines.append("<div class='grid'>")
        for e in entries:
            name = e["name"]
            cls = "card demo" if e.get("demo") else "card"
            lines.append(f"<div class='{cls}'>")
            lines.append(f"<div><b>{name}</b>")
            if e.get("demo"):
                lines.append(" <span class='tag'>demo</span>")
            lines.append("</div>")
            rel = f"{key}/raster/{name}.png"
            if e.get("box") and (OUT / rel).is_file():
                lines.append(f"<img src='{rel}' alt='{name}'>")
                x, y, w, h = e["box"]
                lines.append(f"<div class='meta'>raster {x},{y} {w}×{h} · {e['faces']} tri</div>")
                if e.get("naive"):
                    nx, ny, nw, nh = e["naive"]
                    lines.append(f"<div class='meta'>naive {nx},{ny} {nw}×{nh}</div>")
            else:
                lines.append("<div class='meta warn'>no raster (empty mask on atlas)</div>")
            lines.append("</div>")
        lines.append("</div>")

    if no_geo:
        lines.append("<h2>No UV geometry (in demo but not in sprites.json faces)</h2>")
        lines.append("<p class='meta'>")
        lines.append(", ".join(no_geo))
        lines.append("</p>")

    lines.append("</body></html>")
    html_path = OUT / "index.html"
    html_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {html_path}")


def main() -> None:
    print("Cupcake Crisis sprite export")
    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True, exist_ok=True)

    _, tris, demo, all_names = load_meta()
    print(f"Materials: {len(all_names)}, with UV faces: {len(tris)}, in demo: {len(demo)}")

    atlases = [
        ("sprites_png", ROOT / "assets" / "sprites.png"),
        ("sprites-color_png", ROOT / "assets" / "sprites-color.png"),
    ]
    results: dict[str, list[dict]] = {}
    for key, path in atlases:
        if not path.is_file():
            print(f"skip missing {path}")
            continue
        print(f"Export {key}:")
        results[key] = export_atlas(key, path, tris, demo)

    write_html(results, demo, all_names, tris)
    n_png = len(list(OUT.rglob("*.png")))
    print(f"\nDone: {n_png} PNG files in {OUT}")
    print(f"Open: {OUT / 'index.html'}")


if __name__ == "__main__":
    main()

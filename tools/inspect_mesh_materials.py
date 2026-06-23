#!/usr/bin/env python3
"""Print 3D mesh extent per material (sprites.json type-42 faces)."""
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPRITES_JSON = ROOT / "ignore" / "har_extracted" / "sim-acclaim-cupcakecrisis" / "game" / "sprites.json"


def main() -> None:
    d = json.loads(SPRITES_JSON.read_text(encoding="utf-8"))
    mat = {m["DbgIndex"]: m["DbgName"] for m in d["materials"] if m.get("DbgName")}
    verts = d["vertices"]
    faces = d["faces"]
    tris_by: dict[str, list] = defaultdict(list)
    i = 0
    while i < len(faces):
        if faces[i] == 42:
            mid, a, b, c = faces[i + 1], faces[i + 2], faces[i + 3], faces[i + 4]
            name = mat.get(mid, "?")
            tris_by[name].append((a, b, c))
            i += 5
        else:
            i += 1

    demo = [
        "bart2", "bart3", "cake3", "cake4", "cake8", "cake21", "cake31",
        "cake04", "bart8", "maggie0", "maggie1", "cake43",
    ]
    for probe in demo:
        if probe not in tris_by:
            print(f"{probe}: NO type-42 triangles in sprites.json")
            continue
        xs, ys, zs = [], [], []
        for a, b, c in tris_by[probe]:
            for vi in (a, b, c):
                xs.append(verts[vi * 3])
                ys.append(verts[vi * 3 + 1])
                zs.append(verts[vi * 3 + 2])
        print(
            f"{probe}: {len(tris_by[probe])} tri  "
            f"x=[{min(xs):.3f},{max(xs):.3f}] "
            f"y=[{min(ys):.3f},{max(ys):.3f}] "
            f"z=[{min(zs):.3f},{max(zs):.3f}]"
        )


if __name__ == "__main__":
    main()

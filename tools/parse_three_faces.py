#!/usr/bin/env python3
"""Parse Three.js JSON Model format 3 face array (io_three / sprites.json)."""
from __future__ import annotations

from collections import defaultdict


def parse_faces(faces: list[int]) -> list[dict]:
    """
    Face bitmask (legacy THREE.Geometry):
      bits 0-1: 0=tri, 1=quad
      bit 1 (2): material
      bit 3 (8): face normal
      bit 5 (32): vertex UV indices
    42 = 2+8+32 → triangle + material + face normal + vertex UVs
    """
    out: list[dict] = []
    i = 0
    n = len(faces)
    while i < n:
        face_type = faces[i]
        i += 1
        is_quad = (face_type & 1) == 1
        has_mat = (face_type & 2) == 2
        has_face_uv = (face_type & 4) == 4
        has_vert_uv = (face_type & 8) == 8
        has_face_norm = (face_type & 16) == 16
        has_vert_norm = (face_type & 32) == 32
        has_color = (face_type & 64) == 64
        has_vert_color = (face_type & 128) == 128

        nv = 4 if is_quad else 3
        verts = [faces[i + k] for k in range(nv)]
        i += nv

        entry: dict = {"type": face_type, "verts": verts}
        if has_mat:
            entry["material"] = faces[i]
            i += 1
        if has_face_uv:
            entry["face_uv"] = faces[i]
            i += 1
        if has_vert_uv:
            entry["uvs"] = [faces[i + k] for k in range(nv)]
            i += nv
        if has_face_norm:
            entry["face_normal"] = faces[i]
            i += 1
        if has_vert_norm:
            entry["normals"] = [faces[i + k] for k in range(nv)]
            i += nv
        if has_color:
            entry["color"] = faces[i]
            i += 1
        if has_vert_color:
            entry["colors"] = [faces[i + k] for k in range(nv)]
            i += nv
        out.append(entry)
    return out


def main() -> None:
    import json
    from pathlib import Path

    p = Path(__file__).resolve().parents[1] / "ignore/har_extracted/sim-acclaim-cupcakecrisis/game/sprites.json"
    d = json.loads(p.read_text(encoding="utf-8"))
    mat = {m["DbgIndex"]: m["DbgName"] for m in d["materials"] if "DbgIndex" in m}
    parsed = parse_faces(d["faces"])
    by_mat: dict[str, int] = defaultdict(int)
    for f in parsed:
        if "material" in f:
            by_mat[mat.get(f["material"], "?")] += 1
    print(f"Parsed {len(parsed)} faces (metadata says {d['metadata']['faces']})")
    demo = ["bart2", "bart3", "cake4", "cake21", "cake31", "cake04", "maggie0"]
    for name in demo:
        print(f"  {name}: {by_mat.get(name, 0)} faces")


if __name__ == "__main__":
    main()

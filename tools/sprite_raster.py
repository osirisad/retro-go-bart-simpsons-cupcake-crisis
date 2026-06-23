"""Shared UV triangle rasterization for gen_sprites.py and test/export_sprites.py."""
from __future__ import annotations

from collections import defaultdict

ATLAS_W = ATLAS_H = 1024
ALPHA = 32
MIN_PIX = 15
PAD = 3


def uv_to_px(u: float, v: float) -> tuple[int, int]:
    return int(u * ATLAS_W), int((1.0 - v) * ATLAS_H)


def _parse_three_faces(faces: list[int]) -> list[dict]:
    """Three.js JSON Model format 3 (io_three exporter in sprites.json)."""
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
        entry: dict = {"verts": verts}
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
            i += 1
        if has_vert_color:
            i += nv
        out.append(entry)
    return out


def _uv_points(uvs_flat: list[float], uv_indices: list[int]) -> list[tuple[int, int]]:
    pts: list[tuple[int, int]] = []
    for ui in uv_indices:
        pts.append(uv_to_px(uvs_flat[ui * 2], uvs_flat[ui * 2 + 1]))
    return pts


# RetroFab "play" camera from device.model (Acclaim Superplay).
_CAM_EYE = (0.0012220377894095727, 0.7099274179423528, 0.09400069403620462)
_CAM_TARGET = (0.001125870611699315, 0.0451916275473948, 0.005558898739937952)
_SPRITE_OFFSET = (0.0, 0.01, 0.0)
_CAM_FOV_DEG = 50.0
_CAM_NEAR = 0.01
_CAM_FAR = 50.0


def _vec3_sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _vec3_add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _vec3_scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def _vec3_dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _vec3_cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _vec3_norm(v):
    import math

    l = math.sqrt(_vec3_dot(v, v))
    if l < 1e-12:
        return (0.0, 0.0, 0.0)
    return (v[0] / l, v[1] / l, v[2] / l)


def _look_at(eye, target, up=(0.0, 1.0, 0.0)):
    """Camera basis (right, up, forward) — Three.js Y-up lookAt."""
    f = _vec3_norm(_vec3_sub(target, eye))
    r = _vec3_norm(_vec3_cross(f, up))
    u = _vec3_cross(r, f)
    return r, u, f, eye


def _project_ndc(
    v: tuple[float, float, float],
    eye: tuple[float, float, float],
    target: tuple[float, float, float],
    aspect: float,
) -> tuple[float, float] | None:
    """Perspective project one point to normalized device coords (-1..1)."""
    import math

    r, u, f, eye_p = _look_at(eye, target)
    rel = _vec3_sub(v, eye_p)
    cx = _vec3_dot(rel, r)
    cy = _vec3_dot(rel, u)
    cz = _vec3_dot(rel, f)
    if cz <= _CAM_NEAR:
        return None
    fov = math.radians(_CAM_FOV_DEG)
    f = 1.0 / math.tan(fov / 2.0)
    # NDC (Three.js style, cz is view-space depth toward target)
    ndc_x = (cx * f / aspect) / cz
    ndc_y = (cy * f) / cz
    if abs(ndc_x) > 4 or abs(ndc_y) > 4:
        return None
    return ndc_x, ndc_y


def _fit_ndc_to_lcd(
    ndc_pts: list[tuple[float, float]], lcd_w: int, lcd_h: int
) -> list[tuple[float, float]]:
    if not ndc_pts:
        return []
    xs = [p[0] for p in ndc_pts]
    ys = [p[1] for p in ndc_pts]
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)
    if xmax <= xmin or ymax <= ymin:
        return ndc_pts
    out: list[tuple[float, float]] = []
    for x, y in ndc_pts:
        lx = (x - xmin) / (xmax - xmin) * (lcd_w - 1)
        ly = (ymax - y) / (ymax - ymin) * (lcd_h - 1)
        out.append((lx, ly))
    return out


def collect_mesh_projected_lcd(
    data: dict,
    lcd_w: int = 1024,
    lcd_h: int = 554,
    eye: tuple[float, float, float] = _CAM_EYE,
    target: tuple[float, float, float] = _CAM_TARGET,
) -> dict[str, tuple[int, int, int, int]]:
    """Material -> LCD pixel bounds via play-camera perspective (not flat mesh X/Y)."""
    mat_name = {
        i: m["DbgName"]
        for i, m in enumerate(data.get("materials", []))
        if m.get("DbgName")
    }
    verts = data["vertices"]
    aspect = lcd_w / float(lcd_h)
    all_ndc: list[tuple[float, float]] = []
    per_mat_ndc: dict[str, list[tuple[float, float]]] = {}

    for face in _parse_three_faces(data["faces"]):
        mat_idx = face.get("material")
        if mat_idx is None:
            continue
        name = mat_name.get(mat_idx)
        if not name or name == "sprites":
            continue
        for vi in face["verts"]:
            wx = verts[vi * 3] + _SPRITE_OFFSET[0]
            wy = verts[vi * 3 + 1] + _SPRITE_OFFSET[1]
            wz = verts[vi * 3 + 2] + _SPRITE_OFFSET[2]
            ndc = _project_ndc((wx, wy, wz), eye, target, aspect)
            if not ndc:
                continue
            all_ndc.append(ndc)
            per_mat_ndc.setdefault(name, []).append(ndc)

    fitted_all = _fit_ndc_to_lcd(all_ndc, lcd_w, lcd_h)
    # Re-fit per material using global NDC bounds so coords share one space
    if not all_ndc:
        return {}
    xs = [p[0] for p in all_ndc]
    ys = [p[1] for p in all_ndc]
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)

    out: dict[str, tuple[int, int, int, int]] = {}
    for name, pts in per_mat_ndc.items():
        lxs: list[float] = []
        lys: list[float] = []
        for x, y in pts:
            lxs.append((x - xmin) / (xmax - xmin) * (lcd_w - 1))
            lys.append((ymax - y) / (ymax - ymin) * (lcd_h - 1))
        if not lxs:
            continue
        x0, x1 = int(min(lxs) + 0.5), int(max(lxs) + 0.5)
        y0, y1 = int(min(lys) + 0.5), int(max(lys) + 0.5)
        out[name] = (x0, y0, max(1, x1 - x0 + 1), max(1, y1 - y0 + 1))
    return out


def collect_mesh_lcd_extents(
    data: dict,
    xmin: float,
    xmax: float,
    ymin: float,
    ymax: float,
    lcd_w: int,
    lcd_h: int,
) -> dict[str, tuple[int, int, int, int]]:
    """Material -> (min_lx, min_ly, max_lx, max_ly) in LCD pixels."""
    mat_name = {
        i: m["DbgName"]
        for i, m in enumerate(data.get("materials", []))
        if m.get("DbgName")
    }
    verts = data["vertices"]
    acc: dict[str, list[int]] = {}

    def to_lcd(cx: float, cy: float) -> tuple[int, int]:
        lx = int((cx - xmin) / (xmax - xmin) * (lcd_w - 1) + 0.5)
        ly = int((ymax - cy) / (ymax - ymin) * (lcd_h - 1) + 0.5)
        return lx, ly

    for face in _parse_three_faces(data["faces"]):
        mat_idx = face.get("material")
        if mat_idx is None:
            continue
        name = mat_name.get(mat_idx)
        if not name or name == "sprites":
            continue
        if name not in acc:
            acc[name] = [lcd_w, lcd_h, 0, 0]
        a = acc[name]
        for vi in face["verts"]:
            lx, ly = to_lcd(verts[vi * 3], verts[vi * 3 + 1])
            a[0] = min(a[0], lx)
            a[1] = min(a[1], ly)
            a[2] = max(a[2], lx)
            a[3] = max(a[3], ly)
    return {k: (v[0], v[1], v[2], v[3]) for k, v in acc.items()}


def collect_mesh_centroids(data: dict) -> dict[str, tuple[float, float, float]]:
    """Material name -> centroid of mesh vertices used by its faces."""
    mat_name = {
        i: m["DbgName"]
        for i, m in enumerate(data.get("materials", []))
        if m.get("DbgName")
    }
    verts = data["vertices"]
    acc: dict[str, list[float]] = defaultdict(lambda: [0.0, 0.0, 0.0, 0.0])

    for face in _parse_three_faces(data["faces"]):
        mat_idx = face.get("material")
        if mat_idx is None:
            continue
        name = mat_name.get(mat_idx)
        if not name or name == "sprites":
            continue
        for vi in face["verts"]:
            acc[name][0] += verts[vi * 3]
            acc[name][1] += verts[vi * 3 + 1]
            acc[name][2] += verts[vi * 3 + 2]
            acc[name][3] += 1.0

    out: dict[str, tuple[float, float, float]] = {}
    for name, a in acc.items():
        if a[3] < 1:
            continue
        out[name] = (a[0] / a[3], a[1] / a[3], a[2] / a[3])
    return out


def collect_triangles(data: dict) -> dict[str, list[list[tuple[int, int]]]]:
    """Material name -> atlas UV triangles (from sprites.json face + uvs arrays)."""
    mat_name = {
        i: m["DbgName"]
        for i, m in enumerate(data.get("materials", []))
        if m.get("DbgName")
    }
    uvs_flat = data["uvs"][0]
    tris: dict[str, list[list[tuple[int, int]]]] = defaultdict(list)

    for face in _parse_three_faces(data["faces"]):
        mat_idx = face.get("material")
        if mat_idx is None:
            continue
        name = mat_name.get(mat_idx)
        if not name or name == "sprites":
            continue
        if "uvs" not in face:
            continue
        uvi = face["uvs"]
        if len(uvi) == 3:
            tris[name].append(_uv_points(uvs_flat, uvi))
        elif len(uvi) == 4:
            p = _uv_points(uvs_flat, uvi)
            tris[name].append([p[0], p[1], p[2]])
            tris[name].append([p[0], p[2], p[3]])
    return tris


def rasterize_triangle(mask: list[bytearray], atlas, pts: list[tuple[int, int]]) -> None:
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    x0, x1 = max(0, min(xs)), min(ATLAS_W, max(xs) + 1)
    y0, y1 = max(0, min(ys)), min(ATLAS_H, max(ys) + 1)
    if x1 <= x0 or y1 <= y0:
        return
    ax, ay = pts[0]
    bx, by = pts[1]
    cx, cy = pts[2]
    area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
    if area == 0:
        return
    apx = atlas.load()
    for y in range(y0, y1):
        row = mask[y]
        for x in range(x0, x1):
            w0 = ((bx - ax) * (y - ay) - (by - ay) * (x - ax)) / area
            w1 = ((cx - bx) * (y - by) - (cy - by) * (x - bx)) / area
            w2 = 1.0 - w0 - w1
            if w0 < 0 or w1 < 0 or w2 < 0:
                continue
            if apx[x, y][3] > ALPHA:
                row[x] = 1


def naive_uv_bbox(triangles: list[list[tuple[int, int]]]) -> tuple[int, int, int, int] | None:
    xs: list[int] = []
    ys: list[int] = []
    for pts in triangles:
        for x, y in pts:
            xs.append(x)
            ys.append(y)
    if not xs:
        return None
    x0, x1 = min(xs), max(xs) + 1
    y0, y1 = min(ys), max(ys) + 1
    return (x0, y0, x1 - x0, y1 - y0)


def largest_cc_bbox(mask: list[bytearray]) -> tuple[int, int, int, int] | None:
    """Tight bbox of the largest 4-connected component (reduces digit/LCD strip bleed)."""
    best_n = 0
    best_box: tuple[int, int, int, int] | None = None
    seen = [bytearray(ATLAS_W) for _ in range(ATLAS_H)]

    for y in range(ATLAS_H):
        row = mask[y]
        srow = seen[y]
        for x in range(ATLAS_W):
            if not row[x] or srow[x]:
                continue
            stack = [(x, y)]
            srow[x] = 1
            n = 0
            x0, y0, x1, y1 = x, y, x + 1, y + 1
            while stack:
                cx, cy = stack.pop()
                n += 1
                x0, y0 = min(x0, cx), min(y0, cy)
                x1, y1 = max(x1, cx + 1), max(y1, cy + 1)
                if cy > 0 and mask[cy - 1][cx] and not seen[cy - 1][cx]:
                    seen[cy - 1][cx] = 1
                    stack.append((cx, cy - 1))
                if cy + 1 < ATLAS_H and mask[cy + 1][cx] and not seen[cy + 1][cx]:
                    seen[cy + 1][cx] = 1
                    stack.append((cx, cy + 1))
                if cx > 0 and mask[cy][cx - 1] and not seen[cy][cx - 1]:
                    seen[cy][cx - 1] = 1
                    stack.append((cx - 1, cy))
                if cx + 1 < ATLAS_W and mask[cy][cx + 1] and not seen[cy][cx + 1]:
                    seen[cy][cx + 1] = 1
                    stack.append((cx + 1, cy))
            if n > best_n:
                best_n = n
                best_box = (x0, y0, x1 - x0, y1 - y0)
    if best_n < MIN_PIX:
        return None
    return best_box


def sprite_draw_bbox(
    mask: list[bytearray],
    *,
    pad: int = 16,
    max_up: int = 120,
) -> tuple[int, int, int, int] | None:
    """Largest-CC box, extended upward for detached parts (e.g. Marge hair)."""
    box = largest_cc_bbox(mask)
    if not box:
        return None
    x, y, w, h = box
    y0 = y
    x_left = max(0, x - pad)
    x_right = min(ATLAS_W, x + w + pad)
    for row in range(y - 1, max(0, y - max_up), -1):
        if any(mask[row][col] for col in range(x_left, x_right)):
            y0 = row
    return (x, y0, w, y + h - y0)


def build_mask(atlas, triangles: list[list[tuple[int, int]]]) -> list[bytearray]:
    mask = [bytearray(ATLAS_W) for _ in range(ATLAS_H)]
    for pts in triangles:
        rasterize_triangle(mask, atlas, pts)
    return mask


def mask_centroid(
    box: tuple[int, int, int, int], mask: list[bytearray]
) -> tuple[float, float] | None:
    """Average (x,y) of masked pixels inside box — visual center for LCD anchoring."""
    x, y, w, h = box
    sx = sy = n = 0.0
    y1 = min(ATLAS_H, y + h)
    x1 = min(ATLAS_W, x + w)
    for row in range(max(0, y), y1):
        mrow = mask[row]
        for col in range(max(0, x), x1):
            if mrow[col]:
                sx += col
                sy += row
                n += 1.0
    if n < 1:
        return None
    return sx / n, sy / n


def pack_mask_bits(
    box: tuple[int, int, int, int], mask: list[bytearray]
) -> tuple[bytes, int, int]:
    """Row-major 1bpp mask inside box (w*h bits, MSB-first). Returns (bytes, w, h)."""
    x, y, w, h = box
    nbytes = (w * h + 7) // 8
    out = bytearray(nbytes)
    i = 0
    for row in range(h):
        mrow = mask[y + row]
        for col in range(w):
            if mrow[x + col]:
                out[i >> 3] |= 1 << (7 - (i & 7))
            i += 1
    return bytes(out), w, h


def crop_masked(atlas, box: tuple[int, int, int, int], mask: list[bytearray]):
    from PIL import Image

    x, y, w, h = box
    x0, y0 = max(0, x - PAD), max(0, y - PAD)
    x1, y1 = min(ATLAS_W, x + w + PAD), min(ATLAS_H, y + h + PAD)
    im = atlas.crop((x0, y0, x1, y1)).copy()
    px = im.load()
    cw, ch = im.size
    for row in range(ch):
        for col in range(cw):
            if not mask[y0 + row][x0 + col]:
                px[col, row] = (0, 0, 0, 0)
    return im

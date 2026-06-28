# Sprint boards — start here

**Two boards at the repo root.** Everything else is archived under `docs/archive/`.

| File | Status | Use for |
|------|--------|---------|
| **[GW_SPRINT_BOARD.md](../../GW_SPRINT_BOARD.md)** | **Active — G&W ship** | Device overlay, ABI, `cupcake.bin`, firmware PR, cleanup, optimization |
| **[SPRINT_BOARD.md](../../SPRINT_BOARD.md)** | Active — gameplay | PC port vs original JS (`TASK-*`), entities, scoreboard |

**Rule:** Game & Watch / retro-go / SD card → **GW_SPRINT_BOARD.md**. Game rules / SDL / tests → **SPRINT_BOARD.md**.

### Archived (historical only)

| File | Notes |
|------|--------|
| [docs/archive/OVERLAY_SPRINT_BOARD.md](../archive/OVERLAY_SPRINT_BOARD.md) | Old symbol-import overlay plan (OV-*) — retired |
| [docs/archive/GWHB_SPRINT_BOARD.md](../archive/GWHB_SPRINT_BOARD.md) | Deferred `CUPCAKE.bin` / GWHB plan (GW-*) |

---

## G&W phases (GW_SPRINT_BOARD.md)

| Order | Sprint | Task prefixes | Purpose |
|-------|--------|---------------|---------|
| 1 | RG-1..5 | `RG-*` | ABI link layer, build `cupcake.bin`, assets, hardware test |
| 2 | RG-6 | `CL-*` | Port repo cleanup (drop symbol-import path) |
| 3 | RG-6b | `RB-*` | Fresh upstream firmware + minimal Cupcake PR |
| 4 | RG-7 | `OP-*` | Code optimization (**last**, after sign-off) |

---

## Historical names

| Old file | Now |
|----------|-----|
| `RETROGO_SPRINT_BOARD.md` | `GW_SPRINT_BOARD.md` |
| `OVERLAY_SPRINT_BOARD.md` | `docs/archive/OVERLAY_SPRINT_BOARD.md` |
| `GWHB_SPRINT_BOARD.md` | `docs/archive/GWHB_SPRINT_BOARD.md` |
| OV-* / `firmware_imports.ld` | Retired — use ABI in `GW_SPRINT_BOARD.md` |

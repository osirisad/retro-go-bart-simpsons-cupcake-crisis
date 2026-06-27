# Cupcake Crisis — Overlay / Game & Watch Sprint Board

**Active ship path:** Celeste-style **named overlay** on stable `game-and-watch-retro-go-sd-cupcake` firmware.

**GWHB is deferred** — see [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) (on hold). Do not add GWHB loader code to firmware for this round.

## How it works (player view)

1. Flash **firmware once** (includes Cupcake overlay slot + launcher dispatch).
2. Copy **`cupcake.bin`** from this repo’s Releases to `/roms/homebrew/cupcake.bin` on SD.
3. Launcher loads the bin into RAM at `__RAM_EMU_START__` (`0x2404B000`), zeros overlay BSS, calls **`app_main_cupcake()`**.

Game updates = replace `cupcake.bin` only. No firmware reflash unless the overlay ABI changes.

## Build separation (important)

| Repo | Role |
|------|------|
| **This port repo** | Game source, device host (`platform/gnw/`), **`make cupcake-bin`** → `release/cupcake.bin`, GitHub Releases |
| **Firmware fork** | Linker slot, stub smoke test, launcher dispatch — **minimal upstream PR** (no port sources in firmware Makefile) |

**Port repo is source of truth for game logic.** Firmware ships a tiny in-tree stub; full game is built standalone here via `platform/gnw/Makefile.gnw` + vendored `platform/gnw/sdk/` + `platform/gnw/firmware_imports.ld`.

Plain guide: [docs/RELEASE_CUPCAKE_BIN.md](docs/RELEASE_CUPCAKE_BIN.md).

**Attribution:** [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md).

**Related:** gameplay parity → [SPRINT_BOARD.md](SPRINT_BOARD.md).

---

## Scope

| In **port repo** | In **firmware fork** (upstream / minimal PR) |
|------------------|------------------------------------------------|
| `src/`, `platform/host_*.c`, `platform/gnw/main_cupcake.c` | `.overlay_cupcake` + `.overlay_cupcake_bss` in linker scripts |
| `make cupcake-bin` → `release/cupcake.bin` | In-tree stub `Core/Src/porting/cupcake/main_cupcake.c` |
| `platform/gnw/firmware_imports.ld` (symbol map for link) | `rg_emulators.c` — `strcmp(name,"cupcake")` + `app_main_cupcake` |
| `tools/bundle_overlay_assets.py` → `cupcake_data.h` (OV-3) | Stub `cupcake.bin` in SD homebrew folder on full firmware build |
| PC regression (`make test-overlay-regression-pc`) | `docs/OVERLAY_CUPCAKE.md` |

**Not in firmware PR:** `CUPCAKE_PORT` Makefile wiring, linux emu fixes, or pulling port sources into firmware build.

**RAM budget:** `__RAM_EMU_LENGTH__` ≈ 724 KiB (SD linker). OV-3 embedded assets required to fit masks + art.

---

## Gap analysis — today vs overlay target

| Piece | Today | Target |
|-------|-------|--------|
| Game core | `src/cupcake_game.c` | Unchanged |
| Device host | `platform/gnw/main_cupcake.c` | Done (OV-2) |
| Display | `host_draw.c` + `gw_lcd` blit | Done |
| Audio | Odroid path + SD WAVs | Embedded WAV table (OV-3) |
| Assets | SD PNG/JPG at `/retro-go/cupcake/` (interim) | Build-time bundle → `cupcake_data.h` |
| Firmware integration | Linker + dispatch (upstream) | Minimal — no port Makefile hook |
| Standalone `cupcake.bin` build | Makefile.gnw + sdk; **missing `firmware_imports.ld`** | `make cupcake-bin` + GitHub Releases |
| Ship artifact | PC `cupcake-sdl.exe` | `cupcake.bin` on SD |

---

## Sprint OV-1 — Spike & docs

| Task | Title | Status |
|------|-------|--------|
| OV-01 | `docs/OVERLAY.md` — Celeste-model guide | [x] |
| OV-02 | RAM / overlay size budget vs `__RAM_EMU` | [x] |
| OV-03 | PC regression gate (+ optional linux emu) | [x] |

### OV-01 `docs/OVERLAY.md`

    Acceptance Criteria: Documents Celeste flow, standalone port-repo build, contrast with deferred GWHB. Updated for build separation (no firmware `CUPCAKE_PORT`).

### OV-02 RAM / overlay size budget

    Acceptance Criteria: `docs/OVERLAY_RAM_BUDGET.md` + `tools/overlay_size_estimate.py`. Masks ~671 KiB — compression/embed decision deferred to OV-3.

### OV-03 Regression gate

    Acceptance Criteria: `make test-overlay-regression-pc` (smoke, demo-replay, host-bezel). Linux emu via `Makefile.cupcake` is optional dev-only, not required for ship.

---

## Sprint OV-2 — Port device host

| Task | Title | Status |
|------|-------|--------|
| OV-04 | `platform/gnw/main_cupcake.c` full host | [x] |
| OV-05 | Odroid LCD + input wiring | [x] |
| OV-06 | Odroid audio (SD WAV interim; embedded in OV-3) | [x] |
| OV-07 | `host_draw.c` / `host_audio.c` GNW ifdef pass | [x] |

### OV-04–07 notes

    Host loop, LCD blit, input, odroid audio wired. Interim SD assets via `gnw_assets.c`. `host_audio.c` defaults to `/retro-go/cupcake` when `CUPCAKE_GNW`.

---

## Sprint OV-2b — Standalone `cupcake.bin` build (port repo)

| Task | Title | Status |
|------|-------|--------|
| OV-18 | `platform/gnw/Makefile.gnw` + `overlay.ld` | [x] |
| OV-19 | Vendored `platform/gnw/sdk/` headers | [x] |
| OV-20 | `tools/gen_overlay_imports.sh` + `firmware_symbols.txt` | [x] |
| OV-21 | Commit `platform/gnw/firmware_imports.ld` | [ ] |
| OV-22 | First successful `make cupcake-bin` link | [ ] |
| OV-23 | `.github/workflows/cupcake-bin.yml` + Releases | [ ] |

### OV-21 `firmware_imports.ld`

    Acceptance Criteria: Generated from flashed firmware ELF (`tools/gen_overlay_imports.sh`). Committed so CI and developers need no firmware source checkout.

### OV-22 First link

    Acceptance Criteria: `release/cupcake.bin` produced locally; size checked vs `__RAM_EMU` (expect tight until OV-3).

### OV-23 GitHub Actions

    Acceptance Criteria: Push to `main` builds artifact; Release attaches `cupcake.bin` + attribution. **Blocked until PAT has `workflow` scope** (or workflow added via GitHub UI).

---

## Sprint OV-3 — Embedded assets

| Task | Title | Status |
|------|-------|--------|
| OV-08 | `tools/bundle_overlay_assets.py` | [ ] |
| OV-09 | `cupcake_data.h` generated pack | [ ] |
| OV-10 | Hi-scores on firmware writable path | [~] |

### OV-10 Hi-scores

    Code sets `/retro-go/saves/cupcake_hiscores.dat` in `main_cupcake.c`. Hardware verify pending (OV-14/17).

---

## Sprint OV-4 — Firmware (upstream, minimal)

| Task | Title | Status |
|------|-------|--------|
| OV-11 | Linker `.overlay_cupcake` sections | [x] |
| OV-12 | In-tree stub + SD extract on firmware build | [x] |
| OV-13 | `rg_emulators.c` — `strcmp(name,"cupcake")` | [x] |
| OV-14 | Hardware smoke — stub on device | [ ] |
| OV-15 | Player install doc | [~] |

### OV-11–13 (firmware fork)

    Already in upstream-oriented fork. **OV-12 is stub-only** — not port-repo `CUPCAKE_PORT` integration (reverted to keep upstream PR small).

### OV-14 Hardware smoke (stub)

    Acceptance Criteria: Flash firmware; stock stub `cupcake.bin` runs briefly, returns to menu.

### OV-15 Install doc

    [docs/RELEASE_CUPCAKE_BIN.md](docs/RELEASE_CUPCAKE_BIN.md) drafted. Optional: rename/expand to `docs/OVERLAY_INSTALL.md` with attribution link.

---

## Sprint OV-5 — Polish & ship

| Task | Title | Status |
|------|-------|--------|
| OV-24 | Hardware smoke — full `cupcake.bin` from Releases | [ ] |
| OV-16 | Save states via firmware slots | [ ] |
| OV-17 | Hardware parity sign-off | [ ] |

---

## Dependency graph

```
OV-01 ── OV-02 ── OV-11..13 (firmware, upstream)
  │
OV-03 ── OV-04..07 ── OV-18..20 ── OV-21 ── OV-22 ── OV-23 (standalone build + CI)
                              │
                              └── OV-08 ── OV-09 ── OV-24 ── OV-17
                                        └── OV-10
                    OV-14 (stub) ── OV-15
                              OV-16
```

---

## Next actions (recommended order)

1. **OV-21** — Build or obtain matching `firmware.elf`; run `gen_overlay_imports.sh`; commit `firmware_imports.ld`.
2. **OV-22** — `make cupcake-bin`; fix link errors / size overflow.
3. **OV-23** — Fix GitHub PAT `workflow` scope; push workflow; tag Release.
4. **OV-14** — Stub smoke on hardware (validates firmware slot).
5. **OV-24** — Full game on hardware from Release `cupcake.bin`.
6. **OV-08/09** — Embedded assets (RAM fit).

---

## When to revisit GWHB

Resume [GWHB_SPRINT_BOARD.md](GWHB_SPRINT_BOARD.md) if you later want a self-contained ABI-only image with no firmware symbol imports.

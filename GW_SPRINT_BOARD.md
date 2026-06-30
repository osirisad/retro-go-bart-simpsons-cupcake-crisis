# Game & Watch sprint board (active)

> **This is the only board for shipping on retro-go G&W.** Task IDs: `RG-*`, `CL-*`, `RB-*`, `OP-*`.
>
> Index of all boards: [docs/sprints/README.md](docs/sprints/README.md)

## Goal

Ship **`cupcake.bin`** built **only in this repo** — download, copy to `/roms/homebrew/cupcake.bin`, play. Optional for everyone. Firmware PR stays **minimal** (linker slot + launcher dispatch + in-tree stub).

**Sprint order:** RG-1..5 ship → RG-6 port cleanup → **RG-6b fresh upstream firmware** → RG-7 optimization last (after hardware sign-off).

## How it works

```
Player
  └─ Flash firmware once (Cupcake slot + stub — optional to use)
  └─ Copy Release cupcake.bin to SD (opt-in full game)

Port repo
  └─ make cupcake-bin  →  release/cupcake.bin
  └─ Links against gw_firmware_abi (VTOR+0x400) — NO firmware.elf, NO per-build symbol map

Firmware (minimal PR)
  └─ .overlay_cupcake @ 0x2404B000
  └─ rg_emulators.c: "cupcake" → load bin → app_main_cupcake()
  └─ gw_firmware_abi already exists (PICO-8 / GWHB) — Cupcake reuses it
```

**Why ABI, not symbol imports:** `firmware_imports.ld` breaks whenever upstream rebuilds. ABI v1 stays stable across normal firmware updates; you only rebuild `cupcake.bin` on intentional `GW_FIRMWARE_ABI_VERSION` bumps or overlay layout changes.

---

## Already done (carry-over from overlay spike)

| Area | Status | Notes |
|------|--------|-------|
| Game core + PC regression | [x] | `make test-overlay-regression-pc` |
| Device host loop | [x] | `platform/gnw/main_cupcake.c` |
| LCD / input / audio wiring | [x] | `host_draw.c`, `host_audio.c`, `cupcake_input.c` |
| Interim SD assets | [x] | `gnw_assets.c` → `/retro-go/cupcake/` |
| Standalone Makefile + linker | [x] | `Makefile.gnw`, `overlay.ld` |
| ABI link layer | [x] | `gw_firmware_abi.h`, `rg_abi.h`, `abi_stubs.c` |
| Vendored SDK headers | [x] | `platform/gnw/sdk/` (compile-time types only) |
| ARM compile check | [x] | `make cupcake-compile-check` (14 objects) |
| **`make cupcake-bin`** | [x] | `release/cupcake.bin` — ABI link, no firmware checkout |
| Overlay RAM budget | [x] | Load 148 KiB + BSS 153 KiB ≈ 302 KiB / 724 KiB slot |
| Firmware slot + dispatch + stub | [x] | Fork: linker, `rg_emulators.c`, `main_cupcake.c` stub |
| RAM budget doc + estimator | [x] | `docs/OVERLAY_RAM_BUDGET.md` |
| Player install draft | [x] | `docs/RELEASE_CUPCAKE_BIN.md` (ABI rewrite done) |

**Retired / do not pursue:** OV-21 `firmware_imports.ld`, `tools/gen_overlay_imports.sh` as ship path.

---

## Scope

| In **this port repo** | In **firmware PR** (upstream) |
|------------------------|-------------------------------|
| `src/`, shared `platform/host_*.c` | `.overlay_cupcake` + `.overlay_cupcake_bss` in linker scripts |
| `platform/gnw/main_cupcake.c` | Stub `Core/Src/porting/cupcake/main_cupcake.c` |
| **`platform/gnw/abi_*`** — ABI link layer (new) | `rg_emulators.c` — `strcmp(name,"cupcake")` |
| `make cupcake-bin` → `release/cupcake.bin` | `docs/OVERLAY_CUPCAKE.md` |
| GitHub Releases + CI | Optional: append ABI fields if port cannot workaround |
| Embedded assets (RG-4) | [x] |

**Not in firmware PR:** port sources, `CUPCAKE_PORT`, linux emu wiring, forcing users to install Cupcake.

**Attribution:** [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md).

**Related:** gameplay parity → [SPRINT_BOARD.md](SPRINT_BOARD.md) (TASK-*) · technical background → [docs/OVERLAY.md](docs/OVERLAY.md).

---

## Gap analysis — today vs ship target

| Piece | Today | Target |
|-------|-------|--------|
| Firmware API binding | **`gw_firmware_abi` shims** | Done |
| `make cupcake-bin` | **`release/cupcake.bin` produced** | Done |
| CI / Releases | Port-only workflow; tag `v*` attaches bin | Done |
| Assets on device | SD PNG/JPG/WAV (interim) | Embedded pack (RAM fit) |
| Hardware validation | Not run | Stub smoke → full game smoke |
| Upstream PR | Done in fork | Submit + merge |

---

## Sprint RG-1 — ABI link layer (port repo)

| Task | Title | Status |
|------|-------|--------|
| RG-01 | Vendor `gw_firmware_abi.h` (+ minimal `ff.h` if needed) | [x] |
| RG-02 | `platform/gnw/rg_abi.h` — accessors, version check macro | [x] |
| RG-03 | `platform/gnw/abi_stubs.c` — firmware-facing symbol implementations | [x] |
| RG-04 | `common_emu_state` via `GW_FIRMWARE_ABI.common_emu_state_ptr` | [x] |
| RG-05 | Variadic libc wrappers (`printf`, `snprintf`, `fprintf`) | [x] |
| RG-06 | Startup ABI guard in `main_cupcake.c` | [x] |

### RG-01 Vendor ABI header

    Acceptance Criteria: Copy/sync `gw_firmware_abi.h` from firmware `Core/Inc/retro-go/`. Document sync rule: match firmware when `GW_FIRMWARE_ABI_VERSION` bumps. No firmware source required at build time.

### RG-02 `rg_abi.h`

    Acceptance Criteria: `gw_firmware_abi()` inline matches firmware (VTOR + `0x400`). Helper to assert `version >= 1` and `size >= minimum for v1`.

### RG-03 `abi_stubs.c`

    Acceptance Criteria: Provide link symbols the game already calls, delegating to `GW_FIRMWARE_ABI.*`. Cover at least:

    - LCD: `lcd_swap`, `lcd_get_active_buffer`, `lcd_clear_buffers` (or clear active + inactive)
    - Audio: `audio_start_playing`, `odroid_audio_submit`, `odroid_audio_sample_rate_get`
    - System/input: `odroid_system_*`, `odroid_input_read_gamepad`, `odroid_overlay_alert`
    - Emu loop: `common_emu_*`, `wdog_refresh`
    - Heap/stdio: `ram_malloc`, `malloc`, `free`, `fopen`, `fread`, `fwrite`, `fclose`, `fseek`, `ftell`, `memcpy`, `memset`, `memmove`
    - stb path: use ABI `fopen`/`fread` — drop `__wrap_*` dependency if possible

    Reference: firmware `gw_firmware_abi.c` field list.

### RG-04 `common_emu_state`

    Acceptance Criteria: `main_cupcake.c` reads/writes pause/frame timing through `*(common_emu_state_t *)GW_FIRMWARE_ABI.common_emu_state_ptr`. No `extern common_emu_state` link dependency.

### RG-05 Variadic wrappers

    Acceptance Criteria: `printf` / `snprintf` / `fprintf` compile and link using ABI `vprintf` / `vsnprintf` / `vfprintf` (same pattern as PICO-8 engine / `docs/PICO8_EXTERNAL_MODULE.md` in firmware tree).

### RG-06 Startup guard

    Acceptance Criteria: `app_main_cupcake` checks ABI version/size before game init; shows alert or early return if incompatible (clear message for wrong firmware).

---

## Sprint RG-2 — Standalone build + CI

| Task | Title | Status |
|------|-------|--------|
| RG-07 | Update `Makefile.gnw` — ABI sources, remove imports | [x] |
| RG-08 | First successful `make cupcake-bin` link | [x] |
| RG-09 | Overlay size check vs `__RAM_EMU` (~724 KiB) | [x] |
| RG-10 | Retire imports tooling from docs/CI | [x] |
| RG-11 | `.github/workflows/cupcake-bin.yml` — port-only build | [x] |
| RG-12 | GitHub Release attaches `cupcake.bin` + attribution | [x] |

### RG-07 Makefile

    Acceptance Criteria: `make cupcake-bin` needs only `arm-none-eabi-gcc`. Add `abi_stubs.c`, include path for ABI header. Remove `imports-check`, `FIRMWARE_ELF`, `-T firmware_imports.ld`. Keep `overlay.ld` section layout unchanged.

### RG-08 First link

    Acceptance Criteria: `release/cupcake.bin` produced locally. Fix undefined symbols via stubs or `-lgcc` as needed. Entry `app_main_cupcake` in `.overlay_cupcake`.

    Progress baseline: `make cupcake-compile-check` already passes.

### RG-09 Size

    Acceptance Criteria: Record `.overlay_cupcake` load size + BSS estimate. If over budget, RG-4 (embedded/compressed assets) is blocking for hardware ship.

    **Recorded (2026-06-27):** `cupcake.bin` = **152 212 B** (148 KiB load). BSS = **157 120 B** (153 KiB). Combined runtime ≈ **302 KiB** — **42% of 724 KiB** `__RAM_EMU` slot. Headroom remains for RG-4 embedded assets without blocking hardware smoke.

### RG-10 Retire imports path

    Superseded by **Sprint RG-6** (CL-P01, CL-P04). Mark complete when CL-P01 + CL-P04 done.

    Acceptance Criteria: No references to `firmware_imports.ld` or `gen_overlay_imports.sh` in ship docs or Makefile.

### RG-11–12 CI / Releases

    Acceptance Criteria: Push to `main` builds artifact without firmware checkout. Tag Release → attach `cupcake.bin`. Remove `regen-imports` workflow job if present.

    Note: PAT may need `workflow` scope to push workflow file; alternative — add workflow via GitHub UI once.

---

## Sprint RG-3 — Firmware upstream (minimal)

| Task | Title | Status |
|------|-------|--------|
| RG-13 | PR: linker + dispatch + stub + doc | [~] |
| RG-14 | Optional: append missing ABI fields | [ ] |
| RG-15 | Hardware smoke — firmware stub | [ ] |

### RG-13 Upstream PR

    Acceptance Criteria: Single focused PR to `game-and-watch-retro-go-sd`. Files: linker scripts, `rg_emulators.c`, `Core/Src/porting/cupcake/*`, `docs/OVERLAY_CUPCAKE.md`, `.gitignore` hygiene. No port repo sources. Author can merge without adopting Cupcake as default content.

    Progress: Already implemented in fork (RG-13 code complete; submission pending).

### RG-14 Optional ABI append

    Acceptance Criteria: **Only if RG-03 workarounds fail.** Append to `gw_firmware_abi_t` (end of struct, no reorder): e.g. `odroid_overlay_alert`, `odroid_audio_submit`, `odroid_audio_sample_rate_get`, `lcd_clear_buffers`. Initialize in `gw_firmware_abi.c`. Document append-only rule in PR description.

    Prefer port-side workarounds first to keep firmware diff zero.

### RG-15 Stub hardware smoke

    Acceptance Criteria: Flash PR firmware; Homebrew → **cupcake** with stock stub `cupcake.bin` runs briefly, returns to menu. Validates slot before full game.

---

## Sprint RG-4 — Embedded assets (RAM fit)

| Task | Title | Status |
|------|-------|--------|
| RG-16 | `tools/bundle_overlay_assets.py` | [x] |
| RG-17 | `cupcake_data.h` + link into overlay | [x] |
| RG-18 | Remove interim SD asset requirement for ship | [x] |

### RG-16–17 Asset bundle

    Acceptance Criteria: Atlas masks, bezel, WAV table embedded at build time. `gnw_assets.c` loads from rodata instead of SD. Fits in `__RAM_EMU` per `docs/OVERLAY_RAM_BUDGET.md` (compression if needed).

    **Recorded (2026-06-27):** `tools/bundle_overlay_assets.py` embeds JPEG bezel+atlas (auto quality fit) and IMA ADPCM audio. Linked load **583 456 B** + BSS **156 608 B** = **740 064 B** (~99.8% of 724 KiB slot). `release/cupcake.bin` is self-contained when built with local `assets/`.

### RG-18 Ship criteria

    Acceptance Criteria: Release `cupcake.bin` runs with **only** the bin on SD (plus firmware saves path for hi-scores). Optional: keep SD asset fallback behind ifdef for dev.

    **Done:** `-DCUPCAKE_EMBEDDED_ASSETS` when `assets/screen.jpg` exists; SD fallback remains when assets absent (CI compile without embed).

---

## Sprint RG-5 — Hardware validate & ship

| Task | Title | Status |
|------|-------|--------|
| RG-19 | Hardware smoke — full `cupcake.bin` | [ ] |
| RG-20 | Hi-scores persist on device | [~] |
| RG-21 | Save states via firmware slots | [ ] |
| RG-22 | Hardware parity sign-off | [ ] |
| RG-23 | Player install doc final | [x] |

### RG-19 Full game on device

    Acceptance Criteria: Release `cupcake.bin` from this repo; gameplay, audio, input, bezel match PC/linux emu within known limits.

### RG-20 Hi-scores

    Code uses `/data/homebrew/cupcake_hiscores.dat`. Verify read/write on hardware after RG-19.

### RG-21 Save states

    Acceptance Criteria: `LoadState` / `SaveState` callbacks work with firmware save slots (optional polish — can ship after first playable).

### RG-22 Sign-off

    Acceptance Criteria: Checklist: boot, play, game over, hi-scores, return to menu, no watchdog resets, acceptable frame pacing.

### RG-23 Install doc

    Acceptance Criteria: `docs/RELEASE_CUPCAKE_BIN.md` (or `docs/OVERLAY_INSTALL.md`) — one-page player guide: flash firmware (optional Cupcake slot), download bin, SD path, no rebuild on firmware updates unless ABI version changes.

---

## Sprint RG-6 — Repo cleanup (abandoned paths)

Leftovers from symbol-import linking, firmware-in-tree builds, and deferred GWHB. Run **after RG-08** (first ABI link) or in parallel once RG-07 lands — delete import tooling only after ABI stubs replace it.

### Inventory

| Artifact | Repo | Path | Verdict |
|----------|------|------|---------|
| Symbol import generator | Port | `tools/gen_overlay_imports.sh` | **Deleted** (RG-10) |
| Symbol list | Port | `platform/gnw/firmware_symbols.txt` | **Deleted** (RG-10) |
| Import build wrapper | Port | `tools/build_cupcake_bin.sh` | **Simplified** — calls `make cupcake-bin` |
| Unused source manifest | Port | `platform/gnw/cupcake_overlay.mk` | **Deleted** (CL-P02) |
| Makefile alias | Port | `platform/gnw/Makefile.overlay` | **Kept** as thin alias to `Makefile.gnw` |
| CI imports job | Port | `.github/workflows/cupcake-bin.yml` `regen-imports` | **Deleted** (RG-11) |
| Stale import docs | Port | `docs/RELEASE_CUPCAKE_BIN.md`, `docs/OVERLAY.md`, `platform/gnw/README.md`, `platform/gnw/sdk/README.md` | **Rewritten** (RG-10 / CL-P04) |
| Archived sprint board | Port | `docs/archive/OVERLAY_SPRINT_BOARD.md` | Historical OV-* only |
| GWHB plan | Port | `docs/archive/GWHB_SPRINT_BOARD.md` | Deferred GW-* |
| Linux emu host | Port | `platform/retrogo/Makefile.cupcake` | **Keep** — valid dev regression (not abandoned) |
| Vendored SDK headers | Port | `platform/gnw/sdk/` | **Slim** after ABI — types only; drop fake `extern` firmware API |
| Local build artifacts | Port | `build-gnw/`, `build-cupcake/` (gitignored) | **Purge** locally; confirm `.gitignore` |
| Linux cupcake objects | Firmware | `linux/build-cupcake/` (gitignored) | **Purge** locally if present |
| Stale doc line | Firmware | `docs/OVERLAY_CUPCAKE.md` — `CUPCAKE_C_SOURCES` | **Clarify** stub-only + ABI for external bin |
| Fork-only fixes | Firmware | e.g. gittag bash, MinGW `uint` | **Exclude** from upstream PR — document in CL-F06 |

### Port repo tasks

| Task | Title | Status |
|------|-------|--------|
| CL-P01 | Remove symbol-import toolchain | [x] |
| CL-P02 | Consolidate overlay build entry points | [x] |
| CL-P03 | Slim `platform/gnw/sdk/` for ABI model | [ ] |
| CL-P04 | Rewrite docs + READMEs (ABI, single board link) | [x] |
| CL-P05 | Trim archived `OVERLAY_SPRINT_BOARD.md` | [x] |
| CL-P06 | GWHB deferred hygiene | [x] |
| CL-P07 | `.gitignore` + local artifact purge | [ ] |

### CL-P01 Remove symbol-import toolchain

    Acceptance Criteria: Delete `tools/gen_overlay_imports.sh`, `platform/gnw/firmware_symbols.txt`. Remove `IMPORTS_LD` / `FIRMWARE_ELF` / `imports-check` from `Makefile.gnw`. Delete or simplify `tools/build_cupcake_bin.sh` to call `make cupcake-bin` only. No doc references to regenerating imports.

    **Do after:** RG-07 (ABI Makefile).

### CL-P02 Consolidate overlay build entry points

    Acceptance Criteria: One documented path: `make cupcake-bin` → `Makefile.gnw`. Delete `platform/gnw/cupcake_overlay.mk` (unreferenced). Delete `Makefile.overlay` **or** keep as documented alias only — not both names in docs. Fix stale comment in `platform/gnw/main_cupcake.c` (`Makefile.overlay` → `make cupcake-bin`).

### CL-P03 Slim vendored SDK

    Acceptance Criteria: After `abi_stubs.c` owns firmware calls, `platform/gnw/sdk/` holds **compile-time types only** (`odroid_gamepad_state_t`, `common_emu_state_t`, `appid.h`, etc.). Remove duplicate/misleading extern declarations that imply direct firmware link. Add `sdk/README.md`: “ABI shims in `abi_stubs.c`; sync `gw_firmware_abi.h` on version bump.”

    **Do after:** RG-03.

### CL-P04 Docs sweep

    Acceptance Criteria: All player/dev docs point to [GW_SPRINT_BOARD.md](GW_SPRINT_BOARD.md). Update:

    - `docs/RELEASE_CUPCAKE_BIN.md` — no firmware ELF, no imports file
    - `docs/OVERLAY.md` — ABI binding, `make cupcake-bin`, link RETROGO board not OV board
    - `docs/BUILD.md` — device section matches ABI; GWHB stays “deferred” one paragraph
    - `platform/gnw/README.md`, `platform/README.md`
    - Root `Makefile` comment on `cupcake-bin` target

    Overlaps RG-10 — close both when done.

### CL-P05 Trim archived overlay board

    Acceptance Criteria: Historical OV-* lives only in `docs/archive/OVERLAY_SPRINT_BOARD.md` (no root copy).

### CL-P06 GWHB deferred hygiene

    Acceptance Criteria: GWHB plan only in `docs/archive/GWHB_SPRINT_BOARD.md`. `platform/gwhb/Makefile.gwhb` comment: deferred. No new GWHB code in firmware PR.

### CL-P07 Gitignore and artifacts

    Acceptance Criteria: Confirm `.gitignore` covers `build-gnw/`, `build-gwhb/`, `build-cupcake/`, `release/`. No tracked `*.o` / `*.elf` under port or firmware from experiments. Optional: add `docs/CLEANUP_LOG.md` one-liner listing what was removed (for PR description).

### Firmware repo tasks

| Task | Title | Status |
|------|-------|--------|
| CL-F01 | `docs/OVERLAY_CUPCAKE.md` accuracy pass | [ ] |
| CL-F02 | Purge `linux/build-cupcake/` artifacts | [ ] |
| CL-F03 | Upstream PR scope audit | [ ] |
| CL-F04 | Optional: drop `.gitignore` entry if dir never used | [ ] |

### CL-F01 Firmware Cupcake doc

    **Done in RG-6b** (RB-03 / RB-08). Acceptance Criteria:

    - In-tree build is **stub only** (`main_cupcake.c`); not the port repo game
    - Full game = external `cupcake.bin` via **ABI** (`gw_firmware_abi`), not symbol imports
    - Optional for users; Celeste-style filename `cupcake.bin`

### CL-F02 Local firmware build cruft

    **Done in RG-6b** (RB-01 / RB-07). Delete untracked `linux/build-cupcake/` on old fork before archive.

### CL-F03 Upstream PR scope audit

    **Implemented in Sprint RG-6b** (fresh clone + minimal reapply). Use this list as the allowlist for RB-03 / RB-04:

    - Linker: `.overlay_cupcake` / `_bss`
    - `Core/Src/porting/cupcake/main_cupcake.c` + `main_cupcake.h`
    - `rg_emulators.c` dispatch
    - `Makefile` stub `CUPCAKE_C_SOURCES` (single file)
    - `Makefile.common` cupcake object rules (mirror celeste pattern)
    - `gw_linker.h` externs
    - `docs/OVERLAY_CUPCAKE.md`
    - `.gitignore` for `linux/build-cupcake/` if kept

    **Exclude:** `CUPCAKE_PORT`, port sources in firmware Makefile, `firmware_imports` tooling, linux emu MinGW hacks, gittag bash workaround, unrelated fixes.

### CL-F04 `.gitignore` for `linux/build-cupcake/`

    **Decide in RG-6b** (RB-03): keep on fresh tree only if that path is unused (port linux emu uses its own `build-cupcake/`). Prefer omitting gitignore line if nothing writes there.

---

## Sprint RG-6b — Fresh upstream firmware (rebase)

Replace the experimental **`game-and-watch-retro-go-sd-cupcake`** fork with a **clean checkout of upstream** and re-apply **only** the minimal Cupcake overlay patch (ABI-aware — no symbol-import / in-tree port build cruft). Do this **after** port ABI link works (RG-08) and **before** RG-7 optimization.

### Why

| Old fork | Fresh upstream + minimal patch |
|----------|--------------------------------|
| Mixed “fixes” commits | One reviewable PR for Sylverb |
| Symbol-import era assumptions in docs | Docs say stub + external ABI `cupcake.bin` |
| Local hacks (gittag bash, etc.) | Excluded unless upstream needs them separately |
| Hard to diff vs moving upstream | Easy to rebase on latest `main` later |

### What upstream already has (do not re-add)

- `gw_firmware_abi` / `gw_firmware_abi.c` — **no Cupcake-specific ABI work** unless RG-14 requires append-only fields
- Celeste overlay pattern — **mirror** for Cupcake slot only

### What you must re-apply (allowlist)

Same as CL-F03 — typically **~10 files**, one logical commit:

```
STM32H7B0VBTx_FLASH.ld          — .overlay_cupcake + _bss
STM32H7B0VBTx_SDCARD.ld         — same
Core/Inc/gw_linker.h            — _OVERLAY_CUPCAKE_* externs
Core/Inc/porting/cupcake/main_cupcake.h
Core/Src/porting/cupcake/main_cupcake.c   — stub only
Core/Src/retro-go/rg_emulators.c          — #include + homebrew branch
Makefile                        — CUPCAKE_C_SOURCES = stub .c only
Makefile.common                 — cupcake build/objcopy rules (celeste pattern)
docs/OVERLAY_CUPCAKE.md
.gitignore                      — linux/build-cupcake/ (optional)
scripts/size.sh                 — ram_emu_cupcake line (if celeste has one)
```

**Do not re-apply:** `CUPCAKE_PORT`, port repo paths in Makefile, linux `build-cupcake` emu wiring, `firmware_imports` scripts, full-game sources in firmware tree.

### Tasks

| Task | Title | Status |
|------|-------|--------|
| RB-01 | Fresh clone upstream sd-repo | [ ] |
| RB-02 | Diff old fork → minimal allowlist | [ ] |
| RB-03 | Branch + apply minimal Cupcake patch | [ ] |
| RB-04 | Verify zero overlay-path cruft | [ ] |
| RB-05 | Linux firmware build | [ ] |
| RB-06 | Stub hardware smoke on new build | [ ] |
| RB-07 | Retire / archive old fork | [ ] |
| RB-08 | Upstream PR branch + description | [ ] |
| RB-09 | Port docs: firmware pin note | [ ] |

### RB-01 Fresh clone upstream sd-repo

    Acceptance Criteria: New directory or renamed old tree, e.g. `game-and-watch-retro-go-sd/` cloned from `https://github.com/sylverb/game-and-watch-retro-go-sd` (or official upstream URL). Submodules initialized. **No** Cupcake changes yet. Record upstream commit hash in `docs/FIRMWARE_PIN.md` (port repo) or firmware PR notes.

    Keep old `game-and-watch-retro-go-sd-cupcake/` as read-only reference until RB-03 passes (rename to `…-cupcake-old/` or separate path).

### RB-02 Diff old fork → allowlist

    Acceptance Criteria: From old fork, produce a file list that matches CL-F03 allowlist. Note hunks to **drop** (gittag bash, doc churn, anything not in allowlist). Optional: `git format-patch` or manual copy — prefer **one clean commit** over cherry-picking “fixes/fix/fixes” chain.

    Reference files in current fork: commits `7b238dd7`, `ca5940d9`, `51d9ab2c`, `f71dcec2` (take content, not necessarily history).

### RB-03 Branch + apply minimal patch

    Acceptance Criteria: On fresh clone, branch `cupcake-overlay-minimal`. Apply allowlist only. Single commit message e.g. “Add optional Cupcake homebrew overlay slot (stub)”. `CUPCAKE_C_SOURCES` = stub `main_cupcake.c` only — mirrors Celeste stub model.

    `docs/OVERLAY_CUPCAKE.md` states: full game from external port repo via `cupcake.bin` + **ABI**; optional for users.

### RB-04 Zero overlay-path cruft check

    Acceptance Criteria: Grep new firmware tree — must **not** contain:

    - `CUPCAKE_PORT`, `firmware_imports`, `gen_overlay_imports`
    - Port repo paths in Makefile
    - References to linking external game into firmware ELF

    Must contain: `.overlay_cupcake`, `app_main_cupcake`, `"cupcake"` dispatch only.

### RB-05 Linux firmware build

    Acceptance Criteria: `make` (or `make DOCKER=1` / project-standard flow) produces `build/gw_retro_go.elf`. `objcopy` extracts stub `cupcake.bin`. `./scripts/size.sh` shows `ram_emu_cupcake` line.

    Document build command used in `docs/OVERLAY_CUPCAKE.md` or port `docs/FIRMWARE_PIN.md`.

### RB-06 Stub hardware smoke

    Acceptance Criteria: Flash RB-05 build. Homebrew → **cupcake** with stock stub runs briefly, returns to menu. Same as RG-15 — repeat on clean firmware to validate rebase.

    Then flash + test port-repo `release/cupcake.bin` if RG-08 already done (ABI full game).

### RB-07 Retire old fork

    Acceptance Criteria: Old `game-and-watch-retro-go-sd-cupcake` archived (delete, or mark README “superseded by …”). Git remotes on new repo: `origin` = your fork, `upstream` = Sylverb. **Rotate/remove embedded tokens** from remote URLs.

### RB-08 Upstream PR

    Acceptance Criteria: Push `cupcake-overlay-minimal` to your GitHub fork. Open PR to Sylverb with short body: optional slot, stub only, external full game + ABI, no forced install. Attach file list from RB-02.

    Overlaps former CL-F01 — close when PR description + `OVERLAY_CUPCAKE.md` on new tree are accurate.

### RB-09 Port repo firmware pin

    Acceptance Criteria: Port repo documents which firmware commit/branch to flash (e.g. `docs/FIRMWARE_PIN.md`: “flash upstream + PR branch X until merged”). No firmware source required to build `cupcake.bin` (ABI).

---

## Sprint RG-7 — Code optimization (post-ship, last)

**Do this sprint only after hardware sign-off and fresh firmware rebase.** Optimization can change behavior, size, and timing — fixing regressions mid-ship wastes effort. Treat RG-7 as a dedicated polish pass once RG-1..6, **RG-6b**, and **RG-22** are done.

### Entry gate (required before starting)

| Gate | Task |
|------|------|
| Full game runs on G&W | RG-19 [x] |
| Parity sign-off | RG-22 [x] |
| Repo cleanup done | CL-P01..P07 [x] |
| Fresh firmware rebase | RB-01..RB-08 [x] |
| Regression green | `make test-overlay-regression-pc` (+ linux emu if you use it) |
| Baseline metrics recorded | OP-01 |

**Rule:** One OP task per PR or commit series; re-run regression + quick hardware smoke after each risky change.

### Optimization areas (whole-picture)

| Area | Primary files | Goal |
|------|---------------|------|
| Overlay size / RAM | `src/cupcake_sprite_masks.h`, embed bundle, `host_draw.c`, `gnw_assets.c` | Fit comfortably in ~724 KiB; smaller SD bin |
| Device runtime | `main_cupcake.c`, `host_draw.c`, `host_audio.c` | Stable 30 FPS, no audio underruns |
| Host duplication | `platform/sdl/main.c`, `platform/retrogo/main.c`, `platform/gnw/main_cupcake.c` | Shared helpers; less copy-paste |
| Platform ifdefs | `host_audio.c`, `cupcake_input.c`, headers | Clear host matrix; fewer overlapping macros |
| Game core clarity | `src/cupcake_game.c`, entity modules | Easier maintenance without behavior change |
| Build / link | `Makefile.gnw`, `overlay.ld`, flags | Smaller/faster binary where safe |
| ABI layer | `platform/gnw/abi_stubs.c` | Minimal indirection; no dead wrappers |

---

### RG-7 tasks — measurement & process

| Task | Title | Status |
|------|-------|--------|
| OP-01 | Baseline metrics snapshot | [ ] |
| OP-02 | Optimization log + rollback notes | [ ] |

### OP-01 Baseline metrics snapshot

    Acceptance Criteria: Record **before** any OP work in `docs/OPTIMIZATION_BASELINE.md` (or appendix to `OVERLAY_RAM_BUDGET.md`):

    - `release/cupcake.bin` load size (bytes)
    - `arm-none-eabi-size` on overlay ELF (`.text` / `.data` / `.bss`)
    - PC: `make test-overlay-regression-pc` time (optional)
    - Device: subjective frame pacing + audio notes from RG-22 session
    - Largest rodata contributors (`nm --size-sort` or `size` on objects)

### OP-02 Optimization log

    Acceptance Criteria: Short `docs/OPTIMIZATION_LOG.md`: date, task id, what changed, delta on size/FPS, regression result. Makes bisect easy if something breaks later.

---

### RG-7 tasks — size & RAM (device)

| Task | Title | Status |
|------|-------|--------|
| OP-10 | Sprite mask storage strategy | [ ] |
| OP-11 | Embedded asset compression review | [ ] |
| OP-12 | Audio footprint trim | [ ] |
| OP-13 | Runtime buffer audit | [ ] |
| OP-14 | Link-time size pass (`-Os`, sections, strip) | [ ] |

### OP-10 Sprite masks

    Acceptance Criteria: Masks (~671 KiB rodata today) use a smaller representation: RLE/bitpack, runtime decode to scratch, or mask-only regions vs full triangles — **without** changing visible pixels. Re-measure; target documented in baseline doc.

    Reference: `docs/OVERLAY_RAM_BUDGET.md`, `tools/overlay_size_estimate.py`.

### OP-11 Embedded assets

    Acceptance Criteria: After RG-16..18, review PNG/JPEG/WAV packing — strip metadata, mono 22050 Hz where acceptable, optional compression (zlib/lz4) with one-time decode at init. Document tradeoffs vs load time.

### OP-12 Audio

    Acceptance Criteria: Device path loads minimum PCM needed per SFX; no duplicate buffers; `host_audio_pump` allocation-free in steady state. Compare `.rodata` + heap peak before/after.

### OP-13 Runtime buffers

    Acceptance Criteria: Confirm no 1024×800 RGBA in overlay BSS (use 320×240 RGB565 path only). `gnw_assets.c` / `host_draw.c` — decode temporaries freed after init; peak RAM documented.

### OP-14 Link flags

    Acceptance Criteria: Review `Makefile.gnw` — `-Os`, `--gc-sections`, `-flto` (if toolchain stable), `-g` off for Release builds. Size delta recorded; link still succeeds with ABI.

---

### RG-7 tasks — runtime performance (device)

| Task | Title | Status |
|------|-------|--------|
| OP-20 | Frame loop hot path | [ ] |
| OP-21 | Masked blit efficiency | [ ] |
| OP-22 | Audio sync / frame cadence | [ ] |

### OP-20 Frame loop

    Acceptance Criteria: Profile mentally or with GPIO tick: `main_cupcake.c` loop — avoid redundant `lcd_clear`, redundant blits, duplicate input reads. Match `common_emu_frame_loop` contract. Hardware: no visible stutter vs baseline.

### OP-21 Masked blit

    Acceptance Criteria: `host_draw.c` / mask application — reduce per-pixel overhead (early reject, aligned reads, skip transparent runs) if profiling shows blit dominates. PC regression + visual compare unchanged.

### OP-22 Audio cadence

    Acceptance Criteria: `host_audio_pump` frame count matches `odroid_audio_sample_rate_get() / CUPCAKE_FPS` consistently; no per-frame heap; mute path cheap.

---

### RG-7 tasks — structure & maintainability (all hosts)

| Task | Title | Status |
|------|-------|--------|
| OP-30 | Extract shared host helpers | [ ] |
| OP-31 | Platform ifdef matrix cleanup | [ ] |
| OP-32 | `cupcake_game.c` modularization (optional) | [ ] |
| OP-33 | ABI stubs minimal surface | [ ] |
| OP-34 | Dead code & debug path audit | [ ] |

### OP-30 Shared host helpers

    Acceptance Criteria: Common pieces lifted from three hosts where safe:

    - Save/load state file I/O (duplicated in `main_cupcake.c`, `sdl/main.c`, `retrogo/main.c`)
    - Hi-score path setup
    - Optional: shared “host context” struct for atlas/bezel/LCD rect

    Behavior unchanged; `make test-overlay-regression-pc` green.

### OP-31 Ifdef cleanup

    Acceptance Criteria: Document host matrix in `platform/README.md`:

    | Host | Macros |
    |------|--------|
    | SDL | (default PC) |
    | retrogo linux emu | `LINUX_EMU` |
    | G&W overlay | `CUPCAKE_GNW` / `TARGET_GNW` |

    Consolidate redundant `#if defined(CUPCAKE_GNW)` vs `TARGET_GNW` where they always pair. No functional change.

### OP-32 Game core modularization (optional)

    Acceptance Criteria: **Only if** `cupcake_game.c` (~1.9k lines) blocks future fixes — split entity tick/draw into existing or new `.c` files with unchanged public API. Defer if risky; not required for ship.

### OP-33 ABI stubs

    Acceptance Criteria: After CL-P03, `abi_stubs.c` has no unused wrappers; variadic helpers in one place; inline small LCD clear helper instead of duplicate firmware calls.

### OP-34 Dead code audit

    Acceptance Criteria: Remove unreachable `#ifdef` branches, unused statics, demo-only paths not needed on device (keep PC debug behind `CUPCAKE_DEBUG_*`). Grep for TODO/FIXME from overlay spike; resolve or ticket.

---

### RG-7 tasks — validation

| Task | Title | Status |
|------|-------|--------|
| OP-40 | Post-optimization regression | [ ] |
| OP-41 | Hardware re-validation | [ ] |
| OP-42 | Release notes + size comparison | [ ] |

### OP-40 Regression

    Acceptance Criteria: Full `make test-overlay-regression-pc`; optional linux emu run; `make cupcake-bin` succeeds; bin size ≤ baseline or justified in OPTIMIZATION_LOG.

### OP-41 Hardware

    Acceptance Criteria: Repeat RG-22 checklist on device after OP changes that touch `host_*`, `main_cupcake`, masks, or audio.

### OP-42 Release

    Acceptance Criteria: Tag Release notes: “optimized build” with size before/after; no ABI version change unless intentional.

---

## Dependency graph

```
RG-01 ── RG-02 ── RG-03 ── RG-04 ── RG-05 ── RG-06
                              │
                              └── RG-07 ── RG-08 ── RG-09 ── RG-11 ── RG-12
                                        │
                    RG-13 ── RG-15 ─────┼── RG-19 ── RG-22
                         RG-14 (optional)
                                        │
                              RG-16 ── RG-17 ── RG-18
                                        │
                                        └── RG-19 (RAM fit)

RG-07 ── CL-P01, CL-P02
RG-03 ── CL-P03
RG-08 ── CL-P01 (delete imports)
RG-10 ══ CL-P04 (same doc sweep)
CL-P05, CL-P06, CL-P07 — anytime after RG-06 spike
CL-F01, CL-F02, CL-F04 — fold into RG-6b (RB-*); CL-F03 = allowlist for RB-02

RG-08 ── RB-01 ── RB-02 ── RB-03 ── RB-04 ── RB-05 ── RB-06 ── RB-08
RG-22 + CL-* + RB-* ── OP-01 ── OP-10..14 (size) ── OP-40
              ├── OP-20..22 (runtime) ── OP-41
              └── OP-30..34 (structure) ── OP-40
              OP-02, OP-42 — ongoing / at end
```

---

## Recommended order (next actions)

1. **RG-01 → RG-06** — ABI headers + `abi_stubs.c` + `common_emu_state` ptr + startup guard.
2. **RG-07 → RG-08** — Makefile switch; first `make cupcake-bin`.
3. **RG-15** — Stub smoke on hardware (validates firmware PR while port links).
4. **RG-19** — Full game on device with interim SD assets (proves ABI path before embed work).
5. **RG-16 → RG-18** — Embedded assets if RAM/size requires for ship.
6. **RG-11 → RG-12** — CI + Release.
7. **RG-13** — Open upstream firmware PR if not already submitted.
8. **CL-P01 → CL-P04** — Port repo cleanup (imports, docs).
9. **RB-01 → RB-08** — Fresh upstream firmware + minimal Cupcake patch + PR (**before optimization**).
10. **RG-10, RG-23** — Player install doc (folds into CL-P04 / RB-09).
11. **RG-7 (OP-*)** — **Last.** Only after RG-22 + CL-* + RB-*; baseline → optimize → re-validate.

---

## When firmware updates (maintenance)

| Event | Port action |
|-------|-------------|
| Normal upstream commits | **None** — keep shipping same `cupcake.bin` |
| `GW_FIRMWARE_ABI_VERSION` bump | Sync header, extend stubs if new fields needed, rebuild + Release |
| Overlay slot moved in linker | Rare; rebuild `cupcake.bin`, document pinned firmware |
| New Cupcake feature needs API | Request append-only ABI field in firmware; then stub + rebuild |

---

## Other boards

See [docs/sprints/README.md](docs/sprints/README.md). Archived overlay + deferred GWHB plans live under `docs/archive/`.

# Cupcake Crisis — `cupcake.bin` for Game & Watch (plain guide)

## The simple version

| Who | What you do |
|-----|-------------|
| **Player** | Flash **firmware once** (includes Cupcake support). Download **`cupcake.bin`** from Releases. Copy to SD **`/roms/homebrew/cupcake.bin`**. Play. |
| **You (releases)** | Push to `main` → GitHub Actions builds `cupcake.bin` → tag `v*` → Release attaches the bin. |
| **Developer** | Clone **this repo only**. Install `arm-none-eabi-gcc`. Run `make cupcake-compile-check` then `make cupcake-bin`. |

You do **not** need the firmware source repo to build or ship game updates.

---

## Goal 1 — Firmware (one time)

Flash [game-and-watch-retro-go-sd-cupcake](https://github.com/sylverb/game-and-watch-retro-go-sd-cupcake) (or your fork with the Cupcake overlay slot).

That firmware already knows how to:

1. Find `cupcake.bin` on the SD card  
2. Load it into RAM  
3. Start the game  

After that, you only replace `cupcake.bin` when the game updates.

---

## Goal 2 — Build `cupcake.bin` without the firmware repo

From this port repo:

```bash
# MSYS2 MINGW64 — one-time ARM toolchain:
pacman -S mingw-w64-x86_64-arm-none-eabi-gcc mingw-w64-x86_64-arm-none-eabi-binutils mingw-w64-x86_64-arm-none-eabi-newlib

# Compile all overlay objects (no firmware checkout):
make cupcake-compile-check

# Link release/cupcake.bin (ABI — no firmware.elf):
make cupcake-bin
```

Output: **`release/cupcake.bin`**

### How it works (without jargon)

- `cupcake.bin` is **not** the full firmware — it is **only the game plugin**.
- At runtime the game calls into **flashed firmware** through the stable **`gw_firmware_abi`** table (published at a fixed address in firmware flash).
- The port repo links with **`platform/gnw/abi_stubs.c`** — no per-firmware symbol map, no `firmware.elf` at build time.
- Rebuild `cupcake.bin` only when **you** change the game, or when firmware bumps **`GW_FIRMWARE_ABI_VERSION`** (rare).

---

## Goal 3 — GitHub Actions releases

Workflow: **`.github/workflows/cupcake-bin.yml`**

On push to `main` or PR:

1. Install ARM toolchain  
2. Build `cupcake.bin` from this repo only  
3. Upload artifact  

On tag `v*` (e.g. `v0.9.11`):

1. Same build  
2. Attach `cupcake.bin` + `docs/ATTRIBUTION.md` to the GitHub Release  

Users download **`cupcake.bin`** and copy to SD. No compile step.

---

## SD card layout (player)

```
/roms/homebrew/cupcake.bin          ← game (from Release)
/retro-go/cupcake/screen.jpg        ← assets (interim; RG-4 will embed)
/retro-go/cupcake/sprites-color.png
/retro-go/cupcake/audio/*.wav
```

---

## Related docs

- [OVERLAY.md](OVERLAY.md) — technical overlay details  
- [BUILD_WINDOWS.md](BUILD_WINDOWS.md) — PC + toolchain setup  
- [GW_SPRINT_BOARD.md](../GW_SPRINT_BOARD.md) — active ship board  
- Firmware [OVERLAY_CUPCAKE.md](../../game-and-watch-retro-go-sd-cupcake/docs/OVERLAY_CUPCAKE.md) — firmware-side slot

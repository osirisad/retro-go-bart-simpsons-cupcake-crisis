# Cupcake Crisis — `cupcake.bin` for Game & Watch (plain guide)

## The simple version

| Who | What you do |
|-----|-------------|
| **Player** | Flash **firmware once** (includes Cupcake support). Download **`cupcake.bin`** from Releases. Copy to SD **`/roms/homebrew/cupcake.bin`**. Play. |
| **You (releases)** | Push to `main` → GitHub Actions builds `cupcake.bin` → attach to Release. |
| **Developer** | Clone **this repo only**. Install `arm-none-eabi-gcc`. Run `make cupcake-bin`. |

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
# One-time: install ARM toolchain (see docs/BUILD_WINDOWS.md)
make cupcake-bin
```

Output: **`release/cupcake.bin`**

### How it works (without jargon)

- `cupcake.bin` is **not** the full firmware — it is **only the game plugin**.
- The game calls into code that **already lives in flashed firmware** (buttons, screen, audio, file I/O).
- To compile, we need the **addresses** of those firmware functions for your firmware version — not the firmware source.
- Those addresses live in **`platform/gnw/firmware_imports.ld`** (checked into this repo for releases).

If you flash a **different** firmware version, regenerate the imports file:

```bash
bash tools/gen_overlay_imports.sh /path/to/firmware.elf > platform/gnw/firmware_imports.ld
```

`firmware.elf` is a build artifact (or release download), not the whole source tree.

---

## Goal 3 — GitHub Actions releases

Workflow: **`.github/workflows/cupcake-bin.yml`**

On push to `main`:

1. Install ARM toolchain  
2. Build `cupcake.bin` from this repo  
3. Upload artifact (and attach to GitHub Release when you tag)

Users download **`cupcake.bin`** and copy to SD. No compile step.

---

## SD card layout (player)

```
/roms/homebrew/cupcake.bin          ← game (from Release)
/retro-go/cupcake/screen.jpg        ← assets (interim; OV-3 will embed)
/retro-go/cupcake/sprites-color.png
/retro-go/cupcake/audio/*.wav
```

---

## Related docs

- [OVERLAY.md](OVERLAY.md) — technical overlay details  
- [BUILD_WINDOWS.md](BUILD_WINDOWS.md) — PC + toolchain setup  
- Firmware [OVERLAY_CUPCAKE.md](../../game-and-watch-retro-go-sd-cupcake/docs/OVERLAY_CUPCAKE.md) — firmware-side slot

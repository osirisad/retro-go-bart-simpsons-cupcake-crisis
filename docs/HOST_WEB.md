# Running the original game in a browser (reference / “emulator”)

For **pixel-perfect** graphics and placement, the fastest path is to run RetroFab’s own engine (JavaScript + Three.js) with the HAR assets — not the C 2D blit approximation.

## Quick start (Git Bash / Linux / macOS)

```bash
cd bart_simpson_cupcake_crisis_port/ignore/har_extracted
python -m http.server 8765
```

Open: http://localhost:8765/

Use the itch capture’s `index.html` + `build.js` + `sim-acclaim-cupcakecrisis/` tree. You still need atlas PNGs in `sim-acclaim-cupcakecrisis/game/` (`sprites-color.png`, `screen.jpg`, etc.) — see `docs/HAR.md`.

## C port vs browser host

| | **C + SDL** (`build-pc/cupcake-sdl.exe`) | **Browser / WebView** |
|---|------------------------------------------|------------------------|
| Placement | 2D projection of mesh + masks (approximates 3D LCD) | Exact Three.js + camera from `device.model` |
| Target | retro-go handheld, low RAM | PC dev, fidelity reference |
| Logic | Reimplemented in `cupcake_game.c` | Original `AcclaimCupcakeCrisis.js` |
| Size | Small binary | Chromium/WebView2 + JS bundle |

A **hybrid** workflow works well:

1. Run the **web** build to confirm behavior and screenshots.
2. Use **`demo.model`** frame lists to test the C renderer.
3. Tune C only where needed (audio, handheld I/O).

## Embedded “emulator” on Windows

Possible wrappers: **WebView2** (Edge), **CEF**, or a minimal **Electron** shell that:

- Serves `ignore/har_extracted/` over `http://127.0.0.1/`
- Maps keyboard to RetroFab buttons
- Saves no port of game logic — only hosts JS

That is easier than re-deriving every material position in C, but it does **not** replace a native port for retro-go.

## Why the C port can look “slightly off”

RetroFab draws one **3D LCD mesh**; each material (`bart2`, `cake31`, …) is a subset of faces at different **depths**. A shared camera projects them onto the LCD texture.

The C port:

- Blits **flat** RGBA from `sprites-color.png` using generated LCD x/y
- Uses triangle **masks** so atlas bleed matches the export tool
- Positions come from **projected** mesh bounds (`tools/gen_lcd_positions.py` uses the play camera from `device.model`)

Remaining gaps vs itizso: LCD curvature on the physical case mesh, and any camera mode other than `"play"` (mobile/zoom). PC port uses full visible `screen.jpg` (1024×800) as the sprite LCD. For exact parity, compare against the browser build at the same window size.

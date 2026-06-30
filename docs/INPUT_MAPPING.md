# Input mapping — RetroFab JS → C port

Source of truth for game logic: `AcclaimCupcakeCrisis.onPress` / `onRelease` in `docs/AcclaimCupcakeCrisis.js`.  
Physical labels: `ignore/har_extracted/assets-acclaim-superplay/device/device.model` and Cupcake instructions in `sim-acclaim-cupcakecrisis/themes/notes/notes.model`.

All hosts pack inputs into one `uint16_t` passed to `cupcake_set_buttons()`. Bit indices match `cupcake_handle_input()` and `CUPCAKE_CB_BTN`.

On the real G&W there are **no** separate Level 1 / Level 2 buttons (those are browser-only F1/F2 shortcuts). Choose difficulty by pressing **Select** in attract mode: level cycles **0 → L-1 → L-2 → attract**.

## Button table

| Bit | `CUPCAKE_BTN_*` | JS handler | G&W physical (Acclaim Superplay) | Game role | SDL PC (`platform/sdl/main.c`) | retro-go device | retro-go linux emu |
|-----|-----------------|------------|----------------------------------|-----------|-------------------------------|-----------------|-------------------|
| 0 | `LEFT` | `Left` | D-pad left | `onMove` (play, when enabled) | ← / A | D-pad left | ← |
| 1 | `RIGHT` | `Right` | D-pad right | `onMove` | → / D | D-pad right | → |
| 2 | `UP` | `Up` | D-pad up — **Sit** | `onMove` | ↑ / W | D-pad up | ↑ |
| 3 | `DOWN` | `Down` | D-pad down — **Sit** | `onMove` | ↓ / S | D-pad down | ↓ |
| 4 | `ACTION` | `Action` | **A** + **GAME** — Start / throw | `onAction` | **Z** | **A**, **GAME** | **Z** (SDL keyboard) |
| 5 | `SELECT` | `Select` | **B** + **TIME** — level / **CON** | `onSelect` | **X** | **B**, **TIME** | **X** |
| 6 | `LEVEL1` | `Level1` | *(browser only — hidden on device)* | `onQuickStart(1)` | **1** | — | **1** |
| 7 | `LEVEL2` | `Level2` | *(browser only — hidden on device)* | `onQuickStart(2)` | **2** | — | **2** |
| 8 | `SOUND` | `Sound` (`onRelease`) | **PAUSE** — Sound on/off *(PC / linux emu only; G&W PAUSE is retro-go menu)* | host mute toggle | **F6** | — | **F6** |

\* retro-go `Makefile.cupcake` linux emu build defines `LINUX_EMU` and uses the same SDL keyboard table as the PC port (Z/X/F6/1/2), not the generic `odroid_input.c` key map.

## Attract / level select flow

1. Demo runs with hi-score (level **0**).
2. Press **Select** → **L-1** (demo freezes, beginner selected).
3. Press **Select** again → **L-2** (advanced).
4. Press **Select** again → back to attract (level **0**).
5. Press **Action** with L-1 or L-2 showing → start intro → play.

PC dev keys **1** / **2** still jump straight to a level intro (JS `Level1` / `Level2` shortcuts).

## Not wired in the C port

| JS / device | Notes |
|-------------|--------|
| `Reset` (btnReset) | ACL reset — not implemented in port |

## Implementation

| Host | File | Mapping helper |
|------|------|----------------|
| SDL PC | `platform/sdl/main.c` | `cupcake_input_from_sdl_keyboard()` |
| retro-go | `platform/retrogo/main.c` | `LINUX_EMU`: SDL keyboard helper; device: `cupcake_input_from_odroid()` |
| Shared | `platform/cupcake_input.c` | Both helpers — single bit layout |

## Handler routing (`cupcake_handle_input`)

| Input | Handler | When active |
|-------|---------|-------------|
| L/R/U/D | `cupcake_on_move` | `play` + `enabled` |
| Action | `cupcake_on_action` | CON / demo start / play throw |
| Select | `cupcake_on_select` | Demo level cycle; over → CON |
| Level1 / Level2 | `cupcake_on_quick_start` | Any time (PC dev shortcuts) |
| Sound (release) | `CUPCAKE_CB_SOUND_TOGGLE` | Host toggles `host_audio` mute |

See also `docs/GAME_LOGIC.md` (input summary) and `BUILD_WINDOWS.md` (PC keys).

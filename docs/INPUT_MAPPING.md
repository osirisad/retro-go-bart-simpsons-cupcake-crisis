# Input mapping — RetroFab JS → C port

Source of truth for game logic: `AcclaimCupcakeCrisis.onPress` in `docs/AcclaimCupcakeCrisis.js`.  
Physical labels: `ignore/har_extracted/assets-acclaim-superplay/device/device.model` and Cupcake instructions in `sim-acclaim-cupcakecrisis/themes/notes/notes.model`.

All hosts pack inputs into one `uint16_t` passed to `cupcake_set_buttons()`. Bit indices match `cupcake_handle_input()` and `CUPCAKE_CB_BTN`.

## Button table

| Bit | `CUPCAKE_BTN_*` | JS `onPress` | G&W physical (Acclaim Superplay) | Game role | SDL PC (`platform/sdl/main.c`) | retro-go device | retro-go linux emu (`odroid_input.c` keys) |
|-----|-----------------|--------------|----------------------------------|-----------|-------------------------------|-----------------|---------------------------------------------|
| 0 | `LEFT` | `Left` | btnLeft — move left | `onMove` (play, when enabled) | ← / A | D-pad left | ← |
| 1 | `RIGHT` | `Right` | btnRight — move right | `onMove` | → / D | D-pad right | → |
| 2 | `UP` | `Up` | btnUp — **Sit** (couch) | `onMove` | ↑ / W | D-pad up | ↑ |
| 3 | `DOWN` | `Down` | btnDown — **Sit** | `onMove` | ↓ / S | D-pad down | ↓ |
| 4 | `ACTION` | `Action` | btnAction — **Start / Action** | `onAction` / throw in play | **Z** | **A** button | **Z** → `ODROID_INPUT_B` * |
| 5 | `SELECT` | `Select` | btnSelect — level / **CON** | `onSelect` | **X** | **B** button | **X** → `ODROID_INPUT_A` * |
| 6 | `LEVEL1` | `Level1` | btnLevel1 — Level 1 (Beginner) | `onQuickStart(1)` | **1** | **START** | **LShift** → `ODROID_INPUT_START` |
| 7 | `LEVEL2` | `Level2` | btnLevel2 — Level 2 (Advanced) | `onQuickStart(2)` | **2** | **X** (menu key) | **Q** → `ODROID_INPUT_X` |

\* On the linux retro-go emulator, `odroid_input.c` maps keyboard **Z** to `ODROID_INPUT_B` and **X** to `ODROID_INPUT_A`. The cupcake retro-go host uses the shared SDL keyboard table when `LINUX_EMU` is defined (same as the PC SDL build), so **Z = Action** and **X = Select** match the PC port. On real G&W firmware, **A = Action** and **B = Select** (same bits 4 and 5 as Celeste on device).

## Not wired in the C port

| JS / device | Notes |
|-------------|--------|
| `Sound` (btnSound) | Handheld mute — handled by retro-go OS, not game core |
| `Reset` (btnReset) | ACL reset — not implemented in port |
| Browser-only defaults | RetroFab browser maps Select→F5, Level1→F1, Level2→F2, Action→Ctrl/Enter; PC port uses X/Z/1/2 instead |

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
| Level1 / Level2 | `cupcake_on_quick_start` | Any time (stops sim, start intro) |

See also `docs/GAME_LOGIC.md` (input summary) and `BUILD_WINDOWS.md` (PC keys).

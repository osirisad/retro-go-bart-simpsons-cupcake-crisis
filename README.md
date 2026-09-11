# Bart Simpson's Cupcake Crisis

A C port of the 1990 Acclaim SuperPlay LCD handheld for
[Retro-Go SD](https://github.com/sylverb/game-and-watch-retro-go-sd) on
Nintendo Game & Watch. Game logic and assets come from the
[RetroFab simulation by Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis).

This tree is a **GWHB homebrew** (`PROJECT_KIND=homebrew`, ABI v2 +
`gw_core_bridge`) using sylverb's Retro-Go SD template. It produces a
single `Cupcake.bin` — graphics and audio are embedded; there is no
sidecar `.dat`.

## About the game

**Bart Simpson's™ Cupcake Crisis** (Acclaim Entertainment, SuperPlay model
40214, 1990). Bart's got his hands full with cupcakes — Maggie is throwing
them as fast as he can catch them, and he has to get them over to Marge
before Homer and Lisa join in.

Move Bart **left** and **right** to catch cupcakes from the couch. Miss one
and you lose a life. Bart can hold at most **5** cupcakes; catch a sixth
and you lose a life. When Marge appears, press **Action** to hand them
over. Hand all 5 at once for bonus points.

When Homer appears, move Bart **right** to the couch and press **Sit** to
join the family for TV. The sooner Bart sits, the more points you score —
but if Lisa gets there first, you lose a life. Three lives and it's game
over.

## Installing on Game & Watch

Requires firmware that loads **GWHB** homebrews (ABI v2+). Copy the
release binary to the SD card:

| File | SD path |
|------|---------|
| `Cupcake.bin` | `/homebrews/Cupcake.bin` |

Optional cover override: `/covers/homebrew/Cupcake.img`. Pause + d-pad
adjusts volume/brightness (in-game HUD).

When redistributing builds, include `assets/license.txt` (RetroFab
CC-BY-NC-ND terms).

## Building

### Device (`Cupcake.bin`)

On Windows the reliable path is Docker (image
`sylverb/retro-go-sd-builder`):

```bash
make docker
```

With a local `arm-none-eabi-gcc` toolchain:

```bash
make                    # PROJECT_KIND=homebrew is the default
```

Produces `Cupcake.bin` in the repo root.

Optional SD session trace (bring-up only):

```bash
make CUPCAKE_TRACE_SD=1
```

### Host SDL preview (Linux / macOS / WSL)

Same `app_main` and embedded assets as the device — not a Windows MSYS2
SDL build:

```bash
make host                       # SDL2 → ./cupcake_host
make host HOST_SDL=3            # SDL3
./cupcake_host                  # Esc / close window to quit
```

On macOS, if `pkg-config sdl2` fails:

```bash
export PKG_CONFIG_PATH="$(brew --prefix)/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
```

Controls: arrows = D-pad, `Z`/`X` = B/A, Enter = Start, Shift = Select.
Scale with `HOST_SCALE=2` (default).

## Credit

This port builds on the RetroFab recreation by
**[Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis)** —
sprites, audio, and game rules originate from that work. Device packaging
uses sylverb's Retro-Go SD GWHB template. Unofficial fan project; not
affiliated with Acclaim, Fox, or the original authors.

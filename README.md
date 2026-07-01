# Bart Simpson's Cupcake Crisis

A C port of the 1990 Acclaim SuperPlay LCD handheld, designed to run on [retro-go](https://github.com/ducke1937/retro-go) Game & Watch handhelds. Game logic and assets are ported from the excellent [RetroFab simulation by Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis).

## About the game

**Bart Simpson's™ Cupcake Crisis** (Acclaim Entertainment, SuperPlay model 40214, 1990) is a handheld LCD game based on *The Simpsons*. Bart's got his hands full with cupcakes - Maggie is throwing them as fast as he can catch them, and he has to get them over to Marge before Homer and Lisa join in.

Move Bart **left** and **right** to catch cupcakes from the couch. Miss one and you lose a life. Bart can hold at most **5** cupcakes; catch a sixth and you lose a life. When Marge appears, press **Action** to hand them over. Hand all 5 at once for bonus points.

When Homer appears, move Bart **right** to the couch and press **Sit** to join the family for TV. The sooner Bart sits, the more points you score - but if Lisa gets there first, you lose a life. Three lives and it's game over.

## Installing on Game & Watch

Pre-built files are on [GitHub Releases](https://github.com/osirisad/retro-go-bart-simpsons-cupcake-crisis/releases). Download `cupcake.bin` and `cupcake_assets.dat` for the version you want, then copy them to your retro-go SD card:

| File | SD path |
|------|---------|
| `cupcake.bin` | `/roms/homebrew/cupcake.bin` |
| `cupcake_assets.dat` | `/roms/homebrew/cupcake_assets.dat` |

Both files are required — graphics are in the `.bin`; audio is loaded from the `.dat` at runtime. The game appears in the Homebrew menu after a reboot.

If you redistribute builds, include `license.txt` from the release (RetroFab CC-BY-NC-ND terms).

## Building

**Requirements:** gcc, SDL2, make. On Windows use MSYS2

### PC (SDL) - development

The default PC build includes debug helpers (Alt+1…6 phase jumps, `--start` shortcuts):

```bash
make          # or: ./build.sh on MSYS2
make run
```

Release-style PC build (no debug cheats):

```bash
make clean
make CFLAGS='-std=c99 -Wall -Wextra -O2 -g -MMD -MP'
```

Output: `build-pc/cupcake-sdl` (or `cupcake-sdl.exe` on Windows).

### retro-go (device)

Build the overlay binary locally:

```bash
make cupcake-bin
```

Output in `release/` — install using the same SD paths as above (`cupcake.bin` and `cupcake_assets.dat` → `/roms/homebrew/`).

Optional SD session trace log for bring-up (off by default):

```bash
make cupcake-bin CUPCAKE_TRACE_SD=1
```

Requires `arm-none-eabi-gcc` and assets under `assets/`.

## Credit

This port builds on the RetroFab recreation by **[Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis)** - sprites, audio, and game rules originate from that work. Unofficial fan project; not affiliated with Acclaim, Fox, or the original author.
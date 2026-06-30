# Bart Simpson's Cupcake Crisis

A C port of the 1990 Acclaim SuperPlay LCD handheld, designed to run on [retro-go](https://github.com/ducke1937/retro-go) Game & Watch handhelds. Game logic and assets are ported from the excellent [RetroFab simulation by Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis).

## About the game

**Bart Simpson's™ Cupcake Crisis** (Acclaim Entertainment, SuperPlay model 40214, 1990) is a handheld LCD game based on *The Simpsons*. Bart's got his hands full with cupcakes - Maggie is throwing them as fast as he can catch them, and he has to get them over to Marge before Homer and Lisa join in.

Move Bart **left** and **right** to catch cupcakes from the couch. Miss one and you lose a life. Bart can hold at most **5** cupcakes; catch a sixth and you lose a life. When Marge appears, press **Action** to hand them over. Hand all 5 at once for bonus points.

When Homer appears, move Bart **right** to the couch and press **Sit** to join the family for TV. The sooner Bart sits, the more points you score - but if Lisa gets there first, you lose a life. Three lives and it's game over.

## Building

**Requirements:** gcc, SDL2, make. On Windows use MSYS2 - see [docs/BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md).

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

Build the overlay binary for SD install (`/roms/homebrew/cupcake.bin`):

```bash
make cupcake-bin
```

Optional SD session trace log for bring-up (off by default):

```bash
make cupcake-bin CUPCAKE_TRACE_SD=1
```

Also copy `release/cupcake_assets.dat` to `/roms/homebrew/` on the SD card.

Requires `arm-none-eabi-gcc` and assets under `assets/` (see [docs/BUILD.md](docs/BUILD.md) for full details).

## Credit

This port builds on the RetroFab recreation by **[Itizso](https://itizso.itch.io/acclaim-bart-simpsons-cupcake-crisis)** - sprites, audio, and game rules originate from that work. Unofficial fan project; not affiliated with Acclaim, Fox, or the original author.
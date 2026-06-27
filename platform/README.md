# Platform hosts

| Directory | Role |
|-----------|------|
| `sdl/` | PC test binary — full bezel (`screen.jpg`) + LCD sprite layer |
| `retrogo/` | G&W `linux` emulator — `screen.jpg` bezel + LCD composite @ 320×240 |
| `gnw/` | Device overlay host — `app_main_cupcake()` → `cupcake.bin` on SD |
| `host_draw.c` | Shared atlas → LCD blit (used by all hosts) |
| `stb/` | stb_image for PNG/JPG without SDL_image |

**`platform/sdl/main.c`**, **`platform/retrogo/main.c`**, and **`platform/gnw/main_cupcake.c`** are the three hosts; keep game logic in `src/cupcake_game.c`. See [docs/OVERLAY.md](../docs/OVERLAY.md).

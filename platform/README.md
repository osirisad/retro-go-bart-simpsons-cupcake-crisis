# Platform hosts

| Directory | Role |
|-----------|------|
| `sdl/` | PC test binary — full bezel (`screen.jpg`) + LCD sprite layer |
| `gnw/` | Device overlay host — `app_main_cupcake()` → `cupcake.bin` on SD ([gnw/README.md](gnw/README.md)) |
| `host_draw.c` | Shared atlas → LCD blit (used by all hosts) |
| `stb/` | stb_image for PNG/JPG without SDL_image |

**`platform/sdl/main.c`** and **`platform/gnw/main_cupcake.c`** are the two hosts; keep game logic in `src/cupcake_game.c`.

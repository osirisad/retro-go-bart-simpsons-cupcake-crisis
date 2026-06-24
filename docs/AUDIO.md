# Audio (SFX)

Cupcake Crisis ships **17 sound effects** from `game.model`:

| ID | File | Volume |
|----|------|--------|
| bonus | audio/bonus.mp3 | 0.8 |
| couch | audio/couch.mp3 | 1.0 |
| cupcake | audio/cupcake.mp3 | 1.0 |
| deliver | audio/deliver.mp3 | 0.9 |
| five | audio/five.mp3 | 1.0 |
| marge | audio/marge.mp3 | 1.0 |
| miss | audio/miss.mp3 | 1.0 |
| move | audio/move.mp3 | 0.8 |
| over | audio/over.mp3 | 0.7 |
| pacifier1 | audio/pacifier1.mp3 | 1.0 |
| pacifier2 | audio/pacifier2.mp3 | 1.0 |
| phase | audio/phase.mp3 | 1.0 |
| points | audio/points.wav | 0.8 |
| start | audio/start.mp3 | 0.6 |
| step | audio/step.wav | 1.0 |
| throw | audio/throw.mp3 | 0.84 |
| whoa | audio/whoa.mp3 | 0.8 |

Assets are **not committed** (see [LEGAL.md](LEGAL.md)).

## Extract from HAR

The itch.io HAR capture includes all 17 clips under  
`sim-acclaim-cupcakecrisis/game/audio/`.

From the port root:

```bash
python tools/extract_audio.py
# or
make extract-audio
```

This writes originals to `assets/audio/` (e.g. `bonus.mp3`, `points.wav`).

When **ffmpeg** is on `PATH`, the script also builds `assets/audio/{id}.wav`  
(44.1 kHz stereo) for the PC host (`SDL_mixer` / `Mix_LoadWAV`).

MSYS2:

```bash
pacman -S mingw-w64-x86_64-SDL2_mixer ffmpeg
make extract-audio
mingw32-make
```

## PC playback

`platform/host_audio.c` loads `{id}.wav` and plays via `CUPCAKE_CB_SFX`.  
Special id `stop` halts all channels (JS `sounds.stop()` — TASK-41).

Set `CUPCAKE_ASSETS=assets` if you run the binary from another directory.

## retro-go (linux emu)

`platform/retrogo/main.c` uses the same SDL_mixer backend as the PC host. Build from `game-and-watch-retro-go-sd/linux`:

```bash
export CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port
make extract-audio   # in port tree
make -f $CUPCAKE_PORT/platform/retrogo/Makefile.cupcake
```

Copy `assets/audio/*.wav` to `$CUPCAKE_ASSETS/audio/` (default `/home/odroid/cupcake/audio/`).

## Device firmware (STM32)

Compile `host_audio.c` with `-DCUPCAKE_AUDIO_ODROID` (without `LINUX_EMU`). Loads `{id}.wav`, mixes up to 12 voices, resamples to `odroid_audio_sample_rate_get()`, and submits via `odroid_audio_submit()`. Call `host_audio_pump(sample_rate / 30)` once per game frame from the port main loop.

Use 22050 Hz WAVs for best quality on hardware:

```bash
python tools/extract_audio.py --wav --rate 22050
```

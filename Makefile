# PC/SDL build (default). Device overlay: make cupcake-bin (platform/gnw/).
TARGET   = cupcake-sdl
BUILD    = build-pc
EXE      =
LICENSE_TXT = assets/license.txt

SRCS = \
	src/cupcake_game.c \
	src/cupcake_scoreboard.c \
	src/cupcake_hiscore.c \
	src/cupcake_timer.c \
	src/cupcake_rng.c \
	src/cupcake_state.c \
	platform/cupcake_input.c \
	platform/sdl/main.c \
	platform/host_draw.c \
	platform/host_audio.c \
	platform/host_audio_catalog.c \
	platform/stb/stb_image_impl.c

CC       ?= gcc
CFLAGS   ?= -std=c99 -Wall -Wextra -O2 -g -MMD -MP -DCUPCAKE_DEBUG_CHEATS
INCLUDES = -Isrc -Iplatform -Iplatform/stb
LDFLAGS  ?= -lm

SDL2_CFLAGS ?= $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS   ?= $(shell pkg-config --libs sdl2 2>/dev/null)

SDL2_MIXER_CFLAGS ?= $(shell pkg-config --cflags SDL2_mixer 2>/dev/null)
SDL2_MIXER_LIBS   ?= $(shell pkg-config --libs SDL2_mixer 2>/dev/null)

ifeq ($(SDL2_LIBS),)
  SDL2_CFLAGS := -IC:/msys64/mingw64/include/SDL2 -Dmain=SDL_main
  SDL2_LIBS   := -LC:/msys64/mingw64/lib -lSDL2main -lSDL2
endif

# pkg-config on MSYS2 often omits cflags for SDL2_mixer while still returning -lSDL2_mixer
ifeq ($(SDL2_MIXER_CFLAGS),)
  SDL2_MIXER_CFLAGS := -IC:/msys64/mingw64/include/SDL2
endif
ifeq ($(SDL2_MIXER_LIBS),)
  SDL2_MIXER_LIBS := -LC:/msys64/mingw64/lib -lSDL2_mixer
endif

CFLAGS  += $(SDL2_CFLAGS) $(SDL2_MIXER_CFLAGS) $(INCLUDES)
LDFLAGS += $(SDL2_LIBS) $(SDL2_MIXER_LIBS)

OBJS = $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o)))

vpath %.c src platform platform/sdl platform/stb

# Rebuild when generated headers change (make has no automatic .h deps otherwise).
CUPCAKE_HDRS = src/cupcake.h src/cupcake_port.h src/cupcake_sprites.h \
	src/cupcake_sprite_lcd.h src/cupcake_sprite_masks.h src/cupcake_demo.h \
	src/cupcake_timer.h \
	src/cupcake_rng.h \
	src/cupcake_state.h \
	src/cupcake_hiscore.h \
	platform/host_draw.h platform/sprite_blit.h

all: $(BUILD)/$(TARGET)$(EXE) $(BUILD)/license.txt

$(BUILD)/$(TARGET)$(EXE): $(OBJS) | $(BUILD)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD)/license.txt: $(LICENSE_TXT) | $(BUILD)
	cp $< $@

$(BUILD)/cupcake_game.o: src/cupcake_game.c $(CUPCAKE_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -c src/cupcake_game.c -o $@

$(BUILD)/cupcake_timer.o: src/cupcake_timer.c src/cupcake_timer.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/cupcake_timer.c -o $@

$(BUILD)/cupcake_rng.o: src/cupcake_rng.c src/cupcake_rng.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/cupcake_rng.c -o $@

$(BUILD)/cupcake_state.o: src/cupcake_state.c src/cupcake_state.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/cupcake_state.c -o $@

$(BUILD)/main.o: platform/sdl/main.c $(CUPCAKE_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -c platform/sdl/main.c -o $@

$(BUILD)/host_draw.o: platform/host_draw.c $(CUPCAKE_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) -c platform/host_draw.c -o $@

$(BUILD)/cupcake_input.o: platform/cupcake_input.c platform/cupcake_input.h src/cupcake.h | $(BUILD)
	$(CC) $(CFLAGS) -c platform/cupcake_input.c -o $@

$(BUILD)/stb_image_impl.o: platform/stb/stb_image_impl.c | $(BUILD)
	$(CC) $(CFLAGS) -c platform/stb/stb_image_impl.c -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(wildcard $(BUILD)/*.d)

$(BUILD):
	mkdir -p $(BUILD)

run: all
	CUPCAKE_ASSETS=assets $(BUILD)/$(TARGET)$(EXE)

gen:
	python tools/gen_sprites.py
	python tools/gen_lcd_positions.py
	python tools/gen_demo_data.py

# Bake assets/lcd_tune.txt -> src/cupcake_sprite_lcd.h (no PIL; safe after alignment edits).
bake-lcd:
	python tools/bake_lcd_tune.py

# Audio from itch HAR (assets/audio/ — gitignored). Auto-builds id.wav when ffmpeg is on PATH.
extract-audio:
	python tools/extract_audio.py

clean:
	rm -rf $(BUILD)

gwhb:
	$(MAKE) -f platform/gnw/Makefile.gnw all

# Device overlay — standalone cupcake.bin via ABI (see docs/RELEASE_CUPCAKE_BIN.md)
cupcake-bin:
	$(MAKE) -f platform/gnw/Makefile.gnw all

cupcake-compile-check:
	$(MAKE) -f platform/gnw/Makefile.gnw compile-check

overlay-size-estimate:
	python tools/overlay_size_estimate.py

.PHONY: all run clean gen bake-lcd force-rebuild gwhb cupcake-bin cupcake-compile-check overlay-size-estimate

# Use if alignment edits in .h seem "cached" (also close cupcake-sdl.exe before make).
force-rebuild:
	rm -f $(OBJS) $(BUILD)/$(TARGET)$(EXE)
	$(MAKE)

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

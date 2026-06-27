# Include from firmware Makefile when CUPCAKE_PORT points at this repo:
#   make CUPCAKE_PORT=/path/to/bart_simpson_cupcake_crisis_port

CUPCAKE_C_SOURCES = \
$(CUPCAKE_PORT)/src/cupcake_game.c \
$(CUPCAKE_PORT)/src/cupcake_scoreboard.c \
$(CUPCAKE_PORT)/src/cupcake_hiscore.c \
$(CUPCAKE_PORT)/src/cupcake_timer.c \
$(CUPCAKE_PORT)/src/cupcake_rng.c \
$(CUPCAKE_PORT)/src/cupcake_state.c \
$(CUPCAKE_PORT)/platform/host_draw.c \
$(CUPCAKE_PORT)/platform/host_audio.c \
$(CUPCAKE_PORT)/platform/host_audio_catalog.c \
$(CUPCAKE_PORT)/platform/cupcake_input.c \
$(CUPCAKE_PORT)/platform/stb/stb_image_impl.c \
$(CUPCAKE_PORT)/platform/gnw/gnw_assets.c \
$(CUPCAKE_PORT)/platform/gnw/main_cupcake.c

CUPCAKE_C_INCLUDES = \
-ICore/Inc \
-ICore/Src/porting/lib \
-ICore/Inc/retro-go \
-Iretro-go-stm32/components/odroid \
-I$(CUPCAKE_PORT)/src \
-I$(CUPCAKE_PORT)/platform \
-I$(CUPCAKE_PORT)/platform/stb \
-I./

CUPCAKE_CFLAGS = -DCUPCAKE_GNW -DCUPCAKE_AUDIO_ODROID -DTARGET_GNW

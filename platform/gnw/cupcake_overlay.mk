# Reference source list — duplicated in platform/gnw/Makefile.gnw.
# Upstream firmware builds only the in-tree smoke stub; full game is make cupcake-bin here.

_GNW_MK   := $(abspath $(lastword $(MAKEFILE_LIST)))
PORT_ROOT := $(abspath $(dir $(_GNW_MK))../..)

CUPCAKE_C_SOURCES = \
$(PORT_ROOT)/src/cupcake_game.c \
$(PORT_ROOT)/src/cupcake_scoreboard.c \
$(PORT_ROOT)/src/cupcake_hiscore.c \
$(PORT_ROOT)/src/cupcake_timer.c \
$(PORT_ROOT)/src/cupcake_rng.c \
$(PORT_ROOT)/src/cupcake_state.c \
$(PORT_ROOT)/platform/host_draw.c \
$(PORT_ROOT)/platform/host_audio.c \
$(PORT_ROOT)/platform/host_audio_catalog.c \
$(PORT_ROOT)/platform/cupcake_input.c \
$(PORT_ROOT)/platform/stb/stb_image_impl.c \
$(PORT_ROOT)/platform/gnw/gnw_assets.c \
$(PORT_ROOT)/platform/gnw/main_cupcake.c

CUPCAKE_CFLAGS = -DCUPCAKE_GNW -DCUPCAKE_AUDIO_ODROID -DTARGET_GNW

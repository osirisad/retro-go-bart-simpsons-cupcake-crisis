# Retro-Go SD — Bart Simpson's Cupcake Crisis (GWHB homebrew)
#
#   make                          # PROJECT_KIND=homebrew (default)
#   make docker
#   make CUPCAKE_TRACE_SD=1       # optional SD session trace log
#
# Upstream game: https://github.com/osirisad/retro-go-bart-simpsons-cupcake-crisis
# Verbose compiler lines: make V=

#######################################
# Project identity
#######################################
PROJECT_KIND ?= homebrew

CORE_NAME  := cupcake
CORE_ENTRY := app_main

GNW_CORE_SDK ?= sdk
BUILD_DIR ?= build/$(PROJECT_KIND)

# Generated embedded RGB565 + ADPCM archive (from tools/bundle_assets.py).
CUPCAKE_DATA_C := $(BUILD_DIR)/cupcake_data.c
CUPCAKE_DATA_H := $(BUILD_DIR)/cupcake_data.h
ASSETS_DAT     := $(BUILD_DIR)/cupcake_assets.dat
BUNDLE_PY      := tools/bundle_assets.py
ASSETS_DIR     := assets
COVER_SRC      := assets/screen.jpg
LICENSE_TXT    := assets/license.txt

CUPCAKE_TRACE_SD ?= 0

CORE_C_SOURCES := \
src/main.c \
src/gnw/main_cupcake.c \
src/gnw/gnw_assets.c \
src/gnw/cupcake_adpcm.c \
src/gnw/cupcake_assets_dat.c \
src/gnw/cupcake_compat.c \
src/cupcake/cupcake_game.c \
src/cupcake/cupcake_scoreboard.c \
src/cupcake/cupcake_hiscore.c \
src/cupcake/cupcake_timer.c \
src/cupcake/cupcake_rng.c \
src/cupcake/cupcake_state.c \
src/platform/host_draw.c \
src/platform/host_audio.c \
src/platform/host_audio_catalog.c \
src/platform/cupcake_input.c \
$(CUPCAKE_DATA_C)

ifeq ($(CUPCAKE_TRACE_SD),1)
CORE_C_SOURCES += src/gnw/cupcake_trace.c
endif

CORE_C_INCLUDES := \
-Isrc/cupcake \
-Isrc/platform \
-Isrc/gnw \
-I$(BUILD_DIR)

#######################################
# Kind-specific compile defs + packing
#######################################
ifeq ($(PROJECT_KIND),core)
$(error This project is homebrew-only — use PROJECT_KIND=homebrew)

else ifeq ($(PROJECT_KIND),homebrew)
CORE_C_DEFS := \
-DPROJECT_KIND_HOMEBREW=1 \
-DTARGET_GNW \
-DCUPCAKE_GNW \
-DCUPCAKE_ABI \
-DCUPCAKE_AUDIO_ODROID \
-DCUPCAKE_EMBEDDED_ASSETS

ifeq ($(CUPCAKE_TRACE_SD),1)
CORE_C_DEFS += -DCUPCAKE_TRACE_SD
endif

PACKED_BIN := Cupcake.bin
HB_NAME    := Cupcake Crisis
COVER_JPG  := $(BUILD_DIR)/cover.jpg
COVER_WIDTH  ?= 128
COVER_HEIGHT ?= 96

else
$(error PROJECT_KIND must be 'homebrew' (got '$(PROJECT_KIND)'))
endif

include $(GNW_CORE_SDK)/Makefile

PACK_HOMEBREW := $(GNW_CORE_SDK)/tools/pack_homebrew.py

#######################################
# Packed header version
#######################################
CORE_VERSION ?= $(shell cat VERSION 2>/dev/null || git describe --tags --dirty 2>/dev/null || echo NOTAG)

#######################################
# Asset bundle (RGB565 + ADPCM embedded in cupcake_data.c)
#######################################
.PHONY: bundle-assets

bundle-assets: $(CUPCAKE_DATA_C)

$(CUPCAKE_DATA_C) $(CUPCAKE_DATA_H) $(ASSETS_DAT): $(BUNDLE_PY) \
		$(ASSETS_DIR)/screen.jpg $(ASSETS_DIR)/sprites-color.png \
		src/platform/host_audio_catalog.c \
		$(wildcard $(ASSETS_DIR)/audio/*.wav)
	$(V)$(ECHO) [ BUNDLE ] embedded graphics + audio → $(CUPCAKE_DATA_C)
	$(V)mkdir -p $(BUILD_DIR)
	$(V)python3 $(BUNDLE_PY) \
		--assets $(ASSETS_DIR) \
		--catalog src/platform/host_audio_catalog.c \
		--out-h $(CUPCAKE_DATA_H) \
		--out-c $(CUPCAKE_DATA_C) \
		--export-assets-dat $(ASSETS_DAT)

# cupcake_data.c is listed in CORE_C_SOURCES; force bundle before compile.
$(C_OBJECTS): | $(CUPCAKE_DATA_C)

#######################################
# Pack
#######################################
.PHONY: pack cover

cover: $(COVER_JPG)

# Homebrew cover: ≤186×100 and ≤10 KiB (gui.c COVER_* limits).
$(COVER_JPG): $(COVER_SRC)
	$(V)$(ECHO) "[ COVER ] $(COVER_JPG) $(COVER_WIDTH)x$(COVER_HEIGHT)"
	$(V)mkdir -p $(BUILD_DIR)
	$(V)python3 -c "from pathlib import Path; from PIL import Image; \
img=Image.open('$(COVER_SRC)').convert('RGB'); \
img.thumbnail(($(COVER_WIDTH),$(COVER_HEIGHT))); \
canvas=Image.new('RGB', ($(COVER_WIDTH),$(COVER_HEIGHT)), (8,16,24)); \
x=($(COVER_WIDTH)-img.width)//2; y=($(COVER_HEIGHT)-img.height)//2; \
canvas.paste(img, (x,y)); \
canvas.save('$(COVER_JPG)', 'JPEG', quality=75, optimize=True); \
sz=Path('$(COVER_JPG)').stat().st_size; \
assert sz <= 10*1024, f'cover too big: {sz}'"

pack: $(TARGET_BIN) $(COVER_JPG) $(LICENSE_TXT)
	$(V)$(ECHO) [ PACK GWHB ] $(PACKED_BIN) version=$(CORE_VERSION)
	$(V)python3 $(PACK_HOMEBREW) \
		--elf $(TARGET_ELF) --bin $(TARGET_BIN) \
		--name "$(HB_NAME)" --version "$(CORE_VERSION)" \
		--cover $(COVER_JPG) \
		--out $(PACKED_BIN)
	$(V)$(ECHO) "Install: $(PACKED_BIN) → /homebrews/ (audio embedded)"
	$(V)$(ECHO) "Include $(LICENSE_TXT) when redistributing (RetroFab CC-BY-NC-ND)."

all: pack

# Read-only helpers for CI / scripts.
.PHONY: print-PROJECT_KIND print-PACKED_BIN print-CORE_NAME print-DOCKER_IMAGE \
	print-TARGET_ELF print-TARGET_MAP print-CORE_VERSION
print-PROJECT_KIND:
	@echo $(PROJECT_KIND)
print-PACKED_BIN:
	@echo $(PACKED_BIN)
print-CORE_NAME:
	@echo $(CORE_NAME)
print-DOCKER_IMAGE:
	@echo $(DOCKER_IMAGE)
print-TARGET_ELF:
	@echo $(TARGET_ELF)
print-TARGET_MAP:
	@echo $(BUILD_DIR)/$(CORE_NAME)_core.map
print-CORE_VERSION:
	@echo $(CORE_VERSION)

clean::
	$(V)rm -f $(PACKED_BIN) $(COVER_JPG)
	$(V)rm -f $(CUPCAKE_DATA_C) $(CUPCAKE_DATA_H) $(ASSETS_DAT)

#######################################
# Docker
#######################################
.PHONY: docker docker_pull docker_shell

RELEASE_VERSION ?= v1.5
DOCKER_REPOSITORY ?= sylverb/retro-go-sd-builder
DOCKER_IMAGE ?= $(DOCKER_REPOSITORY):$(RELEASE_VERSION)

DOCKER_TTY_FLAG := $(shell if [ -t 0 ]; then echo -it; else echo; fi)
DOCKER_USER := $(shell id -u):$(shell id -g)
DOCKER_RUN := docker run --rm $(DOCKER_TTY_FLAG) \
	--user $(DOCKER_USER) \
	-v "$(CURDIR):/opt/workdir" \
	-w /opt/workdir \
	$(DOCKER_IMAGE)

docker:
	$(V)$(ECHO) "[ DOCKER ]" $(DOCKER_IMAGE) "PROJECT_KIND=$(PROJECT_KIND)"
	$(V)$(DOCKER_RUN) make --no-print-directory -j$$(nproc) PROJECT_KIND=$(PROJECT_KIND)

docker_pull:
	$(V)$(ECHO) "[ PULL ]" $(DOCKER_IMAGE)
	$(V)docker pull $(DOCKER_IMAGE)

docker_shell:
	$(DOCKER_RUN) bash

#######################################
# Host SDL (same app_main + embedded assets as device)
#######################################
include host/Makefile.host

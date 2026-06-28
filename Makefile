# PC/SDL build (default). Same src/ core links into retro-go via platform/retrogo/Makefile.cupcake.
TARGET   = cupcake-sdl
BUILD    = build-pc
EXE      =

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

all: $(BUILD)/$(TARGET)$(EXE)

$(BUILD)/$(TARGET)$(EXE): $(OBJS) | $(BUILD)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

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

TEST_BUILD = build-test
CORE_GAME_SRCS = src/cupcake_game.c src/cupcake_scoreboard.c src/cupcake_hiscore.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c
TEST_TIMER = $(TEST_BUILD)/cupcake_timer_test$(EXE)

test-timer: $(TEST_TIMER)
	$(TEST_TIMER)

TEST_RNG = $(TEST_BUILD)/cupcake_rng_test$(EXE)

test-rng: $(TEST_RNG)
	$(TEST_RNG)

$(TEST_RNG): test/cupcake_rng_test.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_STATE = $(TEST_BUILD)/cupcake_state_test$(EXE)

test-state: $(TEST_STATE)
	$(TEST_STATE)

$(TEST_STATE): test/cupcake_state_test.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_START = $(TEST_BUILD)/cupcake_start_test$(EXE)

test-start: $(TEST_START)
	$(TEST_START)

$(TEST_START): test/cupcake_start_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_OVER = $(TEST_BUILD)/cupcake_over_test$(EXE)

test-over: $(TEST_OVER)
	$(TEST_OVER)

$(TEST_OVER): test/cupcake_over_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_PAUSE = $(TEST_BUILD)/cupcake_pause_test$(EXE)

test-pause: $(TEST_PAUSE)
	$(TEST_PAUSE)

$(TEST_PAUSE): test/cupcake_pause_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_INPUT = $(TEST_BUILD)/cupcake_input_test$(EXE)

test-input: $(TEST_INPUT)
	$(TEST_INPUT)

$(TEST_INPUT): test/cupcake_input_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_DEMO_START = $(TEST_BUILD)/cupcake_demo_start_test$(EXE)

test-demo-start: $(TEST_DEMO_START)
	$(TEST_DEMO_START)

$(TEST_DEMO_START): test/cupcake_demo_start_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_ACTION = $(TEST_BUILD)/cupcake_action_test$(EXE)

test-action: $(TEST_ACTION)
	$(TEST_ACTION)

$(TEST_ACTION): test/cupcake_action_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_QUICK_START = $(TEST_BUILD)/cupcake_quick_start_test$(EXE)

test-quick-start: $(TEST_QUICK_START)
	$(TEST_QUICK_START)

$(TEST_QUICK_START): test/cupcake_quick_start_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_DEBUG_START = $(TEST_BUILD)/cupcake_debug_start_test$(EXE)

test-debug-start: $(TEST_DEBUG_START)
	$(TEST_DEBUG_START)

$(TEST_DEBUG_START): test/cupcake_debug_start_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_DEMO_REPLAY = $(TEST_BUILD)/cupcake_demo_replay_test$(EXE)

test-demo-replay: $(TEST_DEMO_REPLAY)
	$(TEST_DEMO_REPLAY)

$(TEST_DEMO_REPLAY): test/cupcake_demo_replay_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_SMOKE = $(TEST_BUILD)/cupcake_smoke_test$(EXE)

test-smoke: $(TEST_SMOKE)
	$(TEST_SMOKE)

$(TEST_SMOKE): test/cupcake_smoke_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_SAVE_GAME = $(TEST_BUILD)/cupcake_save_game_test$(EXE)

test-save-game: $(TEST_SAVE_GAME)
	$(TEST_SAVE_GAME)

$(TEST_SAVE_GAME): test/cupcake_save_game_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_HOST_BEZEL = $(TEST_BUILD)/cupcake_host_bezel_test$(EXE)

test-host-bezel: $(TEST_HOST_BEZEL)
	$(TEST_HOST_BEZEL)

$(TEST_HOST_BEZEL): test/cupcake_host_bezel_test.c platform/host_draw.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_ENABLED = $(TEST_BUILD)/cupcake_enabled_test$(EXE)

test-enabled: $(TEST_ENABLED)
	$(TEST_ENABLED)

$(TEST_ENABLED): test/cupcake_enabled_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_DEMO_SCOREBOARD = $(TEST_BUILD)/cupcake_demo_scoreboard_test$(EXE)

test-demo-scoreboard: $(TEST_DEMO_SCOREBOARD)
	$(TEST_DEMO_SCOREBOARD)

$(TEST_DEMO_SCOREBOARD): test/cupcake_demo_scoreboard_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_DEMO_SELECT = $(TEST_BUILD)/cupcake_demo_select_test$(EXE)

test-demo-select: $(TEST_DEMO_SELECT)
	$(TEST_DEMO_SELECT)

$(TEST_DEMO_SELECT): test/cupcake_demo_select_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_SCOREBOARD = $(TEST_BUILD)/cupcake_scoreboard_test$(EXE)

test-scoreboard: $(TEST_SCOREBOARD)
	$(TEST_SCOREBOARD)

$(TEST_SCOREBOARD): test/cupcake_scoreboard_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_ADD_BONUS = $(TEST_BUILD)/cupcake_add_bonus_test$(EXE)

test-add-bonus: $(TEST_ADD_BONUS)
	$(TEST_ADD_BONUS)

$(TEST_ADD_BONUS): test/cupcake_add_bonus_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_HISCORE = $(TEST_BUILD)/cupcake_hiscore_test$(EXE)

test-hiscore: $(TEST_HISCORE)
	$(TEST_HISCORE)

$(TEST_HISCORE): test/cupcake_hiscore_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_ADD_POINTS = $(TEST_BUILD)/cupcake_add_points_test$(EXE)

test-add-points: $(TEST_ADD_POINTS)
	$(TEST_ADD_POINTS)

$(TEST_ADD_POINTS): test/cupcake_add_points_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_PHASE_INDICATOR = $(TEST_BUILD)/cupcake_phase_indicator_test$(EXE)

test-phase-indicator: $(TEST_PHASE_INDICATOR)
	$(TEST_PHASE_INDICATOR)

$(TEST_PHASE_INDICATOR): test/cupcake_phase_indicator_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_LEVEL_INDICATOR = $(TEST_BUILD)/cupcake_level_indicator_test$(EXE)

test-level-indicator: $(TEST_LEVEL_INDICATOR)
	$(TEST_LEVEL_INDICATOR)

$(TEST_LEVEL_INDICATOR): test/cupcake_level_indicator_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_PHASE_COMPLETE = $(TEST_BUILD)/cupcake_phase_complete_test$(EXE)

test-phase-complete: $(TEST_PHASE_COMPLETE)
	$(TEST_PHASE_COMPLETE)

$(TEST_PHASE_COMPLETE): test/cupcake_phase_complete_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_GAME_TICK = $(TEST_BUILD)/cupcake_game_tick_test$(EXE)

test-game-tick: $(TEST_GAME_TICK)
	$(TEST_GAME_TICK)

$(TEST_GAME_TICK): test/cupcake_game_tick_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_RECORD_MODE = $(TEST_BUILD)/cupcake_record_mode_test$(EXE)

test-record-mode: $(TEST_RECORD_MODE)
	$(TEST_RECORD_MODE)

$(TEST_RECORD_MODE): test/cupcake_record_mode_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_START = $(TEST_BUILD)/cupcake_bart_start_test$(EXE)

test-bart-start: $(TEST_BART_START)
	$(TEST_BART_START)

$(TEST_BART_START): test/cupcake_bart_start_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_MOVE = $(TEST_BUILD)/cupcake_bart_move_test$(EXE)

test-bart-move: $(TEST_BART_MOVE)
	$(TEST_BART_MOVE)

$(TEST_BART_MOVE): test/cupcake_bart_move_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_SIT = $(TEST_BUILD)/cupcake_bart_sit_test$(EXE)

test-bart-sit: $(TEST_BART_SIT)
	$(TEST_BART_SIT)

$(TEST_BART_SIT): test/cupcake_bart_sit_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_ACTION = $(TEST_BUILD)/cupcake_bart_action_test$(EXE)

test-bart-action: $(TEST_BART_ACTION)
	$(TEST_BART_ACTION)

$(TEST_BART_ACTION): test/cupcake_bart_action_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_CATCH = $(TEST_BUILD)/cupcake_bart_catch_test$(EXE)

test-bart-catch: $(TEST_BART_CATCH)
	$(TEST_BART_CATCH)

$(TEST_BART_CATCH): test/cupcake_bart_catch_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_POSITION = $(TEST_BUILD)/cupcake_bart_position_test$(EXE)

test-bart-position: $(TEST_BART_POSITION)
	$(TEST_BART_POSITION)

$(TEST_BART_POSITION): test/cupcake_bart_position_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_MISS = $(TEST_BUILD)/cupcake_bart_miss_test$(EXE)

test-bart-miss: $(TEST_BART_MISS)
	$(TEST_BART_MISS)

$(TEST_BART_MISS): test/cupcake_bart_miss_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_BART_POSITION0 = $(TEST_BUILD)/cupcake_bart_position0_test$(EXE)

test-bart-position0: $(TEST_BART_POSITION0)
	$(TEST_BART_POSITION0)

$(TEST_BART_POSITION0): test/cupcake_bart_position0_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_CUPCAKES_STEP = $(TEST_BUILD)/cupcake_cupcakes_step_test$(EXE)

test-cupcakes-step: $(TEST_CUPCAKES_STEP)
	$(TEST_CUPCAKES_STEP)

$(TEST_CUPCAKES_STEP): test/cupcake_cupcakes_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_CUPCAKES_DRAW = $(TEST_BUILD)/cupcake_cupcakes_draw_test$(EXE)

test-cupcakes-draw: $(TEST_CUPCAKES_DRAW)
	$(TEST_CUPCAKES_DRAW)

$(TEST_CUPCAKES_DRAW): test/cupcake_cupcakes_draw_test.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_AIRCAKES_STEP = $(TEST_BUILD)/cupcake_aircakes_step_test$(EXE)

test-aircakes-step: $(TEST_AIRCAKES_STEP)
	$(TEST_AIRCAKES_STEP)

$(TEST_AIRCAKES_STEP): test/cupcake_aircakes_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_AIRCAKES_DRAW = $(TEST_BUILD)/cupcake_aircakes_draw_test$(EXE)

test-aircakes-draw: $(TEST_AIRCAKES_DRAW)
	$(TEST_AIRCAKES_DRAW)

$(TEST_AIRCAKES_DRAW): test/cupcake_aircakes_draw_test.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_MAGGIE_STEP = $(TEST_BUILD)/cupcake_maggie_step_test$(EXE)

test-maggie-step: $(TEST_MAGGIE_STEP)
	$(TEST_MAGGIE_STEP)

$(TEST_MAGGIE_STEP): test/cupcake_maggie_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_MARGE_STEP = $(TEST_BUILD)/cupcake_marge_step_test$(EXE)

test-marge-step: $(TEST_MARGE_STEP)
	$(TEST_MARGE_STEP)

$(TEST_MARGE_STEP): test/cupcake_marge_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_MARGE_COLLECT = $(TEST_BUILD)/cupcake_marge_collect_test$(EXE)

test-marge-collect: $(TEST_MARGE_COLLECT)
	$(TEST_MARGE_COLLECT)

$(TEST_MARGE_COLLECT): test/cupcake_marge_collect_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_COUCH_STEP = $(TEST_BUILD)/cupcake_couch_step_test$(EXE)

test-couch-step: $(TEST_COUCH_STEP)
	$(TEST_COUCH_STEP)

$(TEST_COUCH_STEP): test/cupcake_couch_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_PACIFIER_STEP = $(TEST_BUILD)/cupcake_pacifier_step_test$(EXE)

test-pacifier-step: $(TEST_PACIFIER_STEP)
	$(TEST_PACIFIER_STEP)

$(TEST_PACIFIER_STEP): test/cupcake_pacifier_step_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_ENTITIES_DRAW = $(TEST_BUILD)/cupcake_entities_draw_test$(EXE)

test-entities-draw: $(TEST_ENTITIES_DRAW)
	$(TEST_ENTITIES_DRAW)

$(TEST_ENTITIES_DRAW): test/cupcake_entities_draw_test.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_PLAY_DRAW = $(TEST_BUILD)/cupcake_play_draw_test$(EXE)

test-play-draw: $(TEST_PLAY_DRAW)
	$(TEST_PLAY_DRAW)

$(TEST_PLAY_DRAW): test/cupcake_play_draw_test.c src/cupcake_state.c src/cupcake_timer.c src/cupcake_rng.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_MARGE_LCD = $(TEST_BUILD)/cupcake_marge_lcd_test$(EXE)

test-marge-lcd: $(TEST_MARGE_LCD)
	$(TEST_MARGE_LCD)

$(TEST_MARGE_LCD): test/cupcake_marge_lcd_test.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

TEST_MISS_COUNTER = $(TEST_BUILD)/cupcake_miss_counter_test$(EXE)

test-miss-counter: $(TEST_MISS_COUNTER)
	$(TEST_MISS_COUNTER)

$(TEST_MISS_COUNTER): test/cupcake_miss_counter_test.c $(CORE_GAME_SRCS) | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

test test-all: test-smoke test-host-bezel test-demo-replay test-timer test-rng test-state test-save-game test-start test-over test-pause test-input test-demo-start test-action test-quick-start test-debug-start test-enabled test-demo-scoreboard test-demo-select test-scoreboard test-add-bonus test-hiscore test-add-points test-phase-indicator test-level-indicator test-phase-complete test-game-tick test-record-mode test-bart-start test-bart-move test-bart-sit test-bart-action test-bart-catch test-bart-position test-bart-miss test-bart-position0 test-cupcakes-step test-cupcakes-draw test-aircakes-step test-aircakes-draw test-maggie-step test-marge-step test-marge-collect test-couch-step test-pacifier-step test-entities-draw test-play-draw test-marge-lcd test-miss-counter

$(TEST_TIMER): test/cupcake_timer_test.c src/cupcake_timer.c | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Itest -Isrc $^ -o $@ $(LDFLAGS)

$(TEST_BUILD):
	mkdir -p $(TEST_BUILD)

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
	$(MAKE) -f platform/gwhb/Makefile.gwhb

# Device overlay — standalone cupcake.bin via ABI (see docs/RELEASE_CUPCAKE_BIN.md)
cupcake-bin:
	$(MAKE) -f platform/gnw/Makefile.gnw

cupcake-compile-check:
	$(MAKE) -f platform/gnw/Makefile.gnw compile-check

# OV-03 — run after changes to src/ or shared platform/ (host_*, cupcake_input)
test-overlay-regression-pc: test-smoke test-demo-replay test-host-bezel

test-overlay-regression: test-overlay-regression-pc
	@command -v bash >/dev/null 2>&1 && MAKE="$(MAKE)" SKIP_PC=1 bash scripts/overlay_regression.sh

overlay-size-estimate:
	python tools/overlay_size_estimate.py

.PHONY: all run clean gen bake-lcd force-rebuild gwhb cupcake-bin cupcake-compile-check test test-all test-host-bezel test-smoke test-demo-replay \
	test-overlay-regression test-overlay-regression-pc overlay-size-estimate test-save-game test-debug-start test-timer test-rng test-state test-start test-over test-pause test-input test-demo-start test-action test-quick-start test-enabled test-demo-scoreboard test-demo-select test-scoreboard test-add-bonus test-hiscore test-add-points test-phase-indicator test-level-indicator test-phase-complete test-game-tick test-record-mode test-bart-start test-bart-move test-bart-sit test-bart-action test-bart-catch test-bart-position test-bart-miss test-bart-position0 test-cupcakes-step test-cupcakes-draw test-aircakes-step test-aircakes-draw test-maggie-step test-marge-step test-marge-collect test-couch-step test-pacifier-step test-entities-draw test-play-draw test-marge-lcd test-miss-counter

# Use if alignment edits in .h seem "cached" (also close cupcake-sdl.exe before make).
force-rebuild:
	rm -f $(OBJS) $(BUILD)/$(TARGET)$(EXE)
	$(MAKE)

ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

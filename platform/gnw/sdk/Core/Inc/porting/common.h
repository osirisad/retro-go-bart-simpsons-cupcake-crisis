#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <odroid_system.h>
#include "gw_lcd.h"
#include "gw_audio.h"

#define WIDTH  320
#define HEIGHT 240

typedef void (*void_callback_t)(void);

void common_emu_frame_loop_reset(void);
bool common_emu_frame_loop(void);
void common_emu_input_loop(odroid_gamepad_state_t *joystick, odroid_dialog_choice_t *game_options,
                           void_callback_t repaint);
void common_emu_input_loop_handle_turbo(odroid_gamepad_state_t *joystick);
void common_emu_sound_sync(bool use_nops);

typedef struct {
    uint32_t last_sync_time;
    uint32_t last_overlay_time;
    uint16_t skipped_frames;
    int16_t frame_time_10us;
    uint8_t skip_frames:2;
    uint8_t pause_frames:1;
    uint8_t pause_after_frames:3;
    uint8_t startup_frames:2;
    uint8_t overlay:4;
    uint8_t clear_frames:2;
} common_emu_state_t;

extern common_emu_state_t common_emu_state;

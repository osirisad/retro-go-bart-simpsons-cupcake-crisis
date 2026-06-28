#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "porting.h"
#include "config.h"
#include "odroid_audio.h"
#include "odroid_input.h"
#include "odroid_overlay.h"

typedef bool (*state_handler_t)(const char *filename);
typedef void *(*screenshot_handler_t)(void);
typedef void (*shutdown_handler_t)(void);
typedef void (*sleep_post_wakeup_handler_t)(void);
typedef void (*sram_save_handler_t)(void);

void odroid_system_init(int app_id, int sampleRate);
void odroid_system_emu_init(state_handler_t load_cb, state_handler_t save_cb,
                            screenshot_handler_t screenshot_cb, shutdown_handler_t shutdown_cb,
                            sleep_post_wakeup_handler_t sleep_post_wakeup_cb,
                            sram_save_handler_t sram_save_cb);
bool odroid_system_emu_load_state(int slot);

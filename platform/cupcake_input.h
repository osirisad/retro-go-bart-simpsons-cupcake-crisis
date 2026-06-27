#ifndef CUPCAKE_INPUT_H
#define CUPCAKE_INPUT_H

#include "cupcake.h"

#include <stdint.h>

/* Map SDL keyboard state to CUPCAKE_BTN_* (same bit layout on all hosts). */
void cupcake_input_from_sdl_keyboard(const uint8_t *sdl_keys, uint16_t *buttons);

#if defined(TARGET_GNW) || defined(LINUX_EMU)
#include "odroid_input.h"
/* Map G&W / retro-go gamepad to the same CUPCAKE_BTN_* layout. */
void cupcake_input_from_odroid(const odroid_gamepad_state_t *pad, uint16_t *buttons);
#else
struct odroid_gamepad_state;
void cupcake_input_from_odroid(const struct odroid_gamepad_state *pad, uint16_t *buttons);
#endif

#endif

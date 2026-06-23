#ifndef CUPCAKE_INPUT_H
#define CUPCAKE_INPUT_H

#include "cupcake.h"

#include <stdint.h>

struct odroid_gamepad_state;

/* Map SDL keyboard state to CUPCAKE_BTN_* (same bit layout on all hosts). */
void cupcake_input_from_sdl_keyboard(const uint8_t *sdl_keys, uint16_t *buttons);

/* Map G&W / retro-go gamepad to the same CUPCAKE_BTN_* layout (device firmware). */
void cupcake_input_from_odroid(const struct odroid_gamepad_state *pad, uint16_t *buttons);

#endif

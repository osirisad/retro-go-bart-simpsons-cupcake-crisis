/*
 * Cupcake Crisis — Retro-Go SD GWHB homebrew entry.
 *
 * CORE_ENTRY is app_main (gw_core_entry.S). Game loop lives in
 * src/gnw/main_cupcake.c.
 */
#include <stdint.h>

#include "main_cupcake.h"

void app_main(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    app_main_cupcake(load_state, start_paused, save_slot);
}

/*
 * Entry at overlay offset 0 — firmware jumps to __RAM_EMU_START__ | 1 after
 * loading cupcake.bin (same pattern as PICO-8 .pico8_entry).
 */
#include <stdint.h>

#include "main_cupcake.h"

__attribute__((section(".cupcake_entry"), used, noinline))
void cupcake_overlay_entry(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    extern uint8_t _OVERLAY_CUPCAKE_BSS_START[];
    extern uint8_t _OVERLAY_CUPCAKE_BSS_END[];
    uint32_t *p = (uint32_t *)_OVERLAY_CUPCAKE_BSS_START;
    uint32_t *e = (uint32_t *)_OVERLAY_CUPCAKE_BSS_END;

    while (p < e)
        *p++ = 0;

    app_main_cupcake(load_state, start_paused, save_slot);
}

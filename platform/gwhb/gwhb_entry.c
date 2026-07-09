/*
 * Entry at image offset 512 (sizeof(gwhb_header_t)) — the generic GWHB
 * loader jumps here | 1 after validating the header at offset 0.
 *
 * Self-zeroes BSS and configures RGB565 LCD mode (GWHB loader does neither).
 */
#include <stdint.h>

#include "gw_firmware_abi.h"
#include "main_cupcake.h"
#include "cupcake_trace.h"

/* Matches lcd_mode_t in upstream gw_lcd.h (not vendored in sdk/). */
#ifndef CUPCAKE_LCD_MODE_RGB565
#define CUPCAKE_LCD_MODE_RGB565 0
#endif

__attribute__((section(".gwhb_entry"), used, noinline))
void gwhb_cupcake_entry(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    extern uint8_t _GWHB_BSS_START[];
    extern uint8_t _GWHB_BSS_END[];
    uint32_t *p = (uint32_t *)_GWHB_BSS_START;
    uint32_t *e = (uint32_t *)_GWHB_BSS_END;
    const gw_firmware_abi_t *abi = gw_firmware_abi();

    if (abi && abi->lcd_setup_framebuffers)
        abi->lcd_setup_framebuffers(CUPCAKE_LCD_MODE_RGB565);

    while (p < e)
        *p++ = 0;

    if (abi && abi->ram_start_ptr)
        *abi->ram_start_ptr = (uint32_t)(uintptr_t)_GWHB_BSS_END;

    cupcake_trace_init();
    cupcake_trace("entry: load_state=%u start_paused=%u save_slot=%d",
                  (unsigned)load_state, (unsigned)start_paused, (int)save_slot);
    cupcake_trace("entry: ram_start=0x%08lx bss_end=0x%08lx",
                  abi && abi->ram_start_ptr ? (unsigned long)*abi->ram_start_ptr : 0UL,
                  (unsigned long)(uintptr_t)_GWHB_BSS_END);
    if (abi && abi->ram_get_free_size)
        cupcake_trace("entry: heap free=%u bytes", (unsigned)abi->ram_get_free_size());

    app_main_cupcake(load_state, start_paused, save_slot);

    cupcake_trace("exit: app_main_cupcake returned");
}

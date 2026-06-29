/*
 * Entry at overlay offset 0 — firmware jumps to __RAM_EMU_START__ | 1 after
 * loading cupcake.bin (same pattern as PICO-8 .pico8_entry).
 */
#include <stdint.h>

#include "gw_firmware_abi.h"
#include "main_cupcake.h"
#include "cupcake_trace.h"

__attribute__((section(".cupcake_entry"), used, noinline))
void cupcake_overlay_entry(uint8_t load_state, uint8_t start_paused, int8_t save_slot)
{
    extern uint8_t _OVERLAY_CUPCAKE_BSS_START[];
    extern uint8_t _OVERLAY_CUPCAKE_BSS_END[];
    uint32_t *p = (uint32_t *)_OVERLAY_CUPCAKE_BSS_START;
    uint32_t *e = (uint32_t *)_OVERLAY_CUPCAKE_BSS_END;
    const gw_firmware_abi_t *abi = gw_firmware_abi();

    while (p < e)
        *p++ = 0;

    /* Firmware clears ram_start before dispatch; heap is BSS_END..__RAM_EMU_END__. */
    if (abi && abi->ram_start_ptr)
        *abi->ram_start_ptr = (uint32_t)(uintptr_t)_OVERLAY_CUPCAKE_BSS_END;

    cupcake_trace_init();
    cupcake_trace("entry: load_state=%u start_paused=%u save_slot=%d",
                  (unsigned)load_state, (unsigned)start_paused, (int)save_slot);
    cupcake_trace("entry: ram_start=0x%08lx bss_end=0x%08lx",
                  abi && abi->ram_start_ptr ? (unsigned long)*abi->ram_start_ptr : 0UL,
                  (unsigned long)(uintptr_t)_OVERLAY_CUPCAKE_BSS_END);
    if (abi && abi->ram_get_free_size)
        cupcake_trace("entry: heap free=%u bytes", (unsigned)abi->ram_get_free_size());

    app_main_cupcake(load_state, start_paused, save_slot);

    /* Full port build loops forever; reaching here means stub bin or early crash return. */
    cupcake_trace("exit: app_main_cupcake returned");
}

/*
 * Universal Homebrew Header (GWHB) instance for cupcake.bin.
 *
 * Placed by gwhb.ld as the first 512 bytes of the image. total_size is 0
 * at compile time — tools/patch_gwhb_total_size.py patches the final value
 * after objcopy (see platform/gnw/Makefile.gnw).
 */
#include "gwhb.h"
#include "gw_firmware_abi.h"

__attribute__((section(".gwhb_header"), used))
const gwhb_header_t g_gwhb_header = {
    .magic = GWHB_MAGIC,
    .header_version = 0,
    .required_abi = GW_FIRMWARE_ABI_VERSION,
    .required_abi_min_size = sizeof(gw_firmware_abi_t),
    .total_size = 0,
    .name_table_offset = 0,
    .name_table_size = 0,
    .reserved = {0},
};

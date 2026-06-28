#pragma once

#include "gw_firmware_abi.h"
#include "common.h"

static inline int gw_abi_ok(void)
{
    const gw_firmware_abi_t *abi = gw_firmware_abi();

    if (!abi)
        return 0;
    if (abi->version != GW_FIRMWARE_ABI_VERSION)
        return 0;
    if (abi->size < sizeof(gw_firmware_abi_t))
        return 0;
    return 1;
}

static inline common_emu_state_t *gw_common_emu_state(void)
{
    void *ptr = GW_FIRMWARE_ABI.common_emu_state_ptr;

    if (!ptr)
        return NULL;
    return (common_emu_state_t *)ptr;
}

void gw_abi_bind_stdio(void);

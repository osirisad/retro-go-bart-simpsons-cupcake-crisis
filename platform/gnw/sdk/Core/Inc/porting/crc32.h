#pragma once

#include <stddef.h>
#include <stdint.h>

static inline uint32_t crc32_le(uint32_t crc, const void *buf, size_t len)
{
    (void)crc;
    (void)buf;
    (void)len;
    return 0;
}

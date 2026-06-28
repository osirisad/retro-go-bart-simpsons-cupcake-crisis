/* Minimal FatFs types for gw_firmware_abi.h (plugin build only). */
#pragma once

#include <stdint.h>

typedef char TCHAR;

typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
} FRESULT;

typedef struct {
    uint32_t fsize;
    uint16_t fdate;
    uint16_t ftime;
    uint8_t fattrib;
    char fname[13];
} FILINFO;

typedef struct {
    uint8_t dummy;
} DIR;

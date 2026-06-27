#pragma once

/* Minimal HAL stub for overlay compile — real hardware is in flashed firmware. */
#include <stdint.h>

typedef struct SPI_HandleTypeDef SPI_HandleTypeDef;
typedef struct LTDC_HandleTypeDef LTDC_HandleTypeDef;
typedef struct SAI_HandleTypeDef SAI_HandleTypeDef;
typedef struct DMA_HandleTypeDef DMA_HandleTypeDef;

uint32_t HAL_GetTick(void);

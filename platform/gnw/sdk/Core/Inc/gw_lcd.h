#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GW_LCD_WIDTH  320
#define GW_LCD_HEIGHT 240

typedef uint16_t pixel_t;

#define GW_LCD_FRAME_SIZE ((size_t)(GW_LCD_WIDTH * GW_LCD_HEIGHT * sizeof(pixel_t)))

void lcd_clear_buffers(void);
void lcd_swap(void);
void *lcd_get_active_buffer(void);

#endif

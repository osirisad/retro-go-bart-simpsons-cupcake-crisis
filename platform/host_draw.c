#include "host_draw.h"

#include "cupcake.h"
#include "cupcake_sprite_lcd.h"
#include "sprite_blit.h"

#include <string.h>

#define HOST_SPRITE_MAX_W 400
#define HOST_SPRITE_MAX_H 560

int host_sprite_rect_ok(const char *name)
{
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name(name);
    if (!spr || spr->w < 1 || spr->h < 1)
        return 0;
    if (spr->w > HOST_SPRITE_MAX_W || spr->h > HOST_SPRITE_MAX_H)
        return 0;
    return 1;
}

static inline void put_px(uint8_t *dst, int stride, int x, int y, int w, int h,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (x < 0 || y < 0 || x >= w || y >= h || a < 8)
        return;
    uint8_t *p = dst + y * stride + x * 4;
    if (a >= 250) {
        p[0] = r;
        p[1] = g;
        p[2] = b;
        p[3] = 255;
    } else {
        uint8_t ia = 255 - a;
        p[0] = (uint8_t)((r * a + p[0] * ia) / 255);
        p[1] = (uint8_t)((g * a + p[1] * ia) / 255);
        p[2] = (uint8_t)((b * a + p[2] * ia) / 255);
        p[3] = 255;
    }
}

void host_clear_lcd(host_atlas_t *host, uint8_t r, uint8_t g, uint8_t b)
{
    for (int y = 0; y < host->lcd_h; y++) {
        uint8_t *row = host->lcd_pixels + y * host->lcd_stride;
        for (int x = 0; x < host->lcd_w; x++) {
            row[x * 4 + 0] = r;
            row[x * 4 + 1] = g;
            row[x * 4 + 2] = b;
            row[x * 4 + 3] = 255;
        }
    }
}

void host_clear_lcd_transparent(host_atlas_t *host)
{
    memset(host->lcd_pixels, 0, (size_t)host->lcd_h * host->lcd_stride);
}

void host_draw_sprite(const host_atlas_t *host, const char *name, int lcd_x, int lcd_y)
{
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name(name);
    const cupcake_sprite_lcd_t *lcd;
    if (!host_sprite_rect_ok(name) || !host->atlas)
        return;

    if (lcd_x == CUPCAKE_LCD_AUTO || lcd_y == CUPCAKE_LCD_AUTO) {
        lcd = cupcake_sprite_lcd_by_name(name);
        if (!lcd)
            return;
        lcd_x = lcd->lcd_x;
        lcd_y = cupcake_sprite_lcd_resolve_y(name, lcd->lcd_y);
    }

    sprite_blit_masked(host->atlas, host->atlas_stride, spr, name, host->lcd_pixels,
                       host->lcd_stride, host->lcd_w, host->lcd_h, lcd_x, lcd_y);
}

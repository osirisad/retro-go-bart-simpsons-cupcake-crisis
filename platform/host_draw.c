#include "host_draw.h"

#include "cupcake.h"
#include "cupcake_port.h"
#include "cupcake_sprite_lcd.h"
#include "sprite_blit.h"

#ifdef CUPCAKE_EMBEDDED_ASSETS
#include "cupcake_data.h"
#endif

#include <string.h>

#define HOST_SPRITE_MAX_W 400
#define HOST_SPRITE_MAX_H 560

#define HOST_RGB565(r, g, b) \
    (((uint16_t)((b) >> 3) & 0x1fu) | (((uint16_t)((g) >> 2) & 0x3fu) << 5) | \
     (((uint16_t)((r) >> 3) & 0x1fu) << 11))

static inline void host_rgb565_unpack(uint16_t px, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *b = (uint8_t)((px & 0x1fu) << 3);
    *g = (uint8_t)(((px >> 5) & 0x3fu) << 2);
    *r = (uint8_t)(((px >> 11) & 0x1fu) << 3);
}

static inline uint16_t host_rgb565_blend(uint16_t dst_px, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t dr, dg, db;
    uint8_t ia;

    if (a >= 250)
        return HOST_RGB565(r, g, b);
    if (a < 8)
        return dst_px;

    host_rgb565_unpack(dst_px, &dr, &dg, &db);
    ia = (uint8_t)(255 - a);
    return HOST_RGB565((uint8_t)((r * a + dr * ia) / 255), (uint8_t)((g * a + dg * ia) / 255),
                       (uint8_t)((b * a + db * ia) / 255));
}

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

#ifdef CUPCAKE_EMBEDDED_ASSETS

static inline int gnw_map_lcd_x(int lcd_x, int fb_w)
{
    return lcd_x * fb_w / CUPCAKE_LCD_ATLAS_W;
}

static inline int gnw_map_lcd_y(int lcd_y, int fb_h)
{
    return lcd_y * fb_h / CUPCAKE_BEZEL_VISIBLE_H;
}

static inline int gnw_map_atlas_x(int x, int atlas_w)
{
    return x * atlas_w / CUPCAKE_GNW_ATLAS_SOURCE_W;
}

static inline int gnw_map_atlas_y(int y, int atlas_h)
{
    return y * atlas_h / CUPCAKE_GNW_ATLAS_SOURCE_H;
}

static uint16_t gnw_atlas_sample_nearest(const host_atlas_t *host, int src_x, int src_y)
{
    int aw;
    int ah;
    int sx;
    int sy;

    if (!host || !host->atlas_rgb565)
        return 0;

    aw = host->atlas_w;
    ah = host->atlas_h;
    if (aw < 1 || ah < 1)
        return 0;

    sx = gnw_map_atlas_x(src_x, aw);
    sy = gnw_map_atlas_y(src_y, ah);
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;
    if (sx >= aw)
        sx = aw - 1;
    if (sy >= ah)
        sy = ah - 1;

    return host->atlas_rgb565[sy * aw + sx];
}

void host_draw_sprite_rgb565_fb(const host_atlas_t *host, uint16_t *fb, int fb_w, int fb_h,
                                const char *name, int lcd_x, int lcd_y)
{
    const cupcake_sprite_rect_t *spr;
    const cupcake_sprite_lcd_t *lcd;
    const cupcake_sprite_mask_t *msk;
    int dx0, dy0;
    int fb_sw, fb_sh;
    int row;

    if (!host || !host->atlas_rgb565 || !fb || fb_w < 1 || fb_h < 1)
        return;
    if (!host_sprite_rect_ok(name))
        return;

    spr = cupcake_sprite_by_name(name);
    if (!spr)
        return;

    if (lcd_x == CUPCAKE_LCD_AUTO || lcd_y == CUPCAKE_LCD_AUTO) {
        lcd = cupcake_sprite_lcd_by_name(name);
        if (!lcd)
            return;
        lcd_x = lcd->lcd_x;
        lcd_y = cupcake_sprite_lcd_resolve_y(name, lcd->lcd_y);
    }

    /* Scale from 1024x800 logical LCD space to the device framebuffer (same as lcd blit path). */
    dx0 = gnw_map_lcd_x(lcd_x, fb_w);
    dy0 = gnw_map_lcd_y(lcd_y, fb_h);
    fb_sw = (spr->w * fb_w + CUPCAKE_LCD_ATLAS_W - 1) / CUPCAKE_LCD_ATLAS_W;
    fb_sh = (spr->h * fb_h + CUPCAKE_BEZEL_VISIBLE_H - 1) / CUPCAKE_BEZEL_VISIBLE_H;
    if (fb_sw < 1)
        fb_sw = 1;
    if (fb_sh < 1)
        fb_sh = 1;

    msk = cupcake_sprite_mask_by_name(name);

    for (row = 0; row < fb_sh; row++) {
        int dy = dy0 + row;
        int col;

        if (dy < 0 || dy >= fb_h)
            continue;

        for (col = 0; col < fb_sw; col++) {
            int dx = dx0 + col;
            int src_x;
            int src_y;
            int mask_col;
            int mask_row;
            int masked;
            uint16_t px;

            if (dx < 0 || dx >= fb_w)
                continue;

            src_x = spr->x + col * spr->w / fb_sw;
            src_y = spr->y + row * spr->h / fb_sh;
            mask_col = col * spr->w / fb_sw;
            mask_row = row * spr->h / fb_sh;

            masked = 1;
            if (msk) {
                masked = cupcake_sprite_mask_get(msk, mask_col, mask_row, spr->w, spr->h);
                if (!masked)
                    continue;
            }

            px = gnw_atlas_sample_nearest(host, src_x, src_y);
            if (px == 0)
                continue;

            fb[dy * fb_w + dx] = px;
        }
    }
}

#endif

host_lcd_rect_t host_lcd_rect_for_framebuffer(int fb_w, int fb_h, int bezel_w,
                                              int bezel_visible_h)
{
    host_lcd_rect_t rect;
    float sx;
    float sy;

    if (bezel_w < 1)
        bezel_w = 1;
    if (bezel_visible_h < 1)
        bezel_visible_h = 1;

    sx = (float)fb_w / (float)bezel_w;
    sy = (float)fb_h / (float)bezel_visible_h;

    rect.x = (int)(CUPCAKE_LCD_ATLAS_X * sx + 0.5f);
    rect.y = (int)(CUPCAKE_LCD_ATLAS_Y * sy + 0.5f);
    rect.w = (int)(CUPCAKE_LCD_ATLAS_W * sx + 0.5f);
    rect.h = (int)(CUPCAKE_LCD_ATLAS_H * sy + 0.5f);
    return rect;
}

void host_bezel_blit_rgb565(const host_bezel_t *bezel, uint16_t *dst, int dst_w, int dst_h)
{
    int src_w;
    int src_h;
    int dy;
    int dx;

    if (!bezel || !dst || dst_w < 1 || dst_h < 1)
        return;

    if (bezel->rgb565) {
        src_w = bezel->w;
        src_h = bezel->h;
        if (bezel->visible_h > 0 && bezel->visible_h < src_h)
            src_h = bezel->visible_h;
        if (src_w < 1 || src_h < 1)
            return;

        if (src_w == dst_w && src_h == dst_h) {
            memcpy(dst, bezel->rgb565, (size_t)src_w * (size_t)src_h * sizeof(uint16_t));
            return;
        }

        for (dy = 0; dy < dst_h; dy++) {
            int sy = dy * src_h / dst_h;
            const uint16_t *row = bezel->rgb565 + (size_t)sy * (size_t)src_w;

            for (dx = 0; dx < dst_w; dx++) {
                int sx = dx * src_w / dst_w;
                dst[dy * dst_w + dx] = row[sx];
            }
        }
        return;
    }

    if (!bezel->pixels)
        return;

    src_w = bezel->w;
    src_h = bezel->h;
    if (bezel->visible_h > 0 && bezel->visible_h < src_h)
        src_h = bezel->visible_h;
    if (src_w < 1 || src_h < 1)
        return;

    for (dy = 0; dy < dst_h; dy++) {
        int sy = dy * src_h / dst_h;
        const uint8_t *row = bezel->pixels + (size_t)sy * (size_t)bezel->w * 4u;

        for (dx = 0; dx < dst_w; dx++) {
            int sx = dx * src_w / dst_w;
            const uint8_t *p = row + (size_t)sx * 4u;

            dst[dy * dst_w + dx] = HOST_RGB565(p[0], p[1], p[2]);
        }
    }
}

void host_lcd_blit_rgb565(const host_atlas_t *host, const host_lcd_rect_t *rect, uint16_t *dst,
                          int dst_w, int dst_h)
{
    int dy;
    int dx;

    if (!host || !host->lcd_pixels || !rect || !dst || rect->w < 1 || rect->h < 1)
        return;

    for (dy = 0; dy < rect->h; dy++) {
        int sy = dy * host->lcd_h / rect->h;

        for (dx = 0; dx < rect->w; dx++) {
            int sx = dx * host->lcd_w / rect->w;
            const uint8_t *p =
                host->lcd_pixels + (size_t)sy * (size_t)host->lcd_stride + (size_t)sx * 4u;
            int ox = rect->x + dx;
            int oy = rect->y + dy;
            uint8_t a;

            if (ox < 0 || oy < 0 || ox >= dst_w || oy >= dst_h)
                continue;

            a = p[3];
            if (a < 8)
                continue;

            dst[oy * dst_w + ox] =
                host_rgb565_blend(dst[oy * dst_w + ox], p[0], p[1], p[2], a);
        }
    }
}

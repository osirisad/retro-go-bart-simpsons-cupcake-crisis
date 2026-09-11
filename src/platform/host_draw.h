#ifndef HOST_DRAW_H_
#define HOST_DRAW_H_

#include <stdint.h>

#include "cupcake_sprites.h"

typedef struct {
    const uint8_t *atlas;
    const uint16_t *atlas_rgb565;
    int atlas_w;
    int atlas_h;
    int atlas_stride;
    uint8_t *lcd_pixels;
    int lcd_w;
    int lcd_h;
    int lcd_stride;
} host_atlas_t;

/* Bezel art; rgb565 set for GNW embedded, pixels for SD RGBA path. */
typedef struct {
    const uint8_t *pixels;
    const uint16_t *rgb565;
    int w;
    int h;
    int visible_h;
} host_bezel_t;

typedef struct {
    int x, y, w, h;
} host_lcd_rect_t;

void host_clear_lcd(host_atlas_t *host, uint8_t r, uint8_t g, uint8_t b);
void host_clear_lcd_transparent(host_atlas_t *host);

/* Returns false if sprite name/rect is missing or unreasonably large (bad UV data). */
int host_sprite_rect_ok(const char *name);

void host_draw_sprite(const host_atlas_t *host, const char *name, int lcd_x, int lcd_y);

/* GNW: draw sprite directly into RGB565 framebuffer (embedded atlas, no lcd_pixels). */
void host_draw_sprite_rgb565_fb(const host_atlas_t *host, uint16_t *fb, int fb_w, int fb_h,
                                const char *name, int lcd_x, int lcd_y);

/* LCD destination rect when scaling bezel to fb_w x fb_h (SDL + retro-go parity). */
host_lcd_rect_t host_lcd_rect_for_framebuffer(int fb_w, int fb_h, int bezel_w, int bezel_visible_h);

/* Scale bezel visible region to an RGB565 framebuffer. */
void host_bezel_blit_rgb565(const host_bezel_t *bezel, uint16_t *dst, int dst_w, int dst_h);

/* Alpha-composite LCD RGBA buffer into dst at rect (scaled from logical LCD size). */
void host_lcd_blit_rgb565(const host_atlas_t *host, const host_lcd_rect_t *rect, uint16_t *dst,
                          int dst_w, int dst_h);

#endif

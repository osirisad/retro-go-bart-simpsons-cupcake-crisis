#ifndef HOST_DRAW_H_
#define HOST_DRAW_H_

#include <stdint.h>

#include "cupcake_sprites.h"

typedef struct {
    const uint8_t *atlas;
    int atlas_w;
    int atlas_h;
    int atlas_stride;
    uint8_t *lcd_pixels;
    int lcd_w;
    int lcd_h;
    int lcd_stride;
} host_atlas_t;

void host_clear_lcd(host_atlas_t *host, uint8_t r, uint8_t g, uint8_t b);
void host_clear_lcd_transparent(host_atlas_t *host);

/* Returns false if sprite name/rect is missing or unreasonably large (bad UV data). */
int host_sprite_rect_ok(const char *name);

void host_draw_sprite(const host_atlas_t *host, const char *name, int lcd_x, int lcd_y);

#endif

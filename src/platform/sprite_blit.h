#ifndef SPRITE_BLIT_H_
#define SPRITE_BLIT_H_

#include <stdint.h>

#include "cupcake_sprite_masks.h"
#include "cupcake_sprites.h"

/* Sample atlas RGBA; only pixels covered by the material UV triangle mask. */
static inline void sprite_blit_masked(
    const uint8_t *atlas,
    int atlas_stride,
    const cupcake_sprite_rect_t *spr,
    const char *name,
    uint8_t *dst,
    int dst_stride,
    int dst_w,
    int dst_h,
    int lcd_x,
    int lcd_y)
{
    const cupcake_sprite_mask_t *msk = cupcake_sprite_mask_by_name(name);

    for (int row = 0; row < spr->h; row++) {
        int dy = lcd_y + row;
        if (dy < 0 || dy >= dst_h)
            continue;
        for (int col = 0; col < spr->w; col++) {
            int dx = lcd_x + col;
            if (dx < 0 || dx >= dst_w)
                continue;
            if (msk && !cupcake_sprite_mask_get(msk, col, row, spr->w, spr->h))
                continue;
            const uint8_t *src =
                atlas + (spr->y + row) * atlas_stride + (spr->x + col) * 4;
            if (src[3] < 8)
                continue;
            uint8_t *p = dst + dy * dst_stride + dx * 4;
            uint8_t a = src[3];
            if (a >= 250) {
                p[0] = src[0];
                p[1] = src[1];
                p[2] = src[2];
                p[3] = 255;
            } else {
                uint8_t ia = (uint8_t)(255 - a);
                p[0] = (uint8_t)((src[0] * a + p[0] * ia) / 255);
                p[1] = (uint8_t)((src[1] * a + p[1] * ia) / 255);
                p[2] = (uint8_t)((src[2] * a + p[2] * ia) / 255);
                p[3] = 255;
            }
        }
    }
}

#endif

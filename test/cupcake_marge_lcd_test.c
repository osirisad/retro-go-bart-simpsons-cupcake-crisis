/*
 * Marge1 LCD Y clamp (TASK-50) — run: make test-marge-lcd
 */
#include "cupcake_sprite_lcd.h"
#include "cupcake_sprites.h"

#include <stdio.h>

#define CUPCAKE_LCD_H 800

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_marge1_header_fits_lcd(void)
{
    const cupcake_sprite_lcd_t *lcd = cupcake_sprite_lcd_by_name("marge1");
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name("marge1");
    int draw_y;

    if (!lcd || !spr)
        fail("marge1 missing from generated headers");

    draw_y = cupcake_sprite_lcd_resolve_y("marge1", lcd->lcd_y);
    if (draw_y < 0)
        fail("marge1 resolve_y: hair top must stay on-screen");
    if (draw_y + spr->h > CUPCAKE_LCD_H)
        fail("marge1 resolve_y: sprite bottom exceeds LCD");
    if (draw_y != lcd->lcd_y)
        fail("marge1 header lcd_y should not need clamp");
}

static void test_marge1_clamp_extreme_negative(void)
{
    const cupcake_sprite_rect_t *spr = cupcake_sprite_by_name("marge1");
    int hair_rows;
    int min_y;
    int draw_y;

    if (!spr)
        return;

    hair_rows = CUPCAKE_MARGE1_LEGACY_ATLAS_Y - spr->y;
    if (hair_rows <= 0)
        return;

    min_y = 1 - hair_rows;
    draw_y = cupcake_sprite_lcd_resolve_y("marge1", -999);
    if (draw_y != min_y)
        fail("marge1 resolve_y: extreme negative should clamp to min_y");
    if (draw_y + spr->h > CUPCAKE_LCD_H)
        fail("marge1 resolve_y: clamped draw still exceeds LCD height");
}

int main(void)
{
    test_marge1_header_fits_lcd();
    test_marge1_clamp_extreme_negative();

    if (g_fail)
        return 1;
    printf("cupcake_marge_lcd: all tests passed\n");
    return 0;
}

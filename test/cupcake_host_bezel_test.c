/*
 * Host bezel / LCD rect helpers — run: make test-host-bezel
 */
#include "host_draw.h"
#include "cupcake_port.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_lcd_rect_matches_port_constants(void)
{
    host_lcd_rect_t r;

    r = host_lcd_rect_for_framebuffer(512, 400, 1024, CUPCAKE_BEZEL_VISIBLE_H);
    if (r.x != 0 || r.y != 0)
        fail("lcd rect: origin at 0,0 for full atlas");
    if (r.w != 512 || r.h != 400)
        fail("lcd rect: scales to framebuffer");
}

static void test_bezel_blit_fills_framebuffer(void)
{
    host_bezel_t bez = {
        .pixels = NULL,
        .w = 2,
        .h = 2,
        .visible_h = 2,
    };
    uint8_t px[16] = {
        255, 0, 0, 255,   0, 255, 0, 255,
        0, 0, 255, 255,   255, 255, 0, 255,
    };
    uint16_t fb[4];

    bez.pixels = px;
    memset(fb, 0, sizeof fb);
    host_bezel_blit_rgb565(&bez, fb, 2, 2);
    if (fb[0] == 0 || fb[3] == 0)
        fail("bezel blit: wrote RGB565 pixels");
}

int main(void)
{
    test_lcd_rect_matches_port_constants();
    test_bezel_blit_fills_framebuffer();

    if (g_fail)
        return 1;
    printf("cupcake_host_bezel: all tests passed\n");
    return 0;
}

/*
 * Load PNG/JPG from SD for device overlay (replaced by embedded pack in OV-3).
 */
#include "gnw_assets.h"

#include "cupcake_port.h"
#include "stb_image.h"

#include "gw_malloc.h"

#include <stdio.h>
#include <string.h>

static uint8_t *g_bezel_pixels;
static uint8_t *g_atlas_pixels;

const char *gnw_assets_base(void)
{
    return CUPCAKE_GNW_ASSETS_BASE;
}

static uint8_t *load_rgba_file(const char *path, int *w, int *h)
{
    return stbi_load(path, w, h, NULL, 4);
}

static void asset_path(char *buf, size_t bufsz, const char *name)
{
    snprintf(buf, bufsz, "%s/%s", gnw_assets_base(), name);
}

int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h)
{
    char path[512];
    int aw = 0;
    int ah = 0;
    int bw = 0;
    int bh = 0;
    int visible_h;

    if (!host || !bezel || !bezel_w || !bezel_h)
        return -1;

    memset(host, 0, sizeof *host);
    memset(bezel, 0, sizeof *bezel);

    asset_path(path, sizeof path, "screen.jpg");
    g_bezel_pixels = load_rgba_file(path, &bw, &bh);
    if (!g_bezel_pixels) {
        asset_path(path, sizeof path, "screen.png");
        g_bezel_pixels = load_rgba_file(path, &bw, &bh);
    }

    asset_path(path, sizeof path, "sprites-color.png");
    g_atlas_pixels = load_rgba_file(path, &aw, &ah);
    if (!g_atlas_pixels) {
        asset_path(path, sizeof path, "sprites.png");
        g_atlas_pixels = load_rgba_file(path, &aw, &ah);
    }

    if (!g_bezel_pixels || !g_atlas_pixels || bw < 1 || bh < 1 || aw < 1 || ah < 1) {
        gnw_assets_free(host, bezel);
        return -1;
    }

    visible_h = CUPCAKE_BEZEL_VISIBLE_H;
    if (bh < visible_h)
        visible_h = bh;

    host->lcd_pixels = (uint8_t *)ram_malloc((size_t)CUPCAKE_LCD_W * (size_t)CUPCAKE_LCD_H * 4u);
    if (!host->lcd_pixels) {
        gnw_assets_free(host, bezel);
        return -1;
    }

    host->atlas = g_atlas_pixels;
    host->atlas_w = aw;
    host->atlas_h = ah;
    host->atlas_stride = aw * 4;
    host->lcd_w = CUPCAKE_LCD_W;
    host->lcd_h = CUPCAKE_LCD_H;
    host->lcd_stride = CUPCAKE_LCD_W * 4;

    bezel->pixels = g_bezel_pixels;
    bezel->w = bw;
    bezel->h = bh;
    bezel->visible_h = visible_h;

    *bezel_w = bw;
    *bezel_h = bh;
    return 0;
}

void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel)
{
    (void)bezel;

    if (g_bezel_pixels) {
        stbi_image_free(g_bezel_pixels);
        g_bezel_pixels = NULL;
    }
    if (g_atlas_pixels) {
        stbi_image_free(g_atlas_pixels);
        g_atlas_pixels = NULL;
    }
    if (host && host->lcd_pixels) {
        /* stbi buffers are freed above; lcd buffer from ram_malloc — no free API in gw_malloc header for single free; leak on shutdown is OK for overlay exit. */
        host->lcd_pixels = NULL;
    }
    if (host)
        memset(host, 0, sizeof *host);
}

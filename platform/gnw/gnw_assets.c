/*
 * Load art for device overlay — embedded RGB565 (GNW) or SD PNG/JPG fallback.
 */
#include "gnw_assets.h"

#include "cupcake_port.h"
#include "stb_image.h"

#include "gw_malloc.h"

#include <stdio.h>
#include <string.h>

#ifdef CUPCAKE_EMBEDDED_ASSETS
#include "cupcake_data.h"
#endif

static uint8_t *g_bezel_pixels;
static uint8_t *g_atlas_pixels;

const char *gnw_assets_base(void)
{
#ifdef CUPCAKE_EMBEDDED_ASSETS
    return NULL;
#else
    return CUPCAKE_GNW_ASSETS_BASE;
#endif
}

#ifndef CUPCAKE_EMBEDDED_ASSETS
static uint8_t *load_rgba_file(const char *path, int *w, int *h)
{
    return stbi_load(path, w, h, NULL, 4);
}

static void asset_path(char *buf, size_t bufsz, const char *name)
{
    snprintf(buf, bufsz, "%s/%s", gnw_assets_base(), name);
}
#endif

int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h)
{
    int aw = 0;
    int ah = 0;
    int bw = 0;
    int bh = 0;
    int visible_h;

    if (!host || !bezel || !bezel_w || !bezel_h)
        return -1;

    memset(host, 0, sizeof *host);
    memset(bezel, 0, sizeof *bezel);

#ifdef CUPCAKE_EMBEDDED_ASSETS
    {
        const uint16_t *bezel565 = cupcake_gnw_bezel_rgb565();
        const uint16_t *atlas565 = cupcake_gnw_atlas_rgb565();

        bw = CUPCAKE_GNW_BEZEL_W;
        bh = CUPCAKE_GNW_BEZEL_H;
        aw = CUPCAKE_GNW_ATLAS_W;
        ah = CUPCAKE_GNW_ATLAS_H;

        if (!bezel565 || !atlas565 || bw < 1 || bh < 1 || aw < 1 || ah < 1)
            return -1;

        host->atlas_rgb565 = atlas565;
        host->atlas_w = aw;
        host->atlas_h = ah;
        host->lcd_w = 0;
        host->lcd_h = 0;
        host->lcd_pixels = NULL;

        bezel->rgb565 = bezel565;
        bezel->w = bw;
        bezel->h = bh;
        bezel->pixels = NULL;
        bezel->visible_h = CUPCAKE_BEZEL_VISIBLE_H;
        if (bh < bezel->visible_h)
            bezel->visible_h = bh;

        *bezel_w = bw;
        *bezel_h = bh;
        return 0;
    }
#else
    {
        char path[512];

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
    bezel->rgb565 = NULL;

    *bezel_w = bw;
    *bezel_h = bh;
    return 0;
#endif
}

void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel)
{
    (void)bezel;

#ifndef CUPCAKE_EMBEDDED_ASSETS
    if (g_bezel_pixels) {
        stbi_image_free(g_bezel_pixels);
        g_bezel_pixels = NULL;
    }
    if (g_atlas_pixels) {
        stbi_image_free(g_atlas_pixels);
        g_atlas_pixels = NULL;
    }
#endif
    if (host)
        memset(host, 0, sizeof *host);
}

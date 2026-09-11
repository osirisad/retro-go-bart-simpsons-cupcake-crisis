/*
 * Load embedded RGB565 bezel + atlas for the GWHB build.
 */
#include "gnw_assets.h"

#include "cupcake_port.h"
#include "cupcake_data.h"

#include <string.h>

const char *gnw_assets_base(void)
{
    return NULL;
}

int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h)
{
    int aw;
    int ah;
    int bw;
    int bh;
    const uint16_t *bezel565;
    const uint16_t *atlas565;

    if (!host || !bezel || !bezel_w || !bezel_h)
        return -1;

    memset(host, 0, sizeof *host);
    memset(bezel, 0, sizeof *bezel);

    bezel565 = cupcake_gnw_bezel_rgb565();
    atlas565 = cupcake_gnw_atlas_rgb565();
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

void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel)
{
    (void)bezel;
    if (host)
        memset(host, 0, sizeof *host);
}

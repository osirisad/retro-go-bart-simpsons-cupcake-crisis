#ifndef GNW_ASSETS_H_
#define GNW_ASSETS_H_

#include "host_draw.h"

/* Interim SD layout until OV-3 embeds assets in cupcake.bin. */
#ifndef CUPCAKE_GNW_ASSETS_BASE
#define CUPCAKE_GNW_ASSETS_BASE "/retro-go/cupcake"
#endif

const char *gnw_assets_base(void);

/* Load screen.jpg + sprites-color.png into heap (ram_malloc). Returns 0 on success. */
int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h);

void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel);

#endif

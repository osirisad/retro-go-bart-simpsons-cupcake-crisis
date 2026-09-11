#ifndef GNW_ASSETS_H
#define GNW_ASSETS_H

#include "host_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SD sidecar for ADPCM clips (legacy fallback when not embedded). */
#ifndef CUPCAKE_GNW_ASSETS_DAT_PATH
#define CUPCAKE_GNW_ASSETS_DAT_PATH "/homebrews/cupcake_assets.dat"
#endif

const char *gnw_assets_base(void);

/* Bind embedded RGB565 bezel + atlas. Returns 0 on success. */
int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h);
void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel);

#ifdef __cplusplus
}
#endif

#endif

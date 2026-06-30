#ifndef GNW_ASSETS_H_
#define GNW_ASSETS_H_

#include "host_draw.h"

/* SD layout when CUPCAKE_EMBEDDED_ASSETS is unset (dev fallback). */
#ifndef CUPCAKE_GNW_ASSETS_BASE
#define CUPCAKE_GNW_ASSETS_BASE "/retro-go/cupcake"
#endif

/* Long music ADPCM sidecars — /roms/homebrew/cupcake/audio/ on SD. */
#ifndef CUPCAKE_GNW_SD_AUDIO_DIR
#define CUPCAKE_GNW_SD_AUDIO_DIR "/roms/homebrew/cupcake/audio"
#endif

/* NULL when assets are embedded in cupcake.bin. */
const char *gnw_assets_base(void);

/* Load screen.jpg + sprites-color.png into heap (ram_malloc). Returns 0 on success. */
int gnw_assets_load(host_atlas_t *host, host_bezel_t *bezel, int *bezel_w, int *bezel_h);

void gnw_assets_free(host_atlas_t *host, host_bezel_t *bezel);

#endif

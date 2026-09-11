#ifndef CUPCAKE_ASSETS_DAT_H_
#define CUPCAKE_ASSETS_DAT_H_

#include <stddef.h>
#include <stdint.h>

/* Packed ADPCM archive — built by tools/bundle_assets.py. */
#define CUPCAKE_ASSETS_DAT_MAGIC 0x434b4144u /* 'CKAD' */

typedef struct {
    char name[32];
    uint32_t pcm_samples;
    uint32_t sample_rate;
    uint32_t offset;
    uint32_t adpcm_size;
} cupcake_dat_clip_t;

/* Prefer the in-binary blob; path init is a fallback only. */
int cupcake_assets_dat_init_mem(const uint8_t *data, size_t len);
int cupcake_assets_dat_init(const char *path);
void cupcake_assets_dat_shutdown(void);

int cupcake_assets_dat_clip_count(void);
const cupcake_dat_clip_t *cupcake_assets_dat_clip(int index);

/* Lookup by catalog filename (e.g. "step.wav"). */
int cupcake_assets_dat_lookup(const char *wav_file, cupcake_dat_clip_t *out);

/* Absolute pointer into the open archive (embedded blob or mapped view). */
const uint8_t *cupcake_assets_dat_payload(uint32_t offset, uint32_t len);

/*
 * Copy from the open archive. Prefer cupcake_assets_dat_payload() when the
 * blob is already in RAM so ADPCM can stream without an extra buffer.
 */
int cupcake_assets_dat_read(uint32_t offset, void *buf, size_t len);

#endif

#ifndef CUPCAKE_ADPCM_H_
#define CUPCAKE_ADPCM_H_

#include <stddef.h>
#include <stdint.h>

/*
 * Decode IMA ADPCM produced by tools/bundle_overlay_assets.py.
 * in: adpcm payload after the 7-byte header (see cupcake_embedded_wav_t).
 * out_samples: number of mono int16 samples to write (from header pcm_samples).
 */
int cupcake_adpcm_decode(const uint8_t *in, size_t in_len, int16_t *out, int out_samples);

#endif

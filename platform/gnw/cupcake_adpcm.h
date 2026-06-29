#ifndef CUPCAKE_ADPCM_H_
#define CUPCAKE_ADPCM_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
    const uint8_t *in;
    size_t in_len;
    int predictor;
    int step_index;
    size_t pos;
    int nibble_phase;
    uint8_t cur_byte;
    int samples_left;
    int header_pending;
} cupcake_adpcm_stream_t;

void cupcake_adpcm_stream_init(cupcake_adpcm_stream_t *st, const uint8_t *in, size_t in_len,
                               int pcm_samples);
int16_t cupcake_adpcm_stream_next(cupcake_adpcm_stream_t *st);

/*
 * Decode IMA ADPCM produced by tools/bundle_overlay_assets.py.
 * in: adpcm payload after the 7-byte header (see cupcake_embedded_wav_t).
 * out_samples: number of mono int16 samples to write (from header pcm_samples).
 */
int cupcake_adpcm_decode(const uint8_t *in, size_t in_len, int16_t *out, int out_samples);

#endif

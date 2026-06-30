#ifndef CUPCAKE_ADPCM_H_
#define CUPCAKE_ADPCM_H_

#include <stddef.h>
#include <stdint.h>

#define CUPCAKE_ADPCM_FILE_CHUNK 512

typedef struct {
    const uint8_t *in;
    uint32_t dat_offset;
    uint32_t dat_len;
    uint32_t dat_pos;
    uint8_t file_buf[CUPCAKE_ADPCM_FILE_CHUNK];
    size_t file_buf_pos;
    size_t file_buf_len;
    size_t in_len;
    size_t pos;
    int predictor;
    int step_index;
    int nibble_phase;
    uint8_t cur_byte;
    int samples_left;
    int header_pending;
    unsigned use_dat : 1;
} cupcake_adpcm_stream_t;

void cupcake_adpcm_stream_init(cupcake_adpcm_stream_t *st, const uint8_t *in, size_t in_len,
                               int pcm_samples);
void cupcake_adpcm_stream_init_dat(cupcake_adpcm_stream_t *st, uint32_t offset, uint32_t len,
                                   int pcm_samples);
void cupcake_adpcm_stream_close(cupcake_adpcm_stream_t *st);
int16_t cupcake_adpcm_stream_next(cupcake_adpcm_stream_t *st);

int cupcake_adpcm_decode(const uint8_t *in, size_t in_len, int16_t *out, int out_samples);

#endif

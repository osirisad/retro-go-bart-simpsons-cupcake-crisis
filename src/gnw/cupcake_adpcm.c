/*
 * IMA ADPCM decoder — matches tools/bundle_overlay_assets.py encoder.
 */
#include "cupcake_adpcm.h"

#include <string.h>

#if defined(CUPCAKE_EMBEDDED_ASSETS)
#include "cupcake_data.h"
#endif
#if defined(CUPCAKE_GNW_ASSETS_DAT)
#include "cupcake_assets_dat.h"
#endif
static const int16_t step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
    2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
    6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8_t index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int16_t adpcm_decode_nibble(int *predictor, int *step_index, int nibble)
{
    int step;
    int diff;

    step = step_table[*step_index];
    diff = step >> 3;
    if (nibble & 1)
        diff += step >> 2;
    if (nibble & 2)
        diff += step >> 1;
    if (nibble & 4)
        diff += step;
    if (nibble & 8)
        diff = -diff;

    *predictor += diff;
    if (*predictor > 32767)
        *predictor = 32767;
    if (*predictor < -32768)
        *predictor = -32768;

    *step_index += index_table[nibble];
    if (*step_index < 0)
        *step_index = 0;
    if (*step_index > 88)
        *step_index = 88;

    return (int16_t)*predictor;
}

static void adpcm_stream_reset(cupcake_adpcm_stream_t *st)
{
    if (!st)
        return;
    memset(st, 0, sizeof *st);
}

static int stream_read_byte(cupcake_adpcm_stream_t *st, uint8_t *out)
{
    if (!st || !out)
        return 0;

    if (st->in) {
        if (st->pos >= st->in_len)
            return 0;
        *out = st->in[st->pos++];
        return 1;
    }

    if (st->use_dat) {
        if (st->dat_pos >= st->dat_len)
            return 0;

        if (st->file_buf_pos >= st->file_buf_len) {
            size_t chunk = sizeof st->file_buf;
            uint32_t remain = st->dat_len - st->dat_pos;

            if (chunk > remain)
                chunk = (size_t)remain;
            if (chunk == 0)
                return 0;
            if (cupcake_assets_dat_read(st->dat_offset + st->dat_pos, st->file_buf, chunk) != 0)
                return 0;

            st->file_buf_pos = 0;
            st->file_buf_len = chunk;
            st->dat_pos += (uint32_t)chunk;
        }
        *out = st->file_buf[st->file_buf_pos++];
        return 1;
    }

    return 0;
}

static void adpcm_stream_set_header(cupcake_adpcm_stream_t *st, const uint8_t hdr[3],
                                    int pcm_samples)
{
    st->predictor = (int16_t)((uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8));
    st->step_index = (int)hdr[2];
    if (st->step_index < 0)
        st->step_index = 0;
    if (st->step_index > 88)
        st->step_index = 88;
    st->samples_left = pcm_samples;
    st->header_pending = 1;
    st->nibble_phase = 0;
    st->cur_byte = 0;
}

void cupcake_adpcm_stream_init(cupcake_adpcm_stream_t *st, const uint8_t *in, size_t in_len,
                               int pcm_samples)
{
    uint8_t hdr[3];

    adpcm_stream_reset(st);
    if (!st)
        return;

    st->in = in;
    st->in_len = in_len;

    if (!in || in_len < 3u || pcm_samples < 1) {
        st->samples_left = 0;
        return;
    }

    hdr[0] = in[0];
    hdr[1] = in[1];
    hdr[2] = in[2];
    st->pos = 3;
    adpcm_stream_set_header(st, hdr, pcm_samples);
}

void cupcake_adpcm_stream_init_dat(cupcake_adpcm_stream_t *st, uint32_t offset, uint32_t len,
                                   int pcm_samples)
{
    uint8_t hdr[3];

    adpcm_stream_reset(st);
    if (!st)
        return;

    st->use_dat = 1;
    st->dat_offset = offset;
    st->dat_len = len;
    st->dat_pos = 0;

    if (len < 3u || pcm_samples < 1 ||
        cupcake_assets_dat_read(offset, hdr, 3) != 0) {
        st->samples_left = 0;
        return;
    }

    st->dat_pos = 3;
    adpcm_stream_set_header(st, hdr, pcm_samples);
}

void cupcake_adpcm_stream_close(cupcake_adpcm_stream_t *st)
{
    if (!st)
        return;
    st->in = NULL;
    st->file_buf_pos = 0;
    st->file_buf_len = 0;
}

int16_t cupcake_adpcm_stream_next(cupcake_adpcm_stream_t *st)
{
    int nibble;
    uint8_t b;

    if (!st || st->samples_left <= 0)
        return 0;

    if (st->header_pending) {
        st->header_pending = 0;
        st->samples_left--;
        return (int16_t)st->predictor;
    }

    if (st->nibble_phase == 0) {
        if (!stream_read_byte(st, &b)) {
            st->samples_left = 0;
            return (int16_t)st->predictor;
        }
        st->cur_byte = b;
        nibble = st->cur_byte & 0x0f;
        st->nibble_phase = 1;
    } else {
        nibble = (st->cur_byte >> 4) & 0x0f;
        st->nibble_phase = 0;
    }

    st->samples_left--;
    return adpcm_decode_nibble(&st->predictor, &st->step_index, nibble);
}

int cupcake_adpcm_decode(const uint8_t *in, size_t in_len, int16_t *out, int out_samples)
{
    cupcake_adpcm_stream_t st;
    int i;

    if (!in || !out || out_samples < 1)
        return -1;

    cupcake_adpcm_stream_init(&st, in, in_len, out_samples);
    for (i = 0; i < out_samples; i++)
        out[i] = cupcake_adpcm_stream_next(&st);
    return 0;
}

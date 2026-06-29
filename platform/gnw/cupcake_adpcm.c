/*
 * IMA ADPCM decoder — matches tools/bundle_overlay_assets.py encoder.
 */
#include "cupcake_adpcm.h"

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

void cupcake_adpcm_stream_init(cupcake_adpcm_stream_t *st, const uint8_t *in, size_t in_len,
                               int pcm_samples)
{
    if (!st)
        return;

    st->in = in;
    st->in_len = in_len;
    st->pos = 0;
    st->nibble_phase = 0;
    st->cur_byte = 0;
    st->samples_left = pcm_samples;
    st->header_pending = 0;

    if (!in || in_len < 3u || pcm_samples < 1) {
        st->samples_left = 0;
        st->predictor = 0;
        st->step_index = 0;
        return;
    }

    st->predictor = (int16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
    st->step_index = (int)in[2];
    if (st->step_index < 0)
        st->step_index = 0;
    if (st->step_index > 88)
        st->step_index = 88;
    st->pos = 3;
    st->header_pending = 1;
}

int16_t cupcake_adpcm_stream_next(cupcake_adpcm_stream_t *st)
{
    int nibble;

    if (!st || st->samples_left <= 0)
        return 0;

    if (st->header_pending) {
        st->header_pending = 0;
        st->samples_left--;
        return (int16_t)st->predictor;
    }

    if (st->nibble_phase == 0) {
        if (st->pos >= st->in_len) {
            st->samples_left = 0;
            return (int16_t)st->predictor;
        }
        st->cur_byte = st->in[st->pos++];
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
    int predictor;
    int step_index;
    size_t pos = 0;
    int sample_idx = 0;
    int nibble_phase = 0;
    uint8_t cur_byte = 0;

    if (!in || !out || out_samples < 1)
        return -1;

    if (in_len < 3)
        return -1;

    predictor = (int16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
    step_index = (int)in[2];
    if (step_index < 0)
        step_index = 0;
    if (step_index > 88)
        step_index = 88;
    pos = 3;

    out[sample_idx++] = (int16_t)predictor;

    while (sample_idx < out_samples) {
        int nibble;

        if (nibble_phase == 0) {
            if (pos >= in_len)
                break;
            cur_byte = in[pos++];
            nibble = cur_byte & 0x0f;
            nibble_phase = 1;
        } else {
            nibble = (cur_byte >> 4) & 0x0f;
            nibble_phase = 0;
        }

        out[sample_idx++] = adpcm_decode_nibble(&predictor, &step_index, nibble);
    }

    while (sample_idx < out_samples)
        out[sample_idx++] = (int16_t)predictor;

    return 0;
}

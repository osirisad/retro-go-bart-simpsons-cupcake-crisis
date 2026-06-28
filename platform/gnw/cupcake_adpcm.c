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
        int step;
        int diff;
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

        step = step_table[step_index];
        diff = step >> 3;
        if (nibble & 1)
            diff += step >> 2;
        if (nibble & 2)
            diff += step >> 1;
        if (nibble & 4)
            diff += step;
        if (nibble & 8)
            diff = -diff;

        predictor += diff;
        if (predictor > 32767)
            predictor = 32767;
        if (predictor < -32768)
            predictor = -32768;

        out[sample_idx++] = (int16_t)predictor;

        step_index += index_table[nibble];
        if (step_index < 0)
            step_index = 0;
        if (step_index > 88)
            step_index = 88;
    }

    while (sample_idx < out_samples)
        out[sample_idx++] = (int16_t)predictor;

    return 0;
}

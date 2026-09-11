#ifndef HOST_BUILD
/*
 * Small local shims for APIs that are not trampolined by gw_core_bridge
 * (implemented in firmware headers but not exported on the ABI table).
 */
#include <stdint.h>
#include <stdbool.h>

#include "gw_audio.h"
#include "odroid_overlay.h"
#include "common.h"
#include "stm32h7xx_hal.h"

void odroid_audio_submit(short *stereoAudioBuffer, int frameCount)
{
    int16_t *dst;
    int i;
    uint8_t vol;

    if (!stereoAudioBuffer || frameCount <= 0)
        return;

    if (common_emu_sound_loop_is_muted())
        return;

    dst = audio_get_active_buffer();
    if (!dst)
        return;

    vol = common_emu_sound_get_volume();

    /* >>4 matches GW emu ports (firmware volume stacks on top). */
    for (i = 0; i < frameCount; i++) {
        int32_t sample = (int32_t)stereoAudioBuffer[i] * (int32_t)vol;

        sample >>= 4;
        if (sample > 32767)
            sample = 32767;
        if (sample < -32768)
            sample = -32768;
        dst[i] = (int16_t)sample;
    }
}

void odroid_overlay_alert(const char *text)
{
    if (!text)
        return;
    odroid_overlay_draw_text(8, 40, 304, text, 0xFFFF, 0x0000);
    HAL_Delay(2500);
}
#endif /* !HOST_BUILD */

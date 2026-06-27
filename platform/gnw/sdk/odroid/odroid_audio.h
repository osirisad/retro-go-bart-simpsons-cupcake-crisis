#pragma once

#include <stdbool.h>

void odroid_audio_submit(short *stereoAudioBuffer, int frameCount);
int odroid_audio_sample_rate_get(void);

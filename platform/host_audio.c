/*
 * Cupcake Crisis SFX — SDL_mixer (PC + retro-go LINUX_EMU) or odroid PCM mixer (device).
 */
#include "host_audio.h"
#include "host_audio_catalog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(CUPCAKE_AUDIO_ODROID) && !defined(LINUX_EMU)

#include "odroid_audio.h"

#define HOST_VOICE_MAX 12

typedef struct {
    const int16_t *pcm;
    int len;
    int pos;
    float gain;
    int active;
} host_voice_t;

typedef struct {
    int16_t *pcm;
    int len;
    int sample_rate;
    float volume;
} host_pcm_t;

static host_pcm_t g_pcm[HOST_SFX_COUNT];
static host_voice_t g_voices[HOST_VOICE_MAX];
static char g_audio_dir[512];
static int g_ready;
static int g_device_rate;

static int read_u16_le(const uint8_t *p)
{
    return (int)p[0] | ((int)p[1] << 8);
}

static int read_u32_le(const uint8_t *p)
{
    return (int)p[0] | ((int)p[1] << 8) | ((int)p[2] << 16) | ((int)p[3] << 24);
}

static int load_wav_mono(const char *path, host_pcm_t *out)
{
    FILE *fp;
    uint8_t hdr[12];
    uint8_t chunk[8];
    int channels = 0;
    int bits = 0;
    int rate = 0;
    int data_bytes = 0;
    long data_off = 0;
    int16_t *raw = NULL;
    int16_t *mono = NULL;
    int samples;
    int i;

    if (!path || !out)
        return -1;
    memset(out, 0, sizeof *out);

    fp = fopen(path, "rb");
    if (!fp)
        return -1;
    if (fread(hdr, 1, 12, fp) != 12 || memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        fclose(fp);
        return -1;
    }

    while (fread(chunk, 1, 8, fp) == 8) {
        int sz = read_u32_le(chunk + 4);
        if (memcmp(chunk, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            if (sz < 16 || fread(fmt, 1, 16, fp) != 16) {
                fclose(fp);
                return -1;
            }
            channels = read_u16_le(fmt + 2);
            rate = read_u32_le(fmt + 4);
            bits = read_u16_le(fmt + 14);
            if (sz > 16)
                fseek(fp, sz - 16, SEEK_CUR);
        } else if (memcmp(chunk, "data", 4) == 0) {
            data_bytes = sz;
            data_off = ftell(fp);
            fseek(fp, sz, SEEK_CUR);
        } else {
            fseek(fp, sz, SEEK_CUR);
        }
        if (data_off > 0 && channels > 0 && bits == 16)
            break;
    }

    if (data_off <= 0 || channels < 1 || bits != 16 || rate <= 0) {
        fclose(fp);
        return -1;
    }

    fseek(fp, data_off, SEEK_SET);
    raw = (int16_t *)malloc((size_t)data_bytes);
    if (!raw || fread(raw, 1, (size_t)data_bytes, fp) != (size_t)data_bytes) {
        free(raw);
        fclose(fp);
        return -1;
    }
    fclose(fp);

    samples = data_bytes / (int)sizeof(int16_t) / channels;
    mono = (int16_t *)malloc((size_t)samples * sizeof(int16_t));
    if (!mono) {
        free(raw);
        return -1;
    }

    if (channels == 1) {
        memcpy(mono, raw, (size_t)samples * sizeof(int16_t));
    } else {
        for (i = 0; i < samples; i++) {
            int32_t sum = 0;
            int c;
            for (c = 0; c < channels; c++)
                sum += raw[i * channels + c];
            mono[i] = (int16_t)(sum / channels);
        }
    }
    free(raw);

    out->pcm = mono;
    out->len = samples;
    out->sample_rate = rate;
    return 0;
}

static void resample_to_device(host_pcm_t *pcm)
{
    int16_t *out;
    int out_len;
    int i;
    double src_pos;

    if (!pcm || !pcm->pcm || pcm->sample_rate == g_device_rate)
        return;

    out_len = (int)((double)pcm->len * (double)g_device_rate / (double)pcm->sample_rate);
    if (out_len < 1)
        out_len = 1;
    out = (int16_t *)malloc((size_t)out_len * sizeof(int16_t));
    if (!out)
        return;

    src_pos = 0.0;
    for (i = 0; i < out_len; i++) {
        int idx = (int)src_pos;
        if (idx >= pcm->len)
            idx = pcm->len - 1;
        out[i] = pcm->pcm[idx];
        src_pos += (double)pcm->sample_rate / (double)g_device_rate;
    }

    free(pcm->pcm);
    pcm->pcm = out;
    pcm->len = out_len;
    pcm->sample_rate = g_device_rate;
}

static void load_catalog_pcm(int index)
{
    char path[640];
    const host_sfx_def_t *def = &host_sfx_catalog[index];
    host_pcm_t *pcm = &g_pcm[index];

    if (pcm->pcm)
        return;
    snprintf(path, sizeof path, "%s/%s", g_audio_dir, def->file);
    if (load_wav_mono(path, pcm) != 0) {
        fprintf(stderr, "host_audio: missing %s\n", path);
        return;
    }
    pcm->volume = def->volume;
    resample_to_device(pcm);
}

static void stop_voices(void)
{
    int i;
    for (i = 0; i < HOST_VOICE_MAX; i++)
        g_voices[i].active = 0;
}

int host_audio_init(const char *assets_base)
{
    int i;

    if (g_ready)
        return 0;

    if (!assets_base || !assets_base[0])
        assets_base = "/home/odroid/cupcake";

    g_device_rate = odroid_audio_sample_rate_get();
    if (g_device_rate <= 0)
        g_device_rate = 22050;

    snprintf(g_audio_dir, sizeof g_audio_dir, "%s/audio", assets_base);

    for (i = 0; i < HOST_SFX_COUNT; i++)
        load_catalog_pcm(i);

    g_ready = 1;
    return 0;
}

void host_audio_shutdown(void)
{
    int i;

    if (!g_ready)
        return;

    stop_voices();
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        free(g_pcm[i].pcm);
        memset(&g_pcm[i], 0, sizeof g_pcm[i]);
    }
    g_ready = 0;
}

int host_audio_play(const char *sfx_id)
{
    const host_sfx_def_t *def;
    const host_pcm_t *pcm;
    int i;
    int slot = -1;

    if (!sfx_id || !sfx_id[0] || !g_ready)
        return -1;

    if (strcmp(sfx_id, "stop") == 0) {
        stop_voices();
        return 0;
    }

    def = host_sfx_catalog_find(sfx_id);
    if (!def)
        return -1;

    pcm = NULL;
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        if (strcmp(host_sfx_catalog[i].id, sfx_id) == 0) {
            load_catalog_pcm(i);
            pcm = &g_pcm[i];
            break;
        }
    }
    if (!pcm || !pcm->pcm || pcm->len <= 0)
        return -1;

    for (i = 0; i < HOST_VOICE_MAX; i++) {
        if (!g_voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return -1;

    g_voices[slot].pcm = pcm->pcm;
    g_voices[slot].len = pcm->len;
    g_voices[slot].pos = 0;
    g_voices[slot].gain = pcm->volume;
    g_voices[slot].active = 1;
    return 0;
}

void host_audio_pump(int frame_count)
{
    int16_t stack_buf[1024];
    int16_t *buf = stack_buf;
    int i;
    int v;

    if (!g_ready || frame_count <= 0)
        return;
    if (frame_count > (int)(sizeof stack_buf / sizeof stack_buf[0])) {
        buf = (int16_t *)calloc((size_t)frame_count, sizeof(int16_t));
        if (!buf)
            return;
    } else {
        memset(buf, 0, (size_t)frame_count * sizeof(int16_t));
    }

    for (i = 0; i < frame_count; i++) {
        int32_t mix = 0;
        for (v = 0; v < HOST_VOICE_MAX; v++) {
            host_voice_t *voice = &g_voices[v];
            if (!voice->active)
                continue;
            if (voice->pos >= voice->len) {
                voice->active = 0;
                continue;
            }
            mix += (int32_t)((float)voice->pcm[voice->pos++] * voice->gain);
        }
        if (mix > 32767)
            mix = 32767;
        if (mix < -32768)
            mix = -32768;
        buf[i] = (int16_t)mix;
    }

    odroid_audio_submit(buf, frame_count);
    if (buf != stack_buf)
        free(buf);
}

#else /* SDL_mixer backend */

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

static Mix_Chunk *g_chunks[HOST_SFX_COUNT];
static char g_audio_dir[512];
static int g_ready;

static void load_one(int index)
{
    char path[640];
    const host_sfx_def_t *def = &host_sfx_catalog[index];

    if (g_chunks[index])
        return;
    snprintf(path, sizeof path, "%s/%s", g_audio_dir, def->file);
    g_chunks[index] = Mix_LoadWAV(path);
    if (!g_chunks[index])
        fprintf(stderr, "host_audio: missing or unreadable %s\n", path);
}

int host_audio_init(const char *assets_base)
{
    int i;

    if (g_ready)
        return 0;

    if (!assets_base || !assets_base[0])
        assets_base = "assets";

    snprintf(g_audio_dir, sizeof g_audio_dir, "%s/audio", assets_base);

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        fprintf(stderr, "host_audio: Mix_OpenAudio failed: %s\n", Mix_GetError());
        return -1;
    }

    Mix_AllocateChannels(16);

    for (i = 0; i < HOST_SFX_COUNT; i++)
        load_one(i);

    g_ready = 1;
    return 0;
}

void host_audio_shutdown(void)
{
    int i;

    if (!g_ready)
        return;

    Mix_HaltChannel(-1);
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        if (g_chunks[i]) {
            Mix_FreeChunk(g_chunks[i]);
            g_chunks[i] = NULL;
        }
    }
    Mix_CloseAudio();
    g_ready = 0;
}

int host_audio_play(const char *sfx_id)
{
    const host_sfx_def_t *def;
    Mix_Chunk *chunk;
    int i;
    int vol;
    int ch;

    if (!sfx_id || !sfx_id[0])
        return -1;

    if (!g_ready)
        return -1;

    if (strcmp(sfx_id, "stop") == 0) {
        Mix_HaltChannel(-1);
        return 0;
    }

    def = host_sfx_catalog_find(sfx_id);
    if (!def)
        return -1;

    chunk = NULL;
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        if (strcmp(host_sfx_catalog[i].id, sfx_id) == 0) {
            load_one(i);
            chunk = g_chunks[i];
            break;
        }
    }
    if (!chunk)
        return -1;

    vol = (int)(def->volume * (float)MIX_MAX_VOLUME + 0.5f);
    if (vol < 0)
        vol = 0;
    if (vol > MIX_MAX_VOLUME)
        vol = MIX_MAX_VOLUME;
    Mix_VolumeChunk(chunk, vol);
    ch = Mix_PlayChannel(-1, chunk, 0);
    return ch >= 0 ? 0 : -1;
}

void host_audio_pump(int frame_count)
{
    (void)frame_count;
}

#endif

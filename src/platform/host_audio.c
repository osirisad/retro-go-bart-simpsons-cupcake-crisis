/*
 * Cupcake Crisis SFX — SDL_mixer (PC + retro-go LINUX_EMU) or odroid PCM mixer (device).
 */
#include "host_audio.h"
#include "host_audio_catalog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int g_muted;

#if defined(CUPCAKE_AUDIO_ODROID) && !defined(LINUX_EMU)

#include "odroid_audio.h"

#ifdef CUPCAKE_EMBEDDED_ASSETS
#include "cupcake_data.h"
#include "cupcake_adpcm.h"
#include "gnw_assets.h"
#if defined(CUPCAKE_GNW)
#include "cupcake_trace.h"
#endif
#include "cupcake_assets_dat.h"
#endif

#define HOST_VOICE_MAX 12

typedef struct {
    const int16_t *pcm;
    int len;
    int pos;
    float gain;
    int active;
} host_pcm_voice_t;

#ifdef CUPCAKE_EMBEDDED_ASSETS
typedef struct {
    cupcake_adpcm_stream_t dec;
    int pcm_len;
    int pos;
    int sample_rate;
    uint32_t rate_acc;
    int16_t cur_sample;
    float gain;
    int active;
} host_adpcm_voice_t;
#endif

typedef struct {
    int16_t *pcm;
    int len;
    int sample_rate;
    float volume;
} host_pcm_t;

static host_pcm_t g_pcm[HOST_SFX_COUNT];
#ifdef CUPCAKE_EMBEDDED_ASSETS
static host_adpcm_voice_t g_adpcm_voices[HOST_VOICE_MAX];
#else
static host_pcm_voice_t g_voices[HOST_VOICE_MAX];
static char g_audio_dir[512];
#endif
static int g_ready;
static int g_device_rate;

#ifndef CUPCAKE_EMBEDDED_ASSETS
static int read_u16_le(const uint8_t *p)
{
    return (int)p[0] | ((int)p[1] << 8);
}

static int read_u32_le(const uint8_t *p)
{
    return (int)p[0] | ((int)p[1] << 8) | ((int)p[2] << 16) | ((int)p[3] << 24);
}
#endif

#ifndef CUPCAKE_EMBEDDED_ASSETS
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
#endif

#ifndef CUPCAKE_EMBEDDED_ASSETS
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
#endif

#ifdef CUPCAKE_EMBEDDED_ASSETS
static void adpcm_voice_release(host_adpcm_voice_t *voice)
{
    if (!voice)
        return;
    voice->active = 0;
    cupcake_adpcm_stream_close(&voice->dec);
}
#endif

static void load_catalog_pcm(int index)
{
#ifdef CUPCAKE_EMBEDDED_ASSETS
    (void)index;
#else
    const host_sfx_def_t *def = &host_sfx_catalog[index];
    host_pcm_t *pcm = &g_pcm[index];

    if (pcm->pcm)
        return;

    {
        char path[640];

        snprintf(path, sizeof path, "%s/%s", g_audio_dir, def->file);
        if (load_wav_mono(path, pcm) != 0) {
            fprintf(stderr, "host_audio: missing %s\n", path);
            return;
        }
    }
    pcm->volume = def->volume;
    resample_to_device(pcm);
#endif
}

static void stop_voices(void)
{
    int i;
#ifdef CUPCAKE_EMBEDDED_ASSETS
    for (i = 0; i < HOST_VOICE_MAX; i++)
        adpcm_voice_release(&g_adpcm_voices[i]);
#else
    for (i = 0; i < HOST_VOICE_MAX; i++)
        g_voices[i].active = 0;
#endif
}

int host_audio_init(const char *assets_base)
{
    int i;

    if (g_ready)
        return 0;

    g_device_rate = odroid_audio_sample_rate_get();
    if (g_device_rate <= 0)
        g_device_rate = 22050;

#ifndef CUPCAKE_EMBEDDED_ASSETS
    if (!assets_base || !assets_base[0]) {
#if defined(CUPCAKE_GNW)
        assets_base = "/retro-go/cupcake";
#else
        assets_base = "/home/odroid/cupcake";
#endif
    }
    snprintf(g_audio_dir, sizeof g_audio_dir, "%s/audio", assets_base);
#else
    (void)assets_base;
#if defined(CUPCAKE_GNW_ASSETS_DAT_EMBEDDED)
    if (cupcake_assets_dat_init_mem(cupcake_gnw_assets_dat(),
                                    (size_t)CUPCAKE_GNW_ASSETS_DAT_BYTES) != 0) {
        cupcake_trace("audio: embedded dat invalid");
        return -1;
    }
    cupcake_trace("audio: embedded dat (%u clips, %u B, gain=%.2f)",
                  (unsigned)CUPCAKE_GNW_ASSETS_DAT_CLIPS,
                  (unsigned)CUPCAKE_GNW_ASSETS_DAT_BYTES, (double)CUPCAKE_GNW_PCM_PACK_GAIN);
#else
    if (cupcake_assets_dat_init(CUPCAKE_GNW_ASSETS_DAT_PATH) != 0) {
        cupcake_trace("audio: failed to open %s", CUPCAKE_GNW_ASSETS_DAT_PATH);
        return -1;
    }
    cupcake_trace("audio: dat %s (%u clips, %u B, gain=%.2f)",
                  CUPCAKE_GNW_ASSETS_DAT_PATH, (unsigned)CUPCAKE_GNW_ASSETS_DAT_CLIPS,
                  (unsigned)CUPCAKE_GNW_ASSETS_DAT_BYTES, (double)CUPCAKE_GNW_PCM_PACK_GAIN);
#endif
#endif

    for (i = 0; i < HOST_SFX_COUNT; i++)
        load_catalog_pcm(i);

    g_ready = 1;
    return 0;
}

void host_audio_shutdown(void)
{
#ifndef CUPCAKE_EMBEDDED_ASSETS
    int i;
#endif

    if (!g_ready)
        return;

    stop_voices();
#ifndef CUPCAKE_EMBEDDED_ASSETS
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        free(g_pcm[i].pcm);
        memset(&g_pcm[i], 0, sizeof g_pcm[i]);
    }
#else
    memset(g_pcm, 0, sizeof g_pcm);
    cupcake_assets_dat_shutdown();
#endif
    g_ready = 0;
}

#ifdef CUPCAKE_EMBEDDED_ASSETS
static int start_adpcm_voice_mem(host_adpcm_voice_t *voice, const uint8_t *payload,
                                 size_t payload_len, int pcm_samples, int sample_rate,
                                 float gain)
{
    if (!voice || !payload || payload_len < 3u || pcm_samples < 1)
        return -1;

    adpcm_voice_release(voice);
    cupcake_adpcm_stream_init(&voice->dec, payload, payload_len, pcm_samples);
    if (voice->dec.samples_left <= 0) {
        adpcm_voice_release(voice);
        return -1;
    }
    voice->pcm_len = pcm_samples;
    voice->sample_rate = (sample_rate > 0) ? sample_rate : 22050;
    voice->pos = 0;
    voice->rate_acc = 0;
    voice->cur_sample = 0;
    voice->gain = gain;
    voice->active = 1;
    voice->cur_sample = cupcake_adpcm_stream_next(&voice->dec);
    voice->pos = 1;
    return 0;
}
#endif

int host_audio_play(const char *sfx_id)
{
    const host_sfx_def_t *def;
    int i;
    int slot = -1;

    if (!sfx_id || !sfx_id[0] || !g_ready)
        return -1;

    if (g_muted && strcmp(sfx_id, "stop") != 0)
        return 0;

    if (strcmp(sfx_id, "stop") == 0) {
        stop_voices();
        return 0;
    }

    def = host_sfx_catalog_find(sfx_id);
    if (!def)
        return -1;

#ifdef CUPCAKE_EMBEDDED_ASSETS
    {
        cupcake_dat_clip_t clip;
        const uint8_t *payload;
        int rc;

        if (cupcake_assets_dat_lookup(def->file, &clip) != 0) {
#if defined(CUPCAKE_GNW)
            cupcake_trace("audio: missing in dat %s", def->file);
#endif
            return -1;
        }

        for (i = 0; i < HOST_VOICE_MAX; i++) {
            if (!g_adpcm_voices[i].active) {
                slot = i;
                break;
            }
        }
        if (slot < 0)
            return -1;

        payload = cupcake_assets_dat_payload(clip.offset, clip.adpcm_size);
        if (!payload) {
            /* SD fallback: copy into a temporary stack is too small; stream via dat. */
#if defined(CUPCAKE_GNW)
            cupcake_trace("audio: no mem payload for %s", def->file);
#endif
            return -1;
        }

        rc = start_adpcm_voice_mem(&g_adpcm_voices[slot], payload, (size_t)clip.adpcm_size,
                                   (int)clip.pcm_samples, (int)clip.sample_rate, def->volume);
        if (rc != 0)
            return -1;
#if defined(CUPCAKE_GNW)
        cupcake_trace("audio: mem sfx %s (%u B, %u samples @ %u Hz)", def->file,
                      (unsigned)clip.adpcm_size, (unsigned)clip.pcm_samples,
                      (unsigned)clip.sample_rate);
#endif
        return 0;
    }
#else
    const host_pcm_t *pcm;

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
#endif
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
#ifdef CUPCAKE_EMBEDDED_ASSETS
        for (v = 0; v < HOST_VOICE_MAX; v++) {
            host_adpcm_voice_t *voice = &g_adpcm_voices[v];
            int voice_rate;

            if (!voice->active)
                continue;

            mix += (int32_t)((float)voice->cur_sample * voice->gain);

            voice_rate = voice->sample_rate;
            if (voice_rate < 1)
                voice_rate = g_device_rate;

            voice->rate_acc += (uint32_t)voice_rate;
            while (voice->rate_acc >= (uint32_t)g_device_rate) {
                voice->rate_acc -= (uint32_t)g_device_rate;
                if (voice->pos >= voice->pcm_len || voice->dec.samples_left <= 0) {
                    adpcm_voice_release(voice);
                    break;
                }
                voice->cur_sample = cupcake_adpcm_stream_next(&voice->dec);
                voice->pos++;
            }
        }
#else
        for (v = 0; v < HOST_VOICE_MAX; v++) {
            host_pcm_voice_t *voice = &g_voices[v];
            if (!voice->active)
                continue;
            if (voice->pos >= voice->len) {
                voice->active = 0;
                continue;
            }
            mix += (int32_t)((float)voice->pcm[voice->pos++] * voice->gain);
        }
#endif
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

    if (g_muted && strcmp(sfx_id, "stop") != 0)
        return 0;

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

void host_audio_toggle_mute(void)
{
    g_muted = !g_muted;
    if (g_muted)
        host_audio_play("stop");
}

int host_audio_is_muted(void)
{
    return g_muted;
}

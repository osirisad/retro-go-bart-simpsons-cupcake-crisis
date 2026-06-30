/*
 * Single-file ADPCM archive reader for GNW SD assets (cupcake_assets.dat).
 *
 * FatFs is built with FF_FS_TINY=1: one sector buffer per volume, shared by all
 * FILE objects. Never open the same .dat twice or seek concurrently — use one
 * global FILE* and serialize all reads through cupcake_assets_dat_read().
 */
#include "cupcake_assets_dat.h"

#include <stdio.h>
#include <string.h>

#define CUPCAKE_DAT_MAX_CLIPS 32
#define CUPCAKE_DAT_HEADER_SIZE 12u
#define CUPCAKE_DAT_ENTRY_SIZE 48u

static FILE *g_dat_fp;
static cupcake_dat_clip_t g_clips[CUPCAKE_DAT_MAX_CLIPS];
static int g_clip_count;
static volatile int g_dat_io_lock;

static int read_u32_le(FILE *fp, uint32_t *out)
{
    uint8_t b[4];

    if (!fp || !out || fread(b, 1, 4, fp) != 4)
        return -1;
    *out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
    return 0;
}

static void wav_stem(const char *wav_file, char *stem, size_t stem_sz)
{
    const char *p = wav_file;
    const char *last_dot = NULL;

    if (!wav_file || !stem || stem_sz < 2u) {
        if (stem && stem_sz > 0u)
            stem[0] = '\0';
        return;
    }

    while (*p) {
        if (*p == '.')
            last_dot = p;
        p++;
    }

    if (last_dot && last_dot > wav_file) {
        size_t n = (size_t)(last_dot - wav_file);
        if (n >= stem_sz)
            n = stem_sz - 1u;
        memcpy(stem, wav_file, n);
        stem[n] = '\0';
    } else {
        snprintf(stem, stem_sz, "%s", wav_file);
    }
}

int cupcake_assets_dat_read(uint32_t offset, void *buf, size_t len)
{
    size_t got;

    if (!g_dat_fp || !buf || len == 0u)
        return -1;

    while (g_dat_io_lock)
        ;

    g_dat_io_lock = 1;

    if (fseek(g_dat_fp, (long)offset, SEEK_SET) != 0) {
        g_dat_io_lock = 0;
        return -1;
    }

    got = fread(buf, 1, len, g_dat_fp);
    g_dat_io_lock = 0;

    return got == len ? 0 : -1;
}

int cupcake_assets_dat_init(const char *path)
{
    FILE *fp;
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    int i;

    cupcake_assets_dat_shutdown();

    if (!path || !path[0])
        return -1;

    fp = fopen(path, "rb");
    if (!fp)
        return -1;

    if (read_u32_le(fp, &magic) != 0 || magic != CUPCAKE_ASSETS_DAT_MAGIC) {
        fclose(fp);
        return -1;
    }

    {
        uint8_t hdr[4];
        if (fread(hdr, 1, 4, fp) != 4) {
            fclose(fp);
            return -1;
        }
        version = (uint16_t)hdr[0] | ((uint16_t)hdr[1] << 8);
        count = (uint16_t)hdr[2] | ((uint16_t)hdr[3] << 8);
        (void)version;
    }

    if (fseek(fp, 4, SEEK_CUR) != 0) {
        fclose(fp);
        return -1;
    }

    if (count < 1 || count > CUPCAKE_DAT_MAX_CLIPS) {
        fclose(fp);
        return -1;
    }

    memset(g_clips, 0, sizeof g_clips);
    g_clip_count = (int)count;

    for (i = 0; i < g_clip_count; i++) {
        cupcake_dat_clip_t *clip = &g_clips[i];
        uint8_t name[32];
        uint8_t ent[16];

        if (fread(name, 1, sizeof name, fp) != sizeof name)
            goto fail;
        if (fread(ent, 1, sizeof ent, fp) != sizeof ent)
            goto fail;

        memcpy(clip->name, name, sizeof clip->name);
        clip->name[sizeof clip->name - 1u] = '\0';
        clip->pcm_samples = (uint32_t)ent[0] | ((uint32_t)ent[1] << 8) |
                            ((uint32_t)ent[2] << 16) | ((uint32_t)ent[3] << 24);
        clip->sample_rate = (uint32_t)ent[4] | ((uint32_t)ent[5] << 8) |
                            ((uint32_t)ent[6] << 16) | ((uint32_t)ent[7] << 24);
        clip->offset = (uint32_t)ent[8] | ((uint32_t)ent[9] << 8) |
                       ((uint32_t)ent[10] << 16) | ((uint32_t)ent[11] << 24);
        clip->adpcm_size = (uint32_t)ent[12] | ((uint32_t)ent[13] << 8) |
                           ((uint32_t)ent[14] << 16) | ((uint32_t)ent[15] << 24);

        if (clip->pcm_samples < 1u || clip->sample_rate < 1u || clip->adpcm_size < 3u)
            goto fail;
    }

    g_dat_fp = fp;
    return 0;

fail:
    fclose(fp);
    g_clip_count = 0;
    return -1;
}

void cupcake_assets_dat_shutdown(void)
{
    if (g_dat_fp) {
        fclose(g_dat_fp);
        g_dat_fp = NULL;
    }
    g_clip_count = 0;
    memset(g_clips, 0, sizeof g_clips);
    g_dat_io_lock = 0;
}

int cupcake_assets_dat_clip_count(void)
{
    return g_clip_count;
}

const cupcake_dat_clip_t *cupcake_assets_dat_clip(int index)
{
    if (index < 0 || index >= g_clip_count)
        return NULL;
    return &g_clips[index];
}

int cupcake_assets_dat_lookup(const char *wav_file, cupcake_dat_clip_t *out)
{
    char stem[32];
    int i;

    if (!wav_file || !out)
        return -1;

    wav_stem(wav_file, stem, sizeof stem);
    if (!stem[0])
        return -1;

    for (i = 0; i < g_clip_count; i++) {
        if (strcmp(g_clips[i].name, stem) == 0) {
            *out = g_clips[i];
            return 0;
        }
    }
    return -1;
}

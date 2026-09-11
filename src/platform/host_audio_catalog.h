#ifndef HOST_AUDIO_CATALOG_H
#define HOST_AUDIO_CATALOG_H

typedef struct {
    const char *id;
    const char *file;
    float volume;
} host_sfx_def_t;

#define HOST_SFX_COUNT 17

extern const host_sfx_def_t host_sfx_catalog[HOST_SFX_COUNT];

const host_sfx_def_t *host_sfx_catalog_find(const char *id);

#endif

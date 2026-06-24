#include "host_audio_catalog.h"

#include <string.h>

const host_sfx_def_t host_sfx_catalog[HOST_SFX_COUNT] = {
    {"bonus", "bonus.wav", 0.8f},
    {"couch", "couch.wav", 1.0f},
    {"cupcake", "cupcake.wav", 1.0f},
    {"deliver", "deliver.wav", 0.9f},
    {"five", "five.wav", 1.0f},
    {"marge", "marge.wav", 1.0f},
    {"miss", "miss.wav", 1.0f},
    {"move", "move.wav", 0.8f},
    {"over", "over.wav", 0.7f},
    {"pacifier1", "pacifier1.wav", 1.0f},
    {"pacifier2", "pacifier2.wav", 1.0f},
    {"phase", "phase.wav", 1.0f},
    {"points", "points.wav", 0.8f},
    {"start", "start.wav", 0.6f},
    {"step", "step.wav", 1.0f},
    {"throw", "throw.wav", 0.84f},
    {"whoa", "whoa.wav", 1.0f},
};

const host_sfx_def_t *host_sfx_catalog_find(const char *id)
{
    int i;

    if (!id)
        return NULL;
    for (i = 0; i < HOST_SFX_COUNT; i++) {
        if (strcmp(host_sfx_catalog[i].id, id) == 0)
            return &host_sfx_catalog[i];
    }
    return NULL;
}

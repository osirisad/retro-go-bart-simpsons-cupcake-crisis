/*
 * Per-level hi-score persistence — JS $G.$st.hiscores[level] (TASK-12).
 */
#include "cupcake_hiscore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CUPCAKE_HISCORE_MAGIC   0x48495343u /* "HISC" */
#define CUPCAKE_HISCORE_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t scores[CUPCAKE_HISCORE_LEVELS];
} cupcake_hiscore_file_t;

static uint32_t g_hiscores[CUPCAKE_HISCORE_LEVELS];
static char g_path[512];
static int g_path_set;

static int level_index(int level)
{
    if (level < 0)
        return 0;
    if (level > CUPCAKE_LEVEL_MAX)
        return CUPCAKE_LEVEL_MAX;
    return level;
}

void cupcake_hiscore_default_path(char *buf, size_t bufsz)
{
    if (!buf || bufsz == 0)
        return;

#if defined(CUPCAKE_GNW)
    snprintf(buf, bufsz, "/data/homebrew/cupcake_hiscores.dat");
    return;
#else
    const char *override;

    override = getenv("CUPCAKE_HISCORE_PATH");
    if (override && override[0]) {
        snprintf(buf, bufsz, "%s", override);
        return;
    }

#ifdef _WIN32
    {
        const char *local = getenv("LOCALAPPDATA");
        if (local && local[0]) {
            snprintf(buf, bufsz, "%s\\cupcake_hiscores.dat", local);
            return;
        }
    }
#endif

    {
        const char *home = getenv("HOME");
        if (!home || !home[0])
            home = getenv("USERPROFILE");
        if (home && home[0]) {
            snprintf(buf, bufsz, "%s/.cupcake_hiscores.dat", home);
            return;
        }
    }

    snprintf(buf, bufsz, "cupcake_hiscores.dat");
#endif
}

void cupcake_hiscore_set_path(const char *path)
{
    if (path && path[0]) {
        snprintf(g_path, sizeof g_path, "%s", path);
        g_path_set = 1;
    } else {
        g_path[0] = '\0';
        g_path_set = 0;
    }
}

static const char *hiscore_path(void)
{
    if (!g_path_set)
        cupcake_hiscore_default_path(g_path, sizeof g_path);
    return g_path;
}

uint32_t cupcake_hiscore_attract(const cupcake_play_state_t *play)
{
    uint32_t a;
    uint32_t b;

    if (!play)
        return 0;
    a = play->scoreboard.hi_score[1];
    b = play->scoreboard.hi_score[2];
    return a > b ? a : b;
}

static void apply_to_play(cupcake_play_state_t *play)
{
    if (!play)
        return;
    memcpy(play->scoreboard.hi_score, g_hiscores, sizeof g_hiscores);
    if (play->mode == CUPCAKE_MODE_DEMO && play->level == 0)
        play->scoreboard.value = cupcake_hiscore_attract(play);
}

void cupcake_hiscore_sync_from_play(const cupcake_play_state_t *play)
{
    if (!play)
        return;
    memcpy(g_hiscores, play->scoreboard.hi_score, sizeof g_hiscores);
}

int cupcake_hiscore_load(cupcake_play_state_t *play)
{
    cupcake_hiscore_file_t file;
    FILE *fp;

    memset(g_hiscores, 0, sizeof g_hiscores);

    fp = fopen(hiscore_path(), "rb");
    if (fp) {
        if (fread(&file, 1, sizeof file, fp) == sizeof file &&
            file.magic == CUPCAKE_HISCORE_MAGIC && file.version == CUPCAKE_HISCORE_VERSION)
            memcpy(g_hiscores, file.scores, sizeof g_hiscores);
        fclose(fp);
    }

    apply_to_play(play);
    return 1;
}

int cupcake_hiscore_save(void)
{
    cupcake_hiscore_file_t file;
    FILE *fp;

    file.magic = CUPCAKE_HISCORE_MAGIC;
    file.version = CUPCAKE_HISCORE_VERSION;
    memcpy(file.scores, g_hiscores, sizeof file.scores);

    fp = fopen(hiscore_path(), "wb");
    if (!fp)
        return 0;
    if (fwrite(&file, 1, sizeof file, fp) != sizeof file) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

void cupcake_hiscore_on_score_change(cupcake_play_state_t *play)
{
    int lvl;
    uint32_t prev;

    if (!play)
        return;
    if (play->mode != CUPCAKE_MODE_PLAY && play->mode != CUPCAKE_MODE_OVER)
        return;

    lvl = level_index((int)play->level);
    prev = g_hiscores[lvl];
    if (play->points <= prev)
        return;

    g_hiscores[lvl] = play->points;
    play->scoreboard.hi_score[lvl] = play->points;
    cupcake_hiscore_save();
}

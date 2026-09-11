#ifndef CUPCAKE_HISCORE_H_
#define CUPCAKE_HISCORE_H_

#include "cupcake_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CUPCAKE_HISCORE_LEVELS 3

/* Override save path (else platform default or CUPCAKE_HISCORE_PATH env). */
void cupcake_hiscore_set_path(const char *path);

void cupcake_hiscore_default_path(char *buf, size_t bufsz);

/* Load persisted scores into play->scoreboard.hi_score[]. Returns 1 on success. */
int cupcake_hiscore_load(cupcake_play_state_t *play);

/* Save current hi_score[] to disk. Returns 1 on success. */
int cupcake_hiscore_save(void);

/* JS AcclaimSuperplayDevice.onScoreChange: beat per-level hi-score, persist. */
void cupcake_hiscore_on_score_change(cupcake_play_state_t *play);

/* Copy play hi_score[] into persistence buffer (after load_state, etc.). */
void cupcake_hiscore_sync_from_play(const cupcake_play_state_t *play);

/* JS scoreboard.hiscore: max of level 1 and level 2 persisted scores. */
uint32_t cupcake_hiscore_attract(const cupcake_play_state_t *play);

#ifdef __cplusplus
}
#endif

#endif

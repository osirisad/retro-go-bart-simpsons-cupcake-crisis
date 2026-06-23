#ifndef CUPCAKE_RNG_H_
#define CUPCAKE_RNG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RetroFab $G.random / Math.random(n) parity.
 *
 * Seed strategy:
 *   - Runtime (default): cupcake_rng_init() at cupcake_init(). Uses getenv
 *     "CUPCAKE_RNG_SEED" if set; otherwise mixes time() with a stack address.
 *   - Tests / replay: cupcake_rng_seed(fixed) before gameplay, or set
 *     CUPCAKE_RNG_SEED=12345 in the environment.
 *   - Save states (TASK-03+): persist cupcake_rng_get_state() in the game blob.
 *
 * JS reference ($G.random):
 *   T3.Math.clamp(parseInt(Math.random() * n), 0, n - 1)
 *
 * Array.random() for Aircakes branch lists:
 *   choices[cupcake_rand_pick(choices, count)]
 */

void cupcake_rng_init(void);
void cupcake_rng_seed(uint32_t seed);
uint32_t cupcake_rng_get_state(void);
void cupcake_rng_set_state(uint32_t state);

/* Uniform integer in [0, n - 1]; n <= 0 returns 0 (matches clamped JS). */
int cupcake_rand(int n);

/* Pick one element — JS [a,b,c].random(). */
int cupcake_rand_pick(const int *choices, int count);

/* Couch / Pacifier spawn interval helpers (JS: base + $G.random(span)). */
int cupcake_rand_span(int base, int span);

#ifdef __cplusplus
}
#endif

#endif

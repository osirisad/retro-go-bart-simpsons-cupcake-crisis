/*
 * Portable RNG — RetroFab $G.random / Array.random parity.
 */
#include "cupcake_rng.h"

#include <stddef.h>
#include <stdlib.h>
#include <time.h>

static uint32_t g_rng_state;

static uint32_t rng_next_u32(void)
{
    uint32_t x = g_rng_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rng_state = x ? x : 0xA341316Cu;
    return g_rng_state;
}

/* [0, 1) — same range as Math.random() before parseInt scaling. */
static double rng_unit(void)
{
    return (double)(rng_next_u32() >> 8) / (double)(1u << 24);
}

static int rng_clamp(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

void cupcake_rng_seed(uint32_t seed)
{
    g_rng_state = seed ? seed : 0xC0FFEE42u;
    rng_next_u32();
}

uint32_t cupcake_rng_get_state(void)
{
    return g_rng_state;
}

void cupcake_rng_set_state(uint32_t state)
{
    g_rng_state = state ? state : 0xC0FFEE42u;
}

void cupcake_rng_init(void)
{
#if defined(CUPCAKE_GNW)
    uint32_t seed = 0xC170CAFEu;

    seed ^= (uint32_t)(uintptr_t)&seed;
    cupcake_rng_seed(seed);
#else
    const char *env = getenv("CUPCAKE_RNG_SEED");
    uint32_t seed;

    if (env && env[0]) {
        cupcake_rng_seed((uint32_t)strtoul(env, NULL, 0));
        return;
    }

#ifdef CUPCAKE_RNG_FIXED_SEED
    cupcake_rng_seed((uint32_t)CUPCAKE_RNG_FIXED_SEED);
    return;
#endif

    seed = 0xC170CAFEu;
    seed ^= (uint32_t)(uintptr_t)&seed;
    seed ^= (uint32_t)time(NULL);
    cupcake_rng_seed(seed);
#endif
}

int cupcake_rand(int n)
{
    int v;

    if (n <= 0)
        return 0;
    if (n == 1)
        return 0;

    v = (int)(rng_unit() * (double)n);
    return rng_clamp(v, 0, n - 1);
}

int cupcake_rand_pick(const int *choices, int count)
{
    if (!choices || count <= 0)
        return 0;
    return choices[cupcake_rand(count)];
}

int cupcake_rand_span(int base, int span)
{
    if (span <= 0)
        return base;
    return base + cupcake_rand(span);
}

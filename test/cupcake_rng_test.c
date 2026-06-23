/*
 * Standalone checks for cupcake_rng.c — run: make test-rng
 */
#include "cupcake_rng.h"

#include <stdio.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_deterministic_seed(void)
{
    int a, b;

    cupcake_rng_seed(0x12345678u);
    a = cupcake_rand(20);
    b = cupcake_rand(20);

    cupcake_rng_seed(0x12345678u);
    if (a != cupcake_rand(20))
        fail("deterministic: first draw mismatch after re-seed");
    if (b != cupcake_rand(20))
        fail("deterministic: second draw mismatch after re-seed");
}

static void test_range(void)
{
    int i;

    cupcake_rng_seed(99u);
    for (i = 0; i < 500; i++) {
        int v = cupcake_rand(20);
        if (v < 0 || v > 19)
            fail("rand(20): out of range");
    }
    for (i = 0; i < 100; i++) {
        if (cupcake_rand(1) != 0)
            fail("rand(1): must always be 0");
        if (cupcake_rand(0) != 0)
            fail("rand(0): must be 0");
    }
}

static void test_couch_interval(void)
{
    int i;

    cupcake_rng_seed(0xBEEFu);
    for (i = 0; i < 200; i++) {
        int next = cupcake_rand_span(20, 20);
        if (next < 20 || next > 39)
            fail("couch next: expected 20..39");
    }
}

static void test_marge_roll(void)
{
    int i;
    int hits = 0;

    cupcake_rng_seed(0xFACEu);
    for (i = 0; i < 3000; i++) {
        if (cupcake_rand(3) == 0)
            hits++;
    }
    if (hits < 800 || hits > 1200)
        fail("marge rand(3)==0: expected ~1/3 over 3000 trials");
}

static void test_rand_pick(void)
{
    static const int choices[] = {5, 2};
    int i;
    int ok = 0;

    cupcake_rng_seed(0xABCDu);
    for (i = 0; i < 100; i++) {
        int v = cupcake_rand_pick(choices, 2);
        if (v == 5 || v == 2)
            ok = 1;
        if (v != 5 && v != 2)
            fail("rand_pick: value not in choices");
    }
    if (!ok)
        fail("rand_pick: never returned a valid choice");
}

static void test_save_restore_state(void)
{
    int a, b, c;

    cupcake_rng_seed(777u);
    a = cupcake_rand(20);
    b = cupcake_rand(20);
    c = cupcake_rng_get_state();

    cupcake_rng_set_state(c);
    if (a != cupcake_rand(20))
        fail("save/restore: stream diverged after restore");
    if (b != cupcake_rand(20))
        fail("save/restore: second value diverged after restore");
}

int main(void)
{
    test_deterministic_seed();
    test_range();
    test_couch_interval();
    test_marge_roll();
    test_rand_pick();
    test_save_restore_state();

    if (g_fail)
        return 1;
    printf("cupcake_rng: all tests passed\n");
    return 0;
}

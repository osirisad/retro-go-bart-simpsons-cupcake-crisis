/*
 * TASK-52 record-mode spawn intervals — run: make test-record-mode
 */
#include "cupcake.h"

#include <stdio.h>
#include <string.h>

static int g_fail;

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void test_record_default_off(void)
{
    cupcake_init();
    if (cupcake_record_mode())
        fail("record: default off after init");
}

static void test_normal_spawn_intervals(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.record = 0;
    cupcake_rng_seed(42u);
    cupcake_pacifier_start(&p);
    if (p.pacifier.next < 20 || p.pacifier.next > 39)
        fail("normal pacifier: next in 20+rand(20)");

    cupcake_rng_seed(42u);
    cupcake_couch_start(&p);
    if (p.couch.next < 20 || p.couch.next > 39)
        fail("normal couch: next in 20+rand(20)");
}

static void test_record_spawn_intervals(void)
{
    cupcake_play_state_t p;

    memset(&p, 0, sizeof p);
    p.record = 1;
    cupcake_pacifier_start(&p);
    if (p.pacifier.next != 15)
        fail("record pacifier: next=15");

    cupcake_couch_start(&p);
    if (p.couch.next != 25)
        fail("record couch: next=25");
}

static void test_set_record_api(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    cupcake_set_record(1);
    if (!cupcake_record_mode())
        fail("set_record: enabled");

    p = cupcake_play_state();
    if (!p->record)
        fail("set_record: play state synced");

    cupcake_pacifier_start(p);
    if (p->pacifier.next != 15)
        fail("set_record: pacifier start uses 15");

    cupcake_couch_start(p);
    if (p->couch.next != 25)
        fail("set_record: couch start uses 25");
}

static void test_record_survives_init(void)
{
    cupcake_set_record(1);
    cupcake_init();
    if (!cupcake_record_mode())
        fail("record: preserved across cupcake_init");
}

static void test_phase_start_uses_record(void)
{
    cupcake_play_state_t *p;

    cupcake_init();
    cupcake_set_record(1);
    cupcake_play_state()->level = 1;
    cupcake_on_start(1, 0);
    cupcake_timers_update(cupcake_timers(), 3.16f);
    cupcake_timers_update(cupcake_timers(), 3.16f);

    p = cupcake_play_state();
    if (p->pacifier.next != 15)
        fail("phase start record: pacifier next=15");
    if (p->couch.next != 25)
        fail("phase start record: couch next=25");
}

int main(void)
{
    test_record_default_off();
    test_normal_spawn_intervals();
    test_record_spawn_intervals();
    test_set_record_api();
    test_record_survives_init();
    test_phase_start_uses_record();

    if (g_fail)
        return 1;
    printf("cupcake_record_mode: all tests passed\n");
    return 0;
}

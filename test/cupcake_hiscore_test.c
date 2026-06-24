/*
 * TASK-12 hi-score persistence — run: make test-hiscore
 */
#include "cupcake.h"
#include "cupcake_hiscore.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define unlink_file _unlink
#else
#include <unistd.h>
#define unlink_file unlink
#endif

static int g_fail;
static const char *g_test_path = "build-test/cupcake_hiscore_test.dat";

static void fail(const char *msg)
{
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void use_test_path(void)
{
#ifdef _WIN32
    _putenv("CUPCAKE_HISCORE_PATH=build-test/cupcake_hiscore_test.dat");
#else
    setenv("CUPCAKE_HISCORE_PATH", "build-test/cupcake_hiscore_test.dat", 1);
#endif
    cupcake_hiscore_set_path(g_test_path);
    unlink_file(g_test_path);
}

static void test_save_load_roundtrip(void)
{
    cupcake_play_state_t play;
    cupcake_play_state_t play2;

    use_test_path();
    memset(&play, 0, sizeof play);
    play.scoreboard.hi_score[0] = 1000;
    play.scoreboard.hi_score[1] = 2000;
    play.scoreboard.hi_score[2] = 3000;
    cupcake_hiscore_sync_from_play(&play);
    if (!cupcake_hiscore_save())
        fail("save: write file");

    memset(&play2, 0, sizeof play2);
    cupcake_hiscore_load(&play2);
    if (play2.scoreboard.hi_score[0] != 1000u)
        fail("load: level 0");
    if (play2.scoreboard.hi_score[1] != 2000u)
        fail("load: level 1");
    if (play2.scoreboard.hi_score[2] != 3000u)
        fail("load: level 2");
}

static void test_on_score_change_beats_and_persists(void)
{
    cupcake_play_state_t play;
    cupcake_play_state_t reloaded;

    use_test_path();
    memset(&play, 0, sizeof play);
    play.mode = CUPCAKE_MODE_PLAY;
    play.level = 1;
    play.points = 500;
    cupcake_hiscore_sync_from_play(&play);
    play.scoreboard.hi_score[1] = 500;
    cupcake_hiscore_save();

    play.points = 750;
    play.scoreboard.hi_score[1] = 500;
    cupcake_hiscore_on_score_change(&play);
    if (play.scoreboard.hi_score[1] != 750u)
        fail("onScoreChange: updates beat score");

    play.points = 600;
    cupcake_hiscore_on_score_change(&play);
    if (play.scoreboard.hi_score[1] != 750u)
        fail("onScoreChange: lower score ignored");

    memset(&reloaded, 0, sizeof reloaded);
    cupcake_hiscore_load(&reloaded);
    if (reloaded.scoreboard.hi_score[1] != 750u)
        fail("onScoreChange: persisted to disk");
}

static void test_hiscore_attract_max_level_scores(void)
{
    cupcake_play_state_t play;

    memset(&play, 0, sizeof play);
    play.scoreboard.hi_score[1] = 1200;
    play.scoreboard.hi_score[2] = 3400;
    if (cupcake_hiscore_attract(&play) != 3400u)
        fail("attract: max of level 1 and 2");
    play.scoreboard.hi_score[2] = 800;
    if (cupcake_hiscore_attract(&play) != 1200u)
        fail("attract: level 1 can be higher");
}

static void test_init_loads_hiscores(void)
{
    cupcake_play_state_t play;

    use_test_path();
    memset(&play, 0, sizeof play);
    play.scoreboard.hi_score[1] = 4242;
    cupcake_hiscore_sync_from_play(&play);
    cupcake_hiscore_save();

    cupcake_init();
    if (cupcake_play_state()->scoreboard.hi_score[1] != 4242u)
        fail("init: loads hi-score from file");
    if (cupcake_get_state()->play.scoreboard.value != 4242u)
        fail("init: demo value from attract hi-score");
}

static void test_add_points_triggers_hiscore(void)
{
    use_test_path();
    cupcake_init();
    cupcake_play_state()->level = 2;
    cupcake_play_state()->mode = CUPCAKE_MODE_PLAY;
    cupcake_play_state()->points = 0;
    cupcake_add_points(1500);
    if (cupcake_play_state()->scoreboard.hi_score[2] != 1500u)
        fail("addPoints: hi-score updated via onScoreChange");

    memset(cupcake_play_state(), 0, sizeof(cupcake_play_state_t));
    cupcake_hiscore_load(cupcake_play_state());
    if (cupcake_play_state()->scoreboard.hi_score[2] != 1500u)
        fail("addPoints: hi-score persisted");
}

int main(void)
{
    test_save_load_roundtrip();
    test_on_score_change_beats_and_persists();
    test_hiscore_attract_max_level_scores();
    test_init_loads_hiscores();
    test_add_points_triggers_hiscore();

    unlink_file(g_test_path);

    if (g_fail)
        return 1;
    printf("cupcake_hiscore: all tests passed\n");
    return 0;
}

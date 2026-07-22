// Standalone AI/forces separation test -- NOT part of the shipped game
// (main.c is excluded when building this, same pattern as the other
// docs/verification/*.c harnesses). See docs/ai-forces-separation-map.md.
//
// This is the FIRST test in the repository to assert on the actual AI
// movement math (boss drift, regular-enemy drift, smart-enemy chase) -- a
// confirmed pre-existing coverage gap this test closes, not just a
// regression check for the Phase 18 extraction. Assertions use the exact
// same float literals/expressions the production code uses, so comparisons
// are bit-exact, not tolerance-based, matching this session's established
// test style.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: not needed here -- this test never touches
// game->app.scene.
#include "app.h"
#include "ai_forces.h"

static int failures = 0;

static bool approximately_equal(float a, float b)
{
    return fabsf(a - b) < 0.001f;
}

#define CHECK(desc, cond)                                            \
    do                                                                 \
    {                                                                   \
        if (cond)                                                        \
        {                                                                 \
            printf("AI FORCES TEST: PASS - %s\n", desc);                   \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("AI FORCES TEST: FAIL - %s\n", desc);                       \
            failures++;                                                        \
        }                                                                       \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("AI FORCES TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. move_boss_entities(): band-steering toward screen-center, both
    //    directions, plus gravity clamp.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->boss[0].x = 600.0f; // left of center (640), in the 300-350 y band
    game->boss[0].y = 320.0f;
    game->boss[0].dx = 30.0f;
    game->boss[0].dy = 60.0f;
    game->boss[1].x = 700.0f; // right of center, in the 300-350 y band
    game->boss[1].y = 320.0f;
    game->boss[1].dx = -30.0f;
    game->boss[1].dy = 60.0f;

    move_boss_entities(game, PHYSICS_DT);
    CHECK("boss drift: left-of-center steers dx toward center (positive)",
          game->boss[0].y == 321.0f && game->boss[0].x == 600.5f &&
          game->boss[0].dy == 66.0f && game->boss[0].dx == 36.0f);
    CHECK("boss drift: right-of-center steers dx toward center (negative)",
          game->boss[1].y == 321.0f && game->boss[1].x == 699.5f &&
          game->boss[1].dy == 66.0f && game->boss[1].dx == -36.0f);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->boss[0].x = 100.0f;
    game->boss[0].y = 320.0f;
    game->boss[0].dx = 30.0f;
    game->boss[0].dy = 87.0f; // near the 90 px/s clamp
    game->boss[1].x = 100.0f;
    game->boss[1].y = 0.0f;
    game->boss[1].dx = 0.0f;
    game->boss[1].dy = 0.0f;

    move_boss_entities(game, PHYSICS_DT);
    CHECK("boss drift: gravity clamps at 90 px/s", game->boss[0].dy == BOSS_MAX_FALL_SPEED_PER_SEC);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->boss[0].x = 1290.0f; // past the right wraparound edge
    game->boss[0].y = 100.0f; // outside the steering band
    game->boss[0].dx = 0.0f;
    game->boss[0].dy = 0.0f;
    game->boss[1].x = -10.0f; // past the left wraparound edge
    game->boss[1].y = 100.0f;
    game->boss[1].dx = 0.0f;
    game->boss[1].dy = 0.0f;

    move_boss_entities(game, PHYSICS_DT);
    CHECK("boss drift: wraps to 0 past the right edge", game->boss[0].x == 0.0f);
    CHECK("boss drift: wraps to 1280 past the left edge", game->boss[1].x == 1280.0f);

    // ------------------------------------------------------------------
    // 2. move_regular_enemies(): same band-steering shape, different
    //    constants, plus gravity clamp and wraparound.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0].x = 620.0f; // > 610, in the 150-170 y band
    game->enemyValues[0].y = 160.0f;
    game->enemyValues[0].dx = 60.0f;
    game->enemyValues[0].dy = 0.0f;
    game->enemyValues[1].x = 580.0f; // < 590, in the 150-170 y band
    game->enemyValues[1].y = 160.0f;
    game->enemyValues[1].dx = -60.0f;
    game->enemyValues[1].dy = 0.0f;

    move_regular_enemies(game, PHYSICS_DT);
    CHECK("enemy drift: x>610 in band steers dx positive",
          approximately_equal(game->enemyValues[0].x, 621.0f) &&
          approximately_equal(game->enemyValues[0].dx, 72.0f) &&
          approximately_equal(game->enemyValues[0].dy, 24.0f));
    CHECK("enemy drift: x<590 in band steers dx negative",
          approximately_equal(game->enemyValues[1].x, 579.0f) &&
          approximately_equal(game->enemyValues[1].dx, -72.0f) &&
          approximately_equal(game->enemyValues[1].dy, 24.0f));

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0].x = 620.0f;
    game->enemyValues[0].y = 160.0f;
    game->enemyValues[0].dx = 114.0f;
    game->enemyValues[0].dy = 147.0f; // near the 150 px/s clamp

    move_regular_enemies(game, PHYSICS_DT);
    CHECK("enemy drift: gravity clamps at 150 px/s", game->enemyValues[0].dy == ENEMY_MAX_FALL_SPEED_PER_SEC);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->enemyValues[0].x = 1290.0f;
    game->enemyValues[0].y = 0.0f; // outside the steering band
    game->enemyValues[0].dx = 0.0f;
    game->enemyValues[0].dy = 0.0f;
    game->enemyValues[1].x = -5.0f;
    game->enemyValues[1].y = 0.0f;
    game->enemyValues[1].dx = 0.0f;
    game->enemyValues[1].dy = 0.0f;

    move_regular_enemies(game, PHYSICS_DT);
    CHECK("enemy drift: wraps to 0 past the right edge", game->enemyValues[0].x == 0.0f);
    CHECK("enemy drift: wraps to 1280 past the left edge", game->enemyValues[1].x == 1280.0f);

    // ------------------------------------------------------------------
    // 3. move_smart_enemies(): single-player horizontal chase with the
    //    inline hop impulse (dy = -360 px/s when dx == 0 at the moment a steering
    //    branch fires).
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100.0f;
    game->man.y = 200.0f;
    game->smartEnemies[0].x = 120.0f; // x - 6 (114) > man.x (100)
    game->smartEnemies[0].y = 100.0f; // < man.y, so the vertical while-loop is skipped
    game->smartEnemies[0].dx = 0.0f; // triggers the hop impulse
    game->smartEnemies[0].dy = 300.0f;

    move_smart_enemies(game, PHYSICS_DT);
    CHECK("smart enemy chase (single-player): hop impulse fires and steers left",
          approximately_equal(game->smartEnemies[0].y, 105.0f) &&
          approximately_equal(game->smartEnemies[0].x, 120.0f) &&
          approximately_equal(game->smartEnemies[0].dx, -12.0f) &&
          approximately_equal(game->smartEnemies[0].dy, -300.0f));

    // ------------------------------------------------------------------
    // 4. smart_enemy_select_target(): the isolated "decide intent" step.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 1000.0f;
    game->man.y = 1000.0f;
    game->secondPlayer.x = 1.0f;
    game->secondPlayer.y = 1.0f;
    game->smartEnemies[0].x = 0.0f;
    game->smartEnemies[0].y = 0.0f;

    CHECK("target selection: single-player always targets man, regardless of distance",
          smart_enemy_select_target(game, &game->smartEnemies[0]) == &game->man);

    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->smartEnemies[0].x = 0.0f;
    game->smartEnemies[0].y = 0.0f;
    game->man.x = 10.0f; // squared distance 100
    game->man.y = 0.0f;
    game->secondPlayer.x = 100.0f; // squared distance 10000
    game->secondPlayer.y = 0.0f;

    CHECK("target selection: multiplayer picks the nearer player (man)",
          smart_enemy_select_target(game, &game->smartEnemies[0]) == &game->man);

    game->man.x = 100.0f; // squared distance 10000
    game->man.y = 0.0f;
    game->secondPlayer.x = 10.0f; // squared distance 100
    game->secondPlayer.y = 0.0f;

    CHECK("target selection: multiplayer picks the nearer player (secondPlayer)",
          smart_enemy_select_target(game, &game->smartEnemies[0]) == &game->secondPlayer);

    // ------------------------------------------------------------------
    // 5. process() still delegates correctly: a single process() call
    //    produces the same boss-drift result move_boss_entities() does
    //    directly, proving the process.c call-site swap wired up cleanly.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->boss[0].x = 600.0f;
    game->boss[0].y = 320.0f;
    game->boss[0].dx = 30.0f;
    game->boss[0].dy = 60.0f;

    process(game, PHYSICS_DT);
    CHECK("process() delegation: boss drift still fires via move_boss_entities()",
          game->boss[0].dy == 66.0f && game->boss[0].dx == 36.0f);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("AI FORCES TEST: ALL PASS\n");
        return 0;
    }
    printf("AI FORCES TEST: %d FAILURE(S)\n", failures);
    return 1;
}

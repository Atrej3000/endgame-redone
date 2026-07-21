// Standalone collision ordering test -- NOT part of the shipped game
// (main.c is excluded when building this, same pattern as the other
// docs/verification/*.c harnesses). See docs/collision-ordering-map.md.
//
// This is the FIRST test in the repository to assert on the Arcade
// hazard/game-over-transition logic and the Runner fall-hazard/
// game-over-transition logic in isolation -- previously only reachable via
// a full processEvents()/processEvents2() call with a real event loop. A
// confirmed pre-existing coverage gap this test closes, not just a
// regression check for the Phase 19 extraction.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: not needed here -- every scene value used
// below is set only to exercise resolve_*_game_over_transition()'s own
// guard, not bypassed elsewhere.
#include "app.h"
#include "collision_pipeline.h"

static int failures = 0;

#define CHECK(desc, cond)                                            \
    do                                                                 \
    {                                                                   \
        if (cond)                                                        \
        {                                                                 \
            printf("COLLISION PIPELINE TEST: PASS - %s\n", desc);          \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("COLLISION PIPELINE TEST: FAIL - %s\n", desc);              \
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
        printf("COLLISION PIPELINE TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. resolve_arcade_hazards(): body contact (enemy/boss/smartEnemy),
    //    reached-bottom, and fall-off-screen.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100.0f;
    game->man.y = 100.0f;
    game->man.lives = 3;
    game->enemyValues[0].x = 100.0f; // co-located -> guaranteed collide2d hit
    game->enemyValues[0].y = 100.0f;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: man-vs-enemy contact sets lives to 0",
          game->man.lives == 0 && game->man.y == 1000.0f);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 200.0f;
    game->man.y = 200.0f;
    game->man.lives = 3;
    game->boss[0].x = 200.0f;
    game->boss[0].y = 200.0f;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: man-vs-boss contact sets lives to 0", game->man.lives == 0);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 300.0f;
    game->man.y = 300.0f;
    game->man.lives = 3;
    game->smartEnemies[0].x = 300.0f;
    game->smartEnemies[0].y = 300.0f;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: man-vs-smartEnemy contact sets lives to 0", game->man.lives == 0);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f; // out of contact range of everything
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 3;
    game->kills_score = 0;
    game->tempScore = 0;
    game->enemyValues[0].y = 732.0f; // in the 730-734 reached-bottom band

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: enemy reached bottom decrements gameLives and scores a kill",
          game->gameLives == 2 && game->kills_score == 1 && game->tempScore == 1);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 1; // one enemy reaching bottom will zero this
    game->enemyValues[0].y = 732.0f;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: gameLives reaching 0 via reached-bottom also zeros man.lives",
          game->gameLives == 0 && game->man.lives == 0);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 3;
    game->boss[0].y = 735.0f; // in the 730-740 boss-reached-bottom band

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: boss reached bottom forces gameLives and man.lives to 0",
          game->gameLives == 0 && game->man.lives == 0);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = 720.0f; // >= 719
    game->man.lives = 3;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: fall-off-screen sets man.lives to 0", game->man.lives == 0);

    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->secondPlayer.x = 400.0f;
    game->secondPlayer.y = 400.0f;
    game->secondPlayer.lives = 3;
    game->enemyValues[0].x = 400.0f; // co-located with secondPlayer only
    game->enemyValues[0].y = 400.0f;

    resolve_arcade_hazards(game);
    CHECK("arcade hazard: secondPlayer-vs-enemy contact sets secondPlayer.lives to 0",
          game->secondPlayer.lives == 0 && game->man.lives == 3);

    // ------------------------------------------------------------------
    // 2. resolve_arcade_game_over_transition(): single/multi-player, and
    //    the double-transition guard.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.lives = 0;
    game->app.scene = APP_SCENE_ARCADE_GAME;

    resolve_arcade_game_over_transition(game);
    CHECK("arcade transition: single-player game over transitions to the mode menu",
          game->app.scene == APP_SCENE_ARCADE_MENU);

    game->app.scene = APP_SCENE_MAIN_MENU; // simulate an earlier handler already transitioned
    resolve_arcade_game_over_transition(game);
    CHECK("arcade transition: guard does not overwrite an earlier transition",
          game->app.scene == APP_SCENE_MAIN_MENU);

    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->man.lives = 0;
    game->secondPlayer.lives = 3;
    game->app.scene = APP_SCENE_ARCADE_GAME;

    resolve_arcade_game_over_transition(game);
    CHECK("arcade transition: multiplayer requires both players' lives at 0",
          game->app.scene == APP_SCENE_ARCADE_GAME);

    game->secondPlayer.lives = 0;
    resolve_arcade_game_over_transition(game);
    CHECK("arcade transition: multiplayer transitions once both players are out",
          game->app.scene == APP_SCENE_ARCADE_MENU);

    // ------------------------------------------------------------------
    // 3. resolve_runner_hazard_contact(): star contact triggers the
    //    single-shot death lifecycle.
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 50.0f;
    game->man.y = 50.0f;
    game->stars[0].x = 50.0f; // co-located -> guaranteed collide2d hit
    game->stars[0].y = 50.0f;

    resolve_runner_hazard_contact(game);
    CHECK("runner hazard: star contact triggers death",
          game->man.isDead == 1 && game->deathCountdown == 120);

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;

    resolve_runner_hazard_contact(game);
    CHECK("runner hazard: no contact leaves isDead at 0", game->man.isDead == 0);

    // ------------------------------------------------------------------
    // 4. check_runner_fall_hazard(): falling off the bottom or the left
    //    edge, both players.
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.y = 720.0f; // >= 719

    check_runner_fall_hazard(game);
    CHECK("runner fall hazard: y past the bottom edge triggers death", game->man.isDead == 1);

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5.0f; // < 0

    check_runner_fall_hazard(game);
    CHECK("runner fall hazard: x past the left edge triggers death", game->man.isDead == 1);

    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.y = 720.0f;

    check_runner_fall_hazard(game);
    CHECK("runner fall hazard: secondPlayer past the bottom edge triggers the shared death flag",
          game->man.isDead == 1);

    // ------------------------------------------------------------------
    // 5. resolve_runner_game_over_transition(): score-persist plus the
    //    game-over transition, and the double-transition guard.
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->gameLives = 0;
    game->x_score = 42;
    game->app.scene = APP_SCENE_RUNNER_GAME;

    resolve_runner_game_over_transition(game);
    CHECK("runner transition: score persisted and scene transitions to the mode menu",
          game->x_list[0] == 42 && game->x_i == 1 && game->app.scene == APP_SCENE_RUNNER_MENU);

    game->app.scene = APP_SCENE_MAIN_MENU; // simulate an earlier handler already transitioned
    resolve_runner_game_over_transition(game);
    CHECK("runner transition: guard does not overwrite an earlier transition",
          game->app.scene == APP_SCENE_MAIN_MENU);

    // ------------------------------------------------------------------
    // 6. resolve_projectile_hits(): confirms the extracted function still
    //    works correctly when called directly (the existing
    //    projectile_correctness_test.c already covers this thoroughly via
    //    process()'s delegation).
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->kills_score = 0;
    game->enemyValues[0].x = 40.0f;
    game->enemyValues[0].y = 100.0f;
    game->enemyValues[0].visible = 1;
    game->bullets[0].active = true;
    game->bullets[0].prevX = 30.0f;
    game->bullets[0].x = 50.0f;
    game->bullets[0].y = 110.0f;

    resolve_projectile_hits(game);
    CHECK("projectile hits: direct call still kills the enemy and scores",
          game->enemyValues[0].visible == 0 && game->kills_score == 1);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("COLLISION PIPELINE TEST: ALL PASS\n");
        return 0;
    }
    printf("COLLISION PIPELINE TEST: %d FAILURE(S)\n", failures);
    return 1;
}

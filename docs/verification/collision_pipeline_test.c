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
#include "game_events.h"

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

static void reset_arcade_test(GameState *game, GameMode mode)
{
    arcade_session_reset(game, mode);
    game->app.scene = APP_SCENE_ARCADE_GAME;
}

static void reset_runner_test(GameState *game, GameMode mode)
{
    runner_session_reset(game, mode);
    game->app.scene = APP_SCENE_RUNNER_GAME;
}

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
    // 1. detect_arcade_hazards() + game_events_apply(): body contact (enemy/boss/smartEnemy),
    //    reached-bottom, and fall-off-screen.
    // ------------------------------------------------------------------
    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100.0f;
    game->man.y = 100.0f;
    game->man.lives = 3;
    game->enemyValues[0].x = 100.0f; // co-located -> guaranteed collide2d hit
    game->enemyValues[0].y = 100.0f;
    game->enemyValues[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    CHECK("arcade detection: contact queues an event without changing lives",
          game->events.count >= 1 && game->man.lives == 3 && game->man.y == 100.0f);
    game_events_apply(game);
    CHECK("arcade hazard: man-vs-enemy contact removes exactly one shared life and respawns",
          game->gameLives == 9 && game->man.lives == 3 && game->man.y == 200.0f &&
          game->man.damageInvulnerabilityTicks == PLAYER_DAMAGE_INVULNERABILITY_TICKS);

    game->man.x = 100.0f;
    game->man.y = 100.0f;
    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: contact during invulnerability does not remove another life",
          game->gameLives == 9);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 200.0f;
    game->man.y = 200.0f;
    game->man.lives = 3;
    game->boss[0].x = 200.0f;
    game->boss[0].y = 200.0f;
    game->boss[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: man-vs-boss contact removes exactly one shared life", game->gameLives == 9);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 300.0f;
    game->man.y = 300.0f;
    game->man.lives = 3;
    game->smartEnemies[0].x = 300.0f;
    game->smartEnemies[0].y = 300.0f;
    game->smartEnemies[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: man-vs-smartEnemy contact removes exactly one shared life", game->gameLives == 9);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f; // out of contact range of everything
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 3;
    game->kills_score = 0;
    game->tempScore = 0;
    game->enemyValues[0].y = 732.0f; // in the 730-734 reached-bottom band
    game->enemyValues[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: enemy reached bottom decrements gameLives without awarding a kill",
          game->gameLives == 2 && game->kills_score == 0 && game->tempScore == 0 &&
          game->enemyValues[0].y == 1000.0f && game->enemyValues[0].visible == 0);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 1; // one enemy reaching bottom will zero this
    game->enemyValues[0].y = 732.0f;
    game->enemyValues[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: gameLives reaching 0 via reached-bottom also zeros man.lives",
          game->gameLives == 0 && game->man.lives == 0);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->man.lives = 3;
    game->gameLives = 3;
    game->boss[0].y = 735.0f; // in the 730-740 boss-reached-bottom band
    game->boss[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: boss reached bottom forces gameLives and man.lives to 0",
          game->gameLives == 0 && game->man.lives == 0);

    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = 720.0f; // >= 719
    game->man.lives = 3;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: fall-off-screen removes one shared life and respawns man",
          game->gameLives == 9 && game->man.lives == 3 && game->man.y == 200.0f);

    reset_arcade_test(game, GAME_MODE_MULTIPLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;
    game->secondPlayer.x = 400.0f;
    game->secondPlayer.y = 400.0f;
    game->secondPlayer.lives = 3;
    game->enemyValues[0].x = 400.0f; // co-located with secondPlayer only
    game->enemyValues[0].y = 400.0f;
    game->enemyValues[0].visible = 1;

    game_events_begin(game);
    detect_arcade_hazards(game);
    game_events_apply(game);
    CHECK("arcade hazard: secondPlayer-vs-enemy contact removes one shared life",
          game->gameLives == 9 && game->secondPlayer.lives == 3 && game->man.lives == 3);

    // ------------------------------------------------------------------
    // 2. GAME_EVENT_ARCADE_GAME_OVER_CHECK: single/multi-player, and
    //    the double-transition guard.
    // ------------------------------------------------------------------
    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.lives = 0;
    game->gameLives = 0;
    game->app.scene = APP_SCENE_ARCADE_GAME;

    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("arcade transition: single-player game over transitions to the mode menu",
          game->app.scene == APP_SCENE_ARCADE_MENU);

    game->app.scene = APP_SCENE_MAIN_MENU; // simulate an earlier handler already transitioned
    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("arcade transition: guard does not overwrite an earlier transition",
          game->app.scene == APP_SCENE_MAIN_MENU);

    reset_arcade_test(game, GAME_MODE_MULTIPLAYER);
    game->man.lives = 0;
    game->secondPlayer.lives = 3;
    game->app.scene = APP_SCENE_ARCADE_GAME;

    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("arcade transition: multiplayer requires both players' lives at 0",
          game->app.scene == APP_SCENE_ARCADE_GAME);

    game->secondPlayer.lives = 0;
    game->gameLives = 0;
    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("arcade transition: multiplayer transitions once both players are out",
          game->app.scene == APP_SCENE_ARCADE_MENU);

    // ------------------------------------------------------------------
    // 3. detect_runner_hazard_contacts() + game_events_apply(): star contact triggers the
    //    single-shot death lifecycle.
    // ------------------------------------------------------------------
    reset_runner_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 50.0f;
    game->man.y = 50.0f;
    game->stars[0].x = 50.0f; // co-located -> guaranteed collide2d hit
    game->stars[0].y = 50.0f;

    game_events_begin(game);
    detect_runner_hazard_contacts(game);
    CHECK("runner detection: star contact queues an event without triggering death",
          game->events.count == 1 && game->man.isDead == 0);
    game_events_apply(game);
    CHECK("runner hazard: star contact triggers death",
          game->man.isDead == 1 && game->deathCountdown == 120);

    reset_runner_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5000.0f;
    game->man.y = -5000.0f;

    game_events_begin(game);
    detect_runner_hazard_contacts(game);
    game_events_apply(game);
    CHECK("runner hazard: no contact leaves isDead at 0", game->man.isDead == 0);

    // ------------------------------------------------------------------
    // 4. detect_runner_fixed_hazards() + game_events_apply(): falling off the bottom or the left
    //    edge, both players.
    // ------------------------------------------------------------------
    reset_runner_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.y = 720.0f; // >= 719

    game_events_begin(game);
    detect_runner_fixed_hazards(game);
    game_events_apply(game);
    CHECK("runner fall hazard: y past the bottom edge triggers death", game->man.isDead == 1);

    reset_runner_test(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -5.0f; // < 0

    game_events_begin(game);
    detect_runner_fixed_hazards(game);
    game_events_apply(game);
    CHECK("runner fall hazard: x past the left edge triggers death", game->man.isDead == 1);

    reset_runner_test(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.y = 720.0f;

    game_events_begin(game);
    detect_runner_fixed_hazards(game);
    game_events_apply(game);
    CHECK("runner fall hazard: secondPlayer past the bottom edge triggers the shared death flag",
          game->man.isDead == 1);

    // ------------------------------------------------------------------
    // 5. GAME_EVENT_RUNNER_GAME_OVER_CHECK: score-persist plus the
    //    game-over transition, and the double-transition guard.
    // ------------------------------------------------------------------
    reset_runner_test(game, GAME_MODE_SINGLE_PLAYER);
    game->gameLives = 0;
    game->x_score = 42;
    game->app.scene = APP_SCENE_RUNNER_GAME;

    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("runner transition: score persisted and scene transitions to the mode menu",
          game->x_list[0] == 42 && game->x_i == 1 && game->app.scene == APP_SCENE_RUNNER_MENU);

    game->app.scene = APP_SCENE_MAIN_MENU; // simulate an earlier handler already transitioned
    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
    game_events_apply(game);
    CHECK("runner transition: guard does not overwrite an earlier transition",
          game->app.scene == APP_SCENE_MAIN_MENU);

    // ------------------------------------------------------------------
    // 6. detect_projectile_hits(): detection remains pure and the dedicated
    //    consequence phase performs the kill/score exactly once.
    // ------------------------------------------------------------------
    reset_arcade_test(game, GAME_MODE_SINGLE_PLAYER);
    game->kills_score = 0;
    game->enemyValues[0].x = 40.0f;
    game->enemyValues[0].y = 100.0f;
    game->enemyValues[0].visible = 1;
    game->bullets[0].active = true;
    game->bullets[0].prevX = 30.0f;
    game->bullets[0].x = 50.0f;
    game->bullets[0].y = 110.0f;
    game->perfLoggingEnabled = true;

    game_events_begin(game);
    detect_projectile_hits(game);
    CHECK("projectile detection: queues a hit without mutating gameplay state",
          game->events.count == 1 && game->enemyValues[0].visible == 1 && game->bullets[0].active);
    CHECK("projectile telemetry: one active projectile performs 113 target checks, not 113,000 slot checks",
          game->perfProjectileActiveSamples == 1 && game->perfProjectileTargetChecks == 113);
    game_events_apply(game);
    CHECK("projectile consequences: queued hit kills the enemy and scores",
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

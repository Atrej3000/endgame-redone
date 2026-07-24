// Standalone Runner death-lifecycle test -- NOT part of the shipped game (main.c is excluded
// when building this, same pattern as the other docs/verification/*.c harnesses). Exercises the
// real runner_trigger_death/runner_death_step/runner_frame/process2/collisionDetect2/
// processEvents2/doRender2/runner_session_reset functions directly. See
// docs/runner-death-lifecycle.md for the lifecycle this test guards against regressing.
//
// Test-only exception to the "game->app.scene is written only inside app_change_scene()" invariant
// (see docs/scene-state-map.md): several cases below directly assign game->app.scene/game->gameLives
// to set up a precondition. Production code must never do this -- only this test file does, and
// only for setup.
#include "app.h"
#include "scene.h"
#include "frame.h"

static int failures = 0;

#define CHECK(desc, cond)                                 \
    do                                                     \
    {                                                      \
        if (cond)                                          \
        {                                                  \
            printf("DEATH TEST: PASS - %s\n", desc);       \
        }                                                  \
        else                                               \
        {                                                  \
            printf("DEATH TEST: FAIL - %s\n", desc);       \
            failures++;                                    \
        }                                                  \
    } while (0)

// Drives runner_death_step() until it returns to idle (or a safety cap is hit), counting how
// many times game->gameLives actually changed. Returns the number of decrements observed.
static int drive_death_to_idle(GameState *game)
{
    int decrements = 0;
    int prevLives = game->gameLives;
    for (int i = 0; i < 500 && game->man.isDead; i++)
    {
        runner_death_step(game);
        if (game->gameLives != prevLives)
        {
            decrements++;
            prevLives = game->gameLives;
        }
    }
    return decrements;
}

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("DEATH TEST: app_init failed, cannot run\n");
        return 1;
    }

    CHECK("runner_assets_load() succeeds", runner_assets_load(game) == true);

    // -------------------------------------------------------------------
    // 1. Single death: exactly one decrement, one respawn, returns to idle
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200; // safe position, away from x<0/y>=719
    int gameLivesBefore1 = game->gameLives;

    runner_trigger_death(game);
    CHECK("trigger: isDead becomes true", game->man.isDead == 1);
    CHECK("trigger: deathCountdown set to 120", game->deathCountdown == 120);

    int decrements1 = drive_death_to_idle(game);
    CHECK("single death: exactly one decrement", decrements1 == 1);
    CHECK("single death: gameLives decreased by exactly 1", game->gameLives == gameLivesBefore1 - 1);
    CHECK("single death: isDead returns to idle", game->man.isDead == 0);
    CHECK("single death: deathCountdown returns to idle sentinel", game->deathCountdown == -1);
    CHECK("single death: man.y reset to 0 (respawn)", game->man.y == 0);

    // -------------------------------------------------------------------
    // 2. Repeated trigger while active: no extra decrement, no countdown restart
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200;

    runner_trigger_death(game);
    for (int i = 0; i < 10; i++)
    {
        runner_death_step(game);
    }
    int countdownAfter10 = game->deathCountdown;
    CHECK("repeated trigger: countdown advanced partway", countdownAfter10 == 110);

    runner_trigger_death(game); // called again while already dying
    CHECK("repeated trigger: still dying (idempotent)", game->man.isDead == 1);
    CHECK("repeated trigger: countdown NOT restarted to 120", game->deathCountdown == countdownAfter10);

    int decrements2 = drive_death_to_idle(game);
    CHECK("repeated trigger: still exactly one decrement overall", decrements2 == 1);

    // -------------------------------------------------------------------
    // 3. Game over: last life, correct transition, no respawn, no repeat
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200;
    game->gameLives = 1; // test-only precondition: last life
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition
    game->x_score = 55;
    game->x_i = 0;

    runner_trigger_death(game);
    // Drive the real fixed-tick pipeline (not just runner_death_step) so the game-over
    // transition event in runner_simulate() is exercised too -- stop as soon as the scene actually
    // changes, mirroring what main.c's dispatch would do (it would stop calling runner_frame()
    // once the scene is no longer APP_SCENE_RUNNER_GAME).
    for (int i = 0; i < 200 && game->app.scene == APP_SCENE_RUNNER_GAME; i++)
    {
        runner_frame(game, window, renderer, PHYSICS_DT);
    }

    CHECK("game over: gameLives reached exactly 0", game->gameLives == 0);
    CHECK("game over: correct scene transition", game->app.scene == APP_SCENE_RUNNER_MENU);
    CHECK("game over: no respawn before the transition", game->man.isDead == 1);
    CHECK("game over: score persisted before transition", game->x_list[0] == 55 && game->x_i == 1);

    // Calling processEvents2() once more must not re-fire the transition (Phase 5 guard).
    processEvents2(window, game);
    CHECK("game over: no repeated transition", game->app.scene == APP_SCENE_RUNNER_MENU);

    // -------------------------------------------------------------------
    // 4. Pause freezes death progression; resume continues it correctly
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200;
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition

    runner_trigger_death(game);
    for (int i = 0; i < 20; i++)
    {
        runner_death_step(game);
    }
    int countdownBeforePause = game->deathCountdown;

    game->app.scene = APP_SCENE_RUNNER_PAUSE; // test-only precondition
    for (int i = 0; i < 5; i++)
    {
        doRender_pause(renderer, game);
        pause_events(game);
    }
    CHECK("pause: death countdown unchanged while paused", game->deathCountdown == countdownBeforePause);
    CHECK("pause: isDead unchanged while paused", game->man.isDead == 1);

    app_change_scene(game, APP_SCENE_RUNNER_GAME); // resume
    runner_death_step(game);
    CHECK("resume: countdown continues from where it left off (not reset)",
          game->deathCountdown == countdownBeforePause - 1);

    int decrements4 = drive_death_to_idle(game);
    CHECK("resume: death still resolves to exactly one decrement overall", decrements4 == 1);

    // -------------------------------------------------------------------
    // 5. Session reset clears all death state
    // -------------------------------------------------------------------
    game->man.isDead = 1;
    game->deathCountdown = 50;
    game->man.x = -10;
    game->man.y = 999;
    game->gameLives = 0;

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);

    CHECK("session reset: isDead cleared", game->man.isDead == 0);
    CHECK("session reset: deathCountdown back to idle sentinel", game->deathCountdown == -1);
    CHECK("session reset: gameLives back to 3", game->gameLives == 3);

    // -------------------------------------------------------------------
    // 6. Scene departure: leaving Runner gameplay stops further mutation
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200;
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition

    runner_trigger_death(game);
    for (int i = 0; i < 15; i++)
    {
        runner_death_step(game);
    }
    int countdownBeforeDeparture = game->deathCountdown;
    int livesBeforeDeparture = game->gameLives;

    app_change_scene(game, APP_SCENE_MAIN_MENU); // simulates leaving Runner gameplay (e.g. 'q')
    // Process a frame using the actual functions main.c would call for APP_SCENE_MAIN_MENU --
    // not runner_frame()/runner_death_step(), which production code no longer invokes once the
    // scene has changed.
    menu0_events(game);
    doRender_menu0(renderer, game);

    CHECK("scene departure: death countdown unchanged", game->deathCountdown == countdownBeforeDeparture);
    CHECK("scene departure: isDead unchanged", game->man.isDead == 1);
    CHECK("scene departure: gameLives unchanged", game->gameLives == livesBeforeDeparture);

    // -------------------------------------------------------------------
    // 7. Render purity while dying (doRender2 must not mutate anything, including the new fields)
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = 100;
    game->man.y = 200;
    runner_trigger_death(game);
    for (int i = 0; i < 30; i++)
    {
        runner_death_step(game);
    }

    float manX7 = game->man.x, manY7 = game->man.y;
    int isDead7 = game->man.isDead;
    int countdown7 = game->deathCountdown;
    int gameLives7 = game->gameLives;
    AppScene scene7 = game->app.scene;

    doRender2(renderer, game);

    CHECK("render purity: man.x unchanged", game->man.x == manX7);
    CHECK("render purity: man.y unchanged", game->man.y == manY7);
    CHECK("render purity: isDead unchanged", game->man.isDead == isDead7);
    CHECK("render purity: deathCountdown unchanged", game->deathCountdown == countdown7);
    CHECK("render purity: gameLives unchanged", game->gameLives == gameLives7);
    CHECK("render purity: scene unchanged", game->app.scene == scene7);

    // -------------------------------------------------------------------
    // 8a. Left-edge fall respawn: x is clamped, not left negative (no re-trigger loop)
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.x = -50; // the unsafe condition that itself triggers death
    game->man.y = 200;
    int gameLivesBefore8a = game->gameLives;

    runner_trigger_death(game);
    int decrements8a = drive_death_to_idle(game);

    CHECK("left-edge respawn: exactly one decrement", decrements8a == 1);
    CHECK("left-edge respawn: gameLives decreased by exactly 1", game->gameLives == gameLivesBefore8a - 1);
    CHECK("left-edge respawn: man.x clamped to 0, not left negative", game->man.x == 0);

    // -------------------------------------------------------------------
    // 8b. Multiplayer: secondPlayer frozen during countdown and reset at respawn
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->man.x = 100;
    game->man.y = 200;
    game->secondPlayer.x = -30;
    game->secondPlayer.y = 500;
    game->secondPlayer.dy = 5.0f;
    int secondPlayerXBefore = (int)game->secondPlayer.x;

    runner_trigger_death(game);
    // Mid-countdown: process2() must not move secondPlayer (frozen).
    for (int i = 0; i < 10; i++)
    {
        process2(game, PHYSICS_DT);
    }
    CHECK("multiplayer: secondPlayer.x frozen during countdown", (int)game->secondPlayer.x == secondPlayerXBefore);
    CHECK("multiplayer: secondPlayer.y frozen during countdown", (int)game->secondPlayer.y == 500);

    int decrements8b = drive_death_to_idle(game);
    CHECK("multiplayer: exactly one decrement", decrements8b == 1);
    CHECK("multiplayer: secondPlayer.x clamped to 0, not left negative", game->secondPlayer.x == 0);
    CHECK("multiplayer: secondPlayer.y reset to 0", game->secondPlayer.y == 0);
    CHECK("multiplayer: secondPlayer.dy reset to 0", game->secondPlayer.dy == 0);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("DEATH TEST: ALL PASS\n");
        return 0;
    }
    printf("DEATH TEST: %d FAILURE(S)\n", failures);
    return 1;
}

// Standalone frame-pipeline test -- NOT part of the shipped game (main.c is
// excluded when building this, same pattern as the other docs/verification/
// *.c harnesses). Exercises the real doRender/doRender2/process/process2/
// processEvents/processEvents2/arcade_frame/runner_frame/pause functions
// directly, without needing keyboard/window input injection. See
// docs/frame-pipeline-map.md for the findings this test guards against
// regressing.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant (see docs/scene-state-map.md): a few cases
// below directly assign game->app.scene/game->statusState to set up a
// precondition. Production code must never do this -- only this test file
// does, and only for setup.
#include "app.h"
#include "scene.h"
#include "frame.h"
#include "game_events.h"

static int failures = 0;

#define CHECK(desc, cond)                                    \
    do                                                        \
    {                                                         \
        if (cond)                                             \
        {                                                     \
            printf("FRAME TEST: PASS - %s\n", desc);          \
        }                                                     \
        else                                                  \
        {                                                     \
            printf("FRAME TEST: FAIL - %s\n", desc);          \
            failures++;                                       \
        }                                                     \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("FRAME TEST: app_init failed, cannot run\n");
        return 1;
    }

    CHECK("arcade_assets_load() succeeds", arcade_assets_load(game) == true);
    CHECK("runner_assets_load() succeeds", runner_assets_load(game) == true);

    // -------------------------------------------------------------------
    // 1. Render purity -- doRender() (Arcade), already pure before Phase 5
    // -------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME; // test-only precondition

    float manX1 = game->man.x, manY1 = game->man.y;
    int manLives1 = game->man.lives, gameLives1 = game->gameLives;
    int tempScore1 = game->tempScore, killsScore1 = game->kills_score;
    float enemy0x1 = game->enemyValues[0].x;
    AppScene scene1 = game->app.scene;
    int time1 = game->time;

    doRender(renderer, game);

    CHECK("doRender(): man.x unchanged", game->man.x == manX1);
    CHECK("doRender(): man.y unchanged", game->man.y == manY1);
    CHECK("doRender(): man.lives unchanged", game->man.lives == manLives1);
    CHECK("doRender(): gameLives unchanged", game->gameLives == gameLives1);
    CHECK("doRender(): tempScore unchanged", game->tempScore == tempScore1);
    CHECK("doRender(): kills_score unchanged", game->kills_score == killsScore1);
    CHECK("doRender(): enemyValues[0].x unchanged", game->enemyValues[0].x == enemy0x1);
    CHECK("doRender(): scene unchanged", game->app.scene == scene1);
    CHECK("doRender(): time unchanged", game->time == time1);

    // -------------------------------------------------------------------
    // 2. Render purity -- doRender2() (Runner), confirms the Phase 5 fix:
    // even with man.isDead forced true, doRender2() alone must no longer
    // mutate gameLives/isDead/man.y (that mutation moved to
    // runner_resolve_death(), called separately by runner_frame()).
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME; // test-only precondition
    game->man.isDead = 1;                  // force the previously-mutating branch

    float manX2 = game->man.x, manY2 = game->man.y;
    int gameLives2 = game->gameLives;
    int manIsDead2 = game->man.isDead;
    int xScore2 = game->x_score;
    AppScene scene2 = game->app.scene;

    doRender2(renderer, game);

    CHECK("doRender2(): man.x unchanged", game->man.x == manX2);
    CHECK("doRender2(): man.y unchanged (mutation no longer runs here)", game->man.y == manY2);
    CHECK("doRender2(): gameLives unchanged (mutation no longer runs here)", game->gameLives == gameLives2);
    CHECK("doRender2(): man.isDead unchanged (mutation no longer runs here)", game->man.isDead == manIsDead2);
    CHECK("doRender2(): x_score unchanged", game->x_score == xScore2);
    CHECK("doRender2(): scene unchanged", game->app.scene == scene2);

    // -------------------------------------------------------------------
    // 3. Death-mutation relocation -- runner_frame() end to end still
    // applies the relocated mutation, just after doRender2() instead of
    // inside it.
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition
    game->statusState = STATUS_STATE_GAME;
    game->man.isDead = 1;
    game->man.x = 100; // keep away from the x<0/y>=719 fall-death checks
    game->man.y = 200;
    // runner_session_reset() sets deathCountdown=-1, which combined with
    // isDead=1 would also trigger process2()'s own, separate, already-broken
    // deathCountdown mechanism (documented in frame-pipeline-map.md, not
    // fixed this phase). Neutralize it here so this test isolates just the
    // relocated runner_resolve_death() mutation being verified.
    game->deathCountdown = 0;
    int gameLivesBefore3 = game->gameLives;

    runner_frame(game, window, renderer, PHYSICS_DT);

    CHECK("runner_frame(): gameLives decremented once by the relocated mutation",
          game->gameLives == gameLivesBefore3 - 1);
    CHECK("runner_frame(): man.isDead cleared by the relocated mutation", game->man.isDead == 0);
    CHECK("runner_frame(): man.y reset to 0 by the relocated mutation", game->man.y == 0);

    // -------------------------------------------------------------------
    // 4. Double scene-transition guard -- Runner
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_RUNNER_GAME; // test-only precondition
    game->gameLives = 0;
    game->x_score = 42;
    game->x_i = 0;

    processEvents2(window, game);

    CHECK("processEvents2(): game-over transition fires when scene is still RUNNER_GAME",
          game->app.scene == APP_SCENE_RUNNER_MENU);
    CHECK("processEvents2(): score persisted to x_list", game->x_list[0] == 42);
    CHECK("processEvents2(): x_i incremented", game->x_i == 1);

    // Simulate a transition having already happened earlier in the same
    // call (e.g. SDLK_q) -- the guard must not overwrite it, but the score
    // must still be saved.
    game->app.scene = APP_SCENE_MAIN_MENU; // test-only precondition
    game->gameLives = 0;
    game->x_score = 77;
    game->x_i = 5;

    processEvents2(window, game);

    CHECK("processEvents2(): guard does not overwrite an already-changed scene",
          game->app.scene == APP_SCENE_MAIN_MENU);
    CHECK("processEvents2(): score still persisted even when the guard skips the transition",
          game->x_list[5] == 77);
    CHECK("processEvents2(): x_i still incremented even when the guard skips the transition",
          game->x_i == 6);

    // -------------------------------------------------------------------
    // 5. Double scene-transition guard -- Arcade
    // -------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_ARCADE_GAME; // test-only precondition
    game->man.lives = 0;

    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);

    CHECK("arcade simulation consequence: game-over transition fires when scene is still ARCADE_GAME",
          game->app.scene == APP_SCENE_ARCADE_MENU);

    game->app.scene = APP_SCENE_MAIN_MENU; // test-only precondition, simulating an earlier q press
    game->man.lives = 0;

    game_events_begin(game);
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
    game_events_apply(game);

    CHECK("arcade simulation consequence: guard does not overwrite an already-changed scene",
          game->app.scene == APP_SCENE_MAIN_MENU);

    // -------------------------------------------------------------------
    // 6. Animation wrap/idle -- process() (Arcade man.animFrame)
    // -------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1;
    game->man.dx = 3.0f;
    game->man.slowingDown = 0;
    game->man.animFrame = 0;
    game->time = 2; // process() increments to 3 first -> 3%3==0

    process(game, PHYSICS_DT);

    CHECK("process(): animFrame advances when moving, on-ledge, time%3==0", game->man.animFrame == 1);

    game->man.animFrame = 11;
    game->time = 5; // increments to 6 -> 6%3==0

    process(game, PHYSICS_DT);

    CHECK("process(): animFrame wraps from 11 back to 0", game->man.animFrame == 0);

    game->man.dx = 0; // idle
    game->man.animFrame = 5;
    game->time = 8; // increments to 9 -> 9%3==0

    process(game, PHYSICS_DT);

    CHECK("process(): animFrame does not advance when idle (dx==0)", game->man.animFrame == 5);

    game->man.dx = 3.0f;
    game->man.onLedge = 0; // off-ledge
    game->man.animFrame = 5;
    game->time = 11; // increments to 12 -> 12%3==0

    process(game, PHYSICS_DT);

    CHECK("process(): animFrame does not advance when off-ledge", game->man.animFrame == 5);

    // -------------------------------------------------------------------
    // 7. Animation wrap/idle -- process2() (Runner secondPlayer.animFrameSecond)
    // -------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.onLedge = 1;
    game->secondPlayer.dx = 3.0f;
    game->secondPlayer.slowingDown = 0;
    game->secondPlayer.animFrameSecond = 0;
    game->time = 2; // increments to 3 -> 3%3==0

    process2(game, PHYSICS_DT);

    CHECK("process2(): secondPlayer.animFrameSecond advances when moving, on-ledge, time%3==0",
          game->secondPlayer.animFrameSecond == 1);

    game->secondPlayer.animFrameSecond = 11;
    game->time = 5; // increments to 6 -> 6%3==0

    process2(game, PHYSICS_DT);

    CHECK("process2(): secondPlayer.animFrameSecond wraps from 11 back to 0",
          game->secondPlayer.animFrameSecond == 0);

    game->secondPlayer.dx = 0; // idle
    game->secondPlayer.animFrameSecond = 5;
    game->time = 8; // increments to 9 -> 9%3==0

    process2(game, PHYSICS_DT);

    CHECK("process2(): secondPlayer.animFrameSecond does not advance when idle (dx==0)",
          game->secondPlayer.animFrameSecond == 5);

    // -------------------------------------------------------------------
    // 8. Pause purity -- repeated pause-scene calls never mutate gameplay
    // -------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_ARCADE_PAUSE; // test-only precondition
    game->tempScore = 777;                // sentinel
    game->man.x = 321;                    // sentinel

    for (int i = 0; i < 5; i++)
    {
        doRender_pause(renderer, game);
        pause_events(game);
    }

    CHECK("pause: repeated calls never mutate tempScore", game->tempScore == 777);
    CHECK("pause: repeated calls never mutate man.x", game->man.x == 321);
    CHECK("pause: repeated calls never leave pause on their own", game->app.scene == APP_SCENE_ARCADE_PAUSE);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("FRAME TEST: ALL PASS\n");
        return 0;
    }
    printf("FRAME TEST: %d FAILURE(S)\n", failures);
    return 1;
}

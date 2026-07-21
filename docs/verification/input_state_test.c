// Standalone InputState / edge-triggered jump-request test -- NOT part of the
// shipped game (main.c is excluded when building this, same pattern as the
// other docs/verification/*.c harnesses). Exercises
// consume_arcade_jump_requests()/consume_runner_jump_requests() directly with
// hand-set GameState fields, without needing keyboard/window input injection
// or a real event loop. See docs/input-simulation-separation-map.md.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: not needed here -- this test never touches
// game->app.scene.
#include "app.h"
#include "frame.h"

static int failures = 0;

#define CHECK(desc, cond)                                            \
    do                                                                 \
    {                                                                   \
        if (cond)                                                        \
        {                                                                 \
            printf("INPUT STATE TEST: PASS - %s\n", desc);                 \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("INPUT STATE TEST: FAIL - %s\n", desc);                    \
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
        printf("INPUT STATE TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Edge-triggered single consumption (Arcade, player 1): a grounded
    //    request fires the jump exactly once and clears itself -- a second
    //    consume call afterward (simulating a second physics tick in the
    //    same real frame) does not re-fire even if re-grounded, because the
    //    request flag itself (not just onLedge) gates the jump.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1;
    game->man.dy = 0;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game);
    CHECK("arcade p1: grounded request fires the jump impulse",
          game->man.dy == -JUMP_SPEED_PER_SEC);
    CHECK("arcade p1: onLedge cleared by the jump", game->man.onLedge == 0);
    CHECK("arcade p1: request buffer cleared after consumption",
          game->input.jumpBufferTicksPlayer1 == 0);

    game->man.onLedge = 1;   // re-ground, simulating an instant re-landing
    game->man.dy = 123.0f;   // sentinel -- would be overwritten if a jump re-fired
    consume_arcade_jump_requests(game);
    CHECK("arcade p1: no request left -> a second consume call does not re-jump",
          game->man.dy == 123.0f);

    // ------------------------------------------------------------------
    // 2. Jump buffering + coyote time (Phase 15, see docs/game-feel-map.md) --
    //    deliberately supersedes this test's own prior claim that an airborne
    //    request was dropped immediately. It is not: with no coyote window
    //    available, the request now persists (buffered) across consume calls,
    //    firing the instant the player lands within JUMP_BUFFER_TICKS, and
    //    only fading away with no jump if the window expires first.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = 0; // no coyote grace available
    game->man.dy = 50.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game);
    CHECK("arcade p1: airborne request with no coyote available does not fire the jump",
          game->man.dy == 50.0f);
    CHECK("arcade p1: airborne request persists (buffered), not cleared to 0",
          game->input.jumpBufferTicksPlayer1 == JUMP_BUFFER_TICKS - 1);

    // Keep missing the window: after JUMP_BUFFER_TICKS total consume calls
    // with no landing, the buffered request must fade to 0 with no jump ever
    // firing.
    for (int i = 0; i < JUMP_BUFFER_TICKS - 1; i++)
    {
        consume_arcade_jump_requests(game);
    }
    CHECK("arcade p1: buffered request expires after JUMP_BUFFER_TICKS with no landing",
          game->input.jumpBufferTicksPlayer1 == 0);
    CHECK("arcade p1: an expired buffered request never fired the jump",
          game->man.dy == 50.0f);

    // A buffered request that lands within the window fires immediately.
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = 0;
    game->man.dy = 50.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game); // still airborne -- buffered, not fired
    CHECK("arcade p1: still airborne after one tick -- jump has not fired yet",
          game->man.dy == 50.0f);

    game->man.onLedge = 1; // lands within the buffer window
    consume_arcade_jump_requests(game);
    CHECK("arcade p1: landing within the buffer window fires the buffered jump",
          game->man.dy == -JUMP_SPEED_PER_SEC);
    CHECK("arcade p1: firing clears the buffer", game->input.jumpBufferTicksPlayer1 == 0);

    // ------------------------------------------------------------------
    // 2b. Coyote time: a jump requested just after leaving a ledge (no
    //     ceiling hit, no re-landing) still succeeds while coyoteTicksRemaining
    //     is > 0; it fails once the grace window has fully expired.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = COYOTE_TICKS; // just walked off a ledge
    game->man.dy = 50.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game);
    CHECK("arcade p1: a request within the coyote window fires the jump even though airborne",
          game->man.dy == -JUMP_SPEED_PER_SEC);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = 0; // coyote window fully expired
    game->man.dy = 50.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game);
    CHECK("arcade p1: a request after the coyote window has expired does not fire",
          game->man.dy == 50.0f);

    // ------------------------------------------------------------------
    // 3. Player 2 path (Arcade) -- same edge-trigger/gating behavior.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.onLedge = 1;
    game->secondPlayer.dy = 0;
    game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;

    consume_arcade_jump_requests(game);
    CHECK("arcade p2: grounded request fires the jump impulse",
          game->secondPlayer.dy == -JUMP_SPEED_PER_SEC);
    CHECK("arcade p2: request flag cleared after consumption",
          game->input.jumpBufferTicksPlayer2 == 0);

    // ------------------------------------------------------------------
    // 4. Runner regression-fix proof: consume_runner_jump_requests() must
    //    use JUMP_SPEED_PER_SEC, not the old, never-converted bare -10 --
    //    see docs/input-simulation-separation-map.md section 2.
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1;
    game->man.dy = 0;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_runner_jump_requests(game);
    CHECK("runner p1: jump impulse uses JUMP_SPEED_PER_SEC, not a bare -10",
          game->man.dy == -JUMP_SPEED_PER_SEC);
    CHECK("runner p1: request flag cleared after consumption",
          game->input.jumpBufferTicksPlayer1 == 0);

    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.onLedge = 1;
    game->secondPlayer.dy = 0;
    game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;

    consume_runner_jump_requests(game);
    CHECK("runner p2: jump impulse uses JUMP_SPEED_PER_SEC, not a bare -10",
          game->secondPlayer.dy == -JUMP_SPEED_PER_SEC);
    CHECK("runner p2: request flag cleared after consumption",
          game->input.jumpBufferTicksPlayer2 == 0);

    // ------------------------------------------------------------------
    // 5. Full-pipeline integration: one real arcade_simulate()/
    //    runner_simulate() call confirms the consumption wires correctly
    //    into the existing fixed-tick order (consume -> apply forces ->
    //    process/process2, which adds one tick's gravity on top).
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME;
    game->man.isDead = 0;
    game->man.onLedge = 1;
    game->man.dx = 0;
    game->man.dy = 0;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    arcade_simulate(game, PHYSICS_DT);
    CHECK("arcade: one simulate() tick applies the jump impulse plus one tick's gravity",
          fabsf(game->man.dy - (-JUMP_SPEED_PER_SEC + GRAVITY_PER_SEC2 * PHYSICS_DT)) < 0.0001f);

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.isDead = 0;
    game->man.onLedge = 1;
    game->man.dx = 0;
    game->man.dy = 0;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    runner_simulate(game, PHYSICS_DT);
    CHECK("runner: one simulate() tick applies the jump impulse plus one tick's gravity",
          fabsf(game->man.dy - (-JUMP_SPEED_PER_SEC + GRAVITY_PER_SEC2 * PHYSICS_DT)) < 0.0001f);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("INPUT STATE TEST: ALL PASS\n");
        return 0;
    }
    printf("INPUT STATE TEST: %d FAILURE(S)\n", failures);
    return 1;
}

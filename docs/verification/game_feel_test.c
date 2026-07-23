// Standalone game-feel test (coyote time, jump buffering, variable jump
// height) -- NOT part of the shipped game (main.c is excluded when building
// this, same pattern as the other docs/verification/*.c harnesses). Calls
// consume_arcade_jump_requests()/consume_runner_jump_requests()/
// apply_arcade_player_forces()/apply_runner_player_forces() directly with
// hand-set GameState fields. See docs/game-feel-map.md.
//
// apply_*_player_forces() read game->input's continuous fields (Phase 17,
// see docs/input-snapshot-architecture-map.md) instead of calling
// SDL_GetKeyboardState() themselves. arcade_session_reset()/
// runner_session_reset() explicitly zero those fields, so every call below
// sees jumpHeldPlayer1/2 == false ("no key held") deterministically --
// exploited here to exercise the variable-jump-height release-cut: with the
// held state always false, setting Man.jumpKeyHeldLastTick = true before a
// call always represents "held last tick, released this tick" -- the exact
// release edge the cut logic looks for.
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
            printf("GAME FEEL TEST: PASS - %s\n", desc);                   \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("GAME FEEL TEST: FAIL - %s\n", desc);                      \
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
        printf("GAME FEEL TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Coyote time: the counter decays naturally over COYOTE_TICKS real
    //    consume calls after leaving a ledge (not just a hand-set value) --
    //    a jump requested one tick before it fully expires still succeeds;
    //    one tick after, it doesn't.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0; // just walked off a ledge; coyoteTicksRemaining starts at COYOTE_TICKS
    game->man.coyoteTicksRemaining = COYOTE_TICKS;
    game->man.dy = 50.0f;

    // Let the window decay to exactly 1 tick remaining.
    for (int i = 0; i < COYOTE_TICKS - 1; i++)
    {
        consume_arcade_jump_requests(game);
    }
    CHECK("coyote: window has decayed to exactly 1 tick remaining",
          game->man.coyoteTicksRemaining == 1);

    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    consume_arcade_jump_requests(game);
    CHECK("coyote: a jump requested with 1 coyote tick left still fires",
          game->man.dy == -JUMP_SPEED_PER_SEC);

    // Now let the window fully expire before requesting.
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = COYOTE_TICKS;
    game->man.dy = 50.0f;

    for (int i = 0; i < COYOTE_TICKS; i++)
    {
        consume_arcade_jump_requests(game);
    }
    CHECK("coyote: window has fully decayed to 0", game->man.coyoteTicksRemaining == 0);

    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    consume_arcade_jump_requests(game);
    CHECK("coyote: a jump requested after the window fully expires does not fire",
          game->man.dy == 50.0f);

    // ------------------------------------------------------------------
    // 1b. Double jump: a grounded jump grants exactly one airborne jump;
    // a third press cannot reset vertical velocity until landing again.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    consume_arcade_jump_requests(game);
    CHECK("double jump: first grounded jump grants one airborne jump",
          game->man.dy == -JUMP_SPEED_PER_SEC && game->man.airJumpsRemaining == 1);

    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = 0;
    game->man.dy = -200.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    consume_arcade_jump_requests(game);
    CHECK("double jump: second press in the air applies a full second impulse and animation",
          game->man.dy == -JUMP_SPEED_PER_SEC && game->man.airJumpsRemaining == 0 &&
          game->man.doubleJumpAnimationTicks > 0);

    game->man.dy = -200.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
    consume_arcade_jump_requests(game);
    CHECK("double jump: a third airborne press cannot add another jump",
          game->man.dy == -200.0f);

    // ------------------------------------------------------------------
    // 2. Jump buffering, Runner side (input_state_test.c already covers this
    //    thoroughly for Arcade -- this proves consume_runner_jump_requests()
    //    has the identical behavior, not just the identical impulse
    //    constant proven there).
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 0;
    game->man.coyoteTicksRemaining = 0;
    game->man.dy = 50.0f;
    game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;

    consume_runner_jump_requests(game);
    CHECK("runner buffering: airborne request with no coyote persists, does not fire",
          game->man.dy == 50.0f && game->input.jumpBufferTicksPlayer1 == JUMP_BUFFER_TICKS - 1);

    game->man.onLedge = 1; // lands within the window
    consume_runner_jump_requests(game);
    CHECK("runner buffering: landing within the window fires the buffered jump",
          game->man.dy == -JUMP_SPEED_PER_SEC);

    // ------------------------------------------------------------------
    // 3. Variable jump height (release-cut): releasing the jump key while
    //    still rising fast cuts dy to -JUMP_CUT_SPEED_PER_SEC.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.dy = -500.0f; // rising well faster than the cut speed
    game->man.jumpKeyHeldLastTick = true; // held last tick -> this call sees the release edge

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("variable height: releasing while rising fast cuts dy to -JUMP_CUT_SPEED_PER_SEC",
          game->man.dy == -JUMP_CUT_SPEED_PER_SEC);
    CHECK("variable height: jumpKeyHeldLastTick updates to reflect this tick's (unheld) state",
          game->man.jumpKeyHeldLastTick == false);

    // ------------------------------------------------------------------
    // 4. Variable jump height: releasing after dy has already decayed past
    //    the cut threshold (near apex or falling) leaves dy unaffected --
    //    a "full" jump was already achieved, nothing left to cut short.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.dy = -100.0f; // already slower (less negative) than the cut speed
    game->man.jumpKeyHeldLastTick = true;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("variable height: releasing after dy already decayed past the cut threshold "
          "leaves dy unaffected",
          game->man.dy == -100.0f);

    // ------------------------------------------------------------------
    // 5. No release edge (was not held last tick) -> no cut, regardless of
    //    dy -- the cut only ever fires on a genuine held-to-released
    //    transition, not merely "not held this tick".
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.dy = -500.0f;
    game->man.jumpKeyHeldLastTick = false;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("variable height: no held-to-released edge -> dy is untouched",
          game->man.dy == -500.0f);

    // ------------------------------------------------------------------
    // 6. The removed grounded-hold-thrust quirk is gone: with no release
    //    edge pending, apply_arcade_player_forces()/apply_runner_player_forces()
    //    never modify dy at all -- there is no unconditional per-tick
    //    upward force left, grounded or airborne.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1; // grounded
    game->man.dy = 0.0f;
    game->man.jumpKeyHeldLastTick = false;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("no grounded thrust (Arcade): dy stays exactly 0 with no release edge pending",
          game->man.dy == 0.0f);

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.onLedge = 1;
    game->man.dy = 0.0f;
    game->man.jumpKeyHeldLastTick = false;

    apply_runner_player_forces(game, PHYSICS_DT);
    CHECK("no grounded thrust (Runner): dy stays exactly 0 with no release edge pending",
          game->man.dy == 0.0f);

    // ------------------------------------------------------------------
    // 7. Second player path, both modes -- same release-cut behavior.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.dy = -500.0f;
    game->secondPlayer.jumpKeyHeldLastTick = true;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("variable height (arcade p2): release while rising fast cuts dy",
          game->secondPlayer.dy == -JUMP_CUT_SPEED_PER_SEC);

    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->secondPlayer.dy = -500.0f;
    game->secondPlayer.jumpKeyHeldLastTick = true;

    apply_runner_player_forces(game, PHYSICS_DT);
    CHECK("variable height (runner p2): release while rising fast cuts dy",
          game->secondPlayer.dy == -JUMP_CUT_SPEED_PER_SEC);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("GAME FEEL TEST: ALL PASS\n");
        return 0;
    }
    printf("GAME FEEL TEST: %d FAILURE(S)\n", failures);
    return 1;
}

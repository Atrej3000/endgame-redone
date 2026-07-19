// Standalone player-vs-ledge collision correctness test -- NOT part of the
// shipped game (main.c is excluded when building this, same pattern as the
// other docs/verification/*.c harnesses). Calls collisionDetect()/
// collisionDetect2() directly with a hand-crafted ledge and hand-set player
// state -- no file in docs/verification/*.c has ever called these functions
// directly before this. See docs/collision-correctness-map.md.
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
            printf("COLLISION CORRECTNESS TEST: PASS - %s\n", desc);       \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("COLLISION CORRECTNESS TEST: FAIL - %s\n", desc);          \
            failures++;                                                        \
        }                                                                       \
    } while (0)

// A single known ledge: spans x in [100, 300), y in [300, 320) -- a player
// with the standard 48x48 hitbox rests on top of it at y = 300 - 48 = 252.
// Every other ledge is pushed far out of range first -- arcade_session_reset/
// runner_session_reset populate all 100 ledges with the mode's default
// layout, and a stray default ledge could otherwise land/ceiling-bump
// against the test's own hand-placed player position.
static void set_test_ledge(GameState *game)
{
    for (int i = 0; i < 100; i++)
    {
        game->ledges[i].x = -100000;
        game->ledges[i].y = -100000;
        game->ledges[i].w = 1;
        game->ledges[i].h = 1;
    }
    game->ledges[0].x = 100;
    game->ledges[0].y = 300;
    game->ledges[0].w = 200;
    game->ledges[0].h = 20;
}

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("COLLISION CORRECTNESS TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Walking off a ledge clears onLedge -- the core bug fix. A player
    //    resting on the ledge (onLedge=1) who moves horizontally past its
    //    span, with no ceiling hit and no jump, must lose onLedge once
    //    collisionDetect() reassesses -- prior to this phase, nothing ever
    //    cleared it in this situation.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    set_test_ledge(game);
    game->man.x = 400.0f; // past the ledge's [100,300) span -- walked off
    game->man.y = 252.0f; // still at the resting height
    game->man.prevY = 252.0f;
    game->man.dy = 0.0f;
    game->man.onLedge = 1;

    collisionDetect(game);
    CHECK("arcade man: walking off a ledge (no ceiling hit, no jump) clears onLedge",
          game->man.onLedge == 0);

    // ------------------------------------------------------------------
    // 2. Resting continuously re-affirms onLedge=1 across multiple
    //    consecutive calls -- proves the crossing-based landing check
    //    handles the exact-rest case (my+mh == by), not just the instant of
    //    first contact.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    set_test_ledge(game);
    game->man.x = 150.0f; // within the ledge's [100,300) span
    game->man.y = 252.0f; // resting exactly on top (300 - 48)
    game->man.prevY = 252.0f;
    game->man.dy = 0.0f;
    game->man.onLedge = 0; // start ungrounded; the landing check must re-affirm it

    collisionDetect(game);
    CHECK("arcade man: resting on a ledge sets onLedge=1 (tick 1)", game->man.onLedge == 1);
    CHECK("arcade man: resting position is unchanged", game->man.y == 252.0f);

    game->man.prevY = game->man.y; // simulate the next tick's capture
    collisionDetect(game);
    CHECK("arcade man: resting on a ledge keeps onLedge=1 (tick 2, still at rest)",
          game->man.onLedge == 1);

    // ------------------------------------------------------------------
    // 3. Normal falling-and-landing still works -- regression-equivalence.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    set_test_ledge(game);
    game->man.x = 150.0f;
    game->man.prevY = 200.0f; // well above the ledge at the start of this tick
    game->man.y = 260.0f;     // fell past the landing threshold this tick
    game->man.dy = 50.0f;     // falling
    game->man.onLedge = 0;

    collisionDetect(game);
    CHECK("arcade man: falling onto a ledge still lands correctly", game->man.y == 252.0f);
    CHECK("arcade man: landing zeroes dy", game->man.dy == 0.0f);
    CHECK("arcade man: landing sets onLedge=1", game->man.onLedge == 1);

    // ------------------------------------------------------------------
    // 4. Runner ceiling bump now clears onLedge -- direct regression-fix
    //    proof for the documented bug (was incorrectly set to 1).
    // ------------------------------------------------------------------
    runner_session_reset(game, GAME_MODE_MULTIPLAYER);
    set_test_ledge(game);
    game->man.x = 150.0f;   // horizontal center (150+24=174) within [100,300)
    game->man.y = 301.0f;   // just below the ledge's underside (by=300, by+bh=320)
    game->man.dy = -50.0f;  // moving upward into the ceiling
    game->man.onLedge = 1;

    game->secondPlayer.x = 150.0f;
    game->secondPlayer.y = 301.0f;
    game->secondPlayer.dy = -50.0f;
    game->secondPlayer.onLedge = 1;

    collisionDetect2(game);
    CHECK("runner man: ceiling bump clears onLedge (was incorrectly set to 1)",
          game->man.onLedge == 0);
    CHECK("runner man: ceiling bump zeroes dy", game->man.dy == 0.0f);
    CHECK("runner secondPlayer: ceiling bump clears onLedge (was incorrectly set to 1)",
          game->secondPlayer.onLedge == 0);

    // ------------------------------------------------------------------
    // 5. capture_player_prev_y() sets prevY to the current y for both
    //    players.
    // ------------------------------------------------------------------
    game->man.y = 123.0f;
    game->secondPlayer.y = 456.0f;
    capture_player_prev_y(game);
    CHECK("capture_player_prev_y: man.prevY matches man.y", game->man.prevY == 123.0f);
    CHECK("capture_player_prev_y: secondPlayer.prevY matches secondPlayer.y",
          game->secondPlayer.prevY == 456.0f);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("COLLISION CORRECTNESS TEST: ALL PASS\n");
        return 0;
    }
    printf("COLLISION CORRECTNESS TEST: %d FAILURE(S)\n", failures);
    return 1;
}

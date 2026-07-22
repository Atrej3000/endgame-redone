// Standalone input snapshot test -- NOT part of the shipped game (main.c is
// excluded when building this, same pattern as the other
// docs/verification/*.c harnesses). See docs/input-snapshot-architecture-map.md.
//
// Exercises input_capture_arcade()/input_capture_runner() (src/input_snapshot.c)
// directly against a fabricated SDL_GetKeyboardState()-shaped array, and
// apply_arcade_player_forces()/apply_runner_player_forces() (src/process.c)
// against a hand-set InputState to confirm: (1) capture sets exactly the
// right fields; (2) a real frame producing several physics ticks applies
// held movement on every tick but the release-cut only once, since capture
// happens once per frame, not once per tick; (3) the pre-existing left-wins
// precedence on simultaneous left+right holds; (4) player 1/2 fields don't
// cross-mutate; (5) session reset clears every new field.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: not needed here -- this test never touches
// game->app.scene.
#include "app.h"
#include "input_snapshot.h"

static int failures = 0;

#define CHECK(desc, cond)                                            \
    do                                                                 \
    {                                                                   \
        if (cond)                                                        \
        {                                                                 \
            printf("INPUT SNAPSHOT TEST: PASS - %s\n", desc);              \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("INPUT SNAPSHOT TEST: FAIL - %s\n", desc);                  \
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
        printf("INPUT SNAPSHOT TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. input_capture_arcade(): sets exactly the right fields from a
    //    fabricated key array, both players, including shoot.
    // ------------------------------------------------------------------
    Uint8 keys[SDL_NUM_SCANCODES];
    memset(keys, 0, sizeof(keys));
    keys[SDL_SCANCODE_A] = 1;
    keys[SDL_SCANCODE_W] = 1;
    keys[SDL_SCANCODE_SPACE] = 1;
    keys[SDL_SCANCODE_RIGHT] = 1;
    keys[SDL_SCANCODE_KP_0] = 1;

    InputState captured;
    memset(&captured, 0, sizeof(captured));
    input_capture_arcade(&captured, keys, &game->app.settings);

    CHECK("arcade capture: moveLeftPlayer1 set from SDL_SCANCODE_A",
          captured.moveLeftPlayer1 == true);
    CHECK("arcade capture: moveRightPlayer1 not set (D not held)",
          captured.moveRightPlayer1 == false);
    CHECK("arcade capture: jumpHeldPlayer1 set from SDL_SCANCODE_W",
          captured.jumpHeldPlayer1 == true);
    CHECK("arcade capture: shootHeldPlayer1 set from SDL_SCANCODE_SPACE",
          captured.shootHeldPlayer1 == true);
    CHECK("arcade capture: moveRightPlayer2 set from SDL_SCANCODE_RIGHT",
          captured.moveRightPlayer2 == true);
    CHECK("arcade capture: moveLeftPlayer2 not set (LEFT not held)",
          captured.moveLeftPlayer2 == false);
    CHECK("arcade capture: jumpHeldPlayer2 not set (UP not held)",
          captured.jumpHeldPlayer2 == false);
    CHECK("arcade capture: shootHeldPlayer2 set from SDL_SCANCODE_KP_0",
          captured.shootHeldPlayer2 == true);

    // ------------------------------------------------------------------
    // 2. input_capture_runner(): sets the 6 movement/jump-held fields only
    //    -- Runner has no shoot mechanic, so it must leave the shoot fields
    //    untouched (not force them to false, not read them at all).
    // ------------------------------------------------------------------
    InputState runnerCaptured;
    memset(&runnerCaptured, 0, sizeof(runnerCaptured));
    runnerCaptured.shootHeldPlayer1 = true; // pre-set sentinel value
    runnerCaptured.shootHeldPlayer2 = true; // pre-set sentinel value
    input_capture_runner(&runnerCaptured, keys, &game->app.settings);

    CHECK("runner capture: moveLeftPlayer1 set from SDL_SCANCODE_A",
          runnerCaptured.moveLeftPlayer1 == true);
    CHECK("runner capture: jumpHeldPlayer1 set from SDL_SCANCODE_W",
          runnerCaptured.jumpHeldPlayer1 == true);
    CHECK("runner capture: moveRightPlayer2 set from SDL_SCANCODE_RIGHT",
          runnerCaptured.moveRightPlayer2 == true);
    CHECK("runner capture: shootHeldPlayer1 left untouched (no shoot mechanic)",
          runnerCaptured.shootHeldPlayer1 == true);
    CHECK("runner capture: shootHeldPlayer2 left untouched (no shoot mechanic)",
          runnerCaptured.shootHeldPlayer2 == true);

    // ------------------------------------------------------------------
    // 3. One frame, several physics ticks: held movement applies on every
    //    tick (accumulates), but the release-cut fires only once, since
    //    capture happens exactly once per real frame, not once per tick --
    //    calling apply_arcade_player_forces() 3 times with an identical,
    //    frame-frozen InputState simulates a 3-tick frame.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->input.moveRightPlayer1 = true;
    game->man.dx = 0.0f;

    apply_arcade_player_forces(game, PHYSICS_DT);
    float dxAfterOneTick = game->man.dx;
    apply_arcade_player_forces(game, PHYSICS_DT);
    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("multi-tick: held movement keeps applying on every tick of the frame",
          game->man.dx > dxAfterOneTick && dxAfterOneTick > 0.0f);

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->man.dy = -500.0f; // rising well faster than the cut speed
    game->man.jumpKeyHeldLastTick = true; // held last tick, released this frame
    game->input.jumpHeldPlayer1 = false; // one frame-frozen snapshot: not held

    apply_arcade_player_forces(game, PHYSICS_DT); // tick 1: the one genuine release edge
    apply_arcade_player_forces(game, PHYSICS_DT); // tick 2: same frozen input, no new edge
    apply_arcade_player_forces(game, PHYSICS_DT); // tick 3: same frozen input, no new edge
    CHECK("multi-tick: release-cut applied once, not re-triggered by later ticks "
          "of the same frame",
          game->man.dy == -JUMP_CUT_SPEED_PER_SEC);

    // ------------------------------------------------------------------
    // 4. Simultaneous left+right: preserves the pre-existing, documented
    //    left-wins precedence (if (moveLeft) ... else if (moveRight) ...),
    //    now made an explicit, tested rule rather than an implicit one.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->input.moveLeftPlayer1 = true;
    game->input.moveRightPlayer1 = true;
    game->man.dx = 0.0f;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("left+right precedence: simultaneous hold resolves to left (dx negative)",
          game->man.dx < 0.0f);

    // ------------------------------------------------------------------
    // 5. Multiplayer isolation: player 1 input must not mutate player 2
    //    fields, and vice versa.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->input.moveRightPlayer1 = true;
    game->input.moveLeftPlayer2 = false;
    game->input.moveRightPlayer2 = false;
    game->man.dx = 0.0f;
    game->secondPlayer.dx = 0.0f;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("multiplayer isolation: player 1 movement does not affect secondPlayer.dx",
          game->man.dx > 0.0f && game->secondPlayer.dx == 0.0f);

    arcade_session_reset(game, GAME_MODE_MULTIPLAYER);
    game->input.moveLeftPlayer1 = false;
    game->input.moveRightPlayer1 = false;
    game->input.moveLeftPlayer2 = true;
    game->man.dx = 0.0f;
    game->secondPlayer.dx = 0.0f;

    apply_arcade_player_forces(game, PHYSICS_DT);
    CHECK("multiplayer isolation: player 2 movement does not affect man.dx",
          game->secondPlayer.dx < 0.0f && game->man.dx == 0.0f);

    // ------------------------------------------------------------------
    // 6. Session reset clears every new continuous field, both modes.
    // ------------------------------------------------------------------
    game->input.moveLeftPlayer1 = true;
    game->input.moveRightPlayer1 = true;
    game->input.jumpHeldPlayer1 = true;
    game->input.shootHeldPlayer1 = true;
    game->input.moveLeftPlayer2 = true;
    game->input.moveRightPlayer2 = true;
    game->input.jumpHeldPlayer2 = true;
    game->input.shootHeldPlayer2 = true;

    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    CHECK("arcade session reset: all 6 continuous input fields cleared",
          game->input.moveLeftPlayer1 == false && game->input.moveRightPlayer1 == false &&
          game->input.jumpHeldPlayer1 == false && game->input.shootHeldPlayer1 == false &&
          game->input.moveLeftPlayer2 == false && game->input.moveRightPlayer2 == false &&
          game->input.jumpHeldPlayer2 == false && game->input.shootHeldPlayer2 == false);

    game->input.moveLeftPlayer1 = true;
    game->input.moveRightPlayer1 = true;
    game->input.jumpHeldPlayer1 = true;
    game->input.shootHeldPlayer1 = true;
    game->input.moveLeftPlayer2 = true;
    game->input.moveRightPlayer2 = true;
    game->input.jumpHeldPlayer2 = true;
    game->input.shootHeldPlayer2 = true;

    runner_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    CHECK("runner session reset: all 6 continuous input fields cleared",
          game->input.moveLeftPlayer1 == false && game->input.moveRightPlayer1 == false &&
          game->input.jumpHeldPlayer1 == false && game->input.shootHeldPlayer1 == false &&
          game->input.moveLeftPlayer2 == false && game->input.moveRightPlayer2 == false &&
          game->input.jumpHeldPlayer2 == false && game->input.shootHeldPlayer2 == false);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("INPUT SNAPSHOT TEST: ALL PASS\n");
        return 0;
    }
    printf("INPUT SNAPSHOT TEST: %d FAILURE(S)\n", failures);
    return 1;
}

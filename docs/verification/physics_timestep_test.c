// Standalone fixed-timestep physics test -- NOT part of the shipped game
// (main.c is excluded when building this, same pattern as the other
// docs/verification/*.c harnesses). Exercises process() (Arcade)
// directly with an explicit dt, without needing keyboard/window input
// injection or a real timer. See docs/physics-timestep-map.md.
//
// Test-only exception to the "game->app.scene is written only inside
// app_change_scene()" invariant: not needed here -- this test never
// touches game->app.scene.
#include "app.h"

static int failures = 0;

#define CHECK(desc, cond)                                            \
    do                                                                 \
    {                                                                   \
        if (cond)                                                        \
        {                                                                 \
            printf("PHYSICS TIMESTEP TEST: PASS - %s\n", desc);            \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("PHYSICS TIMESTEP TEST: FAIL - %s\n", desc);                \
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
        printf("PHYSICS TIMESTEP TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Regression-equivalence -- one fixed tick from rest must reproduce
    //    the exact numeric behavior of the old per-frame code (see
    //    docs/physics-timestep-map.md section 3 for the derivation).
    //    Airborne (onLedge=0), isDead=0, statusState=GAME so process()'s
    //    gravity/integration block runs unobstructed by collision.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME; // test-only precondition
    game->man.isDead = 0;
    game->man.onLedge = 0;
    game->man.dx = 0;
    game->man.dy = 0;

    float yBeforeTick1 = game->man.y;
    process(game, PHYSICS_DT); // tick 1: y integrates with the OLD dy (0), then gravity is added

    CHECK("tick 1: y unchanged (old dy was 0, matching old per-frame first-tick behavior)",
          game->man.y == yBeforeTick1);
    CHECK("tick 1: dy becomes exactly GRAVITY_PER_SEC2*PHYSICS_DT (=30.0, old equivalent 0.5/frame)",
          game->man.dy == GRAVITY_PER_SEC2 * PHYSICS_DT);

    float yAfterTick1 = game->man.y;
    process(game, PHYSICS_DT); // tick 2: y integrates with tick 1's dy (30.0)

    CHECK("tick 2: y moves by exactly dy*dt (=0.5px, bit-identical to the old `y += 0.5f`)",
          fabsf((game->man.y - yAfterTick1) - 0.5f) < 0.0001f);
    CHECK("tick 2: dy becomes exactly 2*GRAVITY_PER_SEC2*PHYSICS_DT (=60.0)",
          game->man.dy == 2.0f * GRAVITY_PER_SEC2 * PHYSICS_DT);

    // ------------------------------------------------------------------
    // 2. Refresh-rate independence -- the same total simulated time (1
    //    second = 60 fixed ticks), chunked into different real-frame
    //    patterns (simulating a 240Hz vs. a 30Hz display driving the same
    //    fixed-timestep accumulator main() uses), must produce identical
    //    final state and the identical total tick count.
    // ------------------------------------------------------------------
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME;
    game->man.isDead = 0;
    game->man.onLedge = 0;
    game->man.dx = 0;
    game->man.dy = 0;

    // Pattern A: 240 real "frames" of 1/240s each (a fast display).
    double accumulatorA = 0.0;
    int ticksA = 0;
    for (int frame = 0; frame < 240; frame++)
    {
        accumulatorA += (1.0 / 240.0);
        while (accumulatorA >= (double)PHYSICS_DT && ticksA < 600)
        {
            process(game, PHYSICS_DT);
            accumulatorA -= (double)PHYSICS_DT;
            ticksA++;
        }
    }
    float xA = game->man.x, yA = game->man.y, dxA = game->man.dx, dyA = game->man.dy;

    // Reset to the identical starting state, then replay the identical
    // total simulated time via Pattern B: 30 real "frames" of 1/30s each
    // (a slow display).
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->statusState = STATUS_STATE_GAME;
    game->man.isDead = 0;
    game->man.onLedge = 0;
    game->man.dx = 0;
    game->man.dy = 0;

    double accumulatorB = 0.0;
    int ticksB = 0;
    for (int frame = 0; frame < 30; frame++)
    {
        accumulatorB += (1.0 / 30.0);
        while (accumulatorB >= (double)PHYSICS_DT && ticksB < 600)
        {
            process(game, PHYSICS_DT);
            accumulatorB -= (double)PHYSICS_DT;
            ticksB++;
        }
    }
    float xB = game->man.x, yB = game->man.y, dxB = game->man.dx, dyB = game->man.dy;

    // 240*(1/240) and 30*(1/30) both sum to 1.0 simulated second in exact
    // math, but double-precision accumulation can round either pattern to
    // 59 or 60 ticks -- the real invariant is that both patterns agree with
    // each other (same total accumulated time -> same tick count), not that
    // either hits a bit-exact 60.
    CHECK("refresh-rate independence: both chunking patterns produce the same tick count",
          ticksA == ticksB);
    CHECK("refresh-rate independence: tick count is 1 second's worth (59 or 60 at 60Hz)",
          ticksA >= 59 && ticksA <= 60);
    CHECK("refresh-rate independence: final man.x matches regardless of real-frame chunking",
          xA == xB);
    CHECK("refresh-rate independence: final man.y matches regardless of real-frame chunking",
          yA == yB);
    CHECK("refresh-rate independence: final man.dx matches regardless of real-frame chunking",
          dxA == dxB);
    CHECK("refresh-rate independence: final man.dy matches regardless of real-frame chunking",
          dyA == dyB);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("PHYSICS TIMESTEP TEST: ALL PASS\n");
        return 0;
    }
    printf("PHYSICS TIMESTEP TEST: %d FAILURE(S)\n", failures);
    return 1;
}

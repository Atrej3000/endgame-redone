// Standalone Phase 21 regression test. Verifies that the remaining moving
// Runner presentation/gameplay objects use named per-second constants while
// preserving their legacy 60 Hz displacement.
#include "header.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (condition)                                                      \
        {                                                                   \
            printf("PHYSICS UNITS TEST: PASS - %s\n", description);        \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf("PHYSICS UNITS TEST: FAIL - %s\n", description);        \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static bool approximately_equal(float a, float b)
{
    return fabsf(a - b) < 0.001f;
}

int main(void)
{
    GameState game = {0};
    game.statusState = STATUS_STATE_GAME;
    game.multiPlayer = GAME_MODE_MULTIPLAYER;
    game.time = 199;
    game.scrollX = 100.0f;
    game.stars[0].baseX = 100;
    game.stars[0].baseY = 200;
    game.stars[0].mode = 0;
    game.stars[0].phase = 0.0f;

    process2(&game, PHYSICS_DT);

    CHECK("multiplayer Runner camera moves at 180 px/s (3 px at 60 Hz)",
          approximately_equal(game.scrollX, 97.0f));
    CHECK("moving trap uses its 3.6 rad/s angular speed",
          game.stars[0].x == (int)(100.0f +
              sinf(200.0f * PHYSICS_DT * TRAP_ANGULAR_SPEED_PER_SEC) * 75.0f));

    if (failures == 0)
    {
        printf("PHYSICS UNITS TEST: ALL PASS\n");
        return 0;
    }
    printf("PHYSICS UNITS TEST: %d FAILURE(S)\n", failures);
    return 1;
}

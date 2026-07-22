// Standalone render-interpolation test. No renderer is created: this checks
// the presentation data path directly and proves it never changes the
// authoritative current transforms used by physics and collision code.
#include "header.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (condition)                                                      \
        {                                                                   \
            printf("RENDER INTERPOLATION TEST: PASS - %s\n", description); \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf("RENDER INTERPOLATION TEST: FAIL - %s\n", description); \
            failures++;                                                     \
        }                                                                   \
    } while (0)

int main(void)
{
    GameState game = {0};
    game.man.x = 100.0f;
    game.man.y = 200.0f;
    game.enemyValues[0].x = 300.0f;
    game.enemyValues[0].y = 400.0f;
    game.bullets[0].x = 50.0f;
    game.bullets[0].y = 60.0f;
    game.stars[0].x = 40;
    game.stars[0].y = 80;
    game.scrollX = -20.0f;

    capture_render_transforms(&game);
    game.man.x = 160.0f;
    game.man.y = 260.0f;
    game.enemyValues[0].x = 340.0f;
    game.bullets[0].x = 90.0f;
    game.stars[0].y = 100;
    game.scrollX = -60.0f;

    CHECK("alpha 0 renders the previous position", render_lerp(game.man.prevX, game.man.x, 0.0f) == 100.0f);
    CHECK("alpha 0.5 renders the midpoint", render_lerp(game.man.prevX, game.man.x, 0.5f) == 130.0f);
    CHECK("alpha 1 renders the current position", render_lerp(game.man.prevY, game.man.y, 1.0f) == 260.0f);
    CHECK("alpha is clamped below zero", render_lerp(10.0f, 20.0f, -1.0f) == 10.0f);
    CHECK("alpha is clamped above one", render_lerp(10.0f, 20.0f, 2.0f) == 20.0f);
    CHECK("enemy previous transform was captured", game.enemyValues[0].prevX == 300.0f);
    CHECK("bullet previous transform was captured", game.bullets[0].prevX == 50.0f);
    CHECK("star previous transform was captured", game.stars[0].prevY == 80.0f);
    CHECK("camera previous transform was captured", game.prevScrollX == -20.0f);
    CHECK("interpolation leaves authoritative player state untouched", game.man.x == 160.0f && game.man.y == 260.0f);

    sync_render_transforms(&game);
    CHECK("sync prevents interpolation across a teleport", game.man.prevX == game.man.x && game.prevScrollX == game.scrollX);

    if (failures == 0)
    {
        printf("RENDER INTERPOLATION TEST: ALL PASS\n");
        return 0;
    }
    printf("RENDER INTERPOLATION TEST: %d FAILURE(S)\n", failures);
    return 1;
}

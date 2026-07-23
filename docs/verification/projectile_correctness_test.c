// Standalone bullet pool / move-once-per-step / swept-collision test -- NOT
// part of the shipped game (main.c is excluded when building this, same
// pattern as the other docs/verification/*.c harnesses). Calls
// addBullet/removeBullet/move_arcade_bullets/process directly with
// hand-set GameState fields. See docs/projectile-correctness-map.md.
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
            printf("PROJECTILE CORRECTNESS TEST: PASS - %s\n", desc);      \
        }                                                                   \
        else                                                                 \
        {                                                                     \
            printf("PROJECTILE CORRECTNESS TEST: FAIL - %s\n", desc);          \
            failures++;                                                        \
        }                                                                       \
    } while (0)

static void reset_arcade_test(GameState *game)
{
    arcade_session_reset(game, GAME_MODE_SINGLE_PLAYER);
    game->app.scene = APP_SCENE_ARCADE_GAME;
}

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("PROJECTILE CORRECTNESS TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. Pool spawn/despawn: addBullet claims the first inactive slot,
    //    removeBullet deactivates without disturbing other slots.
    // ------------------------------------------------------------------
    reset_arcade_test(game);

    addBullet(game, 100.0f, 200.0f, 3.0f);
    CHECK("addBullet: slot 0 activated", game->bullets[0].active);
    CHECK("addBullet: x/y/dx set from arguments",
          game->bullets[0].x == 100.0f && game->bullets[0].y == 200.0f && game->bullets[0].dx == 3.0f);
    CHECK("addBullet: prevX initialized to the spawn position", game->bullets[0].prevX == 100.0f);

    addBullet(game, 150.0f, 250.0f, -3.0f);
    CHECK("addBullet: a second shot claims the next slot (slot 1), not slot 0 again",
          game->bullets[1].active && game->bullets[1].x == 150.0f);

    removeBullet(game, 0);
    CHECK("removeBullet: slot 0 deactivated", !game->bullets[0].active);
    CHECK("removeBullet: slot 1 (a different bullet) is untouched",
          game->bullets[1].active && game->bullets[1].x == 150.0f && game->bullets[1].dx == -3.0f);

    // ------------------------------------------------------------------
    // 2. Move-once-per-step: one move_arcade_bullets() call moves a bullet
    //    by BULLET_SPEED_PER_SEC * PHYSICS_DT -- not the old 113x-per-tick bug.
    //    Also confirms the direction (sign of dx at spawn) is preserved.
    // ------------------------------------------------------------------
    reset_arcade_test(game);
    addBullet(game, 100.0f, 200.0f, 3.0f);   // rightward
    addBullet(game, 500.0f, 200.0f, -3.0f);  // leftward

    move_arcade_bullets(game, PHYSICS_DT);
    CHECK("move_arcade_bullets: rightward bullet moves by its per-second speed over one tick",
          fabsf(game->bullets[0].x - (100.0f + BULLET_SPEED_PER_SEC * PHYSICS_DT)) < 0.0001f);
    CHECK("move_arcade_bullets: prevX captured as the pre-move position",
          game->bullets[0].prevX == 100.0f);
    CHECK("move_arcade_bullets: leftward bullet moves by its per-second speed over one tick",
          fabsf(game->bullets[1].x - (500.0f - BULLET_SPEED_PER_SEC * PHYSICS_DT)) < 0.0001f);

    // A second call must move it by the same amount again (once per call,
    // not accumulating any duplicated-loop multiplier).
    float xAfterTick1 = game->bullets[0].x;
    move_arcade_bullets(game, PHYSICS_DT);
    CHECK("move_arcade_bullets: a second call moves the same fixed distance again, not 113x",
          fabsf(game->bullets[0].x - (xAfterTick1 + BULLET_SPEED_PER_SEC * PHYSICS_DT)) < 0.0001f);

    // ------------------------------------------------------------------
    // 3. Swept collision: a bullet whose prevX->x span crosses a target's
    //    rect registers a hit even though its end-of-tick position alone
    //    (past the target) would miss under the old point-only test.
    // ------------------------------------------------------------------
    reset_arcade_test(game);
    game->man.isDead = 0;
    game->statusState = STATUS_STATE_GAME;
    // Move every enemy/smart-enemy/boss out of the way except one target.
    for (int i = 0; i < NUM_ENEMIES; i++) { game->enemyValues[i].x = -10000.0f; game->enemyValues[i].y = -10000.0f; }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++) { game->smartEnemies[i].x = -10000.0f; game->smartEnemies[i].y = -10000.0f; }
    game->boss[0].x = -10000.0f; game->boss[0].y = -10000.0f;
    game->boss[1].x = -10000.0f; game->boss[1].y = -10000.0f;

    game->enemyValues[0].x = 40.0f;
    game->enemyValues[0].y = 100.0f;
    game->enemyValues[0].visible = 1;

    addBullet(game, 0.0f, 110.0f, 3.0f); // y=110 is inside [100,150)
    game->bullets[0].prevX = 0.0f;
    game->bullets[0].x = 100.0f; // jumped clean past the [40,80) target span this "tick"

    process(game, PHYSICS_DT);
    CHECK("swept collision: a bullet whose path crosses the target registers a hit "
          "even though its end position (100) is already past the target span [40,80)",
          !game->enemyValues[0].visible);
    CHECK("swept collision: the bullet that scored the hit is deactivated",
          !game->bullets[0].active);

    // ------------------------------------------------------------------
    // 4. End-to-end regression check: one real process() call with a
    //    bullet positioned to cross an enemy the normal way (via
    //    move_arcade_bullets, not hand-jumped) still kills it.
    // ------------------------------------------------------------------
    reset_arcade_test(game);
    game->man.isDead = 0;
    game->statusState = STATUS_STATE_GAME;
    for (int i = 0; i < NUM_ENEMIES; i++) { game->enemyValues[i].x = -10000.0f; game->enemyValues[i].y = -10000.0f; }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++) { game->smartEnemies[i].x = -10000.0f; game->smartEnemies[i].y = -10000.0f; }
    game->boss[0].x = -10000.0f; game->boss[0].y = -10000.0f;
    game->boss[1].x = -10000.0f; game->boss[1].y = -10000.0f;

    game->enemyValues[0].x = 40.0f;
    game->enemyValues[0].y = 100.0f;
    game->enemyValues[0].visible = 1;
    int killsBefore = game->kills_score;

    addBullet(game, 35.0f, 110.0f, 3.0f); // already inside the enemy's X span
    move_arcade_bullets(game, PHYSICS_DT);
    process(game, PHYSICS_DT);

    CHECK("end-to-end: a bullet crossing an enemy via the real pipeline still kills it",
          !game->enemyValues[0].visible);
    CHECK("end-to-end: kills_score increments", game->kills_score == killsBefore + 1);
    CHECK("end-to-end: the bullet is deactivated after the kill", !game->bullets[0].active);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("PROJECTILE CORRECTNESS TEST: ALL PASS\n");
        return 0;
    }
    printf("PROJECTILE CORRECTNESS TEST: %d FAILURE(S)\n", failures);
    return 1;
}

// Standalone Factory Method test -- NOT part of the shipped game (main.c is
// excluded when building this, same pattern as smoke_init_shutdown.c).
// Exercises enemy_spawn/smart_enemy_spawn/boss_spawn (src/entity_spawn.c)
// directly: confirms each reproduces the exact field values the original 10
// duplicated inline call sites set (see docs/solid-gof-audit.md section
// 7.1), and confirms the new bounds-check rejects an out-of-range index
// without mutating anything -- a correctness guarantee the original inline
// code never had.
#include "app.h"
#include "entity_spawn.h"

static int failures = 0;

#define CHECK(desc, cond)                                        \
    do                                                            \
    {                                                              \
        if (cond)                                                  \
        {                                                           \
            printf("ENTITY SPAWN TEST: PASS - %s\n", desc);          \
        }                                                             \
        else                                                           \
        {                                                               \
            printf("ENTITY SPAWN TEST: FAIL - %s\n", desc);              \
            failures++;                                                   \
        }                                                                  \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("ENTITY SPAWN TEST: app_init failed, cannot run\n");
        return 1;
    }

    // ------------------------------------------------------------------
    // 1. enemy_spawn -- reproduces process.c's time==120 wave exactly
    //    ("enemyValues[i]" at x=620, y=(i)*-200, dy=0, dx=0, visible=1")
    // ------------------------------------------------------------------
    CHECK("enemy_spawn(0, ...) returns true", enemy_spawn(game, 0, 620, 0, 0, 0) == true);
    CHECK("enemy_spawn sets x", game->enemyValues[0].x == 620);
    CHECK("enemy_spawn sets y", game->enemyValues[0].y == 0);
    CHECK("enemy_spawn sets dy", game->enemyValues[0].dy == 0);
    CHECK("enemy_spawn sets dx", game->enemyValues[0].dx == 0);
    CHECK("enemy_spawn sets visible=1", game->enemyValues[0].visible == 1);

    CHECK("enemy_spawn(1, 580, -200, 3, -4) returns true", enemy_spawn(game, 1, 580, -200, 3, -4) == true);
    CHECK("enemy_spawn sets non-zero x/y/dx/dy exactly", game->enemyValues[1].x == 580 && game->enemyValues[1].y == -200 && game->enemyValues[1].dx == 3 && game->enemyValues[1].dy == -4);

    // Out-of-range index: rejected, nothing mutated.
    Enemies before = game->enemyValues[5];
    CHECK("enemy_spawn(-1, ...) returns false", enemy_spawn(game, -1, 999, 999, 999, 999) == false);
    CHECK("enemy_spawn(NUM_ENEMIES, ...) returns false", enemy_spawn(game, NUM_ENEMIES, 999, 999, 999, 999) == false);
    CHECK("out-of-range enemy_spawn call left array untouched", memcmp(&before, &game->enemyValues[5], sizeof(Enemies)) == 0);

    // ------------------------------------------------------------------
    // 2. smart_enemy_spawn -- reproduces process.c's tempScore==31 wave
    //    exactly ("smartEnemies[0]" at x=1000,y=200,dy=0,dx=0,visible=1,
    //    countShots=0)
    // ------------------------------------------------------------------
    CHECK("smart_enemy_spawn(0, ...) returns true", smart_enemy_spawn(game, 0, 1000, 200, 0, 0) == true);
    CHECK("smart_enemy_spawn sets x/y", game->smartEnemies[0].x == 1000 && game->smartEnemies[0].y == 200);
    CHECK("smart_enemy_spawn sets dx/dy", game->smartEnemies[0].dx == 0 && game->smartEnemies[0].dy == 0);
    CHECK("smart_enemy_spawn sets visible=1", game->smartEnemies[0].visible == 1);
    CHECK("smart_enemy_spawn sets countShots=0", game->smartEnemies[0].countShots == 0);

    // Out-of-range index: rejected, nothing mutated.
    Enemies smartBefore = game->smartEnemies[3];
    CHECK("smart_enemy_spawn(-1, ...) returns false", smart_enemy_spawn(game, -1, 1, 1, 1, 1) == false);
    CHECK("smart_enemy_spawn(NUM_SMART_ENEMIES, ...) returns false", smart_enemy_spawn(game, NUM_SMART_ENEMIES, 1, 1, 1, 1) == false);
    CHECK("out-of-range smart_enemy_spawn call left array untouched", memcmp(&smartBefore, &game->smartEnemies[3], sizeof(Enemies)) == 0);

    // ------------------------------------------------------------------
    // 3. boss_spawn -- reproduces process.c's tempScore==345 wave exactly
    //    ("boss[0]" at x=1100,y=0,dy=0,dx=0,countShots=0,visible=1)
    // ------------------------------------------------------------------
    CHECK("boss_spawn(0, ...) returns true", boss_spawn(game, 0, 1100, 0, 0, 0) == true);
    CHECK("boss_spawn sets x/y", game->boss[0].x == 1100 && game->boss[0].y == 0);
    CHECK("boss_spawn sets countShots=0", game->boss[0].countShots == 0);
    CHECK("boss_spawn sets visible=1", game->boss[0].visible == 1);

    CHECK("boss_spawn(1, 100, 0, 0, 0) returns true", boss_spawn(game, 1, 100, 0, 0, 0) == true);
    CHECK("boss_spawn sets boss[1].x", game->boss[1].x == 100);

    // Out-of-range index: rejected, nothing mutated (boss array is fixed
    // size 2, indices 0/1 only).
    Enemies bossBefore = game->boss[0];
    CHECK("boss_spawn(-1, ...) returns false", boss_spawn(game, -1, 1, 1, 1, 1) == false);
    CHECK("boss_spawn(2, ...) returns false", boss_spawn(game, 2, 1, 1, 1, 1) == false);
    CHECK("out-of-range boss_spawn call left boss[0] untouched", memcmp(&bossBefore, &game->boss[0], sizeof(Enemies)) == 0);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("ENTITY SPAWN TEST: ALL PASS\n");
        return 0;
    }
    printf("ENTITY SPAWN TEST: %d FAILURE(S)\n", failures);
    return 1;
}

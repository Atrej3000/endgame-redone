#include "entity_spawn.h"

static bool valid_spawn_parameters(float x, float y, float dx, float dy)
{
    return isfinite(x) && isfinite(y) && isfinite(dx) && isfinite(dy);
}

static void reset_spawn_runtime(Enemies *enemy, float x, float y, float dx, float dy)
{
    enemy->x = x;
    enemy->y = y;
    enemy->prevX = x;
    enemy->prevY = y;
    enemy->dy = dy;
    enemy->dx = dx;
    enemy->slowingDown = 0;
    enemy->onLedge = 0;
    enemy->isdead = 0;
    enemy->visible = 1;
    enemy->countShots = 0;
    enemy->currentSpriteRun = 0;
    enemy->currentSpriteRun2 = 0;
    enemy->animation = (AnimationPlayer){.state = ANIMATION_STATE_IDLE};
    enemy->hitFlashTicks = 0;
}

bool enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (game == NULL || index < 0 || index >= NUM_ENEMIES ||
        !valid_spawn_parameters(x, y, dx, dy))
    {
        return false;
    }

    reset_spawn_runtime(&game->enemyValues[index], x, y, dx, dy);
    return true;
}

bool smart_enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (game == NULL || index < 0 || index >= NUM_SMART_ENEMIES ||
        !valid_spawn_parameters(x, y, dx, dy))
    {
        return false;
    }

    reset_spawn_runtime(&game->smartEnemies[index], x, y, dx, dy);
    return true;
}

bool boss_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (game == NULL || index < 0 || index >= 2 ||
        !valid_spawn_parameters(x, y, dx, dy))
    {
        return false;
    }

    reset_spawn_runtime(&game->boss[index], x, y, dx, dy);
    return true;
}

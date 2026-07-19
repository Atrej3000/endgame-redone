#include "entity_spawn.h"

bool enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (index < 0 || index >= NUM_ENEMIES)
    {
        return false;
    }

    game->enemyValues[index].x = x;
    game->enemyValues[index].y = y;
    game->enemyValues[index].dy = dy;
    game->enemyValues[index].dx = dx;
    game->enemyValues[index].visible = 1;
    return true;
}

bool smart_enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (index < 0 || index >= NUM_SMART_ENEMIES)
    {
        return false;
    }

    game->smartEnemies[index].x = x;
    game->smartEnemies[index].y = y;
    game->smartEnemies[index].dy = dy;
    game->smartEnemies[index].dx = dx;
    game->smartEnemies[index].visible = 1;
    game->smartEnemies[index].countShots = 0;
    return true;
}

bool boss_spawn(GameState *game, int index, float x, float y, float dx, float dy)
{
    if (index < 0 || index >= 2)
    {
        return false;
    }

    game->boss[index].x = x;
    game->boss[index].y = y;
    game->boss[index].dy = dy;
    game->boss[index].dx = dx;
    game->boss[index].countShots = 0;
    game->boss[index].visible = 1;
    return true;
}

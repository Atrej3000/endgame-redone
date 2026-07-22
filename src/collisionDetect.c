#include "header.h"

// Previous transforms are both render interpolation data and the authoritative
// start-of-tick reference used by the shared world solver's landing test.
void capture_player_prev_y(GameState *game)
{
    game->man.prevY = game->man.y;
    game->secondPlayer.prevY = game->secondPlayer.y;
}

float render_lerp(float previous, float current, float alpha)
{
    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    else if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }
    return previous + (current - previous) * alpha;
}

void capture_render_transforms(GameState *game)
{
    game->man.prevX = game->man.x;
    game->man.prevY = game->man.y;
    game->secondPlayer.prevX = game->secondPlayer.x;
    game->secondPlayer.prevY = game->secondPlayer.y;
    game->enemy.prevX = game->enemy.x;
    game->enemy.prevY = game->enemy.y;
    game->prevScrollX = game->scrollX;

    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        game->enemyValues[i].prevX = game->enemyValues[i].x;
        game->enemyValues[i].prevY = game->enemyValues[i].y;
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        game->smartEnemies[i].prevX = game->smartEnemies[i].x;
        game->smartEnemies[i].prevY = game->smartEnemies[i].y;
    }
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].prevX = game->boss[i].x;
        game->boss[i].prevY = game->boss[i].y;
    }
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        game->bullets[i].prevX = game->bullets[i].x;
        game->bullets[i].prevY = game->bullets[i].y;
        game->secondBullets[i].prevX = game->secondBullets[i].x;
        game->secondBullets[i].prevY = game->secondBullets[i].y;
    }
    for (int i = 0; i < NUM_STARS; i++)
    {
        game->stars[i].prevX = (float)game->stars[i].x;
        game->stars[i].prevY = (float)game->stars[i].y;
    }
}

void sync_render_transforms(GameState *game)
{
    capture_render_transforms(game);
}

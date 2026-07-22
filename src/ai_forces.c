#include "ai_forces.h"

// BOSS _________________________________________________________________________________________________________
void move_boss_entities(GameState *game, float dt)
{
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].y += game->boss[i].dy * dt;
        game->boss[i].x += game->boss[i].dx * dt;

        game->boss[i].dy += BOSS_GRAVITY_PER_SEC2 * dt;
        if (game->boss[i].dy > BOSS_MAX_FALL_SPEED_PER_SEC)
        {
            game->boss[i].dy = BOSS_MAX_FALL_SPEED_PER_SEC;
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x < 640)
        {
            game->boss[i].dx += BOSS_HORIZONTAL_ACCEL_PER_SEC2 * dt;
            if (game->boss[i].dx > BOSS_MAX_HORIZONTAL_SPEED_PER_SEC)
            {
                game->boss[i].dx = BOSS_MAX_HORIZONTAL_SPEED_PER_SEC;
            }
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x > 640)
        {
            game->boss[i].dx -= BOSS_HORIZONTAL_ACCEL_PER_SEC2 * dt;
            if (game->boss[i].dx < -BOSS_MAX_HORIZONTAL_SPEED_PER_SEC)
            {
                game->boss[i].dx = -BOSS_MAX_HORIZONTAL_SPEED_PER_SEC;
            }
        }
        if (game->boss[i].x > 1280)
        {
            game->boss[i].x = 0;
            game->boss[i].prevX = game->boss[i].x;
            game->boss[i].prevY = game->boss[i].y;
        }
        if (game->boss[i].x < 0)
        {
            game->boss[i].x = 1280;
            game->boss[i].prevX = game->boss[i].x;
            game->boss[i].prevY = game->boss[i].y;
        }
    }
}
// BOSS ----------------------------------------------------------------------------------------------------------------

// STUPID BOTS
void move_regular_enemies(GameState *game, float dt)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        Enemies *enemy = &game->enemyValues[i];
        enemy->y += enemy->dy * dt;
        enemy->x += enemy->dx * dt;

        if (enemy->x > 610 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx += ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
            if (enemy->dx >= ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
            {
                enemy->dx = ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
            }
        }

        if (enemy->x > 1280)
        {
            enemy->x = 0;
            enemy->prevX = enemy->x;
            enemy->prevY = enemy->y;
        }

        if (enemy->x < 590 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx -= ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
            if (enemy->dx <= -ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
            {
                enemy->dx = -ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
            }
        }
        if (enemy->x < -1)
        {
            enemy->x = 1280;
            enemy->prevX = enemy->x;
            enemy->prevY = enemy->y;
        }
        enemy->dy += ENEMY_GRAVITY_PER_SEC2 * dt;
        if (enemy->dy >= ENEMY_MAX_FALL_SPEED_PER_SEC)
        {
            enemy->dy = ENEMY_MAX_FALL_SPEED_PER_SEC;
        }
    }
}
// END BOTS

Man *smart_enemy_select_target(GameState *game, const Enemies *smartEnemy)
{
    if (!game->multiPlayer)
    {
        return &game->man;
    }
    if ((pow((smartEnemy->x - game->man.x), 2) + pow((smartEnemy->y - game->man.y), 2)) <
        (pow((smartEnemy->x - game->secondPlayer.x), 2) + pow(smartEnemy->y - game->secondPlayer.y, 2)))
    {
        return &game->man;
    }
    return &game->secondPlayer;
}

void move_smart_enemies(GameState *game, float dt)
{
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
        {
            game->smartEnemies[i].y = 200;
            game->smartEnemies[i].prevX = game->smartEnemies[i].x;
            game->smartEnemies[i].prevY = game->smartEnemies[i].y;
        }
        if (!game->multiPlayer)
        {
            game->smartEnemies[i].y += game->smartEnemies[i].dy * dt;
            if (game->smartEnemies[i].y > game->man.y)
            {
                while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                {
                    if (game->smartEnemies[i].x > game->man.x)
                    {
                        game->smartEnemies[i].dx += SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                        if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                        {
                            game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                        }
                    }
                    if (game->smartEnemies[i].x < game->man.x)
                    {
                        game->smartEnemies[i].dx -= SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                        if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                        {
                            game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                        }
                    }
                    game->smartEnemies[i].dy -= SMART_ENEMY_HOP_SPEED_PER_SEC;
                }
            }

            // if (game->smartEnemies[i].y < game->man.y && game->smartEnemies[i].x != 0)
            // {
            //     game->stopTime++;
            //     if (game->stopTime == 200)
            //     {
            //            game->smartEnemies[i].dx += 0.1;
            //            game->smartEnemies[i].x += game->smartEnemies[i].dx;
            //            if (game->smartEnemies[i].dx >= 5)
            //            {
            //                game->smartEnemies[i].dx = 5;
            //            }
            //            game->stopTime--;

            //     }
            // }

            if (game->smartEnemies[i].x + 6 < game->man.x)
            {
                if (game->smartEnemies[i].dx == 0)
                    game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;
                game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                game->smartEnemies[i].dx += SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                {
                    game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                }

                // game->man.facingLeft = 1;
                //game->enemy.slowingDown = 0;
            }
            else if (game->smartEnemies[i].x - 6 > game->man.x)
            {
                if (game->smartEnemies[i].dx == 0)
                    game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;

                game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                game->smartEnemies[i].dx -= SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                {
                    game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                }
                // game->man.facingLeft = 1;
                //  game->enemy.slowingDown = 0;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
            game->smartEnemies[i].dy += SMART_ENEMY_GRAVITY_PER_SEC2 * dt;
        }

        // BOTS FOR MULTIPLAYER
        if (game->multiPlayer)
        {
            if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
            {
                game->smartEnemies[i].y = 200;
                game->smartEnemies[i].prevX = game->smartEnemies[i].x;
                game->smartEnemies[i].prevY = game->smartEnemies[i].y;
            }
            if (smart_enemy_select_target(game, &game->smartEnemies[i]) == &game->man)
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy * dt;
                if (game->smartEnemies[i].y > game->man.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                    {
                        if (game->smartEnemies[i].x > game->man.x)
                        {
                            game->smartEnemies[i].dx += SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                            if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                            {
                                game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                            }
                        }
                        if (game->smartEnemies[i].x < game->man.x)
                        {
                            game->smartEnemies[i].dx -= SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                            if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                            {
                                game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                            }
                        }
                        game->smartEnemies[i].dy -= SMART_ENEMY_HOP_SPEED_PER_SEC;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                    game->smartEnemies[i].dx += SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                    if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                    {
                        game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                    game->smartEnemies[i].dx -= SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                    if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                    {
                        game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += SMART_ENEMY_GRAVITY_PER_SEC2 * dt;
            }
            else
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy * dt;
                if (game->smartEnemies[i].y > game->secondPlayer.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->secondPlayer.y)
                    {
                        if (game->smartEnemies[i].x > game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx += SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                            if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                            {
                                game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                            }
                        }
                        if (game->smartEnemies[i].x < game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx -= SMART_ENEMY_HOP_ACCEL_PER_SEC2 * dt;
                            if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                            {
                                game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                            }
                        }
                        game->smartEnemies[i].dy -= SMART_ENEMY_HOP_SPEED_PER_SEC;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                    game->smartEnemies[i].dx += SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                    if (game->smartEnemies[i].dx >= SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                    {
                        game->smartEnemies[i].dx = SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -SMART_ENEMY_JUMP_SPEED_PER_SEC;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx * dt;
                    game->smartEnemies[i].dx -= SMART_ENEMY_HORIZONTAL_ACCEL_PER_SEC2 * dt;
                    if (game->smartEnemies[i].dx <= -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC)
                    {
                        game->smartEnemies[i].dx = -SMART_ENEMY_MAX_HORIZONTAL_SPEED_PER_SEC;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += SMART_ENEMY_GRAVITY_PER_SEC2 * dt;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
        }
    }
}
// END BOTS

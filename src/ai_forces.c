#include "ai_forces.h"

// BOSS _________________________________________________________________________________________________________
void move_boss_entities(GameState *game)
{
    for (int i = 0; i < 2; i++)
    {
        game->boss[i].y += game->boss[i].dy;
        game->boss[i].x += game->boss[i].dx;

        game->boss[i].dy += 0.1;
        if (game->boss[i].dy > 1.5)
        {
            game->boss[i].dy = 1.5;
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x < 640)
        {
            game->boss[i].dx += 0.1;
            if (game->boss[i].dx > 1)
            {
                game->boss[i].dx = 1;
            }
        }
        if (game->boss[i].y > 300 && game->boss[i].y < 350 && game->boss[i].x > 640)
        {
            game->boss[i].dx -= 0.1;
            if (game->boss[i].dx < -1)
            {
                game->boss[i].dx = -1;
            }
        }
        if (game->boss[i].x > 1280)
        {
            game->boss[i].x = 0;
        }
        if (game->boss[i].x < 0)
        {
            game->boss[i].x = 1280;
        }
    }
}
// BOSS ----------------------------------------------------------------------------------------------------------------

// STUPID BOTS
void move_regular_enemies(GameState *game)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        Enemies *enemy = &game->enemyValues[i];
        enemy->y += enemy->dy;
        enemy->x += enemy->dx;

        if (enemy->x > 610 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx += 0.2;
            if (enemy->dx >= 2)
            {
                enemy->dx = 2;
            }
        }

        if (enemy->x > 1280)
        {
            enemy->x = 0;
        }

        if (enemy->x < 590 && enemy->y > 150 && enemy->y < 170)
        {
            enemy->dx -= 0.2;
            if (enemy->dx <= -2)
            {
                enemy->dx = -2;
            }
        }
        if (enemy->x < -1)
        {
            enemy->x = 1280;
        }
        enemy->dy += 0.4;
        if (enemy->dy >= 2.5)
        {
            enemy->dy = 2.5;
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

void move_smart_enemies(GameState *game)
{
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
        {
            game->smartEnemies[i].y = 200;
        }
        if (!game->multiPlayer)
        {
            game->smartEnemies[i].y += game->smartEnemies[i].dy;
            if (game->smartEnemies[i].y > game->man.y)
            {
                while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                {
                    if (game->smartEnemies[i].x > game->man.x)
                    {
                        game->smartEnemies[i].dx += 1;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx;
                        if (game->smartEnemies[i].dx >= 4)
                        {
                            game->smartEnemies[i].dx = 4;
                        }
                    }
                    if (game->smartEnemies[i].x < game->man.x)
                    {
                        game->smartEnemies[i].dx -= 1;
                        game->smartEnemies[i].x += game->smartEnemies[i].dx;
                        if (game->smartEnemies[i].dx <= -4)
                        {
                            game->smartEnemies[i].dx = -4;
                        }
                    }
                    game->smartEnemies[i].dy -= 10;
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
                    game->smartEnemies[i].dy = -6;
                game->smartEnemies[i].x += game->smartEnemies[i].dx;
                game->smartEnemies[i].dx += 0.2;
                if (game->smartEnemies[i].dx >= 4)
                {
                    game->smartEnemies[i].dx = 4;
                }

                // game->man.facingLeft = 1;
                //game->enemy.slowingDown = 0;
            }
            else if (game->smartEnemies[i].x - 6 > game->man.x)
            {
                if (game->smartEnemies[i].dx == 0)
                    game->smartEnemies[i].dy = -6;

                game->smartEnemies[i].x += game->smartEnemies[i].dx;
                game->smartEnemies[i].dx -= 0.2;
                if (game->smartEnemies[i].dx <= -4)
                {
                    game->smartEnemies[i].dx = -4;
                }
                // game->man.facingLeft = 1;
                //  game->enemy.slowingDown = 0;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
            game->smartEnemies[i].dy += 1;
        }

        // BOTS FOR MULTIPLAYER
        if (game->multiPlayer)
        {
            if (game->smartEnemies[i].y > 720 && game->smartEnemies[i].y < 750)
            {
                game->smartEnemies[i].y = 200;
            }
            if (smart_enemy_select_target(game, &game->smartEnemies[i]) == &game->man)
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy;
                if (game->smartEnemies[i].y > game->man.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->man.y)
                    {
                        if (game->smartEnemies[i].x > game->man.x)
                        {
                            game->smartEnemies[i].dx += 1;
                            if (game->smartEnemies[i].dx >= 4)
                            {
                                game->smartEnemies[i].dx = 4;
                            }
                        }
                        if (game->smartEnemies[i].x < game->man.x)
                        {
                            game->smartEnemies[i].dx -= 1;
                            if (game->smartEnemies[i].dx <= -4)
                            {
                                game->smartEnemies[i].dx = -4;
                            }
                        }
                        game->smartEnemies[i].dy -= 10;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx += 0.2;
                    if (game->smartEnemies[i].dx >= 4)
                    {
                        game->smartEnemies[i].dx = 4;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->man.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx -= 0.2;
                    if (game->smartEnemies[i].dx <= -4)
                    {
                        game->smartEnemies[i].dx = -4;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += 1;
            }
            else
            {
                game->smartEnemies[i].y += game->smartEnemies[i].dy;
                if (game->smartEnemies[i].y > game->secondPlayer.y)
                {
                    while (game->smartEnemies[i].dy == 0 && game->smartEnemies[i].y > game->secondPlayer.y)
                    {
                        if (game->smartEnemies[i].x > game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx += 1;
                            if (game->smartEnemies[i].dx >= 4)
                            {
                                game->smartEnemies[i].dx = 4;
                            }
                        }
                        if (game->smartEnemies[i].x < game->secondPlayer.x)
                        {
                            game->smartEnemies[i].dx -= 1;
                            if (game->smartEnemies[i].dx <= -4)
                            {
                                game->smartEnemies[i].dx = -4;
                            }
                        }
                        game->smartEnemies[i].dy -= 10;
                    }
                }
                if (game->smartEnemies[i].x + 6 < game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;
                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx += 0.2;
                    if (game->smartEnemies[i].dx >= 4)
                    {
                        game->smartEnemies[i].dx = 4;
                    }

                    // game->man.facingLeft = 1;
                    //game->enemy.slowingDown = 0;
                }
                else if (game->smartEnemies[i].x - 6 > game->secondPlayer.x)
                {
                    if (game->smartEnemies[i].dx == 0)
                        game->smartEnemies[i].dy = -6;

                    game->smartEnemies[i].x += game->smartEnemies[i].dx;
                    game->smartEnemies[i].dx -= 0.2;
                    if (game->smartEnemies[i].dx <= -4)
                    {
                        game->smartEnemies[i].dx = -4;
                    }
                    // game->man.facingLeft = 1;
                    //  game->enemy.slowingDown = 0;
                }
                game->smartEnemies[i].dy += 1;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
            }
        }
    }
}
// END BOTS

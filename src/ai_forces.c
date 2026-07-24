#include "ai_forces.h"

static bool valid_ai_step(const GameState *game, float dt)
{
    return game != NULL && isfinite(dt) && dt > 0.0f &&
           dt <= (float)MAX_FRAME_TIME;
}

static bool enemy_runtime_is_finite(const Enemies *enemy)
{
    return enemy != NULL &&
           isfinite(enemy->x) && isfinite(enemy->y) &&
           isfinite(enemy->prevX) && isfinite(enemy->prevY) &&
           isfinite(enemy->dx) && isfinite(enemy->dy);
}

static void deactivate_invalid_enemy(Enemies *enemy)
{
    if (enemy == NULL) return;
    enemy->x = 0.0f;
    enemy->y = 1000.0f;
    enemy->prevX = enemy->x;
    enemy->prevY = enemy->y;
    enemy->dx = 0.0f;
    enemy->dy = 0.0f;
    enemy->visible = 0;
}

void move_boss_entities(GameState *game, float dt)
{
    if (!valid_ai_step(game, dt)) return;
    for (int i = 0; i < 2; i++)
    {
        if (!game->boss[i].visible)
        {
            continue;
        }
        if (!enemy_runtime_is_finite(&game->boss[i]))
        {
            deactivate_invalid_enemy(&game->boss[i]);
            continue;
        }
        if (game->boss[i].y >= 1000.0f)
        {
            continue;
        }
        game->boss[i].y += game->boss[i].dy * dt;
        game->boss[i].x += game->boss[i].dx * dt;
        if (!isfinite(game->boss[i].x) || !isfinite(game->boss[i].y))
        {
            deactivate_invalid_enemy(&game->boss[i]);
            continue;
        }

        game->boss[i].dy += BOSS_GRAVITY_PER_SEC2 * dt;
        if (!isfinite(game->boss[i].dy))
        {
            deactivate_invalid_enemy(&game->boss[i]);
            continue;
        }
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
// STUPID BOTS
void move_regular_enemies(GameState *game, float dt)
{
    if (!valid_ai_step(game, dt)) return;
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        Enemies *enemy = &game->enemyValues[i];
        if (!enemy->visible)
        {
            continue;
        }
        if (!enemy_runtime_is_finite(enemy))
        {
            deactivate_invalid_enemy(enemy);
            continue;
        }
        if (enemy->y >= 1000.0f)
        {
            continue;
        }
        enemy->y += enemy->dy * dt;
        enemy->x += enemy->dx * dt;
        if (!isfinite(enemy->x) || !isfinite(enemy->y))
        {
            deactivate_invalid_enemy(enemy);
            continue;
        }

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
        if (!isfinite(enemy->dy))
        {
            deactivate_invalid_enemy(enemy);
            continue;
        }
        if (enemy->dy >= ENEMY_MAX_FALL_SPEED_PER_SEC)
        {
            enemy->dy = ENEMY_MAX_FALL_SPEED_PER_SEC;
        }
    }
}
Man *smart_enemy_select_target(GameState *game, const Enemies *smartEnemy)
{
    if (game == NULL || smartEnemy == NULL ||
        !isfinite(smartEnemy->x) || !isfinite(smartEnemy->y))
    {
        return NULL;
    }
    const bool firstAvailable = !game->man.isDead &&
                                isfinite(game->man.x) && isfinite(game->man.y);
    const bool secondAvailable = game->multiPlayer &&
                                 !game->secondPlayer.isDead &&
                                 isfinite(game->secondPlayer.x) &&
                                 isfinite(game->secondPlayer.y);
    if (!game->multiPlayer)
    {
        return firstAvailable ? &game->man : NULL;
    }
    if (!firstAvailable)
    {
        return secondAvailable ? &game->secondPlayer : NULL;
    }
    if (!secondAvailable)
    {
        return &game->man;
    }
    const double firstDx = (double)smartEnemy->x - (double)game->man.x;
    const double firstDy = (double)smartEnemy->y - (double)game->man.y;
    const double secondDx = (double)smartEnemy->x - (double)game->secondPlayer.x;
    const double secondDy = (double)smartEnemy->y - (double)game->secondPlayer.y;
    if (firstDx * firstDx + firstDy * firstDy <
        secondDx * secondDx + secondDy * secondDy)
    {
        return &game->man;
    }
    return &game->secondPlayer;
}

void move_smart_enemies(GameState *game, float dt)
{
    if (!valid_ai_step(game, dt)) return;
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (!game->smartEnemies[i].visible)
        {
            continue;
        }
        if (!enemy_runtime_is_finite(&game->smartEnemies[i]))
        {
            deactivate_invalid_enemy(&game->smartEnemies[i]);
            continue;
        }
        if (game->smartEnemies[i].y >= 1000.0f)
        {
            continue;
        }
        Man *target = smart_enemy_select_target(game, &game->smartEnemies[i]);
        if (target == NULL)
        {
            continue;
        }
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
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
                game->smartEnemies[i].visible = 0;
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
            if (target == &game->man)
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
                }
                game->smartEnemies[i].dy += SMART_ENEMY_GRAVITY_PER_SEC2 * dt;
            }
            if (game->smartEnemies[i].y > 800)
            {
                game->smartEnemies[i].y = 1000;
                game->smartEnemies[i].visible = 0;
            }
        }
        if (!enemy_runtime_is_finite(&game->smartEnemies[i]))
        {
            deactivate_invalid_enemy(&game->smartEnemies[i]);
        }
    }
}

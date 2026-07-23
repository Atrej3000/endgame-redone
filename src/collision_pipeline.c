#include "collision_pipeline.h"
#include "game_events.h"
#include "physics_body.h"

static bool projectile_hits_enemy(GameState *game, const Bullet *bullet, const Enemies *enemy)
{
    if (game->perfLoggingEnabled)
    {
        game->perfProjectileTargetChecks++;
    }
    KinematicBody projectileBody = kinematic_body_from_bullet(bullet);
    KinematicBody enemyBody = kinematic_body_from_enemy(enemy);
    Collider projectile = projectile_collider();
    Collider enemyTarget = enemy_projectile_collider();
    return collider_swept_horizontal_overlap(&projectileBody, &projectile, &enemyBody, &enemyTarget);
}

static int collect_active_projectile_indices(const Bullet bullets[MAX_BULLETS], int indices[MAX_BULLETS])
{
    int count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            indices[count++] = i;
        }
    }
    return count;
}

void detect_projectile_hits(GameState *game)
{
    int activeBullets[MAX_BULLETS];
    int activeBulletCount = collect_active_projectile_indices(game->bullets, activeBullets);
    int activeSecondBullets[MAX_BULLETS];
    int activeSecondBulletCount = game->multiPlayer
        ? collect_active_projectile_indices(game->secondBullets, activeSecondBullets)
        : 0;

    if (game->perfLoggingEnabled)
    {
        game->perfProjectileActiveSamples += (Uint64)(activeBulletCount + activeSecondBulletCount);
    }

    for (int j = 0; j < NUM_ENEMIES; j++)
    {
        for (int activeIndex = 0; activeIndex < activeBulletCount; activeIndex++)
        {
            int i = activeBullets[activeIndex];
            if (projectile_hits_enemy(game, &game->bullets[i], &game->enemyValues[j]))
            {
                game_events_push_projectile_hit(game, false, i,
                                                GAME_EVENT_TARGET_REGULAR_ENEMY, j);
            }
        }
    }

    for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int activeIndex = 0; activeIndex < activeBulletCount; activeIndex++)
        {
            int i = activeBullets[activeIndex];
            if (projectile_hits_enemy(game, &game->bullets[i], &game->smartEnemies[j]))
            {
                game_events_push_projectile_hit(game, false, i,
                                                GAME_EVENT_TARGET_SMART_ENEMY, j);
            }
        }
    }

    for (int j = 0; j < 2; j++)
    {
        for (int activeIndex = 0; activeIndex < activeBulletCount; activeIndex++)
        {
            int i = activeBullets[activeIndex];
            if (projectile_hits_enemy(game, &game->bullets[i], &game->boss[j]))
            {
                game_events_push_projectile_hit(game, false, i,
                                                GAME_EVENT_TARGET_BOSS, j);
            }
        }
    }

    // Second player's projectiles

    if (game->multiPlayer)
    {
        for (int j = 0; j < NUM_ENEMIES; j++)
        {
            for (int activeIndex = 0; activeIndex < activeSecondBulletCount; activeIndex++)
            {
                int i = activeSecondBullets[activeIndex];
                if (projectile_hits_enemy(game, &game->secondBullets[i], &game->enemyValues[j]))
                {
                    game_events_push_projectile_hit(game, true, i,
                                                    GAME_EVENT_TARGET_REGULAR_ENEMY, j);
                }
            }
        }

        for (int j = 0; j < NUM_SMART_ENEMIES; j++)
        {
            for (int activeIndex = 0; activeIndex < activeSecondBulletCount; activeIndex++)
            {
                int i = activeSecondBullets[activeIndex];
                if (projectile_hits_enemy(game, &game->secondBullets[i], &game->smartEnemies[j]))
                {
                    game_events_push_projectile_hit(game, true, i,
                                                    GAME_EVENT_TARGET_SMART_ENEMY, j);
                }
            }
        }
        for (int j = 0; j < 2; j++)
        {
            for (int activeIndex = 0; activeIndex < activeSecondBulletCount; activeIndex++)
            {
                int i = activeSecondBullets[activeIndex];
                if (projectile_hits_enemy(game, &game->secondBullets[i], &game->boss[j]))
                {
                    game_events_push_projectile_hit(game, true, i,
                                                    GAME_EVENT_TARGET_BOSS, j);
                }
            }
        }
    }

}

void detect_arcade_hazards(GameState *game)
{
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        if (game->enemyValues[i].y < 1000.0f && game->enemyValues[i].visible)
        {
            if (collide2d(game->man.x, game->man.y, game->enemyValues[i].x, game->enemyValues[i].y,
                          48, 48, 32, 32))
            {
                game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
            }
            if (game->enemyValues[i].y > 730 && game->enemyValues[i].y < 734)
            {
                game_events_push_target_contact(game, GAME_EVENT_ARCADE_ENEMY_ESCAPED, i);
            }
            if (game->multiPlayer && collide2d(game->secondPlayer.x, game->secondPlayer.y,
                                                game->enemyValues[i].x, game->enemyValues[i].y,
                                                48, 48, 32, 32))
            {
                game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
            }
        }
    }
    for (int j = 0; j < 2; j++)
    {
        if (game->boss[j].y >= 1000.0f || !game->boss[j].visible) continue;
        if (collide2d(game->man.x, game->man.y, game->boss[j].x, game->boss[j].y, 48, 48, 32, 32))
        {
            game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
        }
        if (game->multiPlayer && collide2d(game->secondPlayer.x, game->secondPlayer.y,
                                            game->boss[j].x, game->boss[j].y, 30, 30, 30, 30))
        {
            game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
        }
        if (game->boss[j].y > 730 && game->boss[j].y < 740)
        {
            game_events_push_target_contact(game, GAME_EVENT_ARCADE_BOSS_ESCAPED, j);
        }
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].y >= 1000.0f || !game->smartEnemies[i].visible)
        {
            continue;
        }
        if (collide2d(game->man.x, game->man.y, game->smartEnemies[i].x, game->smartEnemies[i].y, 30, 30, 30, 30))
        {
            game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
        }
        if (game->multiPlayer)
        {
            if (collide2d(game->secondPlayer.x, game->secondPlayer.y, game->smartEnemies[i].x, game->smartEnemies[i].y, 30, 30, 30, 30))
            {
                game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
            }
        }
    }
    // Fall hazards

    if (game->man.y >= 719)
    {
        game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_FELL, 0);
    }

    if (game->secondPlayer.y >= 719)
    {
        game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_FELL, 1);
    }
    game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
}

void detect_runner_hazard_contacts(GameState *game)
{
    for (int i  = 0; i < 100; i++)
    {
        const float starX = (float)game->stars[i].x;
        const float starY = (float)game->stars[i].y;
        if (collide2d(game->man.x, game->man.y, starX, starY, 30.0f, 30.0f, 30.0f, 30.0f))
        {
            game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_HIT, 0);
        }
        if (collide2d(game->secondPlayer.x, game->secondPlayer.y, starX, starY, 30.0f, 30.0f, 30.0f, 30.0f))
        {
            game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_HIT, 1);
        }
    }
}

void detect_runner_fall_hazards(GameState *game)
{
    if (game->man.y >= 719 || game->man.x < 0)
    {
        game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_FELL, 0);
    }

    if (game->multiPlayer)
    {
        if (game->secondPlayer.y >= 719 || game->secondPlayer.x < 0)
        {
            game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_FELL, 1);
        }
    }
    game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
}

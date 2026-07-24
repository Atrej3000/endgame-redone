#include "collision_pipeline.h"
#include "game_events.h"
#include "physics_body.h"

#include <float.h>

static bool projectile_hits_enemy(GameState *game, const Bullet *bullet,
                                  GameEventTarget target, const Enemies *enemy)
{
    if (game->perfLoggingEnabled)
    {
        if (game->perfProjectileTargetChecks < UINT64_MAX)
        {
            game->perfProjectileTargetChecks++;
        }
    }
    if (bullet == NULL || enemy == NULL || !bullet->active ||
        !enemy->visible || enemy->y >= 1000.0f)
    {
        return false;
    }
    KinematicBody projectileBody = kinematic_body_from_bullet(bullet);
    KinematicBody enemyBody = kinematic_body_from_enemy(enemy);
    Collider projectile = projectile_collider();
    Collider enemyTarget = arcade_enemy_collider(target);
    return collider_swept_horizontal_overlap(&projectileBody, &projectile, &enemyBody, &enemyTarget);
}

static float projectile_target_distance(const Bullet *bullet, const Enemies *enemy)
{
    const float targetLeft = enemy->x;
    const float targetRight = enemy->x + enemy_projectile_collider().width;
    if (bullet->prevX < targetLeft)
    {
        return targetLeft - bullet->prevX;
    }
    if (bullet->prevX > targetRight)
    {
        return bullet->prevX - targetRight;
    }
    return 0.0f;
}

static void consider_projectile_target(GameState *game, const Bullet *bullet,
                                       GameEventTarget target, int targetIndex,
                                       const Enemies *enemy, bool *found,
                                       float *bestDistance, GameEventTarget *bestTarget,
                                       int *bestTargetIndex)
{
    if (!projectile_hits_enemy(game, bullet, target, enemy))
    {
        return;
    }
    const float distance = projectile_target_distance(bullet, enemy);
    if (!*found || distance < *bestDistance)
    {
        *found = true;
        *bestDistance = distance;
        *bestTarget = target;
        *bestTargetIndex = targetIndex;
    }
}

static void queue_nearest_projectile_hit(GameState *game, const Bullet *bullet,
                                         bool secondPlayerProjectile, int projectileIndex)
{
    bool found = false;
    float bestDistance = FLT_MAX;
    GameEventTarget bestTarget = GAME_EVENT_TARGET_REGULAR_ENEMY;
    int bestTargetIndex = -1;

    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        consider_projectile_target(game, bullet, GAME_EVENT_TARGET_REGULAR_ENEMY, i,
                                   &game->enemyValues[i], &found, &bestDistance,
                                   &bestTarget, &bestTargetIndex);
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        consider_projectile_target(game, bullet, GAME_EVENT_TARGET_SMART_ENEMY, i,
                                   &game->smartEnemies[i], &found, &bestDistance,
                                   &bestTarget, &bestTargetIndex);
    }
    for (int i = 0; i < 2; i++)
    {
        consider_projectile_target(game, bullet, GAME_EVENT_TARGET_BOSS, i,
                                   &game->boss[i], &found, &bestDistance,
                                   &bestTarget, &bestTargetIndex);
    }

    if (found)
    {
        (void)game_events_push_projectile_hit(game, secondPlayerProjectile,
                                              projectileIndex, bestTarget, bestTargetIndex);
    }
}

static int collect_active_projectile_indices(const Bullet bullets[MAX_BULLETS], int indices[MAX_BULLETS])
{
    if (bullets == NULL || indices == NULL) return 0;
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
    if (game == NULL) return;
    int activeBullets[MAX_BULLETS];
    int activeBulletCount = collect_active_projectile_indices(game->bullets, activeBullets);
    int activeSecondBullets[MAX_BULLETS];
    int activeSecondBulletCount = game->multiPlayer
        ? collect_active_projectile_indices(game->secondBullets, activeSecondBullets)
        : 0;

    if (game->perfLoggingEnabled)
    {
        const Uint64 activeCount = (Uint64)(activeBulletCount + activeSecondBulletCount);
        if (UINT64_MAX - game->perfProjectileActiveSamples < activeCount)
        {
            game->perfProjectileActiveSamples = UINT64_MAX;
        }
        else
        {
            game->perfProjectileActiveSamples += activeCount;
        }
    }

    for (int activeIndex = 0; activeIndex < activeBulletCount; activeIndex++)
    {
        const int i = activeBullets[activeIndex];
        queue_nearest_projectile_hit(game, &game->bullets[i], false, i);
    }

    if (game->multiPlayer)
    {
        for (int activeIndex = 0; activeIndex < activeSecondBulletCount; activeIndex++)
        {
            const int i = activeSecondBullets[activeIndex];
            queue_nearest_projectile_hit(game, &game->secondBullets[i], true, i);
        }
    }
}

static bool player_overlaps_arcade_target(const Man *player, const Enemies *enemy,
                                          GameEventTarget target)
{
    if (player == NULL || enemy == NULL || player->isDead ||
        !enemy->visible || enemy->y >= 1000.0f)
    {
        return false;
    }
    const KinematicBody playerBody = kinematic_body_from_man(player);
    const KinematicBody enemyBody = kinematic_body_from_enemy(enemy);
    const Collider playerCollider = player_world_collider();
    const Collider enemyCollider = arcade_enemy_collider(target);
    return collider_bounds_overlap(&playerBody, &playerCollider,
                                   &enemyBody, &enemyCollider);
}

void detect_arcade_hazards(GameState *game)
{
    if (game == NULL) return;
    for (int i = 0; i < NUM_ENEMIES; i++)
    {
        if (game->enemyValues[i].y < 1000.0f && game->enemyValues[i].visible)
        {
            // Reaching the bottom and touching a player are mutually
            // exclusive consequences for one target in one tick.
            if (game->enemyValues[i].y >= (float)HEIGHT)
            {
                (void)game_events_push_target_contact(game, GAME_EVENT_ARCADE_ENEMY_ESCAPED, i);
                continue;
            }
            if (player_overlaps_arcade_target(&game->man, &game->enemyValues[i],
                                              GAME_EVENT_TARGET_REGULAR_ENEMY))
            {
                (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
            }
            if (game->multiPlayer &&
                player_overlaps_arcade_target(&game->secondPlayer, &game->enemyValues[i],
                                              GAME_EVENT_TARGET_REGULAR_ENEMY))
            {
                (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
            }
        }
    }
    for (int j = 0; j < 2; j++)
    {
        if (game->boss[j].y >= 1000.0f || !game->boss[j].visible) continue;
        if (game->boss[j].y >= (float)HEIGHT)
        {
            (void)game_events_push_target_contact(game, GAME_EVENT_ARCADE_BOSS_ESCAPED, j);
            continue;
        }
        if (player_overlaps_arcade_target(&game->man, &game->boss[j],
                                          GAME_EVENT_TARGET_BOSS))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
        }
        if (game->multiPlayer &&
            player_overlaps_arcade_target(&game->secondPlayer, &game->boss[j],
                                          GAME_EVENT_TARGET_BOSS))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
        }
    }
    for (int i = 0; i < NUM_SMART_ENEMIES; i++)
    {
        if (game->smartEnemies[i].y >= 1000.0f || !game->smartEnemies[i].visible)
        {
            continue;
        }
        if (player_overlaps_arcade_target(&game->man, &game->smartEnemies[i],
                                          GAME_EVENT_TARGET_SMART_ENEMY))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 0);
        }
        if (game->multiPlayer &&
            player_overlaps_arcade_target(&game->secondPlayer, &game->smartEnemies[i],
                                          GAME_EVENT_TARGET_SMART_ENEMY))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_HIT, 1);
        }
    }
    // Fall hazards

    if (!game->man.isDead &&
        game->man.y >= (float)HEIGHT)
    {
        (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_FELL, 0);
    }

    if (game->multiPlayer && !game->secondPlayer.isDead &&
        game->secondPlayer.y >= (float)HEIGHT)
    {
        (void)game_events_push_player_contact(game, GAME_EVENT_ARCADE_PLAYER_FELL, 1);
    }
    (void)game_events_push_transition_check(game, GAME_EVENT_ARCADE_GAME_OVER_CHECK);
}

static bool player_overlaps_runner_hazard(const Man *player, float hazardX, float hazardY)
{
    if (player == NULL || player->isDead ||
        !isfinite(hazardX) || !isfinite(hazardY))
    {
        return false;
    }
    const KinematicBody playerBody = kinematic_body_from_man(player);
    const KinematicBody hazardBody = {hazardX, hazardY, hazardX, hazardY, 0.0f, 0.0f};
    const Collider playerCollider = player_world_collider();
    const Collider hazardCollider = runner_hazard_collider();
    return collider_bounds_overlap(&playerBody, &playerCollider,
                                   &hazardBody, &hazardCollider);
}

void detect_runner_hazard_contacts(GameState *game)
{
    if (game == NULL) return;
    for (int i  = 0; i < 100; i++)
    {
        const float starX = (float)game->stars[i].x;
        const float starY = (float)game->stars[i].y;
        if (!game->man.isDead &&
            player_overlaps_runner_hazard(&game->man, starX, starY))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_HIT, 0);
        }
        if (game->multiPlayer &&
            player_overlaps_runner_hazard(&game->secondPlayer, starX, starY))
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_HIT, 1);
        }
    }
}

void detect_runner_fixed_hazards(GameState *game)
{
    if (game == NULL) return;
    if (!game->man.isDead &&
        (game->man.y >= (float)HEIGHT || game->man.x < 0.0f))
    {
        (void)game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_FELL, 0);
    }

    if (game->multiPlayer && !game->secondPlayer.isDead)
    {
        if (game->secondPlayer.y >= (float)HEIGHT || game->secondPlayer.x < 0.0f)
        {
            (void)game_events_push_player_contact(game, GAME_EVENT_RUNNER_PLAYER_FELL, 1);
        }
    }
    (void)game_events_push_transition_check(game, GAME_EVENT_RUNNER_GAME_OVER_CHECK);
}

void detect_runner_fall_hazards(GameState *game)
{
    // Deprecated real-frame compatibility hook. Authoritative fall/game-over
    // detection is owned by runner_simulate().
    (void)game;
}

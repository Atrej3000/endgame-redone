#include "collision_pipeline.h"
#include "physics_body.h"

static void resolve_player_world_collision(Man *player, const Ledge *ledges, int ledgeCount)
{
    KinematicBody body = kinematic_body_from_man(player);
    Collider collider = player_world_collider();
    WorldCollisionResult result = resolve_kinematic_world(&body, &collider, ledges, ledgeCount);
    kinematic_body_apply_to_man(player, &body);
    player->onLedge = result.grounded ? 1 : 0;
}

static void resolve_enemy_world_collision(Enemies *enemy, const Ledge *ledges, int ledgeCount)
{
    KinematicBody body = kinematic_body_from_enemy(enemy);
    Collider collider = player_world_collider();
    WorldCollisionResult result = resolve_kinematic_world(&body, &collider, ledges, ledgeCount);
    kinematic_body_apply_to_enemy(enemy, &body);
    enemy->onLedge = result.grounded ? 1 : 0;
}

static void resolve_smart_enemy_pair_contacts(GameState *game)
{
    // Kept separate from world resolution: these are dynamic-vs-dynamic
    // contacts, not static ledges, and will move into the contact-event
    // phase later. The established 48x48 behavior is preserved here.
    for (int j = 0; j < NUM_SMART_ENEMIES; j++)
    {
        for (int i = 0; i < NUM_SMART_ENEMIES; i++)
        {
            if (i == j)
            {
                continue;
            }
            Enemies *moving = &game->smartEnemies[j];
            Enemies *other = &game->smartEnemies[i];
            const float size = 48.0f;

            if (moving->y + size <= other->y || moving->y >= other->y + size)
            {
                continue;
            }
            if (moving->dx < 0.0f && moving->x < other->x + size && moving->x + size > other->x + size)
            {
                moving->x = other->x + size;
                moving->dx = 0.0f;
            }
            else if (moving->dx > 0.0f && moving->x + size > other->x && moving->x < other->x)
            {
                moving->x = other->x - size;
                moving->dx = 0.0f;
            }
        }
    }
}

void collisionDetect(GameState *game)
{
    resolve_player_world_collision(&game->man, game->ledges, 100);
    for (int i = 0; i < 2; i++)
    {
        resolve_enemy_world_collision(&game->boss[i], game->ledges, 100);
    }
    resolve_player_world_collision(&game->secondPlayer, game->ledges, 100);
    resolve_smart_enemy_pair_contacts(game);
}

void collisionDetect2(GameState *game)
{
    // Preserve Runner's established ordering: hazard contact happens before
    // static-world resolution.
    detect_runner_hazard_contacts(game);
    resolve_player_world_collision(&game->man, game->ledges, 100);
    resolve_player_world_collision(&game->secondPlayer, game->ledges, 100);
}

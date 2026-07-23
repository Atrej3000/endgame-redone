#include "collision_pipeline.h"
#include "physics_body.h"

static void resolve_player_world_collision(Man *player, const Ledge *ledges, int ledgeCount)
{
    if (player == NULL || player->isDead || ledges == NULL || ledgeCount <= 0) return;
    KinematicBody body = kinematic_body_from_man(player);
    Collider collider = player_world_collider();
    WorldCollisionResult result = resolve_kinematic_world(&body, &collider, ledges, ledgeCount);
    kinematic_body_apply_to_man(player, &body);
    player->onLedge = result.grounded ? 1 : 0;
}

static void resolve_enemy_world_collision(Enemies *enemy, const Ledge *ledges, int ledgeCount)
{
    if (enemy == NULL || ledges == NULL || ledgeCount <= 0) return;
    KinematicBody body = kinematic_body_from_enemy(enemy);
    Collider collider = arcade_enemy_collider(GAME_EVENT_TARGET_BOSS);
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
            if (!moving->visible || moving->y >= 1000.0f ||
                !other->visible || other->y >= 1000.0f)
            {
                continue;
            }
            const Collider smartCollider =
                arcade_enemy_collider(GAME_EVENT_TARGET_SMART_ENEMY);
            const float width = smartCollider.width;
            const float height = smartCollider.height;

            if (moving->y + height <= other->y ||
                moving->y >= other->y + height)
            {
                continue;
            }
            if (moving->dx < 0.0f && moving->x < other->x + width &&
                moving->x + width > other->x + width)
            {
                moving->x = other->x + width;
                moving->dx = 0.0f;
            }
            else if (moving->dx > 0.0f &&
                     moving->x + width > other->x && moving->x < other->x)
            {
                moving->x = other->x - width;
                moving->dx = 0.0f;
            }
        }
    }
}

void collisionDetect(GameState *game)
{
    if (game == NULL) return;
    resolve_player_world_collision(&game->man, game->ledges, NUM_STARS);
    for (int i = 0; i < 2; i++)
    {
        if (game->boss[i].visible && game->boss[i].y < 1000.0f)
        {
            resolve_enemy_world_collision(&game->boss[i], game->ledges, NUM_STARS);
        }
    }
    if (game->multiPlayer)
    {
        resolve_player_world_collision(&game->secondPlayer, game->ledges, NUM_STARS);
    }
    resolve_smart_enemy_pair_contacts(game);
}

void collisionDetect2(GameState *game)
{
    if (game == NULL) return;
    // Resolve authoritative world position before testing dynamic hazards.
    resolve_player_world_collision(&game->man, game->ledges, NUM_STARS);
    if (game->multiPlayer)
    {
        resolve_player_world_collision(&game->secondPlayer, game->ledges, NUM_STARS);
    }
    detect_runner_hazard_contacts(game);
}

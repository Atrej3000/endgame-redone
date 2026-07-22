// Standalone Phase 22 test for KinematicBody/Collider adapters, collision
// layers, and the projectile sweep that migrated to the shared model.
#include "physics_body.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (condition)                                                      \
        {                                                                   \
            printf("PHYSICS BODY TEST: PASS - %s\n", description);         \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf("PHYSICS BODY TEST: FAIL - %s\n", description);         \
            failures++;                                                     \
        }                                                                   \
    } while (0)

int main(void)
{
    Man man = {0};
    man.x = 10.0f;
    man.y = 20.0f;
    man.prevX = 8.0f;
    man.prevY = 18.0f;
    man.dx = 30.0f;
    man.dy = 40.0f;

    KinematicBody body = kinematic_body_from_man(&man);
    CHECK("Man adapter copies current, previous, and velocity state",
          body.x == 10.0f && body.y == 20.0f && body.prevX == 8.0f &&
          body.prevY == 18.0f && body.vx == 30.0f && body.vy == 40.0f);

    body.x = 100.0f;
    body.vy = -50.0f;
    kinematic_body_apply_to_man(&man, &body);
    CHECK("Man adapter writes body changes back", man.x == 100.0f && man.dy == -50.0f);

    Collider player = player_world_collider();
    Collider projectile = projectile_collider();
    Collider enemy = enemy_projectile_collider();
    CHECK("player world collider has the established ledge hitbox",
          player.width == PLAYER_LEDGE_HITBOX_W && player.height == PLAYER_LEDGE_HITBOX_H);
    CHECK("projectiles and enemies mutually opt into collision", colliders_can_interact(&projectile, &enemy));
    CHECK("players do not collide with projectiles by default", !colliders_can_interact(&player, &projectile));

    KinematicBody bullet = {100.0f, 110.0f, 0.0f, 110.0f, 0.0f, 0.0f};
    KinematicBody target = {40.0f, 100.0f, 40.0f, 100.0f, 0.0f, 0.0f};
    CHECK("swept projectile detects a crossed 40x50 enemy target",
          collider_swept_horizontal_overlap(&bullet, &projectile, &target, &enemy));

    bullet.y = 160.0f;
    CHECK("swept projectile rejects a path outside target height",
          !collider_swept_horizontal_overlap(&bullet, &projectile, &target, &enemy));

    if (failures == 0)
    {
        printf("PHYSICS BODY TEST: ALL PASS\n");
        return 0;
    }
    printf("PHYSICS BODY TEST: %d FAILURE(S)\n", failures);
    return 1;
}

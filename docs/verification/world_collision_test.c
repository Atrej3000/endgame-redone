// Standalone Phase 23 test for the shared axis-separated world solver.
#include "physics_body.h"

static int failures = 0;

#define CHECK(description, condition)                                      \
    do                                                                      \
    {                                                                       \
        if (condition)                                                      \
        {                                                                   \
            printf("WORLD COLLISION TEST: PASS - %s\n", description);     \
        }                                                                   \
        else                                                                \
        {                                                                   \
            printf("WORLD COLLISION TEST: FAIL - %s\n", description);     \
            failures++;                                                     \
        }                                                                   \
    } while (0)

int main(void)
{
    Collider collider = player_world_collider();
    Ledge ledge = {100, 300, 200, 20};

    KinematicBody falling = {150.0f, 260.0f, 150.0f, 200.0f, 0.0f, 50.0f};
    WorldCollisionResult landing = resolve_kinematic_world(&falling, &collider, &ledge, 1);
    CHECK("downward crossing lands on the ledge", falling.y == 252.0f && falling.vy == 0.0f && landing.grounded);

    KinematicBody resting = {150.0f, 252.0f, 150.0f, 252.0f, 0.0f, 0.0f};
    WorldCollisionResult rest = resolve_kinematic_world(&resting, &collider, &ledge, 1);
    CHECK("exact rest remains grounded", resting.y == 252.0f && rest.grounded);

    KinematicBody rising = {150.0f, 301.0f, 150.0f, 301.0f, 0.0f, -50.0f};
    WorldCollisionResult ceiling = resolve_kinematic_world(&rising, &collider, &ledge, 1);
    CHECK("ceiling hit stops upward velocity without grounding", rising.y == 320.0f && rising.vy == 0.0f &&
          ceiling.hitCeiling && !ceiling.grounded);

    Ledge wall = {300, 300, 48, 100};
    KinematicBody rightward = {290.0f, 320.0f, 280.0f, 320.0f, 100.0f, 0.0f};
    WorldCollisionResult right = resolve_kinematic_world(&rightward, &collider, &wall, 1);
    CHECK("rightward side hit resolves X before Y", rightward.x == 252.0f && rightward.vx == 0.0f && right.hitRight);

    KinematicBody leftward = {340.0f, 320.0f, 350.0f, 320.0f, -100.0f, 0.0f};
    WorldCollisionResult left = resolve_kinematic_world(&leftward, &collider, &wall, 1);
    CHECK("leftward side hit resolves X before Y", leftward.x == 348.0f && leftward.vx == 0.0f && left.hitLeft);

    if (failures == 0)
    {
        printf("WORLD COLLISION TEST: ALL PASS\n");
        return 0;
    }
    printf("WORLD COLLISION TEST: %d FAILURE(S)\n", failures);
    return 1;
}

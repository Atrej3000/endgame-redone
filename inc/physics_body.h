#pragma once

// Phase 22's reusable, presentation-independent physics representation.
// The legacy Man/Enemies/Bullet storage remains during the staged migration;
// adapters below make the shared body/collider rules available without
// changing existing gameplay behavior before Phase 23 replaces the solver.
#include <stdint.h>

#include "header.h"

typedef struct
{
    float x;
    float y;
    float prevX;
    float prevY;
    float vx;
    float vy;
} KinematicBody;

typedef struct
{
    float offsetX;
    float offsetY;
    float width;
    float height;
    uint32_t layer;
    uint32_t mask;
} Collider;

typedef enum
{
    COLLISION_LAYER_PLAYER = 1u << 0,
    COLLISION_LAYER_ENEMY = 1u << 1,
    COLLISION_LAYER_WORLD = 1u << 2,
    COLLISION_LAYER_PROJECTILE = 1u << 3,
    COLLISION_LAYER_HAZARD = 1u << 4
} CollisionLayer;

typedef struct
{
    bool grounded;
    bool hitCeiling;
    bool hitLeft;
    bool hitRight;
} WorldCollisionResult;

KinematicBody kinematic_body_from_man(const Man *man);
KinematicBody kinematic_body_from_enemy(const Enemies *enemy);
KinematicBody kinematic_body_from_bullet(const Bullet *bullet);
void kinematic_body_apply_to_man(Man *man, const KinematicBody *body);
void kinematic_body_apply_to_enemy(Enemies *enemy, const KinematicBody *body);

Collider player_world_collider(void);
Collider projectile_collider(void);
// Authoritative gameplay target boxes. Rendering may use a larger sprite
// footprint, but every gameplay contact query for a given entity type uses
// these adapters instead of repeating interaction-specific literals.
Collider arcade_enemy_collider(GameEventTarget target);
Collider runner_hazard_collider(void);
// Compatibility alias for older callers/tests. Equivalent to the regular
// Arcade enemy gameplay collider.
Collider enemy_projectile_collider(void);
bool colliders_can_interact(const Collider *first, const Collider *second);
// Collision queries reject null, non-finite, or negative-size operands.
bool collider_bounds_overlap(const KinematicBody *firstBody, const Collider *firstCollider,
                             const KinematicBody *secondBody, const Collider *secondCollider);
bool collider_swept_horizontal_overlap(const KinematicBody *movingBody, const Collider *movingCollider,
                                       const KinematicBody *targetBody, const Collider *targetCollider);
WorldCollisionResult resolve_kinematic_world(KinematicBody *body, const Collider *collider,
                                             const Ledge *ledges, int ledgeCount);

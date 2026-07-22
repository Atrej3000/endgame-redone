#include "physics_body.h"

KinematicBody kinematic_body_from_man(const Man *man)
{
    return (KinematicBody){man->x, man->y, man->prevX, man->prevY, man->dx, man->dy};
}

KinematicBody kinematic_body_from_enemy(const Enemies *enemy)
{
    return (KinematicBody){enemy->x, enemy->y, enemy->prevX, enemy->prevY, enemy->dx, enemy->dy};
}

KinematicBody kinematic_body_from_bullet(const Bullet *bullet)
{
    return (KinematicBody){bullet->x, bullet->y, bullet->prevX, bullet->prevY, bullet->dx, 0.0f};
}

void kinematic_body_apply_to_man(Man *man, const KinematicBody *body)
{
    man->x = body->x;
    man->y = body->y;
    man->prevX = body->prevX;
    man->prevY = body->prevY;
    man->dx = body->vx;
    man->dy = body->vy;
}

void kinematic_body_apply_to_enemy(Enemies *enemy, const KinematicBody *body)
{
    enemy->x = body->x;
    enemy->y = body->y;
    enemy->prevX = body->prevX;
    enemy->prevY = body->prevY;
    enemy->dx = body->vx;
    enemy->dy = body->vy;
}

Collider player_world_collider(void)
{
    return (Collider){0.0f, 0.0f, PLAYER_LEDGE_HITBOX_W, PLAYER_LEDGE_HITBOX_H,
                      COLLISION_LAYER_PLAYER, COLLISION_LAYER_WORLD | COLLISION_LAYER_ENEMY |
                          COLLISION_LAYER_HAZARD};
}

Collider projectile_collider(void)
{
    // Existing projectile hit testing treats a bullet as a swept point.
    return (Collider){0.0f, 0.0f, 0.0f, 0.0f, COLLISION_LAYER_PROJECTILE,
                      COLLISION_LAYER_ENEMY};
}

Collider enemy_projectile_collider(void)
{
    // Preserves the established projectile target rectangle (40x50), which
    // remains intentionally distinct from sprite geometry until Phase 23.
    return (Collider){0.0f, 0.0f, 40.0f, 50.0f, COLLISION_LAYER_ENEMY,
                      COLLISION_LAYER_PROJECTILE | COLLISION_LAYER_PLAYER};
}

bool colliders_can_interact(const Collider *first, const Collider *second)
{
    return (first->mask & second->layer) != 0u && (second->mask & first->layer) != 0u;
}

bool collider_bounds_overlap(const KinematicBody *firstBody, const Collider *firstCollider,
                             const KinematicBody *secondBody, const Collider *secondCollider)
{
    if (!colliders_can_interact(firstCollider, secondCollider))
    {
        return false;
    }

    float firstX = firstBody->x + firstCollider->offsetX;
    float firstY = firstBody->y + firstCollider->offsetY;
    float secondX = secondBody->x + secondCollider->offsetX;
    float secondY = secondBody->y + secondCollider->offsetY;
    return firstX + firstCollider->width > secondX && firstX < secondX + secondCollider->width &&
           firstY + firstCollider->height > secondY && firstY < secondY + secondCollider->height;
}

bool collider_swept_horizontal_overlap(const KinematicBody *movingBody, const Collider *movingCollider,
                                       const KinematicBody *targetBody, const Collider *targetCollider)
{
    if (!colliders_can_interact(movingCollider, targetCollider))
    {
        return false;
    }

    float movingStartX = movingBody->prevX + movingCollider->offsetX;
    float movingEndX = movingBody->x + movingCollider->offsetX;
    float pathLeft = fminf(movingStartX, movingEndX);
    float pathRight = fmaxf(movingStartX, movingEndX);
    float targetX = targetBody->x + targetCollider->offsetX;
    float targetY = targetBody->y + targetCollider->offsetY;
    float movingY = movingBody->y + movingCollider->offsetY;

    return pathRight > targetX && pathLeft < targetX + targetCollider->width &&
           movingY > targetY && movingY < targetY + targetCollider->height;
}

WorldCollisionResult resolve_kinematic_world(KinematicBody *body, const Collider *collider,
                                             const Ledge *ledges, int ledgeCount)
{
    WorldCollisionResult result = {false, false, false, false};

    // X is resolved first. This makes side impacts independent of a later
    // vertical correction, which is simpler and more stable than the former
    // repeated all-sides-in-one-loop approach.
    for (int i = 0; i < ledgeCount; i++)
    {
        float ledgeX = (float)ledges[i].x;
        float ledgeY = (float)ledges[i].y;
        float ledgeW = (float)ledges[i].w;
        float ledgeH = (float)ledges[i].h;
        float bodyY = body->y + collider->offsetY;

        if (bodyY + collider->height <= ledgeY || bodyY >= ledgeY + ledgeH)
        {
            continue;
        }

        float bodyX = body->x + collider->offsetX;
        if (body->vx < 0.0f && bodyX < ledgeX + ledgeW && bodyX + collider->width > ledgeX + ledgeW)
        {
            body->x = ledgeX + ledgeW - collider->offsetX;
            body->vx = 0.0f;
            result.hitLeft = true;
        }
        else if (body->vx > 0.0f && bodyX + collider->width > ledgeX && bodyX < ledgeX)
        {
            body->x = ledgeX - collider->width - collider->offsetX;
            body->vx = 0.0f;
            result.hitRight = true;
        }
    }

    // Then resolve Y. grounded is reset for every call and set only by a
    // downward crossing (or exact rest), never by a ceiling impact.
    for (int i = 0; i < ledgeCount; i++)
    {
        float ledgeX = (float)ledges[i].x;
        float ledgeY = (float)ledges[i].y;
        float ledgeW = (float)ledges[i].w;
        float ledgeH = (float)ledges[i].h;
        float bodyX = body->x + collider->offsetX;
        float bodyY = body->y + collider->offsetY;

        if (bodyX + collider->width <= ledgeX || bodyX >= ledgeX + ledgeW)
        {
            continue;
        }

        if (body->vy < 0.0f && bodyY < ledgeY + ledgeH && bodyY > ledgeY)
        {
            body->y = ledgeY + ledgeH - collider->offsetY;
            body->vy = 0.0f;
            result.hitCeiling = true;
        }
        else if (body->vy >= 0.0f && body->prevY + collider->offsetY + collider->height <= ledgeY &&
                 bodyY + collider->height >= ledgeY && bodyY < ledgeY + ledgeH)
        {
            body->y = ledgeY - collider->height - collider->offsetY;
            body->vy = 0.0f;
            result.grounded = true;
        }
    }

    return result;
}

#include "physics_body.h"

#include <float.h>

static bool body_is_finite(const KinematicBody *body)
{
    return body != NULL &&
           isfinite(body->x) && isfinite(body->y) &&
           isfinite(body->prevX) && isfinite(body->prevY) &&
           isfinite(body->vx) && isfinite(body->vy);
}

static bool collider_is_valid(const Collider *collider)
{
    return collider != NULL &&
           isfinite(collider->offsetX) && isfinite(collider->offsetY) &&
           isfinite(collider->width) && isfinite(collider->height) &&
           collider->width >= 0.0f && collider->height >= 0.0f;
}

KinematicBody kinematic_body_from_man(const Man *man)
{
    if (man == NULL) return (KinematicBody){0};
    return (KinematicBody){man->x, man->y, man->prevX, man->prevY, man->dx, man->dy};
}

KinematicBody kinematic_body_from_enemy(const Enemies *enemy)
{
    if (enemy == NULL) return (KinematicBody){0};
    return (KinematicBody){enemy->x, enemy->y, enemy->prevX, enemy->prevY, enemy->dx, enemy->dy};
}

KinematicBody kinematic_body_from_bullet(const Bullet *bullet)
{
    if (bullet == NULL) return (KinematicBody){0};
    return (KinematicBody){bullet->x, bullet->y, bullet->prevX, bullet->prevY, bullet->dx, 0.0f};
}

void kinematic_body_apply_to_man(Man *man, const KinematicBody *body)
{
    if (man == NULL || body == NULL) return;
    man->x = body->x;
    man->y = body->y;
    man->prevX = body->prevX;
    man->prevY = body->prevY;
    man->dx = body->vx;
    man->dy = body->vy;
}

void kinematic_body_apply_to_enemy(Enemies *enemy, const KinematicBody *body)
{
    if (enemy == NULL || body == NULL) return;
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

Collider arcade_enemy_collider(GameEventTarget target)
{
    if (target != GAME_EVENT_TARGET_REGULAR_ENEMY &&
        target != GAME_EVENT_TARGET_SMART_ENEMY &&
        target != GAME_EVENT_TARGET_BOSS)
    {
        return (Collider){0};
    }

    // One gameplay box is shared by projectile and player-contact queries.
    // It deliberately remains smaller than some rendered sprites because
    // their source sheets include transparent/ornamental pixels.
    return (Collider){0.0f, 0.0f, 40.0f, 50.0f, COLLISION_LAYER_ENEMY,
                      COLLISION_LAYER_PROJECTILE | COLLISION_LAYER_PLAYER};
}

Collider runner_hazard_collider(void)
{
    return (Collider){0.0f, 0.0f, 30.0f, 30.0f, COLLISION_LAYER_HAZARD,
                      COLLISION_LAYER_PLAYER};
}

Collider enemy_projectile_collider(void)
{
    return arcade_enemy_collider(GAME_EVENT_TARGET_REGULAR_ENEMY);
}

bool colliders_can_interact(const Collider *first, const Collider *second)
{
    if (!collider_is_valid(first) || !collider_is_valid(second)) return false;
    return (first->mask & second->layer) != 0u && (second->mask & first->layer) != 0u;
}

bool collider_bounds_overlap(const KinematicBody *firstBody, const Collider *firstCollider,
                             const KinematicBody *secondBody, const Collider *secondCollider)
{
    if (!body_is_finite(firstBody) || !body_is_finite(secondBody) ||
        !colliders_can_interact(firstCollider, secondCollider))
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
    if (!body_is_finite(movingBody) || !body_is_finite(targetBody) ||
        !colliders_can_interact(movingCollider, targetCollider))
    {
        return false;
    }

    const float movingStartX = movingBody->prevX + movingCollider->offsetX;
    const float movingEndX = movingBody->x + movingCollider->offsetX;
    const float targetX = targetBody->x + targetCollider->offsetX;
    const float targetRight = targetX + targetCollider->width;
    const float targetY = targetBody->y + targetCollider->offsetY;
    const float targetBottom = targetY + targetCollider->height;
    const float movingY = movingBody->y + movingCollider->offsetY;

    // A stationary point uses half-open AABB membership. A moving segment is
    // closed at its end, so arriving exactly at the near face counts as the
    // continuous collision that happened during this tick.
    if (movingStartX == movingEndX)
    {
        return movingEndX >= targetX && movingEndX < targetRight &&
               movingY >= targetY && movingY < targetBottom;
    }
    const float pathLeft = fminf(movingStartX, movingEndX);
    const float pathRight = fmaxf(movingStartX, movingEndX);
    return pathRight >= targetX && pathLeft < targetRight &&
           movingY >= targetY && movingY < targetBottom;
}

WorldCollisionResult resolve_kinematic_world(KinematicBody *body, const Collider *collider,
                                             const Ledge *ledges, int ledgeCount)
{
    WorldCollisionResult result = {false, false, false, false};
    if (!body_is_finite(body) || !collider_is_valid(collider) ||
        ledgeCount <= 0 || ledges == NULL)
    {
        return result;
    }

    // Resolve X at the earliest swept impact, independent of ledge array
    // order. The fallback handles bodies that begin a tick overlapping a
    // wall, while the sweep prevents high-speed tunnelling through it.
    bool foundHorizontalSweep = false;
    float bestHorizontalTime = FLT_MAX;
    float bestHorizontalSurface = 0.0f;
    bool foundHorizontalPenetration = false;
    float shallowestHorizontalPenetration = FLT_MAX;
    float horizontalPenetrationSurface = 0.0f;
    bool horizontalSweepHitLeft = false;
    bool horizontalPenetrationHitLeft = false;
    const float previousLeft = body->prevX + collider->offsetX;
    const float currentLeft = body->x + collider->offsetX;
    const float horizontalTravel = currentLeft - previousLeft;
    for (int i = 0; i < ledgeCount; i++)
    {
        float ledgeX = (float)ledges[i].x;
        float ledgeY = (float)ledges[i].y;
        float ledgeW = (float)ledges[i].w;
        float ledgeH = (float)ledges[i].h;
        if (ledgeW <= 0.0f || ledgeH <= 0.0f) continue;
        float bodyY = body->y + collider->offsetY;

        if (bodyY + collider->height <= ledgeY || bodyY >= ledgeY + ledgeH)
        {
            continue;
        }

        const float ledgeRight = ledgeX + ledgeW;
        const float previousRight = previousLeft + collider->width;
        const float currentRight = currentLeft + collider->width;
        if (horizontalTravel < 0.0f &&
            previousLeft >= ledgeRight && currentLeft <= ledgeRight)
        {
            const float impactTime = (ledgeRight - previousLeft) / horizontalTravel;
            if (!foundHorizontalSweep || impactTime < bestHorizontalTime)
            {
                foundHorizontalSweep = true;
                bestHorizontalTime = impactTime;
                bestHorizontalSurface = ledgeRight;
                horizontalSweepHitLeft = true;
            }
        }
        else if (horizontalTravel > 0.0f &&
                 previousRight <= ledgeX && currentRight >= ledgeX)
        {
            const float impactTime = (ledgeX - previousRight) / horizontalTravel;
            if (!foundHorizontalSweep || impactTime < bestHorizontalTime)
            {
                foundHorizontalSweep = true;
                bestHorizontalTime = impactTime;
                bestHorizontalSurface = ledgeX;
                horizontalSweepHitLeft = false;
            }
        }
        else if (horizontalTravel < 0.0f &&
                 currentLeft < ledgeRight && currentRight > ledgeRight)
        {
            const float penetration = ledgeRight - currentLeft;
            if (!foundHorizontalPenetration ||
                penetration < shallowestHorizontalPenetration)
            {
                foundHorizontalPenetration = true;
                shallowestHorizontalPenetration = penetration;
                horizontalPenetrationSurface = ledgeRight;
                horizontalPenetrationHitLeft = true;
            }
        }
        else if (horizontalTravel > 0.0f &&
                 currentRight > ledgeX && currentLeft < ledgeX)
        {
            const float penetration = currentRight - ledgeX;
            if (!foundHorizontalPenetration ||
                penetration < shallowestHorizontalPenetration)
            {
                foundHorizontalPenetration = true;
                shallowestHorizontalPenetration = penetration;
                horizontalPenetrationSurface = ledgeX;
                horizontalPenetrationHitLeft = false;
            }
        }
        else if (horizontalTravel == 0.0f &&
                 currentLeft < ledgeRight && currentRight > ledgeX)
        {
            const float pushRight = ledgeRight - currentLeft;
            const float pushLeft = currentRight - ledgeX;
            const bool pushToRight = pushRight < pushLeft;
            const float penetration = pushToRight ? pushRight : pushLeft;
            const float pushUp = bodyY + collider->height - ledgeY;
            const float pushDown = ledgeY + ledgeH - bodyY;
            const float verticalPenetration = fminf(pushUp, pushDown);
            if (penetration <= verticalPenetration &&
                (!foundHorizontalPenetration ||
                penetration < shallowestHorizontalPenetration)
               )
            {
                foundHorizontalPenetration = true;
                shallowestHorizontalPenetration = penetration;
                horizontalPenetrationSurface = pushToRight ? ledgeRight : ledgeX;
                horizontalPenetrationHitLeft = pushToRight;
            }
        }
    }
    if (foundHorizontalSweep || foundHorizontalPenetration)
    {
        const float surface = foundHorizontalSweep
            ? bestHorizontalSurface : horizontalPenetrationSurface;
        const bool hitLeft = foundHorizontalSweep
            ? horizontalSweepHitLeft : horizontalPenetrationHitLeft;
        if (hitLeft)
        {
            body->x = surface - collider->offsetX;
            result.hitLeft = true;
        }
        else
        {
            body->x = surface - collider->width - collider->offsetX;
            result.hitRight = true;
        }
        body->vx = 0.0f;
    }

    // Then resolve Y by earliest time of impact, independent of ledge array
    // order. A penetration fallback keeps hand-constructed/spawn-overlap
    // states recoverable without overriding a valid swept hit.
    bool foundSweep = false;
    float bestSweepTime = FLT_MAX;
    float bestSurface = 0.0f;
    bool foundPenetration = false;
    float shallowestPenetration = FLT_MAX;
    float penetrationSurface = 0.0f;
    bool penetrationIsCeiling = false;
    for (int i = 0; i < ledgeCount; i++)
    {
        float ledgeX = (float)ledges[i].x;
        float ledgeY = (float)ledges[i].y;
        float ledgeW = (float)ledges[i].w;
        float ledgeH = (float)ledges[i].h;
        if (ledgeW <= 0.0f || ledgeH <= 0.0f) continue;
        float bodyX = body->x + collider->offsetX;
        float bodyY = body->y + collider->offsetY;

        if (bodyX + collider->width <= ledgeX || bodyX >= ledgeX + ledgeW)
        {
            continue;
        }

        if (body->vy < 0.0f)
        {
            const float previousTop = body->prevY + collider->offsetY;
            const float currentTop = bodyY;
            const float ledgeBottom = ledgeY + ledgeH;
            const float travel = currentTop - previousTop;
            if (travel < 0.0f && previousTop >= ledgeBottom && currentTop <= ledgeBottom)
            {
                const float impactTime = (ledgeBottom - previousTop) / travel;
                if (!foundSweep || impactTime < bestSweepTime)
                {
                    foundSweep = true;
                    bestSweepTime = impactTime;
                    bestSurface = ledgeBottom;
                }
            }
            else if (currentTop > ledgeY && currentTop < ledgeBottom)
            {
                const float penetration = ledgeBottom - currentTop;
                if (!foundPenetration || penetration < shallowestPenetration)
                {
                    foundPenetration = true;
                    shallowestPenetration = penetration;
                    penetrationSurface = ledgeBottom;
                    penetrationIsCeiling = true;
                }
            }
        }
        else
        {
            const float previousBottom = body->prevY + collider->offsetY + collider->height;
            const float currentBottom = bodyY + collider->height;
            const float travel = currentBottom - previousBottom;
            if (previousBottom <= ledgeY && currentBottom >= ledgeY)
            {
                const float impactTime = travel > 0.0f
                    ? (ledgeY - previousBottom) / travel
                    : 0.0f;
                if (!foundSweep || impactTime < bestSweepTime)
                {
                    foundSweep = true;
                    bestSweepTime = impactTime;
                    bestSurface = ledgeY;
                }
            }
            else if (body->vy == 0.0f &&
                     bodyY < ledgeY + ledgeH && currentBottom > ledgeY)
            {
                const float pushUp = currentBottom - ledgeY;
                const float pushDown = ledgeY + ledgeH - bodyY;
                const bool pushBelow = pushDown < pushUp;
                const float penetration = pushBelow ? pushDown : pushUp;
                if (!foundPenetration || penetration < shallowestPenetration)
                {
                    foundPenetration = true;
                    shallowestPenetration = penetration;
                    penetrationSurface = pushBelow ? ledgeY + ledgeH : ledgeY;
                    penetrationIsCeiling = pushBelow;
                }
            }
            else if (bodyY < ledgeY && currentBottom > ledgeY)
            {
                const float penetration = currentBottom - ledgeY;
                if (!foundPenetration || penetration < shallowestPenetration)
                {
                    foundPenetration = true;
                    shallowestPenetration = penetration;
                    penetrationSurface = ledgeY;
                    penetrationIsCeiling = false;
                }
            }
        }
    }

    if (foundSweep || foundPenetration)
    {
        const float surface = foundSweep ? bestSurface : penetrationSurface;
        const bool resolvesCeiling = foundSweep
            ? body->vy < 0.0f
            : penetrationIsCeiling;
        if (resolvesCeiling)
        {
            body->y = surface - collider->offsetY;
            result.hitCeiling = true;
        }
        else
        {
            body->y = surface - collider->height - collider->offsetY;
            result.grounded = true;
        }
        body->vy = 0.0f;
    }

    return result;
}

#pragma once

// Collision/hazard/transition resolution, extracted from process()/
// processEvents()/collisionDetect() (Phase 19, see
// docs/collision-ordering-map.md). Each function's statement order and
// arithmetic are moved verbatim from their original location -- this is a
// structural extraction only, not a behavior change or a reordering.
#include "header.h"
#include "scene.h"

// Arcade: bullet-vs-enemy/smartEnemy/boss collision (both players), with
// kill/score/removal consequences fused inline. Was process.c:352-524.
void resolve_projectile_hits(GameState *game);

// Arcade: player-vs-enemy/boss/smartEnemy body contact, enemy/boss
// reached-bottom, and fall-off-screen -- all colocated at the same
// once-per-real-frame cadence in the original code, so kept as one
// function. Was processEvents.c:294-392.
void resolve_arcade_hazards(GameState *game);

// Arcade: the game-over scene transition following the hazards above,
// guarded against double-firing. Was processEvents.c:394-416.
void resolve_arcade_game_over_transition(GameState *game);

// Runner: player-vs-star hazard contact, checked once per fixed physics
// tick inside collisionDetect2() -- deliberately still called *before*
// ledge resolution there (see docs/collision-ordering-map.md section 2,
// Mismatch B: no demonstrated defect motivates reordering it). Was
// collisionDetect.c:656-666.
void resolve_runner_hazard_contact(GameState *game);

// Runner: fall-off-screen/off-left-edge hazard, checked once per real
// frame in processEvents2() -- a different cadence than the star check
// above, preserved as-is. Was processEvents.c:569-580.
void check_runner_fall_hazard(GameState *game);

// Runner: score-persist plus the game-over scene transition, guarded
// against double-firing. Was processEvents.c:583-599.
void resolve_runner_game_over_transition(GameState *game);

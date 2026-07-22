#pragma once

// Collision/hazard/transition resolution, extracted from process()/
// processEvents()/collisionDetect() (Phase 19, see
// docs/collision-ordering-map.md). Each function's statement order and
// arithmetic are moved verbatim from their original location -- this is a
// structural extraction only, not a behavior change or a reordering.
#include "header.h"
#include "scene.h"

// Arcade: pure bullet-vs-enemy/smartEnemy/boss contact detection (both
// players). Consequences are applied later by game_events_apply().
void detect_projectile_hits(GameState *game);

// Arcade: pure body-contact, reached-bottom, and fall detection. Its
// transition-check event preserves the legacy once-per-real-frame cadence.
void detect_arcade_hazards(GameState *game);

// Runner: pure star-contact detection (once per fixed tick) and pure fall
// detection plus a game-over check (once per real frame).
void detect_runner_hazard_contacts(GameState *game);
void detect_runner_fall_hazards(GameState *game);

#pragma once

// Collision/hazard/transition resolution. Hazard detection is kept in the
// fixed simulation path so its consequences cannot vary with render rate.
#include "header.h"
#include "scene.h"

// Arcade: pure bullet-vs-enemy/smartEnemy/boss contact detection (both
// players). Consequences are applied later by game_events_apply().
void detect_projectile_hits(GameState *game);

// Arcade: pure body-contact, reached-bottom, and fall detection. Its
// transition-check event is evaluated once per fixed physics tick.
void detect_arcade_hazards(GameState *game);

// Runner: pure star-contact detection and fall/game-over detection. Both
// authoritative paths run once per fixed tick.
void detect_runner_hazard_contacts(GameState *game);
void detect_runner_fixed_hazards(GameState *game);

// Compatibility entry point retained for the old real-frame event-poll path.
// It intentionally emits no gameplay events; use detect_runner_fixed_hazards()
// from runner_simulate().
void detect_runner_fall_hazards(GameState *game);

#pragma once

// AI movement, extracted from process() (Phase 18, see
// docs/ai-forces-separation-map.md). Each function's statement order and
// arithmetic are moved verbatim from their original location in process() --
// this is a structural extraction only, not a behavior change.
#include "header.h"

// Boss drift pattern: self-referential (never reads player position) --
// gravity accumulation, position-band steering toward screen-center, and
// horizontal wraparound. Was src/process.c:636-670.
void move_boss_entities(GameState *game);

// Regular enemy ("STUPID BOTS") drift pattern -- the same shape as
// move_boss_entities() with different band/gravity constants, also
// self-referential. Was src/process.c:673-710.
void move_regular_enemies(GameState *game);

// Smart-enemy chase logic, both single-player (always targets `man`) and
// multiplayer (targets whichever of `man`/`secondPlayer` is nearer, via
// smart_enemy_select_target()). Was src/process.c:712-923.
void move_smart_enemies(GameState *game);

// The one piece of the four-box decide/apply/integrate pipeline that's
// safely extractable as a true, standalone "decide intent" step: a pure,
// side-effect-free nearest-target comparison. Returns &game->man
// unconditionally when !game->multiPlayer (matching that the single-player
// chase always targets `man`); otherwise the nearer of &game->man/
// &game->secondPlayer by squared distance (pow(), double precision,
// replicating src/process.c:805-806 exactly).
Man *smart_enemy_select_target(GameState *game, const Enemies *smartEnemy);

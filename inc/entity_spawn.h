#pragma once

// Factory-Method-style spawn helpers centralizing the enemy/smart-enemy/boss
// initialization invariants that were previously hand-duplicated at 10
// separate call sites across src/process.c and src/loadGame.c (see
// docs/solid-gof-audit.md, section 7.1). Each function bounds-checks `index`
// against the real array size and sets exactly the fields the original 10
// call sites set -- no new behavior, only a centralized, bounds-checked
// creation point.
//
// Returns false (and mutates nothing) when `index` is out of range for the
// target array -- a correctness guarantee the original inline code never had.
#include "header.h"

bool enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy);
bool smart_enemy_spawn(GameState *game, int index, float x, float y, float dx, float dy);
bool boss_spawn(GameState *game, int index, float x, float y, float dx, float dy);

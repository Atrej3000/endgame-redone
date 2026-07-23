#pragma once

// Deterministic combat feedback and per-entity animation clocks (Phase 34).
// Public entry points are null-safe; player-indexed triggers accept only 0/1.
#include "header.h"

void combat_feedback_reset(GameState *game);
void combat_feedback_trigger_player_spawn(GameState *game, int playerIndex);
void combat_feedback_trigger_shot(GameState *game, int playerIndex, float x, float y);
void combat_feedback_trigger_enemy_hit(GameState *game, Enemies *enemy, bool defeated, bool boss);
void combat_feedback_trigger_player_hit(GameState *game, int playerIndex);

// Called once for every attempted fixed tick. Returns true when hit-stop
// consumes this tick, so callers can keep polling input/rendering while the
// authoritative world remains frozen.
bool combat_feedback_step(GameState *game);
void combat_feedback_update_animations(GameState *game);

void combat_feedback_begin_render(SDL_Renderer *renderer, const GameState *game);
void combat_feedback_draw(SDL_Renderer *renderer, const GameState *game, float worldOffsetX);
void combat_feedback_end_render(SDL_Renderer *renderer);

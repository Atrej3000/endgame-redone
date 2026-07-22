#pragma once

// Phase 24 contact-event queue and its single consequence phase.
#include "header.h"

void game_events_begin(GameState *game);
bool game_events_push_projectile_hit(GameState *game, bool secondPlayerProjectile,
                                     int projectileIndex, GameEventTarget target, int targetIndex);
bool game_events_push_player_contact(GameState *game, GameEventType type, int playerIndex);
bool game_events_push_target_contact(GameState *game, GameEventType type, int targetIndex);
bool game_events_push_transition_check(GameState *game, GameEventType type);
void game_events_apply(GameState *game);

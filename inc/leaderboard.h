#pragma once

#include "header.h"

#define LEADERBOARD_CAPACITY 25

// Runner scores are kept in descending order. Recording a score updates the
// in-memory table immediately and then attempts an atomic best-effort save.
void leaderboard_reset(GameState *game);
void leaderboard_load(GameState *game);
bool leaderboard_save(const GameState *game);
void leaderboard_record_score(GameState *game, int score);

// Copies at most capacity scores in descending order without mutating game.
// The return value is the number of copied scores.
size_t leaderboard_copy_top_scores(const GameState *game, int *scores,
                                   size_t capacity);

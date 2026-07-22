#pragma once

#include "header.h"

// Shared UI audio is ready before the first menu is rendered. Mode loaders
// call their matching function before a session can begin; no gameplay path
// is allowed to load an audio file on demand.
bool audio_assets_load_shared(GameState *game);
bool audio_assets_load_arcade(GameState *game);
bool audio_assets_load_runner(GameState *game);

void audio_assets_unload_shared(GameState *game);
void audio_assets_unload_arcade(GameState *game);
void audio_assets_unload_runner(GameState *game);

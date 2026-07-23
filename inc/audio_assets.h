#pragma once

#include "header.h"

// Shared UI audio is ready before the first menu is rendered. Mode loaders
// call their matching function before a session can begin; no gameplay path
// is allowed to load an audio file on demand.
bool audio_assets_load_shared(GameState *game);
bool audio_assets_load_arcade(GameState *game);
bool audio_assets_load_runner(GameState *game);

// Scene routing owns background-music selection. These functions are
// intentionally null-safe so the game can continue silently when no audio
// device is available; when an audio device is open, missing/corrupt assets
// remain hard load failures in the functions above.
bool audio_assets_output_available(void);
bool audio_assets_play_menu(GameState *game);
void audio_assets_sync_scene(GameState *game, AppScene previousScene, AppScene nextScene);

void audio_assets_unload_shared(GameState *game);
void audio_assets_unload_arcade(GameState *game);
void audio_assets_unload_runner(GameState *game);

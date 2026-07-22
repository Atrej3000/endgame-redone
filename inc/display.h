#pragma once

#include "header.h"

// Display settings persist in SDL's platform-specific preference directory.
// Game-world coordinates remain WIDTH x HEIGHT; SDL scales presentation only.
void display_settings_defaults(DisplaySettings *settings);
void display_settings_sanitize(DisplaySettings *settings);
void display_settings_load(DisplaySettings *settings);
bool display_settings_save(const DisplaySettings *settings);

bool display_configure_renderer(SDL_Renderer *renderer);
bool display_toggle_fullscreen(GameState *game);
void display_handle_event(GameState *game, const SDL_Event *event);
void display_capture_window_settings(GameState *game);

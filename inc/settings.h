#pragma once

#include "header.h"

void settings_defaults(GameSettings *settings);
void settings_load(GameSettings *settings);
bool settings_save(const GameSettings *settings);
void settings_apply_audio(const GameSettings *settings);
void settings_reset(GameSettings *settings);
bool settings_rebind(GameSettings *settings, int bindingIndex, SDL_Scancode scancode);

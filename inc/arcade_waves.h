#pragma once

// Data-driven Arcade progression (Phase 32). The module owns immutable wave
// definitions and transition rules; GameState owns only current wave state.
#include "header.h"

typedef struct WaveDefinition
{
    int regularEnemies;
    int smartEnemies;
    int bosses;
    int spawnIntervalTicks;
} WaveDefinition;

void arcade_waves_reset(GameState *game);
void arcade_waves_update(GameState *game);
const WaveDefinition *arcade_waves_get_definition(int waveNumber);
void arcade_waves_draw_hud(SDL_Renderer *renderer, const GameState *game);

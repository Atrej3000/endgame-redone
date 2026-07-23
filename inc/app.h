#pragma once

#include "header.h"

// Error-aware application startup. Output containers must initially be NULL;
// on success, *outGame/*outWindow/*outRenderer are all non-NULL and fully
// usable. Accelerated rendering falls back to a
// software renderer; an unavailable audio device produces a silent but usable
// application. Missing assets and other unrecoverable failures run
// app_shutdown on whatever partially succeeded and return false. ENDGAME_SEED
// accepts an unsigned decimal seed for reproducible sessions.
bool app_init(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer);

// Centralized, idempotent shutdown. Safe to call with a fully initialized
// state, a partially initialized state, or more than once in a row. The
// outWindow/outRenderer containers themselves may be NULL; when outGame owns
// a state, its window/renderer handles are authoritative and optional output
// containers normally alias them. If corrupted/mismatched containers hold a
// distinct resource, shutdown destroys each distinct handle exactly once.
// Every supplied pointer is nulled after its resource is destroyed.
void app_shutdown(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer);

#pragma once

#include "header.h"

// Error-aware application startup. On success, *outGame/*outWindow/*outRenderer
// are all non-NULL and fully usable, and returns true. On any failure, prints
// which step failed, runs app_shutdown on whatever partially succeeded, and
// returns false -- the caller should exit with a non-zero status rather than
// calling exit()/SDL_Quit() itself.
bool app_init(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer);

// Centralized, idempotent shutdown. Safe to call with a fully initialized
// state, a partially initialized state (any subset of *outGame/*outWindow/
// *outRenderer may already be NULL), or more than once in a row -- every
// resource is destroyed only if its pointer is still non-NULL, and every
// pointer (including *outGame/*outWindow/*outRenderer themselves) is set to
// NULL immediately after being freed/destroyed.
void app_shutdown(GameState **outGame, SDL_Window **outWindow, SDL_Renderer **outRenderer);

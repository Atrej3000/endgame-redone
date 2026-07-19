#pragma once

// Focused header for the per-mode frame pipeline (src/frame.c) -- see
// docs/frame-pipeline-map.md and docs/solid-gof-audit.md section 7.3.
// arcade_frame()/runner_frame() have exactly one definer (src/frame.c) and
// one caller (src/main.c) in the whole codebase, so their prototypes are
// fully moved here rather than left duplicated in header.h.
#include "header.h"

void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer);
void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer);

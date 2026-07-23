#pragma once

// Focused header for the per-mode frame pipeline (src/frame.c) -- see
// docs/frame-pipeline-map.md and docs/solid-gof-audit.md section 7.3.
// arcade_frame()/runner_frame() have exactly one definer (src/frame.c) and
// one caller (src/main.c) in the whole codebase, so their prototypes are
// fully moved here rather than left duplicated in header.h.
#include "header.h"

// Simulate-only halves of the frame pipeline (Phase 11 -- see
// docs/physics-timestep-map.md): called from main()'s fixed-timestep
// accumulator, 0-N times per real frame, independent of rendering/input.
// arcade_simulate = process + world/contact collision; runner_simulate additionally
// includes runner_update_death (the death-lifecycle step belongs with
// simulation, not rendering).
void arcade_simulate(GameState *game, float dt);
void runner_simulate(GameState *game, float dt);

// Full one-shot pass (simulate + render + input), kept for direct callers
// that want everything in one call with an explicit dt -- no longer called
// from main()'s gameplay-scene cases (which use arcade_simulate/
// runner_simulate + doRender/processEvents directly so render/input can run
// at the real frame rate while simulation runs at the fixed rate).
void arcade_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt);
void runner_frame(GameState *game, SDL_Window *window, SDL_Renderer *renderer, float dt);

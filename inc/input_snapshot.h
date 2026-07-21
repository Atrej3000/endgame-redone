#pragma once

// Continuous held-key input capture (Phase 17, see
// docs/input-snapshot-architecture-map.md), complementing input_command.h's
// discrete keycode-to-action translation. Each function is a pure,
// synchronous read of a caller-supplied SDL_GetKeyboardState() array into an
// InputState -- no GameState dependency, no SDL_PollEvent, no side effects --
// intended to be called exactly once per real frame, before the fixed-step
// physics accumulator loop runs.
#include "header.h"

// Arcade's continuous fields: movement, jump-held, and shoot-held for both
// players.
void input_capture_arcade(InputState *input, const Uint8 *keyboardState);

// Runner's continuous fields: movement and jump-held for both players.
// Runner has no shoot mechanic, so the shoot fields are left untouched.
void input_capture_runner(InputState *input, const Uint8 *keyboardState);

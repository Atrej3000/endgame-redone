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
void input_capture_arcade(InputState *input, const Uint8 *keyboardState, const GameSettings *settings);

// Runner's continuous fields: movement and jump-held for both players.
// Runner has no shoot mechanic, so the shoot fields are explicitly cleared
// to prevent an Arcade snapshot surviving a mode transition.
void input_capture_runner(InputState *input, const Uint8 *keyboardState, const GameSettings *settings);

// Clears both continuous and buffered input. Used when keyboard focus is lost
// so held keys and pending jumps cannot remain stuck while SDL is no longer
// delivering keyboard input for the window.
void input_state_clear(InputState *input);

// Standard SDL game-controller support for player 1. D-pad/left stick move,
// A jumps, and X shoots in Arcade. Controller state is merged with the
// keyboard snapshot so either device (or both) can be used in the same frame.
void input_apply_controller(InputState *input, AppContext *app, bool includeShoot);

// App-context controller lifecycle. The first compatible connected controller
// is opened at startup and replacement devices are picked up after hot-plug.
void input_controller_open_first(AppContext *app);
void input_controller_handle_event(AppContext *app, const SDL_Event *event);

// Samples the current A-button state without generating a jump edge. Call
// this when focus, scenes, or sessions change so a button held across that
// boundary must be released before it can trigger a new jump.
void input_controller_sync_jump_edge(AppContext *app);

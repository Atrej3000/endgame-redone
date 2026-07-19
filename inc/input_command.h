#pragma once

// Command-pattern-style input translation boundary, centralizing the
// keycode-to-action mapping that processEvents()/processEvents2() (the two
// largest, most parallel files in the codebase) previously duplicated
// independently, and formalizing menu0_events()'s existing dual-keycode
// fall-through pattern. See docs/solid-gof-audit.md section 7.2.
//
// Deliberately NOT a queue or heap-allocated command object: each
// translate_*_command() is a pure, synchronous SDL_Keycode -> GameCommand
// function, called and consumed immediately within the same SDL_KEYDOWN
// handling as before. Continuous held-key movement/shooting
// (SDL_GetKeyboardState-based) is untouched by this file entirely -- there is
// no GAME_COMMAND_MOVE_* value in this enum, deliberately.
#include "header.h"

typedef enum GameCommand
{
    GAME_COMMAND_NONE,
    GAME_COMMAND_QUIT_TO_MODE_MENU,
    GAME_COMMAND_QUIT_TO_MAIN_MENU,
    GAME_COMMAND_PAUSE,
    GAME_COMMAND_JUMP_PLAYER1,
    GAME_COMMAND_JUMP_PLAYER2,
    GAME_COMMAND_SELECT_ARCADE,
    GAME_COMMAND_SELECT_RUNNER,
    GAME_COMMAND_QUIT_GAME
} GameCommand;

// processEvents()'s discrete-action keys: ESCAPE/p/w/UP/q.
GameCommand translate_arcade_command(SDL_Keycode key);

// processEvents2()'s discrete-action keys: ESCAPE/p/w/UP/q (the SDLK_0 cheat
// code is intentionally not represented here -- it is not duplicated
// elsewhere, so routing it through this boundary would add indirection with
// no deduplication benefit).
GameCommand translate_runner_command(SDL_Keycode key);

// menu0_events()'s dual-keycode pairs: b/1, r/2, q/ESCAPE.
GameCommand translate_menu_command(SDL_Keycode key);

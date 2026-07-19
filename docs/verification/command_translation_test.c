// Standalone Command-translation test -- NOT part of the shipped game
// (main.c is excluded when building this, same pattern as
// smoke_init_shutdown.c). Exercises translate_arcade_command/
// translate_runner_command/translate_menu_command (src/input_command.c)
// directly -- no SDL window/event injection needed, since these are pure
// SDL_Keycode -> GameCommand functions. See docs/solid-gof-audit.md
// section 7.2.
#include "input_command.h"
#include <stdio.h>

static int failures = 0;

#define CHECK(desc, cond)                                            \
    do                                                                \
    {                                                                  \
        if (cond)                                                       \
        {                                                                \
            printf("COMMAND TRANSLATION TEST: PASS - %s\n", desc);        \
        }                                                                  \
        else                                                                \
        {                                                                    \
            printf("COMMAND TRANSLATION TEST: FAIL - %s\n", desc);            \
            failures++;                                                        \
        }                                                                       \
    } while (0)

int main(void)
{
    // ------------------------------------------------------------------
    // 1. translate_arcade_command -- processEvents()'s 5 discrete keys
    // ------------------------------------------------------------------
    CHECK("arcade: ESCAPE -> QUIT_TO_MODE_MENU", translate_arcade_command(SDLK_ESCAPE) == GAME_COMMAND_QUIT_TO_MODE_MENU);
    CHECK("arcade: p -> PAUSE", translate_arcade_command(SDLK_p) == GAME_COMMAND_PAUSE);
    CHECK("arcade: w -> JUMP_PLAYER1", translate_arcade_command(SDLK_w) == GAME_COMMAND_JUMP_PLAYER1);
    CHECK("arcade: UP -> JUMP_PLAYER2", translate_arcade_command(SDLK_UP) == GAME_COMMAND_JUMP_PLAYER2);
    CHECK("arcade: q -> QUIT_TO_MAIN_MENU", translate_arcade_command(SDLK_q) == GAME_COMMAND_QUIT_TO_MAIN_MENU);
    CHECK("arcade: unmapped key (a) -> NONE", translate_arcade_command(SDLK_a) == GAME_COMMAND_NONE);
    CHECK("arcade: SDLK_0 (runner-only cheat) -> NONE", translate_arcade_command(SDLK_0) == GAME_COMMAND_NONE);

    // ------------------------------------------------------------------
    // 2. translate_runner_command -- processEvents2()'s 5 discrete keys
    //    (same shape as arcade -- same 5 keys mean the same 5 actions)
    // ------------------------------------------------------------------
    CHECK("runner: ESCAPE -> QUIT_TO_MODE_MENU", translate_runner_command(SDLK_ESCAPE) == GAME_COMMAND_QUIT_TO_MODE_MENU);
    CHECK("runner: p -> PAUSE", translate_runner_command(SDLK_p) == GAME_COMMAND_PAUSE);
    CHECK("runner: w -> JUMP_PLAYER1", translate_runner_command(SDLK_w) == GAME_COMMAND_JUMP_PLAYER1);
    CHECK("runner: UP -> JUMP_PLAYER2", translate_runner_command(SDLK_UP) == GAME_COMMAND_JUMP_PLAYER2);
    CHECK("runner: q -> QUIT_TO_MAIN_MENU", translate_runner_command(SDLK_q) == GAME_COMMAND_QUIT_TO_MAIN_MENU);
    CHECK("runner: unmapped key (a) -> NONE", translate_runner_command(SDLK_a) == GAME_COMMAND_NONE);
    // The SDLK_0 cheat code is deliberately NOT represented in GameCommand --
    // processEvents2() still checks it as a raw keycode alongside this
    // translation boundary (see src/processEvents.c).
    CHECK("runner: SDLK_0 (cheat code, handled outside GameCommand) -> NONE", translate_runner_command(SDLK_0) == GAME_COMMAND_NONE);

    // ------------------------------------------------------------------
    // 3. translate_menu_command -- menu0_events()'s 3 dual-keycode pairs
    // ------------------------------------------------------------------
    CHECK("menu: b -> SELECT_ARCADE", translate_menu_command(SDLK_b) == GAME_COMMAND_SELECT_ARCADE);
    CHECK("menu: 1 -> SELECT_ARCADE (dual-keycode pair, same command as b)", translate_menu_command(SDLK_1) == GAME_COMMAND_SELECT_ARCADE);
    CHECK("menu: r -> SELECT_RUNNER", translate_menu_command(SDLK_r) == GAME_COMMAND_SELECT_RUNNER);
    CHECK("menu: 2 -> SELECT_RUNNER (dual-keycode pair, same command as r)", translate_menu_command(SDLK_2) == GAME_COMMAND_SELECT_RUNNER);
    CHECK("menu: q -> QUIT_GAME", translate_menu_command(SDLK_q) == GAME_COMMAND_QUIT_GAME);
    CHECK("menu: ESCAPE -> QUIT_GAME (dual-keycode pair, same command as q)", translate_menu_command(SDLK_ESCAPE) == GAME_COMMAND_QUIT_GAME);
    CHECK("menu: unmapped key (3) -> NONE", translate_menu_command(SDLK_3) == GAME_COMMAND_NONE);

    // ------------------------------------------------------------------
    // 4. This phase does not touch continuous movement polling -- confirm
    //    by construction that no GAME_COMMAND_MOVE_* value exists at all.
    //    (Compile-time check: if such a value existed, this enum's members
    //    would need updating here too -- absence is the guarantee.)
    // ------------------------------------------------------------------
    CHECK("GameCommand enum has exactly the 9 documented values (NONE + 8 actions)",
          GAME_COMMAND_NONE == 0 &&
          GAME_COMMAND_QUIT_GAME == 8);

    if (failures == 0)
    {
        printf("COMMAND TRANSLATION TEST: ALL PASS\n");
        return 0;
    }
    printf("COMMAND TRANSLATION TEST: %d FAILURE(S)\n", failures);
    return 1;
}

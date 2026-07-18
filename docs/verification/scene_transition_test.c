// Standalone scene-transition test -- NOT part of the shipped game (main.c
// is excluded when building this, same pattern as smoke_init_shutdown.c).
// Exercises app_change_scene() and its enter-hooks without needing keyboard/
// window input injection, which this environment has no way to provide.
//
// Test-only exception to the "game->scene is written only inside
// app_change_scene()" invariant (see docs/scene-state-map.md): a few cases
// below directly assign game->scene to set up a precondition before calling
// app_change_scene(), which is the thing actually under test. Production
// code must never do this -- only this test file does, and only for setup.
#include "app.h"

static int failures = 0;

#define CHECK(desc, cond)                                                    \
    do                                                                       \
    {                                                                        \
        if (cond)                                                           \
        {                                                                   \
            printf("SCENE TEST: PASS - %s\n", desc);                        \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            printf("SCENE TEST: FAIL - %s\n", desc);                        \
            failures++;                                                     \
        }                                                                    \
    } while (0)

int main(void)
{
    GameState *game = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&game, &window, &renderer))
    {
        printf("SCENE TEST: app_init failed, cannot run\n");
        return 1;
    }

    // Mirror main()'s own bootstrap sequence.
    game->x_score = 0;
    for (game->x_i = 0; game->x_i < 25; game->x_i++)
    {
        game->x_list[game->x_i] = 0;
    }
    load_menu0(game);
    game->scene = APP_SCENE_MAIN_MENU;

    // 1. Initial scene.
    CHECK("initial scene is MAIN_MENU", game->scene == APP_SCENE_MAIN_MENU);

    // 2. Valid main-menu -> Arcade-menu transition.
    app_change_scene(game, APP_SCENE_ARCADE_MENU);
    CHECK("MAIN_MENU -> ARCADE_MENU sets scene", game->scene == APP_SCENE_ARCADE_MENU);
    CHECK("arcade_menu_enter loaded assets (arcadeAssetsLoaded)", game->arcadeAssetsLoaded == true);
    CHECK("arcade_menu_enter from MAIN_MENU reset leaderboard (x_score==0)", game->x_score == 0);

    // 3. Runner-menu transition (starting fresh from Main menu again).
    app_change_scene(game, APP_SCENE_MAIN_MENU);
    app_change_scene(game, APP_SCENE_RUNNER_MENU);
    CHECK("MAIN_MENU -> RUNNER_MENU sets scene", game->scene == APP_SCENE_RUNNER_MENU);
    CHECK("runner_menu_enter loaded assets (runnerAssetsLoaded)", game->runnerAssetsLoaded == true);
    CHECK("runner_menu_enter from MAIN_MENU reset leaderboard (x_score==0)", game->x_score == 0);

    // 4. Gameplay -> menu transition: load_menu1() must NOT re-fire (leaderboard
    // untouched) when returning from gameplay, only when arriving from the Main
    // menu. Test-only direct scene assignment to simulate "currently in
    // Arcade gameplay" without a real transition (see file header comment).
    app_change_scene(game, APP_SCENE_MAIN_MENU);
    app_change_scene(game, APP_SCENE_ARCADE_MENU); // real entry, resets x_score to 0
    game->x_score = 42;                            // sentinel
    game->man.lives = 0;                           // sentinel gameplay state, should get reset
    game->scene = APP_SCENE_ARCADE_GAME;           // test-only: simulate "currently playing"
    app_change_scene(game, APP_SCENE_ARCADE_MENU); // the real transition under test
    CHECK("ARCADE_GAME -> ARCADE_MENU sets scene", game->scene == APP_SCENE_ARCADE_MENU);
    CHECK("returning from gameplay does NOT re-fire load_menu1 (x_score untouched)", game->x_score == 42);
    CHECK("returning from gameplay DOES reset gameplay state (man.lives back to 3)", game->man.lives == 3);

    // 5. Quit transition.
    app_change_scene(game, APP_SCENE_QUIT);
    CHECK("any scene -> QUIT sets scene", game->scene == APP_SCENE_QUIT);

    // 6. Invalid enum handling: out-of-range value must force a safe quit,
    // not crash or fall into undefined switch behavior.
    game->scene = APP_SCENE_MAIN_MENU; // test-only reset for a clean precondition
    app_change_scene(game, (AppScene)9999);
    CHECK("invalid scene value forces APP_SCENE_QUIT safely", game->scene == APP_SCENE_QUIT);

    // 7. Pause resumes to the correct originating mode. Test-only direct
    // assignment to set up "currently playing Arcade/Runner" preconditions;
    // the resume expression itself (the ternary) is an exact copy of
    // pause_events.c's real resume logic, exercised here without a real
    // SDL_Event.
    game->scene = APP_SCENE_ARCADE_GAME;
    app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
    CHECK("ARCADE_GAME -> ARCADE_PAUSE sets scene", game->scene == APP_SCENE_ARCADE_PAUSE);
    app_change_scene(game, (game->scene == APP_SCENE_RUNNER_PAUSE) ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);
    CHECK("ARCADE_PAUSE resumes to ARCADE_GAME (not RUNNER_GAME)", game->scene == APP_SCENE_ARCADE_GAME);

    game->scene = APP_SCENE_RUNNER_GAME;
    app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
    CHECK("RUNNER_GAME -> RUNNER_PAUSE sets scene", game->scene == APP_SCENE_RUNNER_PAUSE);
    app_change_scene(game, (game->scene == APP_SCENE_RUNNER_PAUSE) ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);
    CHECK("RUNNER_PAUSE resumes to RUNNER_GAME (not ARCADE_GAME)", game->scene == APP_SCENE_RUNNER_GAME);

    app_shutdown(&game, &window, &renderer);

    if (failures == 0)
    {
        printf("SCENE TEST: ALL PASS\n");
        return 0;
    }
    printf("SCENE TEST: %d FAILURE(S)\n", failures);
    return 1;
}

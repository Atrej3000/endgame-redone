#include "header.h"

// Entry hook for APP_SCENE_ARCADE_MENU. load_menu1() (texture guard, music
// restart, x_score/x_list reset) only fires on the true Main-menu -> Arcade
// transition, matching how often it ran before this refactor. loadGame()'s
// asset guard + full gameplay-state reset fires on every arrival at this
// scene, matching the *observable* effect of its old every-frame call site
// (nothing renders or reads gameplay state while sitting on this menu).
static void arcade_menu_enter(GameState *game, AppScene previous_scene)
{
    if (previous_scene == APP_SCENE_MAIN_MENU)
    {
        load_menu1(game);
    }
    loadGame(game);
}

// Entry hook for APP_SCENE_RUNNER_MENU -- mirrors arcade_menu_enter.
static void runner_menu_enter(GameState *game, AppScene previous_scene)
{
    if (previous_scene == APP_SCENE_MAIN_MENU)
    {
        load_menu2(game);
    }
    loadGame2(game);
}

void app_change_scene(GameState *game, AppScene next_scene)
{
    if (next_scene < APP_SCENE_MAIN_MENU || next_scene > APP_SCENE_QUIT)
    {
        fprintf(stderr, "app_change_scene: invalid scene %d, forcing quit\n", (int)next_scene);
        game->scene = APP_SCENE_QUIT;
        return;
    }

    AppScene previous_scene = game->scene;
    game->scene = next_scene;

    switch (next_scene)
    {
    case APP_SCENE_ARCADE_MENU:
        arcade_menu_enter(game, previous_scene);
        break;
    case APP_SCENE_RUNNER_MENU:
        runner_menu_enter(game, previous_scene);
        break;
    default:
        break;
    }
}

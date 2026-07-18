#include "header.h"

// Entry hook for APP_SCENE_ARCADE_MENU. load_menu1() (texture guard, music
// restart, x_score/x_list reset) only fires on the true Main-menu -> Arcade
// transition. arcade_assets_load() is guarded by arcadeAssetsLoaded and so
// also only does real work once; on failure, falls back to the Main menu
// rather than showing a scene with missing textures (see
// docs/game-session-lifecycle.md). Session reset no longer happens here --
// it's triggered explicitly when a new game is actually selected
// (src/menu_events.c), not merely by arriving at this menu.
static void arcade_menu_enter(GameState *game, AppScene previous_scene)
{
    if (previous_scene == APP_SCENE_MAIN_MENU)
    {
        load_menu1(game);
    }
    if (!game->arcadeAssetsLoaded && !arcade_assets_load(game))
    {
        fprintf(stderr, "arcade_menu_enter: asset load failed, returning to main menu\n");
        app_change_scene(game, APP_SCENE_MAIN_MENU);
    }
}

// Entry hook for APP_SCENE_RUNNER_MENU -- mirrors arcade_menu_enter.
static void runner_menu_enter(GameState *game, AppScene previous_scene)
{
    if (previous_scene == APP_SCENE_MAIN_MENU)
    {
        load_menu2(game);
    }
    if (!game->runnerAssetsLoaded && !runner_assets_load(game))
    {
        fprintf(stderr, "runner_menu_enter: asset load failed, returning to main menu\n");
        app_change_scene(game, APP_SCENE_MAIN_MENU);
    }
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

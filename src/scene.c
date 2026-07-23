#include "header.h"
#include "audio_assets.h"
#include "input_snapshot.h"
#include "scene.h"

// Entry hook for APP_SCENE_ARCADE_MENU. load_menu1() runs on every entry so
// direct cross-mode transitions also initialize menu state; its texture guard
// makes repeat entries cheap. Leaderboard state is application-lifetime
// persistent and is not owned by mode-menu entry.
// arcade_assets_load() verifies its complete-group invariant and only does
// real work when a pointer or lifecycle flag is missing; on failure, falls
// back to the Main menu
// rather than showing a scene with missing textures (see
// docs/game-session-lifecycle.md). Session reset no longer happens here --
// it's triggered explicitly when a new game is actually selected
// (src/menu_events.c), not merely by arriving at this menu.
static void arcade_menu_enter(GameState *game, AppScene previous_scene)
{
    (void)previous_scene;
    load_menu1(game);
    if (game->app.scene != APP_SCENE_ARCADE_MENU)
    {
        return;
    }
    if (!arcade_assets_load(game))
    {
        fprintf(stderr, "arcade_menu_enter: asset load failed, returning to main menu\n");
        app_change_scene(game, APP_SCENE_MAIN_MENU);
    }
}

// Entry hook for APP_SCENE_RUNNER_MENU -- mirrors arcade_menu_enter.
static void runner_menu_enter(GameState *game, AppScene previous_scene)
{
    (void)previous_scene;
    load_menu2(game);
    if (game->app.scene != APP_SCENE_RUNNER_MENU)
    {
        return;
    }
    if (!runner_assets_load(game))
    {
        fprintf(stderr, "runner_menu_enter: asset load failed, returning to main menu\n");
        app_change_scene(game, APP_SCENE_MAIN_MENU);
    }
}

void app_change_scene(GameState *game, AppScene next_scene)
{
    if (game == NULL)
    {
        fprintf(stderr, "app_change_scene: game must not be NULL\n");
        return;
    }
    if (next_scene < APP_SCENE_MAIN_MENU || next_scene > APP_SCENE_QUIT)
    {
        fprintf(stderr, "app_change_scene: invalid scene %d, forcing quit\n", (int)next_scene);
        next_scene = APP_SCENE_QUIT;
    }

    AppScene previous_scene = game->app.scene;
    game->app.scene = next_scene;
    if (previous_scene != next_scene)
    {
        // Neither held input nor buffered key edges belong to the destination
        // scene. Keep non-key lifecycle events (quit/window/controller) queued.
        game->input = (InputState){0};
        input_controller_sync_jump_edge(&game->app);
        SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);
    }

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

    // An entry hook may redirect to a safe fallback scene. Only synchronize
    // audio for this transition when its requested destination is still the
    // active scene; the nested fallback transition already synchronized its
    // own music.
    if (game->app.scene == next_scene)
    {
        audio_assets_sync_scene(game, previous_scene, next_scene);
    }
}

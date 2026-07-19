#include "header.h"
#include "app.h"
#include "scene.h"
#include "frame.h"

int main()
{
    GameState *gameState = NULL;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if (!app_init(&gameState, &window, &renderer))
    {
        return 1;
    }

    gameState->x_score = 0;
    for(gameState->x_i = 0; gameState->x_i < 25; gameState->x_i++) {
        gameState->x_list[gameState->x_i] = 0;
        //gameState.x_names[gameState.x_i] = "";
    }

    load_menu0(gameState);
    // Bootstrap only -- the one direct assignment to `scene` outside
    // app_change_scene(), since there is no "previous scene" transition to
    // run an enter-hook for at program start. calloc already zero-initializes
    // `scene` to APP_SCENE_MAIN_MENU (its first member); set explicitly here
    // for clarity rather than relying on that implicitly.
    gameState->scene = APP_SCENE_MAIN_MENU;

    while (gameState->scene != APP_SCENE_QUIT) {
        switch (gameState->scene) {
            case APP_SCENE_MAIN_MENU:
                menu0_events(gameState);
                doRender_menu0(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_MENU:
                menu_events(gameState);
                doRender_menu1(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_GAME:
                arcade_frame(gameState, window, renderer);
                break;

            case APP_SCENE_ARCADE_LEADERBOARD:
                processEvents(window, gameState);
                doRender_leaderboard(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_PAUSE:
                doRender_pause(renderer, gameState);
                pause_events(gameState);
                break;

            case APP_SCENE_RUNNER_MENU:
                menu_events(gameState);
                doRender_menu2(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_GAME:
                runner_frame(gameState, window, renderer);
                break;

            case APP_SCENE_RUNNER_LEADERBOARD:
                processEvents2(window, gameState);
                doRender_leaderboard2(renderer, gameState);
                break;

            case APP_SCENE_RUNNER_PAUSE:
                doRender_pause(renderer, gameState);
                pause_events(gameState);
                break;

            default:
                app_change_scene(gameState, APP_SCENE_QUIT);
                break;
        }
    }

    app_shutdown(&gameState, &window, &renderer);
    return 0;
}

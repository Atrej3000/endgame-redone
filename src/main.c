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
    gameState->app.scene = APP_SCENE_MAIN_MENU;

    // Fixed-timestep physics (Phase 11) -- see docs/physics-timestep-map.md.
    // Real elapsed time is measured every loop iteration regardless of
    // scene, so the timestamp stays accurate across scene changes; the
    // accumulator itself only accrues time while a gameplay scene is
    // active, so no backlog builds up while paused/in menus/leaderboards.
    Uint64 perfFrequency = SDL_GetPerformanceFrequency();
    Uint64 previousCounter = SDL_GetPerformanceCounter();
    double accumulator = 0.0;

    while (gameState->app.scene != APP_SCENE_QUIT) {
        Uint64 currentCounter = SDL_GetPerformanceCounter();
        double frameTime = (double)(currentCounter - previousCounter) / (double)perfFrequency;
        previousCounter = currentCounter;
        if (frameTime > MAX_FRAME_TIME)
        {
            frameTime = MAX_FRAME_TIME;
        }

        switch (gameState->app.scene) {
            case APP_SCENE_MAIN_MENU:
                menu0_events(gameState);
                doRender_menu0(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_MENU:
                menu_events(gameState);
                doRender_menu1(renderer, gameState);
                break;

            case APP_SCENE_ARCADE_GAME:
            {
                accumulator += frameTime;
                int steps = 0;
                while (accumulator >= (double)PHYSICS_DT && steps < MAX_PHYSICS_STEPS_PER_FRAME)
                {
                    arcade_simulate(gameState, PHYSICS_DT);
                    accumulator -= (double)PHYSICS_DT;
                    steps++;
                }
                doRender(renderer, gameState);
                processEvents(window, gameState);
                break;
            }

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
            {
                accumulator += frameTime;
                int steps = 0;
                while (accumulator >= (double)PHYSICS_DT && steps < MAX_PHYSICS_STEPS_PER_FRAME)
                {
                    runner_simulate(gameState, PHYSICS_DT);
                    accumulator -= (double)PHYSICS_DT;
                    steps++;
                }
                doRender2(renderer, gameState);
                runner_update_death(gameState); // see src/runner_death.c, docs/runner-death-lifecycle.md --
                                                 // kept after doRender2, matching the pre-Phase-11
                                                 // arcade_frame/runner_frame order exactly
                processEvents2(window, gameState);
                break;
            }

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

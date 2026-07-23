#include "header.h"
#include "scene.h"
#include "input_snapshot.h"
#include "display.h"

static bool pause_focus_was_lost(const SDL_Event *event)
{
    return event->type == SDL_WINDOWEVENT &&
           event->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
}

int pause_events(GameState *gameState)
{
    if (!gameState ||
        (gameState->app.scene != APP_SCENE_ARCADE_PAUSE &&
         gameState->app.scene != APP_SCENE_RUNNER_PAUSE))
    {
        return 0;
    }
    SDL_Event event;
    const AppScene pollingScene = gameState->app.scene;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&gameState->app, &event);
        display_handle_event(gameState, &event);
        if (gameState->app.scene != pollingScene || pause_focus_was_lost(&event))
        {
            return gameState->menu_status;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
        switch (event.key.keysym.sym) {
            case SDLK_q:
                app_change_scene(gameState, APP_SCENE_MAIN_MENU);
                return gameState->menu_status;
            case SDLK_ESCAPE:
                if (gameState->audio.select != NULL)
                {
                    Mix_PlayChannel(-1, gameState->audio.select, 0);
                }
                app_change_scene(gameState, (pollingScene == APP_SCENE_RUNNER_PAUSE)
                                                 ? APP_SCENE_RUNNER_GAME
                                                 : APP_SCENE_ARCADE_GAME);
                return gameState->menu_status;
        }
        }
    }
    return gameState->menu_status;
}

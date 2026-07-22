#include "header.h"
#include "scene.h"
#include "display.h"

int pause_events(GameState *gameState)
{
    SDL_Event event;

    if (gameState->select == NULL) {
        gameState->select = Mix_LoadWAV("resource/sounds/select.wav");
    }

    while (SDL_PollEvent(&event))
    {
        display_handle_event(gameState, &event);
        if (event.type == SDL_KEYDOWN)
        {
        switch (event.key.keysym.sym) {
            case SDLK_q:
                app_change_scene(gameState, APP_SCENE_MAIN_MENU);
                break;
            case SDLK_ESCAPE:
                app_change_scene(gameState, (gameState->app.scene == APP_SCENE_RUNNER_PAUSE)
                                                 ? APP_SCENE_RUNNER_GAME
                                                 : APP_SCENE_ARCADE_GAME);
                Mix_PlayChannel(-1, gameState->select, 0);
                Mix_ResumeMusic();
                break;
        }
        }
    }
    return gameState->menu_status;
}

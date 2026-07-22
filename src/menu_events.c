#include "header.h"
#include "scene.h"
#include "input_command.h"
#include "input_snapshot.h"
#include "display.h"

void menu_events(GameState *gameState)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&gameState->app, &event);
        display_handle_event(gameState, &event);
        if (event.type == SDL_KEYDOWN)
        {
        bool isRunner = (gameState->app.scene == APP_SCENE_RUNNER_MENU);
        switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                // Already at this menu; no scene change.
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case SDLK_1:
                if (isRunner)
                {
                    runner_session_reset(gameState, GAME_MODE_SINGLE_PLAYER);
                }
                else
                {
                    arcade_session_reset(gameState, GAME_MODE_SINGLE_PLAYER);
                }
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);

                Mix_VolumeChunk(gameState->audio.select, 32);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case SDLK_2:
                if (isRunner)
                {
                    runner_session_reset(gameState, GAME_MODE_MULTIPLAYER);
                }
                else
                {
                    arcade_session_reset(gameState, GAME_MODE_MULTIPLAYER);
                }
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);

                Mix_VolumeChunk(gameState->audio.select, 32);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case SDLK_3:
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_LEADERBOARD : APP_SCENE_ARCADE_LEADERBOARD);

                Mix_VolumeChunk(gameState->audio.select, 32);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case SDLK_q:
                app_change_scene(gameState, APP_SCENE_MAIN_MENU);

                Mix_VolumeChunk(gameState->audio.select, 32);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            default:
                // Already at this menu; no scene change.
                break;
        }
        }
    }
}

void menu0_events(GameState *gameState)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&gameState->app, &event);
        display_handle_event(gameState, &event);
        if (event.type == SDL_KEYDOWN)
        {
        switch (translate_menu_command(event.key.keysym.sym)) {
            case GAME_COMMAND_NONE:
                if (event.key.keysym.sym == SDLK_s)
                {
                    app_change_scene(gameState, APP_SCENE_SETTINGS);
                }
                break;
            case GAME_COMMAND_SELECT_ARCADE:
                app_change_scene(gameState, APP_SCENE_ARCADE_MENU);

                Mix_VolumeChunk(gameState->audio.select, 16);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case GAME_COMMAND_SELECT_RUNNER:
                app_change_scene(gameState, APP_SCENE_RUNNER_MENU);

                Mix_VolumeChunk(gameState->audio.select, 16);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            case GAME_COMMAND_QUIT_GAME:
                app_change_scene(gameState, APP_SCENE_QUIT);

                Mix_VolumeChunk(gameState->audio.select, 32);
                Mix_PlayChannel(-1, gameState->audio.select, 0);
                break;
            default:
                // Already at the main menu; no scene change.
                break;
        }
        }
    }
}

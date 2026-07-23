#include "header.h"
#include "scene.h"
#include "input_command.h"
#include "input_snapshot.h"
#include "display.h"

static bool menu_focus_was_lost(const SDL_Event *event)
{
    return event->type == SDL_WINDOWEVENT &&
           event->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
}

static void play_menu_select(GameState *game, int volume)
{
    if (game->audio.select != NULL)
    {
        Mix_VolumeChunk(game->audio.select, volume);
        Mix_PlayChannel(-1, game->audio.select, 0);
    }
}

void menu_events(GameState *gameState)
{
    if (!gameState ||
        (gameState->app.scene != APP_SCENE_ARCADE_MENU &&
         gameState->app.scene != APP_SCENE_RUNNER_MENU))
    {
        return;
    }
    SDL_Event event;
    const AppScene pollingScene = gameState->app.scene;
    const bool isRunner = pollingScene == APP_SCENE_RUNNER_MENU;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&gameState->app, &event);
        display_handle_event(gameState, &event);
        if (gameState->app.scene != pollingScene || menu_focus_was_lost(&event))
        {
            return;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
        switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                // Already at this menu; no scene change.
                play_menu_select(gameState, 32);
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
                play_menu_select(gameState, 32);
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);
                return;
            case SDLK_2:
                if (isRunner)
                {
                    runner_session_reset(gameState, GAME_MODE_MULTIPLAYER);
                }
                else
                {
                    arcade_session_reset(gameState, GAME_MODE_MULTIPLAYER);
                }
                play_menu_select(gameState, 32);
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_GAME : APP_SCENE_ARCADE_GAME);
                return;
            case SDLK_3:
                play_menu_select(gameState, 32);
                app_change_scene(gameState, isRunner ? APP_SCENE_RUNNER_LEADERBOARD : APP_SCENE_ARCADE_LEADERBOARD);
                return;
            case SDLK_q:
                play_menu_select(gameState, 32);
                app_change_scene(gameState, APP_SCENE_MAIN_MENU);
                return;
            default:
                // Already at this menu; no scene change.
                break;
        }
        }
    }
}

void menu0_events(GameState *gameState)
{
    if (!gameState || gameState->app.scene != APP_SCENE_MAIN_MENU)
    {
        return;
    }
    SDL_Event event;
    const AppScene pollingScene = gameState->app.scene;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&gameState->app, &event);
        display_handle_event(gameState, &event);
        if (gameState->app.scene != pollingScene || menu_focus_was_lost(&event))
        {
            return;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
        switch (translate_menu_command(event.key.keysym.sym)) {
            case GAME_COMMAND_NONE:
                if (event.key.keysym.sym == SDLK_s)
                {
                    app_change_scene(gameState, APP_SCENE_SETTINGS);
                    return;
                }
                break;
            case GAME_COMMAND_SELECT_ARCADE:
                play_menu_select(gameState, 16);
                app_change_scene(gameState, APP_SCENE_ARCADE_MENU);
                return;
            case GAME_COMMAND_SELECT_RUNNER:
                play_menu_select(gameState, 16);
                app_change_scene(gameState, APP_SCENE_RUNNER_MENU);
                return;
            case GAME_COMMAND_QUIT_GAME:
                play_menu_select(gameState, 32);
                app_change_scene(gameState, APP_SCENE_QUIT);
                return;
            default:
                // Already at the main menu; no scene change.
                break;
        }
        }
    }
}

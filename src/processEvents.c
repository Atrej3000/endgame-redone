#include "header.h"
#include "scene.h"
#include "input_command.h"
#include "input_snapshot.h"
#include "display.h"

static bool focus_was_lost(const SDL_Event *event)
{
    return event->type == SDL_WINDOWEVENT &&
           event->window.event == SDL_WINDOWEVENT_FOCUS_LOST;
}

static void play_select_sound(GameState *game)
{
    if (game->audio.select != NULL)
    {
        Mix_VolumeChunk(game->audio.select, 32);
        Mix_PlayChannel(-1, game->audio.select, 0);
    }
}

int processEvents(SDL_Window *window, GameState *game)
{
    (void)window;
    if (game == NULL) return 0;
    SDL_Event event;
    const AppScene pollingScene = game->app.scene;
    const bool gameplay = pollingScene == APP_SCENE_ARCADE_GAME;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&game->app, &event);
        display_handle_event(game, &event);
        if (game->app.scene != pollingScene || focus_was_lost(&event))
        {
            return 0;
        }
        if (event.type != SDL_KEYDOWN || event.key.repeat != 0)
        {
            continue;
        }
        if (event.key.keysym.sym == SDLK_F11)
        {
            continue;
        }

        GameCommand command = translate_gameplay_command(event.key.keysym.sym,
                                                          event.key.keysym.scancode,
                                                          &game->app.settings);
        if (!gameplay)
        {
            if (command == GAME_COMMAND_QUIT_TO_MODE_MENU)
            {
                play_select_sound(game);
                app_change_scene(game, APP_SCENE_ARCADE_MENU);
                return 0;
            }
            if (command == GAME_COMMAND_QUIT_TO_MAIN_MENU)
            {
                app_change_scene(game, APP_SCENE_MAIN_MENU);
                return 0;
            }
            continue;
        }

        switch (command)
        {
        case GAME_COMMAND_QUIT_TO_MODE_MENU:
            play_select_sound(game);
            app_change_scene(game, APP_SCENE_ARCADE_MENU);
            return 0;
        case GAME_COMMAND_PAUSE:
            play_select_sound(game);
            app_change_scene(game, APP_SCENE_ARCADE_PAUSE);
            return 0;
        case GAME_COMMAND_JUMP_PLAYER1:
            game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
            break;
        case GAME_COMMAND_JUMP_PLAYER2:
            if (game->multiPlayer)
            {
                game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;
            }
            break;
        case GAME_COMMAND_QUIT_TO_MAIN_MENU:
            app_change_scene(game, APP_SCENE_MAIN_MENU);
            return 0;
        default:
            break;
        }
    }
    return 1;
}

int processEvents2(SDL_Window *window, GameState *game)
{
    (void)window;
    if (game == NULL) return 0;
    SDL_Event event;
    const AppScene pollingScene = game->app.scene;
    const bool gameplay = pollingScene == APP_SCENE_RUNNER_GAME;

    while (SDL_PollEvent(&event))
    {
        input_controller_handle_event(&game->app, &event);
        display_handle_event(game, &event);
        if (game->app.scene != pollingScene || focus_was_lost(&event))
        {
            return 0;
        }
        if (event.type != SDL_KEYDOWN || event.key.repeat != 0)
        {
            continue;
        }
        if (event.key.keysym.sym == SDLK_F11)
        {
            continue;
        }

        GameCommand command = translate_gameplay_command(event.key.keysym.sym,
                                                          event.key.keysym.scancode,
                                                          &game->app.settings);
        if (!gameplay)
        {
            if (command == GAME_COMMAND_QUIT_TO_MODE_MENU)
            {
                play_select_sound(game);
                app_change_scene(game, APP_SCENE_RUNNER_MENU);
                return 0;
            }
            if (command == GAME_COMMAND_QUIT_TO_MAIN_MENU)
            {
                app_change_scene(game, APP_SCENE_MAIN_MENU);
                return 0;
            }
            continue;
        }

        switch (command)
        {
        case GAME_COMMAND_QUIT_TO_MODE_MENU:
            play_select_sound(game);
            app_change_scene(game, APP_SCENE_RUNNER_MENU);
            return 0;
        case GAME_COMMAND_PAUSE:
            play_select_sound(game);
            app_change_scene(game, APP_SCENE_RUNNER_PAUSE);
            return 0;
        case GAME_COMMAND_JUMP_PLAYER1:
            game->input.jumpBufferTicksPlayer1 = JUMP_BUFFER_TICKS;
            break;
        case GAME_COMMAND_QUIT_TO_MAIN_MENU:
            app_change_scene(game, APP_SCENE_MAIN_MENU);
            return 0;
        case GAME_COMMAND_JUMP_PLAYER2:
            if (game->multiPlayer)
            {
                game->input.jumpBufferTicksPlayer2 = JUMP_BUFFER_TICKS;
            }
            break;
        default:
            break;
        }
    }
    return 1;
}

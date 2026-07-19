#include "input_command.h"

GameCommand translate_arcade_command(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_ESCAPE:
        return GAME_COMMAND_QUIT_TO_MODE_MENU;
    case SDLK_p:
        return GAME_COMMAND_PAUSE;
    case SDLK_w:
        return GAME_COMMAND_JUMP_PLAYER1;
    case SDLK_UP:
        return GAME_COMMAND_JUMP_PLAYER2;
    case SDLK_q:
        return GAME_COMMAND_QUIT_TO_MAIN_MENU;
    default:
        return GAME_COMMAND_NONE;
    }
}

GameCommand translate_runner_command(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_ESCAPE:
        return GAME_COMMAND_QUIT_TO_MODE_MENU;
    case SDLK_p:
        return GAME_COMMAND_PAUSE;
    case SDLK_w:
        return GAME_COMMAND_JUMP_PLAYER1;
    case SDLK_UP:
        return GAME_COMMAND_JUMP_PLAYER2;
    case SDLK_q:
        return GAME_COMMAND_QUIT_TO_MAIN_MENU;
    default:
        return GAME_COMMAND_NONE;
    }
}

GameCommand translate_menu_command(SDL_Keycode key)
{
    switch (key)
    {
    case SDLK_b:
    case SDLK_1:
        return GAME_COMMAND_SELECT_ARCADE;
    case SDLK_r:
    case SDLK_2:
        return GAME_COMMAND_SELECT_RUNNER;
    case SDLK_q:
    case SDLK_ESCAPE:
        return GAME_COMMAND_QUIT_GAME;
    default:
        return GAME_COMMAND_NONE;
    }
}

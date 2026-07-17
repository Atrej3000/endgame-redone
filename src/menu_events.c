#include "header.h"

void menu_events(GameState *gameState)
{
    SDL_Event event;

    if (gameState->select == NULL) {
        gameState->select = Mix_LoadWAV("resource/sounds/select.wav");
    }

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
        switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                gameState->menu_status = 0;

                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_1:
                gameState->menu_status = 1;
                gameState->multiPlayer = 0;

                Mix_VolumeChunk(gameState->select, 32);
                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_2:
                gameState->menu_status = 2;
                gameState->multiPlayer = 1;

                Mix_VolumeChunk(gameState->select, 32);
                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_3:
                gameState->menu_status = 3;

                Mix_VolumeChunk(gameState->select, 32);
                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_q:
                gameState->menu_status = 4;

                Mix_VolumeChunk(gameState->select, 32);
                Mix_PlayChannel(-1, gameState->select, 0);
//                Mix_FreeChunk(gameState->select);
//                Mix_FreeMusic(gameState->menuMus);
                break;
//            case SDLK_ESCAPE:
//                gameState->menu0_status = 0;
//                break;
            default:
                gameState->menu_status = 0;
                break;
        }
        }
    }
//    return done;
}

void menu0_events(GameState *gameState)
{
    SDL_Event event;

    if (gameState->select == NULL) {
        gameState->select = Mix_LoadWAV("resource/sounds/select.wav");
    }

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_KEYDOWN)
        {
        switch (event.key.keysym.sym) {
            case SDLK_b:
            case SDLK_1:
                gameState->menu0_status = 1;

                Mix_VolumeChunk(gameState->select, 16);
                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_r:
            case SDLK_2:
                gameState->menu0_status = 2;

                Mix_VolumeChunk(gameState->select, 16);
                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            case SDLK_q:
            case SDLK_ESCAPE:
                gameState->menu0_status = 3;

                Mix_VolumeChunk(gameState->select, 32);
                Mix_PlayChannel(-1, gameState->select, 0);
//                Mix_FreeChunk(gameState->select);
//                Mix_FreeMusic(gameState->menuMus);

//                Mix_VolumeChunk(gameState->select, 32);
//                Mix_PlayChannel(-1, gameState->select, 0);
                break;
            default:
                gameState->menu0_status = 0;
                break;
        }
        }
    }
}

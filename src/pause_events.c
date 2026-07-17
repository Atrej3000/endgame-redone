#include "header.h"

int pause_events(GameState *gameState)
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
//            case SDLK_p:
//                gameState->menu_status = 5;
//                break;
            case SDLK_q:
                gameState->menu_status = 4;
                break;
            case SDLK_ESCAPE:
                gameState->menu_status = 1;
                Mix_PlayChannel(-1, gameState->select, 0);
                Mix_ResumeMusic();
                break;
        }
        }
    }
//    const Uint8 *state = SDL_GetKeyboardState(NULL);
//    if (state[SDL_SCANCODE_ESCAPE]) {
//        gameState->menu_status = 1;
//    }
    return gameState->menu_status;
}

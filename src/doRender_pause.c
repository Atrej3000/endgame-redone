#include "header.h"

int doRender_pause(SDL_Renderer *renderer, GameState *game)
{
    int paus = 0;
    SDL_Rect pause = {0, 0, 1279, 720};
    SDL_RenderCopy(renderer, game->pause, NULL, &pause);
    SDL_RenderPresent(renderer);
    return paus;
}
